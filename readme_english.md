 [Русская версия](README.md)

### Yoradio for ESP32 RGB Panel Displays (Arduino_GFX, .pioarduino / PlatformIO)

- Based on `e2002/yoradio` (`https://github.com/e2002/yoradio`). This fork adds support for ESP32‑S3 boards with RGB Panel displays using `Arduino_GFX` and `.pioarduino`/PlatformIO.

### Supported dev boards

#### 4848S040 (ST7701S, 480x480, 4.0" square)
- [AliExpress link](https://aliexpress.ru/item/1005008214872438.html?)
- [How to connect this board to the project →](README_4848S040_english.md)
- ![photo](https://github.com/user-attachments/assets/8feae43d-82f8-464f-848d-d09c33db8234)

#### UEDX48480021-MD80ET (ST7701S, 480x480, 2.1" round)
- [AliExpress link](https://aliexpress.ru/item/1005007576008287.html?)
- [How to connect this board to the project →](README_UEDX48480021_english.md)
- ![photo](https://github.com/user-attachments/assets/360799e8-da95-4c77-8ad9-c10b85be3855)

### Changelog

- 12.10.2025 — Project created. Added boards 4848S040 and UEDX48480021‑MD80ET.

### Important notes

- Russian font: replace `.pio/libdeps/<env>/GFX Library for Arduino/src/font/glcdfont.h` with the file from `fonts/glcdfont.h` (where `<env>` is the PlatformIO environment).

- High bitrate radio stability: replace IDF libs with prebuilt ones from `library!/esp32s3_5.4/`:
  - `libesp_netif.a`
  - `liblwip.a`
  - Copy to (Windows): `%USERPROFILE%\.platformio\packages\framework-arduinoespressif32-libs\esp32s3\lib\`
  - Supported: ESP‑IDF 5.4 / 5.5. Restart PlatformIO and rebuild after replacing.

- First boot / after erase flash: screen can stay black for ~60 seconds (FS init). This is normal — just wait.

- How to work with the project: follow the original `e2002/yoradio` docs and examples.

### License

MIT — see `LICENSE`.


