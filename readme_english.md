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

#### JC3248W535C (AXS15231B, 320x480, 3.5")
- [AliExpress link](https://aliexpress.ru/item/1005007566332450.html)
- [How to connect this board to the project →](README_JC3248W535C_english.md)
- ![placeholder](https://via.placeholder.com/300x400?text=JC3248W535C)

### Project Features

Differences from original Yoradio:

1. **Refactored to Arduino_GFX** — using modern Arduino_GFX library with RGB Panel support and latest ESP-IDF 5.4/5.5.
2. **U8g2 fonts support** — Arduino_GFX library supports U8g2 fonts, you can use them in the project.
3. **Spectrum Analyzer** — added spectrum analyzer with VU ↔ SA switching directly from web interface (Settings). VU-meter now has scale (adjustable in `widgets.cpp`).
4. **SD ↔ Radio switching** — added ability to switch via touchscreen (simultaneous two-finger tap).
5. **CPU Load widget** — shows load of both processor cores.
6. **Format support** — OGG, OPUS, VORBIS, FLAC streams.
7. **Auto-dimming** — widget in `main.cpp` (AUTOBACKLIGHT settings in `myoptions.h`).
8. **Battery** — code built into display files, activated by uncommenting `#define BATTERY_OFF` (for UEDX48480021 requires free pins).
9. **Updated libraries** — AudioI2S from Wolle (schreibfaul1) & Maleksm, Version 3.4.2p.
10. **Many bugfixes** — stability and performance improvements.

### Changelog

- 23.10.2025 — Added support for JC3248W535C board (AXS15231B QSPI, 320x480, 3.5").
- 23.10.2025 — Updated liblwip.a and libesp_netif.a libraries for ESP-IDF 5.5 (stable). Optimized LwIP parameters to improve audio streaming stability and web server functionality. Libraries in `library!/esp32s3_5.5_stable/`. 
- 19.10.2025 — Updated audioI2S library from version 3.3.2r to 3.4.2p. Improved logging system, added NetworkClient support, updated all decoders (AAC, FLAC, MP3, Opus, Vorbis). 
- 12.10.2025 — Project created. Added boards 4848S040 and UEDX48480021‑MD80ET.

### Important notes

- Russian font: replace `.pio/libdeps/<env>/GFX Library for Arduino/src/font/glcdfont.h` with the file from `fonts/glcdfont.h` (where `<env>` is the PlatformIO environment).

- High bitrate radio stability: replace IDF libs with prebuilt ones from `library!/esp32s3_5.5_stable/`:
  - `libesp_netif.a`
  - `liblwip.a`
  - Copy to (Windows): `%USERPROFILE%\.platformio\packages\framework-arduinoespressif32-libs\esp32s3\lib\`
  - Version: ESP-IDF 5.5 (stable, commit 07e9bf4970). Restart PlatformIO and rebuild after replacing.
  
- First boot / after erase flash: screen can stay black for ~60 seconds (FS init). This is normal — just wait.

- How to work with the project: follow the original `e2002/yoradio` docs and examples.

### Acknowledgments

Special thanks to:
- **e2002** — author of the original Yoradio project
- **Wolle (schreibfaul1)** — for the excellent AudioI2S library
- **Maleksm** (4pda.to) — for AudioI2S improvements and enhancements
- **moononournation** — for the Arduino_GFX library

### License

MIT — see `LICENSE`.


