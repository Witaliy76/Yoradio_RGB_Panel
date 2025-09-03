/**
 * @file touchscreen.cpp
 * @brief Драйвер для работы с сенсорным экраном
 * @author W76W
 * @version 1.0
 * @date 2024
 * 
 * Реализация обработки касаний и свайпов для сенсорного экрана
 * Поддерживает:
 * - Одиночные касания (короткое/долгое)
 * - Свайпы влево/вправо для управления громкостью
 * - Свайпы вверх/вниз для управления списком станций
 * - Мультитач (два пальца) для переключения режимов
 * - Защиту от случайных касаний
 * - Улучшенную обработку последовательных свайпов
 */

#include "options.h"
#if (TS_MODEL!=TS_MODEL_UNDEFINED) && (DSP_MODEL!=DSP_DUMMY)

#include "touchscreen.h"
#include "config.h"
#include "controls.h"
#include "display.h"
#include "player.h"

#ifndef TS_X_MIN
  #define TS_X_MIN              400
#endif
#ifndef TS_X_MAX
  #define TS_X_MAX              3800
#endif
#ifndef TS_Y_MIN
  #define TS_Y_MIN              260
#endif
#ifndef TS_Y_MAX
  #define TS_Y_MAX              3800
#endif
#ifndef TS_STEPS
  #define TS_STEPS              40
#endif

#if TS_MODEL==TS_MODEL_XPT2046
  #ifdef TS_SPIPINS
    SPIClass  TSSPI(HSPI);
  #endif
  #include <XPT2046_Touchscreen.h>
  XPT2046_Touchscreen ts(TS_CS);
  typedef TS_Point TSPoint;
#elif TS_MODEL==TS_MODEL_GT911
  #include "../GT911_Touchscreen/TAMC_GT911.h"
  TAMC_GT911 ts = TAMC_GT911(TS_SDA, TS_SCL, TS_INT, TS_RST, 480, 480);
  typedef TP_Point TSPoint;
#elif TS_MODEL==TS_MODEL_AXS15231B
  #include "../AXS15231B_touch/AXS15231B_Touch.h"
  AXS15231B_Touch ts = AXS15231B_Touch(TS_SDA, TS_SCL, TS_INT, TS_RST, 0, 0);
  typedef TP_Point TSPoint;
#endif

/*
 * Константы для настройки работы тачскрина:
 * 
 * SWIPE_THRESHOLD (15) - минимальное расстояние в пикселях для определения свайпа
 *    - Меньшее значение: более чувствительные свайпы, но больше ложных срабатываний
 *    - Большее значение: более точные свайпы, но требуется большее движение
 * 
 * SWIPE_MIN_DISTANCE (20) - минимальное общее расстояние для подтверждения свайпа
 *    - Влияет на то, насколько длинным должен быть свайп
 *    - Помогает отфильтровать случайные касания
 * 
 * SWIPE_MAX_TIME (300) - максимальное время в миллисекундах для выполнения свайпа
 *    - Если свайп длится дольше, он игнорируется
 *    - Помогает отличить свайп от долгого касания
 * 
 * SWIPE_DEADZONE (3) - зона нечувствительности в пикселях
 *    - Игнорирует случайные микродвижения
 *    - Уменьшает дребезг при касании
 * 
 * SWIPE_ANGLE_THRESHOLD (22.5) - угол в градусах для определения направления свайпа
 *    - Определяет сектора для каждого направления (вверх, вниз, влево, вправо)
 *    - Меньший угол: более точное определение направления
 *    - Больший угол: более свободное определение направления
 * 
 * MOVEMENT_THRESHOLD (5) - порог для определения значимого движения
 *    - Минимальное накопленное движение для срабатывания
 *    - Влияет на плавность изменения громкости и переключения станций
 * 
 * MOVEMENT_HISTORY_SIZE (3) - количество точек для сглаживания движения
 *    - Большее значение: более плавное, но медленное срабатывание
 *    - Меньшее значение: более быстрое, но менее плавное срабатывание
 */

