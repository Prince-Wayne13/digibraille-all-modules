/**
 * Digibraille v2 - SPI Flash Audio Storage (PERSISTENT FILE TABLE)
 * ESP32 Firmware for PlatformIO
 * 
 * Protocol:
 *   ERASE\n                    → Erase chip + reset table
 *   WRITE:filename:size\n      → Prepare receive (reply: OK:READY:addr\n)
 *   [raw binary: size bytes]   → Flash write (reply: OK:DONE\n)
 *   LIST\n                     → List stored files
 *   DUMP\n                     → Auto-dump first 4 files
 *   PING\n                     → Handshake test
 */

#include <Arduino.h>
#include <SPI.h>

// ─── SPI PIN CONFIG ─────────────────────────────────────────
#define CS    5
#define SCK   18
#define MISO  19
#define MOSI  23

// ─── FLASH SETTINGS ─────────────────────────────────────────
#define FLASH_PAGE_SIZE   256
#define FLASH_SECTOR_SIZE 4096
#define MAX_FILES         32
#define FILENAME_MAX_LEN  48

// ─── PERSISTENT TABLE CONFIG ────────────────────────────────
#define TABLE_ADDR        0x0000          // First 4KB sector reserved for table
#define FILE_START_ADDR   0x1000          // Audio files start after table sector

// ─── FILE TABLE STRUCT (Packed to avoid padding issues) ─────
#pragma pack(push, 1)
struct FileEntry {
  char name[FILENAME_MAX_LEN];
  uint32_t address;
  uint32_t size;
  bool used;
};
#pragma pack(pop)

// ─── GLOBALS (Declared ONCE) ────────────────────────────────
uint32_t writeAddress = FILE_START_ADDR;
FileEntry fileTable[MAX_FILES];

// Binary receive state
bool receivingBinary = false;
uint32_t expectedBytes = 0;
uint32_t receivedBytes = 0;
char currentFilename[FILENAME_MAX_LEN];
uint8_t pageBuffer[FLASH_PAGE_SIZE];
uint16_t pageOffset = 0;

// ─── FORWARD DECLARATIONS (Fixes "not declared in scope") ───
void initFileTable();
void saveFileTable();
void loadFileTable();
// ─────────────────────────────────────────────────────────────

// ─── SPI HELPERS ────────────────────────────────────────────
inline void csLow()  { digitalWrite(CS, LOW); }
inline void csHigh() { digitalWrite(CS, HIGH); }

void writeEnable() {
  csLow();
  SPI.transfer(0x06);
  csHigh();
  delayMicroseconds(1);
}

uint8_t readStatusRegister() {
  csLow();
  SPI.transfer(0x05);
  uint8_t status = SPI.transfer(0x00);
  csHigh();
  return status;
}

void waitReady() {
  unsigned long start = millis();
  while (readStatusRegister() & 0x01) {
    if (millis() - start > 30000) {
      Serial.println("ERR:FLASH_TIMEOUT");
      return;
    }
    delay(1);
  }
}

// ─── FLASH OPERATIONS ───────────────────────────────────────
void unlockFlash() {
  writeEnable();
  csLow();
  SPI.transfer(0x01);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  csHigh();
  waitReady();
  Serial.println("FLASH:UNLOCKED");
}

void eraseChip() {
  writeEnable();
  csLow();
  SPI.transfer(0xC7);
  csHigh();
  Serial.println("ERASING CHIP...");
  waitReady();
  Serial.println("ERASE:COMPLETE");
}

void readJEDEC() {
  csLow();
  SPI.transfer(0x9F);
  uint8_t m = SPI.transfer(0x00);
  uint8_t t = SPI.transfer(0x00);
  uint8_t c = SPI.transfer(0x00);
  csHigh();
  Serial.printf("FLASH:JEDEC=%02X%02X%02X ", m, t, c);
  if (m == 0xFF || m == 0x00) Serial.println("NOT_DETECTED");
  else Serial.printf("DETECTED(capacity=%02X)\n", c);
}

void writePage(uint32_t addr, uint8_t* data, uint16_t len) {
  if (len == 0 || len > FLASH_PAGE_SIZE) return;
  writeEnable();
  csLow();
  SPI.transfer(0x02);
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >> 8) & 0xFF);
  SPI.transfer(addr & 0xFF);
  for (uint16_t i = 0; i < len; i++) SPI.transfer(data[i]);
  csHigh();
  waitReady();
}

