#include <Arduino.h>
#include <SPI.h>

#define CS    5
#define SCK   18
#define MISO  19
#define MOSI  23

// =====================================================
// SPI HELPERS
// =====================================================

void csLow() {
  digitalWrite(CS, LOW);
}

void csHigh() {
  digitalWrite(CS, HIGH);
}

// -----------------------------------------------------

void writeEnable() {

  csLow();

  SPI.transfer(0x06); // Write Enable

  csHigh();
}

// -----------------------------------------------------

uint8_t readStatusRegister() {

  csLow();

  SPI.transfer(0x05);

  uint8_t status = SPI.transfer(0x00);

  csHigh();

  return status;
}

// -----------------------------------------------------

void waitReady() {

  unsigned long start = millis();

  while (readStatusRegister() & 0x01) {

    if (millis() - start > 10000) {

      Serial.println("ERR:FLASH_TIMEOUT");

      return;
    }

    delay(1);
  }
}

// =====================================================
// FLASH FUNCTIONS
// =====================================================

void unlockFlash() {

  writeEnable();

  csLow();

  SPI.transfer(0x01); // Write Status Register
  SPI.transfer(0x00); // Clear protection bits

  csHigh();

  waitReady();

  Serial.println("FLASH UNLOCKED");
}

// -----------------------------------------------------

void eraseChip() {

  writeEnable();

  csLow();

  SPI.transfer(0xC7); // Chip Erase

  csHigh();

  Serial.println("WAITING_FOR_ERASE...");

  waitReady();
}

// -----------------------------------------------------

void readJEDEC() {

  csLow();

  SPI.transfer(0x9F);

  uint8_t b1 = SPI.transfer(0x00);
  uint8_t b2 = SPI.transfer(0x00);
  uint8_t b3 = SPI.transfer(0x00);

  csHigh();

  Serial.print("JEDEC RAW: ");

  if (b1 < 16) Serial.print("0");
  Serial.print(b1, HEX);
  Serial.print(" ");

  if (b2 < 16) Serial.print("0");
  Serial.print(b2, HEX);
  Serial.print(" ");

  if (b3 < 16) Serial.print("0");
  Serial.println(b3, HEX);

  if (b1 == 0xFF || b1 == 0x00) {
    Serial.println("FLASH NOT DETECTED");
  }
  else {
    Serial.println("FLASH DETECTED");
  }
}

// =====================================================
// WRITE / READ
// =====================================================

void writePage(uint32_t addr, uint8_t* data, uint16_t len) {

  if (len > 256) {

    Serial.println("ERR:PAGE_TOO_LARGE");

    return;
  }

  writeEnable();

  csLow();

  SPI.transfer(0x02); // Page Program

  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >> 8) & 0xFF);
  SPI.transfer(addr & 0xFF);

  for (uint16_t i = 0; i < len; i++) {
    SPI.transfer(data[i]);
  }

  csHigh();

  waitReady();

  Serial.println("PAGE WRITE COMPLETE");
}

// -----------------------------------------------------

uint8_t readByte(uint32_t addr) {

  csLow();

  SPI.transfer(0x03); // Read Data

  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >> 8) & 0xFF);
  SPI.transfer(addr & 0xFF);

  uint8_t val = SPI.transfer(0x00);

  csHigh();

  return val;
}

// =====================================================
// GLOBALS
// =====================================================

uint32_t writeAddress = 0;

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  pinMode(CS, OUTPUT);

  digitalWrite(CS, HIGH);

  SPI.begin(SCK, MISO, MOSI, CS);

  SPI.beginTransaction(
    SPISettings(1000000, MSBFIRST, SPI_MODE0)
  );

  Serial.println();
  Serial.println("=================================");
  Serial.println("ESP32 SPI FLASH TERMINAL");
  Serial.println("=================================");

  readJEDEC();

  unlockFlash();

  Serial.println();
  Serial.println("COMMANDS:");
  Serial.println("ERASE");
  Serial.println("STATUS");
  Serial.println("WRITE:your text");
  Serial.println("READ:number_of_bytes");
  Serial.println();

  Serial.println("READY");
}

// =====================================================
// LOOP
// =====================================================

void loop() {

  if (Serial.available()) {

    String cmd = Serial.readStringUntil('\n');

    cmd.trim();

    // =================================================
    // ERASE
    // =================================================

    if (cmd == "ERASE") {

      Serial.println("ERASING CHIP...");

      eraseChip();

      writeAddress = 0;

      Serial.println("OK:ERASED");
    }

    // =================================================
    // STATUS
    // =================================================

    else if (cmd == "STATUS") {

      Serial.print("CURRENT ADDRESS: ");

      Serial.println(writeAddress);
    }

    // =================================================
    // WRITE
    // =================================================

    else if (cmd.startsWith("WRITE:")) {

      String text = cmd.substring(6);

      uint16_t len = text.length();

      if (len == 0) {

        Serial.println("ERR:NO_TEXT");

        return;
      }

      if (len > 256) {

        Serial.println("ERR:MAX_256_BYTES");

        return;
      }

      writePage(
        writeAddress,
        (uint8_t*)text.c_str(),
        len
      );

      Serial.print("WRITTEN TEXT: ");
      Serial.println(text);

      writeAddress += len;

      Serial.print("NEW ADDRESS: ");
      Serial.println(writeAddress);

      Serial.println("OK:TEXT_WRITTEN");
    }

    // =================================================
    // READ
    // =================================================

    else if (cmd.startsWith("READ:")) {

      int len = cmd.substring(5).toInt();

      if (len <= 0) {

        Serial.println("ERR:INVALID_LENGTH");

        return;
      }

      Serial.println("HEX DATA:");

      for (int i = 0; i < len; i++) {

        uint8_t val = readByte(i);

        if (val < 16) {
          Serial.print("0");
        }

        Serial.print(val, HEX);
        Serial.print(" ");
      }

      Serial.println();
      Serial.println();

      Serial.println("ASCII DATA:");

      for (int i = 0; i < len; i++) {

        uint8_t val = readByte(i);

        if (val >= 32 && val <= 126) {
          Serial.write(val);
        }
        else {
          Serial.print(".");
        }
      }

      Serial.println();
      Serial.println("OK:READ_DONE");
    }

    // =================================================
    // UNKNOWN COMMAND
    // =================================================

    else {

      Serial.println("ERR:UNKNOWN_COMMAND");
    }
  }
}