// Определения констант
#define SWIPE_THRESHOLD        30    // Минимальное расстояние для определения свайпа
#define SWIPE_MIN_DISTANCE     35    // Минимальное общее расстояние для свайпа
#define SWIPE_MAX_TIME        300    // Максимальное время для выполнения свайпа
#define SWIPE_DEADZONE         3     // Зона нечувствительности
#define SWIPE_ANGLE_THRESHOLD  22.5  // Угол для определения направления
#define MOVEMENT_THRESHOLD     8     // Порог значимого движения
#define MOVEMENT_HISTORY_SIZE  5     // Размер истории для сглаживания

// Специальные настройки для GT911 на квадратном экране 480x480
#if TS_MODEL==TS_MODEL_GT911
  // Уменьшенные пороги для высокого разрешения
  #define GT911_SWIPE_THRESHOLD      20    // Было 30, уменьшаем для 480x480
  #define GT911_SWIPE_MIN_DISTANCE   25    // Было 35, оптимизируем для квадратного экрана
  #define GT911_MOVEMENT_THRESHOLD   6     // Было 8, более чувствительно для 480x480
  #define GT911_SWIPE_DEADZONE       2     // Было 3, уменьшаем для точности
  #define GT911_SWIPE_ANGLE_THRESHOLD 25.0 // Было 22.5, увеличиваем для стабильности
#else
  // Стандартные настройки для других тачскринов
  #define GT911_SWIPE_THRESHOLD      SWIPE_THRESHOLD
  #define GT911_SWIPE_MIN_DISTANCE   SWIPE_MIN_DISTANCE
  #define GT911_MOVEMENT_THRESHOLD   MOVEMENT_THRESHOLD
  #define GT911_SWIPE_DEADZONE       SWIPE_DEADZONE
  #define GT911_SWIPE_ANGLE_THRESHOLD SWIPE_ANGLE_THRESHOLD
#endif

#ifndef TOUCH_MULTI_DELAY
  #define TOUCH_MULTI_DELAY       1000  // Задержка между мультитач-событиями
#endif
#ifndef TOUCH_MODE_DELAY
  #define TOUCH_MODE_DELAY        100   // Задержка после смены режима
#endif
#ifndef SWIPE_COOLDOWN
  #define SWIPE_COOLDOWN         100   // Задержка между свайпами
#endif
#ifndef TOUCH_BUFFER_TIMEOUT
  #define TOUCH_BUFFER_TIMEOUT    100   // Таймаут для буфера касаний
#endif
#ifndef TOUCH_READ_DELAY
  #define TOUCH_READ_DELAY        20    // Задержка чтения тачскрина
#endif

// Объединенная структура для касаний и движений
struct TouchPoint {
    uint16_t x;
    uint16_t y;
    uint32_t timestamp;
    bool valid;
    int16_t movement[2];  // [dx, dy]
};

// Функция фильтрации координат GT911 (как в AXS15231B)
#if TS_MODEL==TS_MODEL_GT911
bool TouchScreen::_filterGT911Coordinates(uint16_t rawX, uint16_t rawY) {
    // Проверяем валидность координат GT911 для экрана 480x480
    // GT911 выдает координаты в диапазоне 0-480, а не 100-3800
    if (rawX > 480 || rawY > 480) {
        // Координаты выходят за пределы экрана - игнорируем
        return false;
    }
    
    // Проверяем на "мертвые" координаты GT911
    if (rawX == 0 && rawY == 0) {
        // Нулевые координаты - игнорируем
        return false;
    }
    
    // Проверяем на подозрительно одинаковые координаты (только если они не в углах)
    if (rawX == rawY && rawX > 10 && rawX < 470) {
        // Возможные некорректные значения в центре экрана - игнорируем
        return false;
    }
    
    return true;
}
#endif

