[Русская версия](README_JC3248W535C.md)

### Yoradio for JC3248W535C (AXS15231B QSPI 320x480, 3.5")

1) How to enable this board

- Copy `src/myoptions_JC3248W535C.h` over `src/myoptions.h`.
- **IMPORTANT:** In PlatformIO select environment `[env:JC3248W535C]` from `platformio.ini` (or use universal `[env:yoradio-esp32s3]`).
- Then run project clean in `.pioarduino`/PlatformIO to apply config.

2) Photos

<!-- Placeholder for photos - add your photos here -->
- ![photo1_placeholder](https://via.placeholder.com/800x600?text=JC3248W535C+Photo+1)
- ![photo2_placeholder](https://via.placeholder.com/800x600?text=JC3248W535C+Photo+2)

<!-- Uncomment and replace with actual photo URLs when ready:
- ![photo1](URL_TO_PHOTO_1)
- ![photo2](URL_TO_PHOTO_2)
-->

3) Changelog

- 23.10.2025 — Initial README for JC3248W535C board.

4) Features & pins

- Display: AXS15231B, 320x480, QSPI interface (4 data lines: D0-D3)
- Touch: AXS15231B (I2C: SDA=4, SCL=8, INT=3)
- Backlight: GPIO1 (PWM, standard)
- Audio (I2S): DOUT=41, BCLK=42, LRC=2
- SD card (HSPI): CS=10, SCK=12, MISO=13, MOSI=11
- Battery: ADC=5, voltage divider R1=68kΩ, R2=100kΩ, correction DELTA_BAT=0.127V
- Free pins: depends on config (encoder, buttons, IR — not used)

Specs

| Item | Value |
|------|-------|
| **CPU** | ESP32-S3-WROOM-1-N16R8 (240MHz, dual-core) |
| **Memory** | 520KB RAM + 16MB PSRAM + 8MB Flash |
| **Display** | AXS15231B QSPI 320x480 (4 data lines) |
| **Touch** | AXS15231B (I2C: SDA=4, SCL=8, INT=3) |
| **Audio** | I2S (BCLK=42, LRCLK=2, DOUT=41) |
| **Backlight** | PWM (GPIO1) |
| **SD card** | HSPI (CS=10, SCK=12, MISO=13, MOSI=11) |
| **Battery** | ADC monitoring (GPIO5, R1=68kΩ, R2=100kΩ) |

QSPI Display Pinout

| Signal | GPIO | Description |
|--------|------|-------------|
| TFT_CS | 45 | Chip Select |
| TFT_RST | -1 | Reset (connected to EN) |
| TFT_SCK | 47 | Serial Clock / DC |
| TFT_D0 | 21 | QSPI Data 0 |
| TFT_D1 | 48 | QSPI Data 1 |
| TFT_D2 | 40 | QSPI Data 2 |
| TFT_D3 | 39 | QSPI Data 3 |
| GFX_BL | 1 | Backlight (PWM) |

5) Wiring notes

- Display uses QSPI interface (faster than standard SPI due to 4 data lines).
- Backlight controlled via PWM on GPIO1 (standard logic, not inverted).
- I2S audio output on standard pins: DOUT=41, BCLK=42, LRC=2.
- SD card connected via HSPI to avoid conflicts with display.

<!-- Placeholder for wiring photos - add photos here if needed -->
<!-- 
- ![wiring1](URL_TO_WIRING_PHOTO_1)
- ![wiring2](URL_TO_WIRING_PHOTO_2)
-->

Battery

- Built-in battery voltage monitoring via voltage divider on GPIO5.
- Divider: R1=68kΩ (to +), R2=100kΩ (to GND).
- Voltage correction: DELTA_BAT=0.127V (calibrated for specific board).
- Display: charge level (%), voltage (V), charging status.
- To disable battery monitoring, uncomment `#define BATTERY_OFF` in `myoptions.h`.

Touch gestures

- Touch is supported (AXS15231B, I2C).
- Left/Right — volume
- Up/Down — station select
- Tap — play/pause
- **Two-finger tap simultaneously** — switch SD ⇄ Radio

SD status

- SD card support fully implemented.
- Indexing works.
- File playback from SD card works.
- **Two-finger gesture** to switch between Radio and SD modes.

Spectrum Analyzer

- Built-in spectrum analyzer (15 bands).
- Uses PSRAM for FFT buffers.
- Switch VU-meter ⇄ Spectrum Analyzer in settings.

CPU Load Widget

- CPU load widget (averaging across two cores).
- Displayed in player mode.
- To disable, comment out `#define CPU_LOAD` in `myoptions.h`.

Closing

- Contributions, testing and PRs are welcome.

