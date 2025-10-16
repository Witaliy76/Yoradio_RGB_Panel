#ifndef sdmanager_h
#define sdmanager_h
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "vfs_api.h"
#include "sd_diskio.h"
#include "options.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class SDManager : public SDFS {
  public:
    bool ready;
    SemaphoreHandle_t sdMutex;
  public:
    SDManager(FSImplPtr impl) : SDFS(impl) {
        sdMutex = xSemaphoreCreateMutex();
    }
    ~SDManager() {
        if(sdMutex != NULL) {
            vSemaphoreDelete(sdMutex);
        }
    }
    bool start();
    void stop();
    bool cardPresent();
    void listSD(File &plSDfile, File &plSDindex, const char * dirname, uint8_t levels);
    void indexSDPlaylist();
  private:
    uint32_t _sdFCount;
  private:
    bool _checkNoMedia(const char* path, bool mutexAlreadyTaken = false);
    bool _endsWith (const char* base, const char* str);
};

extern SDManager sdman;
#if defined(SD_SPIPINS) || SD_HSPI
extern SPIClass  SDSPI;
#endif
#endif
