#include "sdmanager.h"
#include "sd_spi_config.h"


// Тестовая функция для проверки SD карты
void testSDCard() {
    Serial.println("\n=== SD Card Test ===");
    
    // Диагностика конфигурации
    Serial.println("=== SD Card Configuration ===");
    Serial.printf("  USE_SD: enabled\n");
    Serial.printf("  SD_DEBUG_ENABLED: enabled\n");
    Serial.printf("  SD_HSPI: enabled\n");
    Serial.println();
    
    Serial.println("=== SD Card Pin Status ===");
    Serial.printf("  CS: GPIO%d (Chip Select)\n", SDC_CS);
    Serial.printf("  SCK: GPIO%d (Clock)\n", SD_SCK);
    Serial.printf("  MISO: GPIO%d (Data In)\n", SD_MISO);
    Serial.printf("  MOSI: GPIO%d (Data Out)\n", SD_MOSI);
    Serial.printf("  SPI Speed: %d Hz\n", SDSPISPEED);
    Serial.printf("  HSPI: %s\n", SD_HSPI ? "true" : "false");
    Serial.println();
    
    // Проверка инициализации SPI
    if (!SDSPIConfig::init()) {
        Serial.println("❌ SPI initialization failed");
        return;
    }
    Serial.println("✅ SPI initialized successfully");
    
    // Проверка инициализации SD карты
    if (!sdman.start()) {
        Serial.println("❌ SD card initialization failed");
        return;
    }
    Serial.println("✅ SD card initialized successfully");
    
    // Проверка наличия карты
    if (!sdman.cardPresent()) {
        Serial.println("❌ SD card not present");
        return;
    }
    Serial.println("✅ SD card is present");
    
    // Получение информации о карте
    uint8_t cardType = sdman.cardType();
    uint64_t cardSize = sdman.cardSize();
    
    Serial.printf("Card Type: %s\n", 
        cardType == CARD_MMC ? "MMC" :
        cardType == CARD_SD ? "SD" :
        cardType == CARD_SDHC ? "SDHC" : "UNKNOWN");
    
    Serial.printf("Card Size: %.2f GB\n", cardSize / (1024.0 * 1024.0 * 1024.0));
    
    // Тест чтения корневой директории
    File root = sdman.open("/");
    if (!root) {
        Serial.println("❌ Failed to open root directory");
        return;
    }
    
    Serial.println("✅ Root directory opened successfully");
    Serial.println("Files in root directory:");
    
    File file = root.openNextFile();
    int fileCount = 0;
    while (file && fileCount < 10) {
        if (!file.isDirectory()) {
            Serial.printf("  %s (%d bytes)\n", file.name(), file.size());
            fileCount++;
        }
        file = root.openNextFile();
    }
    
    if (fileCount == 0) {
        Serial.println("  No files found");
    }
    
    root.close();
    
    // Тест записи файла
    File testFile = sdman.open("/test.txt", "w");
    if (testFile) {
        testFile.println("SD Card Test - OK");
        testFile.close();
        Serial.println("✅ Test file written successfully");
        
        // Проверяем, что файл создался
        if (sdman.exists("/test.txt")) {
            Serial.println("✅ Test file exists");
            
            // Читаем файл
            File readFile = sdman.open("/test.txt", "r");
            if (readFile) {
                String content = readFile.readString();
                Serial.printf("File content: %s", content.c_str());
                readFile.close();
                
                // Удаляем тестовый файл
                sdman.remove("/test.txt");
                Serial.println("✅ Test file removed");
            }
        }
    } else {
        Serial.println("❌ Failed to create test file");
    }
    
    Serial.println("=== SD Card Test Complete ===\n");
}
