#include <Arduino.h>
#include <SPI.h>

#define CS    5
#define SCK   18
#define MISO  19
#define MOSI  23

// ── Low level helpers ──────────────────────────────

void csLow()  { digitalWrite(CS, LOW);  }
void csHigh() { digitalWrite(CS, HIGH); }

void waitReady() {
  csLow();
  SPI.transfer(0x05); // Read Status Register
  while (SPI.transfer(0x00) & 0x01); // wait while BUSY bit set
  csHigh();
}

void writeEnable() {
  csLow();
  SPI.transfer(0x06); // WREN command
  csHigh();
  delayMicroseconds(10);
}

// ── Public functions ───────────────────────────────

void eraseChip() {
  writeEnable();
  csLow();
  SPI.transfer(0xC7); // Full chip erase
  csHigh();
  waitReady(); // takes a few seconds
}

void writePage(uint32_t addr, uint8_t* data, uint16_t len) {
  writeEnable();
  csLow();
  SPI.transfer(0x02);           // Page Program
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >> 8)  & 0xFF);
  SPI.transfer( addr        & 0xFF);
  for (uint16_t i = 0; i < len; i++) SPI.transfer(data[i]);
  csHigh();
  waitReady();
}

uint8_t readByte(uint32_t addr) {
  csLow();
  SPI.transfer(0x03); // Read
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >> 8)  & 0xFF);
  SPI.transfer( addr        & 0xFF);
  uint8_t val = SPI.transfer(0x00);
  csHigh();
  return val;
}

// ── Main ───────────────────────────────────────────

uint32_t writeAddress = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  SPI.begin(SCK, MISO, MOSI, CS);
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  Serial.println("READY");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "ERASE") {
      Serial.println("Erasing...");
      eraseChip();
      writeAddress = 0;
      Serial.println("OK:ERASED");
    }

    else if (cmd.startsWith("WRITE:")) {
      int sep = cmd.indexOf(':', 6);
      uint32_t fileSize = cmd.substring(sep + 1).toInt();

      Serial.print("OK:READY:");
      Serial.println(writeAddress);

      uint32_t received = 0;
      uint8_t buf[256];

      while (received < fileSize) {
        uint16_t chunk = min((uint32_t)256, fileSize - received);
        uint16_t got = 0;
        unsigned long timeout = millis();

        while (got < chunk && (millis() - timeout) < 5000) {
          if (Serial.available()) {
            buf[got++] = Serial.read();
            timeout = millis();
          }
        }

        if (got > 0) {
          writePage(writeAddress, buf, got);
          writeAddress += got;
          received += got;
        } else {
          Serial.println("ERR:TIMEOUT");
          return;
        }
      }
      Serial.println("OK:DONE");
    }

    else if (cmd == "STATUS") {
      Serial.print("OK:ADDR:");
      Serial.println(writeAddress);
    }
  }
}