#pragma once
#include "canvas/Arduino_Canvas.h"
#include <stdarg.h>
#include "utf8RusGFX.h"

// Текст
// UTF-8 input: конвертирует через utf8Rus() и печатает
void gfxDrawTextUtf8(Arduino_Canvas* gfx, int x, int y, const char* text, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font = nullptr, bool uppercase = false);
// 1-byte input: печатает уже сконвертированную строку без повторной конвертации
void gfxDrawText1b(Arduino_Canvas* gfx, int x, int y, const char* text, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font = nullptr, bool uppercase = false);
// Legacy wrapper: вызывает gfxDrawTextUtf8() для обратной совместимости
void gfxDrawText(Arduino_Canvas* gfx, int x, int y, const char* text, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font = nullptr, bool uppercase = false);
void gfxDrawNumber(Arduino_Canvas* gfx, int x, int y, int num, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font = nullptr);
void gfxDrawFormatted(Arduino_Canvas* gfx, int x, int y, const char* fmt, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font, ...);

// Графика
void gfxDrawPixel(Arduino_Canvas* gfx, int x, int y, uint16_t color);
void gfxDrawLine(Arduino_Canvas* gfx, int x0, int y0, int x1, int y1, uint16_t color);
void gfxDrawRect(Arduino_Canvas* gfx, int x, int y, int w, int h, uint16_t color);
void gfxFillRect(Arduino_Canvas* gfx, int x, int y, int w, int h, uint16_t color);
void gfxDrawBitmap(Arduino_Canvas* gfx, int x, int y, const uint16_t* bitmap, int w, int h);

// Очистка
void gfxClearArea(Arduino_Canvas* gfx, int x, int y, int w, int h, uint16_t bgcolor);
void gfxClearScreen(Arduino_Canvas* gfx, uint16_t bgcolor);
void gfxFlushScreen(Arduino_Canvas* gfx); 