void TouchScreen::loop(){
  uint16_t touchX, touchY;
  static bool wastouched = true;
  static uint32_t touchLongPress;
  static tsDirection_e direct;
  static uint16_t touchVol, touchStation;
    static bool wasTwoFingerTouch = false;
    static uint32_t lastMultiTouchTime = 0;
    static uint32_t lastSwipeTime = 0;
  static bool wasSwiped = false;
    static uint32_t swipeStartTime = 0;
    static int16_t lastProcessedX = 0;
    static int16_t lastProcessedY = 0;
    static TouchPoint touchBuffer[3] = {0}; // Буфер для сглаживания касаний и движений
    static uint8_t bufferIndex = 0;
    static bool modeChangeInProgress = false;
    static uint8_t multiTouchCount = 0;  // Счетчик мультитач-событий

    if (!_checklpdelay(TOUCH_READ_DELAY, _touchdelay)) return;
    
#if TS_MODEL==TS_MODEL_GT911
  ts.read();
#endif
#if TS_MODEL==TS_MODEL_AXS15231B
  ts.read();
#endif
    
  bool istouched = _istouched();
  if(istouched){
    #if TS_MODEL==TS_MODEL_XPT2046
      TSPoint p = ts.getPoint();
      touchX = map(p.x, TS_X_MIN, TS_X_MAX, 0, _width);
      touchY = map(p.y, TS_Y_MIN, TS_Y_MAX, 0, _height);
    #elif TS_MODEL==TS_MODEL_GT911
      TSPoint p = ts.points[0];
      
      // Применяем фильтрацию координат GT911
      if (!_filterGT911Coordinates(p.x, p.y)) {
          // Координаты не прошли фильтр - игнорируем
          return;
      }
      
      // Маппирование координат GT911 (координаты уже в диапазоне 0-480)
      // GT911 выдает координаты напрямую, маппирование не требуется
      touchX = p.y;  // Оси перепутаны, но координаты уже правильные
      touchY = p.x;
      
      // Дополнительная проверка границ экрана
      if (touchX >= _width) touchX = _width - 1;
      if (touchY >= _height) touchY = _height - 1;
      if (touchX < 0) touchX = 0;
      if (touchY < 0) touchY = 0;
      
      // Обработка мультитача
      if (ts.touches > 1) {
                uint32_t currentTime = millis();
                if (currentTime - lastMultiTouchTime > TOUCH_MULTI_DELAY) {
                    multiTouchCount++;
                    
                    if (multiTouchCount >= 2) {
        wasTwoFingerTouch = true;
                        lastMultiTouchTime = currentTime;
                        multiTouchCount = 0;
                        
                        bool pir = player.isRunning();
                        
                        if(config.getMode()==PM_SDCARD) {
                            config.sdResumePos = player.getFilePos();
                        }
                        
                        if (display.mode() == PLAYER && !modeChangeInProgress) {
                            modeChangeInProgress = true;
                            display.putRequest(NEWMODE, SDCHANGE);
                            while(display.mode() != SDCHANGE) {
                                delay(10);
                            }
                            delay(TOUCH_MODE_DELAY/2);
                            
#ifdef USE_SD
        config.changeMode();
#endif
                            delay(TOUCH_MODE_DELAY);
                            
                            if (pir) {
                                player.sendCommand({PR_PLAY, config.getMode()==PM_WEB?config.store.lastStation:config.store.lastSdStation});
                            }
                            modeChangeInProgress = false;
                        }
                        
        return;
                    }
                } else {
                    if (millis() - lastMultiTouchTime > TOUCH_MULTI_DELAY) {
                        wasTwoFingerTouch = false;
                        multiTouchCount = 0;
                    }
                }
      }
    #elif TS_MODEL==TS_MODEL_AXS15231B
      TSPoint p = ts.points[0];
      touchX = p.x;
      touchY = p.y;
      
            // Обработка мультитача
      if (ts.touches > 1) {
                uint32_t currentTime = millis();
                if (currentTime - lastMultiTouchTime > TOUCH_MULTI_DELAY) {
                    multiTouchCount++;
                    
                    if (multiTouchCount >= 2) {
        wasTwoFingerTouch = true;
                        lastMultiTouchTime = currentTime;
                        multiTouchCount = 0;
                        
                        bool pir = player.isRunning();
                        
                        if(config.getMode()==PM_SDCARD) {
                            config.sdResumePos = player.getFilePos();
                        }
                        
                        if (display.mode() == PLAYER && !modeChangeInProgress) {
                            modeChangeInProgress = true;
                            display.putRequest(NEWMODE, SDCHANGE);
                            while(display.mode() != SDCHANGE) {
                                delay(10);
                            }
                            delay(TOUCH_MODE_DELAY/2);
                            
#ifdef USE_SD
        config.changeMode();
#endif
                            delay(TOUCH_MODE_DELAY);
                            
                            if (pir) {
                                player.sendCommand({PR_PLAY, config.getMode()==PM_WEB?config.store.lastStation:config.store.lastSdStation});
                            }
                            modeChangeInProgress = false;
                        }
                        
        return;
                    }
                } else {
                    if (millis() - lastMultiTouchTime > TOUCH_MULTI_DELAY) {
                        wasTwoFingerTouch = false;
                        multiTouchCount = 0;
                    }
                }
      }
    #endif
        
        // Буферизация касаний
        touchBuffer[bufferIndex].x = touchX;
        touchBuffer[bufferIndex].y = touchY;
        touchBuffer[bufferIndex].timestamp = millis();
        touchBuffer[bufferIndex].valid = true;
        bufferIndex = (bufferIndex + 1) % 3;
        
        // Вычисляем среднее значение из буфера с весами для GT911
        int32_t avgX = 0, avgY = 0;
        uint8_t validCount = 0;
        uint32_t currentTime = millis();
        
        #if TS_MODEL==TS_MODEL_GT911
        // Для GT911 используем весовую обработку (новые данные важнее)
        for (int i = 0; i < 3; i++) {
            if (touchBuffer[i].valid && (currentTime - touchBuffer[i].timestamp < TOUCH_BUFFER_TIMEOUT)) {
                // Новые данные получают больший вес
                uint8_t weight = (i == ((bufferIndex + 2) % 3)) ? 2 : 1;  // Самые новые данные
                avgX += touchBuffer[i].x * weight;
                avgY += touchBuffer[i].y * weight;
                validCount += weight;
            }
        }
        #else
        // Стандартная обработка для других тачскринов
        for (int i = 0; i < 3; i++) {
            if (touchBuffer[i].valid && (currentTime - touchBuffer[i].timestamp < TOUCH_BUFFER_TIMEOUT)) {
                avgX += touchBuffer[i].x;
                avgY += touchBuffer[i].y;
                validCount++;
            }
        }
        #endif
        
        if (validCount > 0) {
            touchX = avgX / validCount;
            touchY = avgY / validCount;
        }
        
        if (!wastouched) { /*     START TOUCH     */
      _oldTouchX = touchX;
      _oldTouchY = touchY;
      touchVol = touchY;  // Для громкости используем Y координату
      touchStation = touchX;  // Для станций используем X координату
      direct = TDS_REQUEST;
      touchLongPress=millis();
      swipeStartTime = millis();
      wasSwiped = false;
            // Сброс буфера
            for (int i = 0; i < 3; i++) {
                touchBuffer[i].valid = false;
                touchBuffer[i].movement[0] = 0;
                touchBuffer[i].movement[1] = 0;
            }
            lastProcessedX = touchX;
            lastProcessedY = touchY;
        } else { /*     SWIPE TOUCH     */
            int16_t dX = touchX - lastProcessedX;
            int16_t dY = touchY - lastProcessedY;
            
            // Проверяем минимальное движение
            if (abs(dX) > SWIPE_DEADZONE || abs(dY) > SWIPE_DEADZONE) {
                // Сохраняем движение в буфер
                touchBuffer[bufferIndex].movement[0] = dX;
                touchBuffer[bufferIndex].movement[1] = dY;
                
                // Вычисляем накопленное движение
                int32_t totalX = 0, totalY = 0;
                for (int i = 0; i < 3; i++) {
                    if (touchBuffer[i].valid) {
                        totalX += touchBuffer[i].movement[0];
                        totalY += touchBuffer[i].movement[1];
                    }
                }
                
                if (abs(totalX) > MOVEMENT_THRESHOLD || abs(totalY) > MOVEMENT_THRESHOLD) {
                    uint32_t currentTime = millis();
                    if (currentTime - lastSwipeTime >= SWIPE_COOLDOWN) {
      direct = _tsDirection(touchX, touchY);
                        
                        // Используем направление из _tsDirection
      switch (direct) {
        case TSD_LEFT:
        case TSD_RIGHT: {
            wasSwiped = true;
            touchLongPress = millis();
            if(display.mode()==PLAYER || display.mode()==VOL){
              display.putRequest(NEWMODE, VOL);
              // Влево-вправо для изменения громкости
              bool volumeUp = totalX < 0;  // totalX < 0 = влево (громкость ВВЕРХ), totalX > 0 = вправо (громкость ВНИЗ)
              controlsEvent(volumeUp);
              lastProcessedX = touchX;
              lastSwipeTime = currentTime;
            }
            break;
        }
        case TSD_UP:
        case TSD_DOWN: {
            wasSwiped = true;
            touchLongPress = millis();
            if(display.mode()==PLAYER || display.mode()==STATIONS){
                  display.putRequest(NEWMODE, STATIONS);
                  // Вверх-вниз для выбора станций
                  bool nextStation = totalY > 0;  // totalY > 0 = вниз (следующая станция), totalY < 0 = вверх (предыдущая станция)
                  controlsEvent(nextStation);
                  lastProcessedY = touchY;
                  lastSwipeTime = currentTime;
            }
            break;
        }
        default:
            break;
      }
    }
                }
            }
    }
    } else {
        if (wastouched) { /* END TOUCH */
            if (!wasSwiped && direct == TDS_REQUEST) {
                uint32_t pressTicks = millis() - touchLongPress;
                if (pressTicks < BTN_PRESS_TICKS * 2) {
                    if (pressTicks > 50) onBtnClick(EVT_BTNCENTER);
                } else {
          display.putRequest(NEWMODE, display.mode() == PLAYER ? STATIONS : PLAYER);
        }
      }
      direct = TSD_STAY;
      wasSwiped = false;
            modeChangeInProgress = false;
            
            // Очищаем буфер при отпускании
            for (int i = 0; i < 3; i++) {
                touchBuffer[i].valid = false;
                touchBuffer[i].movement[0] = 0;
                touchBuffer[i].movement[1] = 0;
            }
    }
  }
  wastouched = istouched;
}

