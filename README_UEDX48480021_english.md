 [Русская версия](README_UEDX48480021.md)

### Yoradio for UEDX48480021‑MD80ET (ST7701S RGB 480x480, 2.1" round)

1) How to enable this board

- Copy `src/myoptions_UEDX48480021.h` over `src/myoptions.h`.
- **IMPORTANT:** In PlatformIO select environment `[env:UEDX48480021]` from `platformio.ini` (or use universal `[env:yoradio-esp32s3]`).
- Then run project clean in `.pioarduino`/PlatformIO to apply config.

2) Photos

- ![photo](https://github.com/user-attachments/assets/f06130b1-4985-4aac-a4c8-4d64715574f6)

3) Changelog

- 12.10.2025 — Initial README for UEDX48480021‑MD80ET.

4) Features & pins

- Round ST7701S 480x480 (effective ~400x400), Type4 preset, BGR, RGB→BGR swap in Arduino_ESP32RGBPanel
- Backlight: GPIO7 (Active LOW)
- Touch: CST826 (I2C: SDA=16, SCL=15)
- Audio (I2S): DOUT=43, BCLK=44, LRC=4 — on some revisions you must remove capacitor C9 (not present on all boards)
- ST7701S signals:
  - SWSPI: CS=18, SCK=13, SDA=12, RST=8
  - Sync: DE=17, VSYNC=3, HSYNC=46, PCLK=9
  - Backlight: BL=7 (Active LOW)

Specs

| Item | Value |
|------|-------|
| CPU | ESP32‑S3‑WROOM‑1‑N16R8 (240MHz, dual‑core) |
| Memory | 520KB RAM + 16MB PSRAM + 8MB Flash |
| Display | ST7701S RGB Panel 480x480 (round) |
| Touch | CST826 (I2C: SDA=16, SCL=15) |
| Audio | I2S (BCLK=44, LRCLK=4, DOUT=43) |
| Backlight | GPIO7 (Active LOW) |
| SD card | — |

5) Wiring notes

- Backlight is inverted (Active LOW).
- For LRC=4, some revisions require removing capacitor C9 (if present):
  - Disassembly steps:
    - Bend 4 outer ring clips and remove the ring.
    - ![step1](https://github.com/user-attachments/assets/8be70571-9024-4a63-91a6-3c2394c92d2a)
    - Bend 3 encoder clips with a thin screwdriver and remove the encoder housing.
    - ![step2a](https://github.com/user-attachments/assets/e1b6acf0-c527-4daa-b166-3ff372f00699)
    - ![step2b](https://github.com/user-attachments/assets/0d730ca2-a490-4c5f-b240-656071dcc73e)
    - Result:
    - ![result](https://github.com/user-attachments/assets/ddeb0397-27bf-4f21-93d8-5b0cc2094198)
    - C9 capacitor location (left side). Remove it to avoid audio noise.
    - ![c9](https://github.com/user-attachments/assets/2bbd5d59-d398-4760-97ba-eb520bfc0b2a)
- I2S via the small expansion board shipped with the dev board:
  - DB7 → LRCK (LRC)
  - RX1 → BCK (BCLK)
  - TX1 → DATA (DOUT)
  - ![i2s1](https://github.com/user-attachments/assets/2b1d9067-b890-4dd8-9816-b73f115dcd0d)
  - ![i2s2](https://github.com/user-attachments/assets/aee614b5-7fed-4b88-acdc-973f90691559)

Touch

- Touch is supported (CST826). Some kits do not include touch — confirm with the seller.
- If your kit has no touch, you may reuse the freed I2C pins (SDA=16, SCL=15). This requires opening the module and soldering directly to ESP32‑S3. Do it only if you know what you're doing.

Closing

- Contributions, testing and PRs are welcome.


