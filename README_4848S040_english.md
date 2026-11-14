 [Русская версия](README_4848S040.md)

### Yoradio for ESP32‑4848S040 (ST7701S RGB 480x480, 4.0")

1) How to enable this board

- Copy `src/myoptions_4848S040.h` over `src/myoptions.h`.
- **IMPORTANT:** In PlatformIO select environment `[env:4848S040]` from `platformio.ini` (or use universal `[env:yoradio-esp32s3]`).
- Then run project clean in `.pioarduino`/PlatformIO to apply config.

2) Photos

- ![photo1](https://github.com/user-attachments/assets/acb966ff-f32c-4d74-968a-618a090062a4)
- ![photo2](https://github.com/user-attachments/assets/fa5d9a84-035f-4691-b40a-9d20b6746dda)

3) Changelog

- 14.11.2026 — Version 0.9.434m-r2.03:
  - SD card works: mount, indexing, MP3 and FLAC playback, resume from the stop position, fallback to Radio when the card is removed.
  - Two-finger touch switches SD<->Radio (GT911).
  - Fixed heap corruption in the GT911 touch driver (185-byte config read into the 128-byte I2C buffer). It showed up as random reboots, mostly when SD was in use.
  - Fixed a crash on FLAC files with tags (Vorbis comments).
  - Removed the "memory check" from SDManager that itself caused reboots during playback.
- 05.02.2026 — Switch to Arduino_GFX type9 init, drop post-init/stabilize steps, set PCLK to 12MHz.
- 12.10.2025 — Initial README for 4848S040.

4) Features & pins

- Display: ST7701S, 480x480, RGB Panel (16 data lines)
- Touch: GT911 (I2C: SDA=19, SCL=45)
- Backlight: GPIO38 (PWM)
- Audio (I2S): DOUT=40, BCLK=1, LRC=2
- SD card (SPI): CS=42, SCK=48, MISO=41, MOSI=47, 20 MHz
- GPIO48 and GPIO47 are shared by the SD card and the ST7701 command bus. GPIO48 is also the module's on-board RGB LED, so it is disabled in `myoptions.h` (`USE_BUILTIN_LED false`) — otherwise the SD card cannot be read. Enabling it stops the build with an error.
- Free pins: depends on board revision.

Specs

| Item | Value |
|------|-------|
| CPU | ESP32‑S3‑WROOM‑1‑N16R8 (240MHz, dual‑core) |
| Memory | 520KB RAM + 16MB PSRAM + 8MB Flash |
| Display | ST7701 RGB Panel 480x480 (16 data lines) |
| Touch | GT911 (I2C: SDA=19, SCL=45) |
| Audio | I2S (BCLK=1, LRCLK=2, DOUT=40) |
| Backlight | PWM (GPIO38) |
| SD card | FSPI (CS=42, SCK=48, MISO=41, MOSI=47) |

5) Wiring notes

- On‑board mono DAC is disabled by default (lines routed to rear header).
- ![connector](https://github.com/user-attachments/assets/a40160ec-31af-463c-88bb-d60b51b0e37b)
- To enable on‑board DAC: solder jumpers R21, R22, R23.
- ![jumpers](https://github.com/user-attachments/assets/d2fbc24c-9036-4c09-ace8-5323dc4e30fb)
- If rear header is not used — R26, R27, R25 can stay in place.
- ![back](https://github.com/user-attachments/assets/34dd64a3-305e-4486-a07c-fedf7416baa0)

Touch gestures

- Touch is supported.
- Left/Right — volume
- Up/Down — station select
- Tap — play/pause
- Short two-finger touch (about half a second) — switch SD<->Radio. If the player was playing, playback continues in the new mode; if it was stopped, it stays stopped.
- The mode can also be switched from the web interface.

SD status

- SD card is supported since version 0.9.434m-r2.03: MP3 and FLAC, FAT32 card.
- Settings in `myoptions.h`: `SDC_CS 42`, `SD_SPIPINS 48, 41, 47`, `SDSPISPEED 20000000`, `USE_BUILTIN_LED false`.
- If the card is removed during playback, the device returns to Radio mode.
- Known limitations:
  - display inversion (`invert`) is not applied while the SD card is mounted;
  - rare picture jitter is possible during playback;
  - when switching Radio -> SD during playback, the line `audio is not a file` flashes on the screen for a moment — it does not affect operation.

Closing

- Contributions, testing and PRs are welcome.