bool TouchScreen::_checklpdelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

bool TouchScreen::_istouched(){
#if TS_MODEL==TS_MODEL_XPT2046
  return ts.touched();
#elif TS_MODEL==TS_MODEL_GT911
  return ts.isTouched;
#elif TS_MODEL==TS_MODEL_AXS15231B
  return ts.isTouched;
#endif
}

tsDirection_e TouchScreen::_tsDirection(uint16_t x, uint16_t y) {
    int16_t dX = x - _oldTouchX;
    int16_t dY = y - _oldTouchY;
    
    #if TS_MODEL==TS_MODEL_GT911
    // Специальная логика для GT911 на квадратном экране
    if (abs(dX) < GT911_SWIPE_DEADZONE && abs(dY) < GT911_SWIPE_DEADZONE) {
        return TDS_REQUEST;
    }
    
    // Для квадратного экрана используем более точную логику
    float angle = atan2(dY, dX) * 180.0 / PI;
    if (angle < 0) angle += 360.0;
    
    // Улучшенное определение направления для квадратного экрана
    if (abs(dX) > GT911_SWIPE_THRESHOLD || abs(dY) > GT911_SWIPE_THRESHOLD) {
        if (angle >= (360 - GT911_SWIPE_ANGLE_THRESHOLD) || angle < GT911_SWIPE_ANGLE_THRESHOLD) {
            _oldTouchX = x;
            _oldTouchY = y;
            return TSD_RIGHT;  // Вправо (0° ± 25°)
        } else if (angle >= (90 - GT911_SWIPE_ANGLE_THRESHOLD) && angle < (90 + GT911_SWIPE_ANGLE_THRESHOLD)) {
            _oldTouchX = x;
            _oldTouchY = y;
            return TSD_DOWN;   // Вниз (90° ± 25°)
        } else if (angle >= (180 - GT911_SWIPE_ANGLE_THRESHOLD) && angle < (180 + GT911_SWIPE_ANGLE_THRESHOLD)) {
            _oldTouchX = x;
            _oldTouchY = y;
            return TSD_LEFT;   // Влево (180° ± 25°)
        } else if (angle >= (270 - GT911_SWIPE_ANGLE_THRESHOLD) && angle < (270 + GT911_SWIPE_ANGLE_THRESHOLD)) {
            _oldTouchX = x;
            _oldTouchY = y;
            return TSD_UP;     // Вверх (270° ± 25°)
        }
    }
    #else
    // Стандартная логика для других тачскринов
    // Проверяем мертвую зону
    if (abs(dX) < SWIPE_DEADZONE && abs(dY) < SWIPE_DEADZONE) {
        return TDS_REQUEST;
    }
    
    // Вычисляем угол движения в градусах
    float angle = atan2(dY, dX) * 180.0 / PI;
    if (angle < 0) angle += 360.0;
    
    // Определяем направление на основе угла
    if (abs(dX) > SWIPE_THRESHOLD || abs(dY) > SWIPE_THRESHOLD) {
        if (angle >= (360 - SWIPE_ANGLE_THRESHOLD) || angle < SWIPE_ANGLE_THRESHOLD) {
            _oldTouchX = x;
            _oldTouchY = y;
            return TSD_DOWN;  // Вправо
        } else if (angle >= (90 - SWIPE_ANGLE_THRESHOLD) && angle < (90 + SWIPE_ANGLE_THRESHOLD)) {
            _oldTouchX = x;
            _oldTouchY = y;
            return TSD_RIGHT;  // Вниз
        } else if (angle >= (180 - SWIPE_ANGLE_THRESHOLD) && angle < (180 + SWIPE_ANGLE_THRESHOLD)) {
            _oldTouchX = x;
            _oldTouchY = y;
            return TSD_UP;  // Влево
        } else if (angle >= (270 - SWIPE_ANGLE_THRESHOLD) && angle < (270 + SWIPE_ANGLE_THRESHOLD)) {
            _oldTouchX = x;
            _oldTouchY = y;
            return TSD_LEFT;  // Вверх
        }
    }
    #endif
    
    return TDS_REQUEST;
}