void readBytes(uint32_t addr, uint8_t* buffer, uint32_t len) {
  csLow();
  SPI.transfer(0x03);
  SPI.transfer((addr >> 16) & 0xFF);
  SPI.transfer((addr >> 8) & 0xFF);
  SPI.transfer(addr & 0xFF);
  for (uint32_t i = 0; i < len; i++) buffer[i] = SPI.transfer(0x00);
  csHigh();
}

// ─── PERSISTENT TABLE FUNCTIONS ─────────────────────────────
void saveFileTable() {
  // Erase table sector first (required before reprogramming flash)
  writeEnable();
  csLow();
  SPI.transfer(0x20); // Sector Erase (4KB)
  SPI.transfer((TABLE_ADDR >> 16) & 0xFF);
  SPI.transfer((TABLE_ADDR >> 8) & 0xFF);
  SPI.transfer(TABLE_ADDR & 0xFF);
  csHigh();
  waitReady();

  // Write table data in 256-byte pages
  uint16_t totalSize = sizeof(fileTable);
  for (uint16_t i = 0; i < totalSize; i += FLASH_PAGE_SIZE) {
    uint16_t chunk = (totalSize - i < FLASH_PAGE_SIZE) ? (totalSize - i) : FLASH_PAGE_SIZE;
    writePage(TABLE_ADDR + i, ((uint8_t*)fileTable) + i, chunk);
  }
  Serial.println("TABLE:SAVED");
}

void loadFileTable() {
  readBytes(TABLE_ADDR, (uint8_t*)fileTable, sizeof(fileTable));
  
  // Validate: if first byte is 0xFF (erased flash), clear table
  if (fileTable[0].name[0] == (char)0xFF || fileTable[0].name[0] == 0x00) {
    initFileTable();
    writeAddress = FILE_START_ADDR;
    Serial.println("TABLE:FRESH/ERASED");
  } else {
    // Reconstruct writeAddress from loaded table
    writeAddress = FILE_START_ADDR;
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
      if (fileTable[i].used) {
        count++;
        uint32_t end = fileTable[i].address + fileTable[i].size;
        if (end > writeAddress) writeAddress = end;
      }
    }
    Serial.printf("TABLE:LOADED (%d files, next write @ 0x%08X)\n", count, writeAddress);
  }
}

// ─── FILE TABLE MANAGEMENT (RAM) ────────────────────────────
void initFileTable() {
  for (int i = 0; i < MAX_FILES; i++) {
    fileTable[i].used = false;
    fileTable[i].name[0] = '\0';
    fileTable[i].address = 0;
    fileTable[i].size = 0;
  }
}

int findFile(const char* name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (fileTable[i].used && strcmp(fileTable[i].name, name) == 0) return i;
  }
  return -1;
}

bool addFileEntry(const char* name, uint32_t addr, uint32_t size) {
  int existing = findFile(name);
  if (existing >= 0) {
    fileTable[existing].address = addr;
    fileTable[existing].size = size;
    return true;
  }
  for (int i = 0; i < MAX_FILES; i++) {
    if (!fileTable[i].used) {
      strncpy(fileTable[i].name, name, FILENAME_MAX_LEN - 1);
      fileTable[i].name[FILENAME_MAX_LEN - 1] = '\0';
      fileTable[i].address = addr;
      fileTable[i].size = size;
      fileTable[i].used = true;
      return true;
    }
  }
  return false;
}

int countFiles() {
  int c = 0;
  for (int i = 0; i < MAX_FILES; i++) if (fileTable[i].used) c++;
  return c;
}

// ─── BINARY RECEIVE STATE MACHINE ───────────────────────────
void flushPageBuffer() {
  if (pageOffset > 0) {
    writePage(writeAddress, pageBuffer, pageOffset);
    writeAddress += pageOffset;
    pageOffset = 0;
  }
}

void handleBinaryByte(uint8_t b) {
  if (!receivingBinary) return;
  pageBuffer[pageOffset++] = b;
  receivedBytes++;
  
  if (pageOffset >= FLASH_PAGE_SIZE) flushPageBuffer();
  
  if (receivedBytes >= expectedBytes) {
    flushPageBuffer();
    uint32_t startAddr = writeAddress - expectedBytes;
    if (addFileEntry(currentFilename, startAddr, expectedBytes)) {
      saveFileTable(); // ← PERSIST TO FLASH
      Serial.println("OK:DONE");
    } else {
      Serial.println("ERR:TABLE_FULL");
    }
    receivingBinary = false;
    expectedBytes = 0; receivedBytes = 0;
    currentFilename[0] = '\0'; pageOffset = 0;
  }
}

