[English version](readme_english.md)

### Yoradio для ESP32 RGB Panel дисплеев (Arduino_GFX, .pioarduino / PlatformIO)

- Основано на проекте `e2002/yoradio` (`https://github.com/e2002/yoradio`). Этот форк добавляет поддержку RGB Panel дисплеев на ESP32-S3, с библиотекой `Arduino_GFX` и платформой `.pioarduino`/PlatformIO.

### Поддерживаемые dev boards

#### 4848S040 (ST7701S, 480x480, 4.0" квадратный)
- [AliExpress ссылка](https://aliexpress.ru/item/1005008214872438.html?)
- [Порядок подключения модуля к проекту →](README_4848S040.md)
- ![20251012_175502](https://github.com/user-attachments/assets/8feae43d-82f8-464f-848d-d09c33db8234)

#### UEDX48480021-MD80ET (ST7701S, 480x480, 2.1" круглый)
- [AliExpress ссылка](https://aliexpress.ru/item/1005007576008287.html?)
- [Порядок подключения модуля к проекту →](README_UEDX48480021.md)
- ![20251012_175649](https://github.com/user-attachments/assets/360799e8-da95-4c77-8ad9-c10b85be3855)

#### JC3248W535C (AXS15231B, 320x480, 3.5")
- [AliExpress ссылка](https://aliexpress.ru/item/1005007566332450.html)
- [Порядок подключения модуля к проекту →](README_JC3248W535C.md)
- ![placeholder](https://via.placeholder.com/300x400?text=JC3248W535C)

### Особенности проекта

Отличия от оригинального Yoradio:

1. **Рефакторинг на Arduino_GFX** — использование современной библиотеки Arduino_GFX с поддержкой RGB Panel и последнего ESP-IDF 5.4/5.5.
2. **Поддержка U8g2 шрифтов** — библиотека Arduino_GFX поддерживает U8g2 шрифты, их можно использовать в проекте.
3. **Spectrum Analyzer** — добавлен спектроанализатор с переключением VU ↔ SA прямо из веб-интерфейса (Settings). VU-метр теперь со шкалой (настраивается в `widgets.cpp`).
4. **Переключение SD ↔ Radio** — добавлена возможность переключения через тачскрин (одновременное нажатие двумя пальцами).
5. **CPU Load виджет** — показывает загрузку обоих ядер процессора.
6. **Поддержка форматов** — OGG, OPUS, VORBIS, FLAC потоки.
7. **Автоматическое притухание яркости** — виджет в `main.cpp` (настройки AUTOBACKLIGHT в `myoptions.h`).
8. **Батарея** — код встроен в файлы дисплея, активируется закомментированием `#define BATTERY_OFF` (для UEDX48480021 требуются свободные пины).
9. **Обновлённые библиотеки** — AudioI2S от Wolle (schreibfaul1) & Maleksm, Version 3.4.2p.
10. **Исправлено множество багов** — стабильность и производительность.

### Лог изменений

- 25.10.2025
  - Исправлено: обновление строки Wi‑Fi на бутскрине и перебор нескольких SSID.
  - Добавлена поддержка платы JC3248W535C (AXS15231B QSPI, 320x480, 3.5").
  - Обновлены библиотеки liblwip.a и libesp_netif.a для ESP-IDF 5.5 (stable), оптимизации LwIP.
  - Обновлена библиотека audioI2S до 3.4.2p (улучшения логирования, NetworkClient, декодеры).
  - Улучшена обработка тачскринов: DEBUG_TOUCH через веб-интерфейс, защита от ложных кликов после свайпа, корректная работа мультитача с SD.
- 12.10.2025 — Создание репозитория и добавление плат 4848S040 и UEDX48480021-MD80ET.

### Важные замечания

- Русификация шрифта: замените файл `.pio/libdeps/<env>/GFX Library for Arduino/src/font/glcdfont.h` на файл из репозитория `fonts/glcdfont.h` (где `<env>` — имя окружения из `platformio.ini`).

- Стабильность радио на высоких битрейтах: замените библиотеки IDF перекомпилированными версиями из `library!/esp32s3_5.5_stable/`:
  - `libesp_netif.a`
  - `liblwip.a`
  - Куда копировать (Windows): `%USERPROFILE%\.platformio\packages\framework-arduinoespressif32-libs\esp32s3\lib\`
  - Версия: ESP-IDF 5.5 (stable, commit 07e9bf4970). После замены перезапустите PlatformIO и пересоберите проект.
  
- Первый запуск/после erase flash: до ~60 секунд возможен чёрный экран (инициализация ФС). Это нормально — дождитесь старта.

- Порядок работы и примеры: ориентируйтесь на оригинальный проект автора `e2002/yoradio`.

### Благодарности

Особая благодарность:
- **e2002** — автор оригинального проекта Yoradio
- **Wolle (schreibfaul1)** — за отличную библиотеку AudioI2S
- **Maleksm** (4pda.to) — за доработки и улучшения AudioI2S
- **moononournation** — за библиотеку Arduino_GFX

### Лицензия

MIT — см. `LICENSE`.

---



