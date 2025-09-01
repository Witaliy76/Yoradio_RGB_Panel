#ifndef sd_spi_config_h
#define sd_spi_config_h

#include <Arduino.h>
#include <SPI.h>
#include "options.h"

// Конфигурация отдельной SPI шины для SD карты
// Используем FSPI (SPI2_HOST) для избежания конфликтов с дисплеем ST7701
class SDSPIConfig {
private:
    static SPIClass* sdSPI;
    static bool initialized;
    
public:
    static bool init() {
        if (initialized) return true;
        
        // Создаем отдельную SPI шину для SD карты
        #ifdef FSPI
            sdSPI = new SPIClass(FSPI);
        #else
            sdSPI = new SPIClass(SPI2_HOST);
        #endif
        
        // Инициализация с пользовательскими пинами
        if (sdSPI->begin(SD_SCK, SD_MISO, SD_MOSI, SDC_CS)) {
            initialized = true;
            return true;
        }
        return false;
    }
    
    static SPIClass* getSPI() {
        if (!initialized) {
            init();
        }
        return sdSPI;
    }
    
    static void deinit() {
        if (sdSPI && initialized) {
            sdSPI->end();
            delete sdSPI;
            sdSPI = nullptr;
            initialized = false;
        }
    }
    
    static bool isInitialized() {
        return initialized;
    }
};

// Статические члены класса определены в sd_spi_config.cpp

#endif // sd_spi_config_h