// ─── STREAM FILE TO SERIAL (HEX + ASCII) ──────────────────
void streamFileHex(const char* filename) {
  int idx = findFile(filename);
  if (idx < 0) { Serial.println("ERR:FILE_NOT_FOUND"); return; }

  uint32_t addr = fileTable[idx].address;
  uint32_t size = fileTable[idx].size;
  Serial.printf("\n>>> STREAMING: %s (%lu bytes) @ 0x%08X\n", filename, size, addr);

  const uint16_t LINE = 16;
  uint8_t buf[LINE];
  uint32_t offset = 0;

  while (offset < size) {
    uint16_t len = (offset + LINE <= size) ? LINE : (size - offset);
    readBytes(addr + offset, buf, len);
    Serial.printf("%08X: ", offset);
    for (uint16_t i = 0; i < len; i++) {
      if (buf[i] < 0x10) Serial.print("0");
      Serial.print(buf[i], HEX); Serial.print(" ");
    }
    for (uint16_t i = len; i < LINE; i++) Serial.print("   ");
    Serial.print("| ");
    for (uint16_t i = 0; i < len; i++) {
      char c = (buf[i] >= 32 && buf[i] <= 126) ? buf[i] : '.';
      Serial.write(c);
    }
    Serial.println();
    offset += len;
    if (offset % 256 == 0) { yield(); delay(1); }
  }
  Serial.println(">>> STREAM COMPLETE\n");
}

void dump_first_4_auto() {
  Serial.println("\n🔍 AUTO-DUMP: First 4 file slots...");
  int dumped = 0;
  for (int i = 0; i < MAX_FILES && dumped < 4; i++) {
    if (fileTable[i].used) {
      Serial.printf("📂 [%d] %s (%lu bytes)\n", dumped, fileTable[i].name, fileTable[i].size);
      streamFileHex(fileTable[i].name);
      dumped++;
      Serial.println("---");
    }
  }
  if (dumped == 0) Serial.println("⚠️ No files stored.");
  else Serial.printf("✅ Dumped %d file(s)\n", dumped);
}

// ─── SETUP ──────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  SPI.begin(SCK, MISO, MOSI);
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  
  Serial.println("\n=== DIGIBRAILLE FLASH v2 ===");
  readJEDEC();
  unlockFlash();
  
  // Load persistent table from flash sector 0
  loadFileTable();
  
  Serial.println("READY");
}

// ─── MAIN LOOP ──────────────────────────────────────────────
void loop() {
  if (receivingBinary) {
    while (Serial.available() && receivedBytes < expectedBytes) {
      handleBinaryByte(Serial.read());
    }
    return;
  }
  
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) return;
    
    if (cmd == "ERASE") {
      Serial.println("ERASE:START");
      eraseChip();
      initFileTable();
      writeAddress = FILE_START_ADDR;
      saveFileTable(); // Save clean table state
      Serial.println("OK:ERASED");
    }
    else if (cmd == "STATUS") {
      Serial.printf("STATUS:ADDR=%lu\n", writeAddress);
      Serial.printf("STATUS:FILES=%d/%d\n", countFiles(), MAX_FILES);
      Serial.println("OK:STATUS");
    }
    else if (cmd.startsWith("WRITE:")) {
      int nameEnd = cmd.indexOf(':', 6);
      if (nameEnd < 0) { Serial.println("ERR:BAD_FORMAT"); return; }
      String fname = cmd.substring(6, nameEnd);
      String fsizeStr = cmd.substring(nameEnd + 1);
      if (fname.length() == 0 || fname.length() >= FILENAME_MAX_LEN) {
        Serial.println("ERR:BAD_FILENAME"); return;
      }
      uint32_t fsize = fsizeStr.toInt();
      if (fsize == 0) { Serial.println("ERR:BAD_SIZE"); return; }
      
      strncpy(currentFilename, fname.c_str(), FILENAME_MAX_LEN - 1);
      currentFilename[FILENAME_MAX_LEN - 1] = '\0';
      expectedBytes = fsize; receivedBytes = 0; pageOffset = 0;
      receivingBinary = true;
      Serial.printf("OK:READY:%lu\n", writeAddress);
    }
    else if (cmd == "LIST") {
      Serial.println("FILES:START");
      for (int i = 0; i < MAX_FILES; i++) {
        if (fileTable[i].used) {
          Serial.printf("FILE:%s:%lu:%lu\n", fileTable[i].name, fileTable[i].size, fileTable[i].address);
        }
      }
      Serial.println("FILES:END");
      Serial.println("OK:LIST_DONE");
    }
    else if (cmd == "DUMP") {
      dump_first_4_auto();
    }
    else if (cmd == "PING") {
      Serial.println("PONG");
    }
    else {
      Serial.printf("ERR:UNKNOWN:[%s]\n", cmd.c_str());
    }
  }
}