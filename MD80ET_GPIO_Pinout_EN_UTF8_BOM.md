ESP32-S3-WROOM-2 → UEDX48480021-MD80ET (incl. ET variant with touch)
GPIO / Pin Map — consolidated table for the WROOM module and MD80ET board
Generated: 2025-09-27 06:03:10
----------------------------------------------------------------------------------------------------
Pin# (WROOM)   GPIO   Status on MD80ET     Assignment / notes
----------------------------------------------------------------------------------------------------
4              4      not used             Not routed on board
5              5      in use               Encoder PHB
6              6      in use               Encoder PHA
7              7      in use               BACKLIGHT (PWM)
8              15     in use               Touch SCL (I²C) — ET variant
9              16     in use               Touch SDA (I²C) — ET variant
10             17     in use               DE (Data Enable)
11             18     in use               SPI_CS (panel command SPI)
12             8      in use               LCD_RST
13             19     in use               USB D−
14             20     in use               USB D+
15             3      in use (strap)       VSYNC; strap/JTAG-source
16             46     in use (strap)       HSYNC; boot strap
17             9      in use               PCLK
18             10     in use               LCD DATA0
19             11     in use               LCD DATA1
20             12     in use               LCD DATA2 / SPI_SDA (initialization)
21             13     in use               LCD DATA3 / SPI_SCK (initialization)
22             14     in use               LCD DATA4
23             21     in use               LCD DATA5
24             47     in use               LCD DATA6
25             48     in use               LCD DATA7
26             45     in use (strap)       LCD DATA8; VDD_SPI strap
27             0      in use (strap)       BOOT button (LOW → download mode)
28             35     do not use           Reserved by Octal PSRAM
29             36     do not use           Reserved by Octal PSRAM
30             37     do not use           Reserved by Octal PSRAM
31             38     in use               LCD DATA9
32             39     in use               LCD DATA10 (shares MTCK)
33             40     in use               LCD DATA11 (shares MTDO)
34             41     in use               LCD DATA12 (shares MTDI)
35             42     in use               LCD DATA13 (shares MTMS)
36             44     in use (UART)        UART0 TX (board wiring; chip default is U0RXD)
37             43     in use (UART)        UART0 RX (board wiring; chip default is U0TXD)
38             2      in use               LCD DATA14
39             1      in use               LCD DATA15
----------------------------------------------------------------------------------------------------
CONCLUSIONS:
1) On MD80ET all routed GPIOs are already used: RGB (16-bit) + HS/VS/DE/PCLK, backlight, RST, USB D±, UART0, encoder, I²C touch.
2) No spare GPIOs without compromises/modding. The only one physically present on the module but not routed on the board is GPIO4 (Pin#4).
3) GPIO35/36/37 must not be used — reserved by the Octal PSRAM interface on the ESP32-S3-R8 module.
4) Strap pins: GPIO0, GPIO45, GPIO46 (and GPIO3 as JTAG strap) — do not force levels during boot or you may break boot.
5) If you need a 3-wire I²S DAC (BCK, WS/LRCK, DOUT):
   • Option A (no soldering): drop the external UART0 and move console to USB-CDC; use BCK=GPIO43, WS=GPIO44, DOUT=GPIO18 (keep CS high after display init; stop toggling SPI).
   • Option B: sacrifice the encoder (BCK=GPIO6, WS=GPIO5, DOUT=GPIO18).
   • Option C: if touch is not needed — BCK=GPIO15, WS=GPIO16, DOUT=GPIO18.
   • Option D (hardware mod): solder to the module pin GPIO4 (Pin#4) and use it for DOUT or any I²S line.
6) Do not repurpose USB D± (GPIO19/20) or strap pins for I²S; avoid 35/36/37 due to PSRAM.
