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


### Лог изменений

- 12.10.2025 — Создание репозитория и добавление плат 4848S040 и UEDX48480021-MD80ET.

### Важные замечания

- Русификация шрифта: замените файл `.pio/libdeps/<env>/GFX Library for Arduino/src/font/glcdfont.h` на файл из репозитория `fonts/glcdfont.h` (где `<env>` — имя окружения из `platformio.ini`).

- Стабильность радио на высоких битрейтах: замените библиотеки IDF перекомпилированными версиями из `library!/esp32s3_5.4/`:
  - `libesp_netif.a`
  - `liblwip.a`
  - Куда копировать (Windows): `%USERPROFILE%\.platformio\packages\framework-arduinoespressif32-libs\esp32s3\lib\`
  - Поддерживаемые версии: ESP-IDF 5.4 / 5.5. После замены перезапустите PlatformIO и пересоберите проект.

- Первый запуск/после erase flash: до ~60 секунд возможен чёрный экран (инициализация ФС). Это нормально — дождитесь старта.

- Порядок работы и примеры: ориентируйтесь на оригинальный проект автора `e2002/yoradio`.

### Лицензия

MIT — см. `LICENSE`.

---



