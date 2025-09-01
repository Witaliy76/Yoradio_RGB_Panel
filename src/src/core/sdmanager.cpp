#if SDC_CS!=255

#define USE_SD
#include "sdmanager.h"
#include "display.h"
#include "player.h"
#include "sd_spi_config.h"

// Используем новую конфигурацию SPI
#define SDREALSPI (*SDSPIConfig::getSPI())

SDManager sdman(FSImplPtr(new VFSImpl()));

bool SDManager::start(){
  if(xSemaphoreTake(sdMutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }
  
  // Инициализация SPI шины для SD карты
  if (!SDSPIConfig::init()) {
    Serial.println("❌ Failed to initialize SD SPI bus");
    xSemaphoreGive(sdMutex);
    return false;
  }
  
  // Диагностика при включенной отладке
  #ifdef SD_DEBUG_ENABLED
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
  #endif
  
  // Попытки инициализации SD карты с повторными попытками
  ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  vTaskDelay(10);
  if(!ready) {
    Serial.println("⚠️  First SD init attempt failed, retrying...");
    ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  }
  vTaskDelay(10);
  if(!ready) {
    Serial.println("⚠️  Second SD init attempt failed, retrying...");
    ready = begin(SDC_CS, SDREALSPI, SDSPISPEED);
  }
  
  if (ready) {
    Serial.println("✅ SD card initialized successfully");
  } else {
    Serial.println("❌ SD card initialization failed");
  }
  
  xSemaphoreGive(sdMutex);
  return ready;
}

void SDManager::stop(){
  if(xSemaphoreTake(sdMutex, portMAX_DELAY) != pdTRUE) {
    return;
  }
  
  end();
  ready = false;
  
  // Завершение работы SPI шины
  SDSPIConfig::deinit();
  
  #ifdef SD_DEBUG_ENABLED
    Serial.println("🔄 SD card stopped and SPI bus deinitialized");
  #endif
  
  xSemaphoreGive(sdMutex);
}

#include "diskio_impl.h"
bool SDManager::cardPresent() {
  if(xSemaphoreTake(sdMutex, portMAX_DELAY) != pdTRUE) {
    return false;
  }

  if(!ready) {
    xSemaphoreGive(sdMutex);
    return false;
  }
  
  if(sectorSize()<1) {
    xSemaphoreGive(sdMutex);
    return false;
  }
  
  uint8_t buff[sectorSize()] = { 0 };
  bool bread = readRAW(buff, 1);
  
  xSemaphoreGive(sdMutex);
  if(sectorSize()>0 && !bread) return false;
  return bread;
}

bool SDManager::_checkNoMedia(const char* path, bool mutexAlreadyTaken){
  bool mutexTakenHere = false;
  
  // Захватываем mutex только если он еще не захвачен
  if (!mutexAlreadyTaken) {
    if(xSemaphoreTake(sdMutex, portMAX_DELAY) != pdTRUE) {
      return false;
    }
    mutexTakenHere = true;
  }
  
  // Основная логика проверки .nomedia файла
  char nomedia[BUFLEN]= {0};
  strlcat(nomedia, path, BUFLEN);
  strlcat(nomedia, "/.nomedia", BUFLEN);
  bool nm = exists(nomedia);
  
  // Освобождаем mutex только если мы его захватывали
  if (mutexTakenHere) {
    xSemaphoreGive(sdMutex);
  }
  
  return nm;
}

bool SDManager::_endsWith (const char* base, const char* str) {
  int slen = strlen(str) - 1;
  const char *p = base + strlen(base) - 1;
  while(p > base && isspace(*p)) p--;
  p -= slen;
  if (p < base) return false;
  return (strncmp(p, str, slen) == 0);
}

void SDManager::listSD(File &plSDfile, File &plSDindex, const char* dirname, uint8_t levels) {
    if(xSemaphoreTake(sdMutex, portMAX_DELAY) != pdTRUE) {
        return;
    }
    
    File root = sdman.open(dirname);
    if (!root) {
        xSemaphoreGive(sdMutex);
        return;
    }
    if (!root.isDirectory()) {
        xSemaphoreGive(sdMutex);
        return;
    }

    uint32_t pos = 0;
    char* filePath;
    while (true) {
        vTaskDelay(2);
        player.loop();
        bool isDir;
        String fileName = root.getNextFileName(&isDir);
        if (fileName.isEmpty()) break;
        
        filePath = (char*)malloc(fileName.length() + 1);
        if (filePath == NULL) {
            break;
        }
        strcpy(filePath, fileName.c_str());
        const char* fn = strrchr(filePath, '/') + 1;
        if (isDir) {
            if (levels && !_checkNoMedia(filePath, true)) {
                xSemaphoreGive(sdMutex);  // Освобождаем перед рекурсией
                listSD(plSDfile, plSDindex, filePath, levels - 1);
                if(xSemaphoreTake(sdMutex, portMAX_DELAY) != pdTRUE) {
                    free(filePath);
                    root.close();
                    return;
                }
            }
        } else {
            if (_endsWith(strlwr((char*)fn), ".mp3") || _endsWith(fn, ".m4a") || _endsWith(fn, ".aac") ||
                _endsWith(fn, ".wav") || _endsWith(fn, ".flac")) {
                pos = plSDfile.position();
                plSDfile.printf("%s\t%s\t0\n", fn, filePath);
                plSDindex.write((uint8_t*)&pos, 4);
                Serial.print(".");
                if(display.mode()==SDCHANGE) display.putRequest(SDFILEINDEX, _sdFCount+1);
                _sdFCount++;
                if (_sdFCount % 64 == 0) Serial.println();
            }
        }
        free(filePath);
    }
    root.close();
    xSemaphoreGive(sdMutex);
}

void SDManager::indexSDPlaylist() {
    if(xSemaphoreTake(sdMutex, portMAX_DELAY) != pdTRUE) {
        return;
    }
    
    _sdFCount = 0;
    if(exists(PLAYLIST_SD_PATH)) remove(PLAYLIST_SD_PATH);
    if(exists(INDEX_SD_PATH)) remove(INDEX_SD_PATH);
    File playlist = open(PLAYLIST_SD_PATH, "w", true);
    if (!playlist) {
        xSemaphoreGive(sdMutex);
        return;
    }
    File index = open(INDEX_SD_PATH, "w", true);
    
    xSemaphoreGive(sdMutex);  // Освобождаем перед длительной операцией
    
    listSD(playlist, index, "/", SD_MAX_LEVELS);
    
    if(xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {
        index.flush();
        index.close();
        playlist.flush();
        playlist.close();
        xSemaphoreGive(sdMutex);
    }
    
    Serial.println();
    delay(50);
}
#endif