void TouchScreen::init() {
    #if TS_MODEL==TS_MODEL_XPT2046
        #ifdef TS_SPIPINS
            TSSPI.begin(TS_SPIPINS);
            ts.begin(TSSPI);
        #else
            #if TS_HSPI
                ts.begin(SPI2);
            #else
                ts.begin();
            #endif
        #endif
        ts.setRotation(config.store.fliptouch?3:1);
    #endif
    #if TS_MODEL==TS_MODEL_GT911
        ts.begin();
        ts.setRotation(config.store.fliptouch?0:2);
        ts.setResolution(_width, _height);
        
        // Автоматическая настройка порогов для квадратного экрана
        if (_width == _height && _width >= 480) {
            // Для квадратного экрана 480x480 и больше используем оптимизированные настройки
            // Эти настройки уже определены в константах выше
        }
    #endif
    #if TS_MODEL==TS_MODEL_AXS15231B
        ts.begin();
        ts.setRotation(config.store.fliptouch?0:2);
    #endif
    _width  = dsp.width();
    _height = dsp.height();
    #if TS_MODEL==TS_MODEL_GT911
        ts.setResolution(_width, _height);
    #endif
    #if TS_MODEL==TS_MODEL_AXS15231B
        ts.setResolution(_width, _height);
    #endif
}

void TouchScreen::flip() {
    #if TS_MODEL==TS_MODEL_XPT2046
        ts.setRotation(config.store.fliptouch?3:1);
    #endif
    #if TS_MODEL==TS_MODEL_GT911
        ts.setRotation(config.store.fliptouch?0:2);
    #endif
    #if TS_MODEL==TS_MODEL_AXS15231B
        ts.setRotation(config.store.fliptouch?0:2);
    #endif
}

#endif  // TS_MODEL!=TS_MODEL_UNDEFINED
