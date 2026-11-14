# Prebuilt firmware / Готовые прошивки — 0.9.434m-r2.03

One folder per board, `RU` and `EN` builds inside. / По папке на плату, внутри сборки `RU` и `EN`.

| File | Flash address |
|------|---------------|
| `bootloader.bin` | `0x0` |
| `partitions.bin` | `0x8000` |
| `firmware.bin` | `0x10000` |
| `littlefs.bin` | `0x670000` |

Flash settings: ESP32-S3, 16 MB flash.

- `littlefs.bin` is the file system image (web interface, default playlist, AI prompt). Since 07.03.2026 the firmware uses LittleFS; the old `spiffs.bin` images are not compatible and were removed.
- `littlefs.bin` — образ файловой системы (веб-интерфейс, плейлист по умолчанию, AI prompt). С 07.03.2026 прошивка использует LittleFS; старые образы `spiffs.bin` несовместимы и удалены.
- The image contains no Wi-Fi settings: on first start the device opens its own access point for setup.
- В образе нет настроек Wi-Fi: при первом запуске устройство поднимает свою точку доступа для настройки.
- To update only the firmware and keep your settings and playlist, flash `firmware.bin` alone.
- Чтобы обновить только прошивку и сохранить настройки и плейлист, прошейте один `firmware.bin`.
