#include "GFX_Canvas_screen.h"
#include <stdio.h>

void gfxDrawText(Arduino_Canvas* gfx, int x, int y, const char* text, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font) {
    if (font) gfx->setFont(font);
    else gfx->setFont();
    gfx->setTextColor(color, bgcolor);
    gfx->setTextSize(size);
    gfx->setCursor(x, y);
    gfx->print(utf8Rus(text, true));
}

void gfxDrawNumber(Arduino_Canvas* gfx, int x, int y, int num, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", num);
    gfxDrawText(gfx, x, y, buf, color, bgcolor, size, font);
}

void gfxDrawFormatted(Arduino_Canvas* gfx, int x, int y, const char* fmt, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font, ...) {
    char buf[64];
    va_list args;
    va_start(args, font);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    gfxDrawText(gfx, x, y, buf, color, bgcolor, size, font);
}

void gfxDrawPixel(Arduino_Canvas* gfx, int x, int y, uint16_t color) {
    gfx->drawPixel(x, y, color);
}

void gfxDrawLine(Arduino_Canvas* gfx, int x0, int y0, int x1, int y1, uint16_t color) {
    gfx->drawLine(x0, y0, x1, y1, color);
}

void gfxDrawRect(Arduino_Canvas* gfx, int x, int y, int w, int h, uint16_t color) {
    gfx->drawRect(x, y, w, h, color);
}

void gfxFillRect(Arduino_Canvas* gfx, int x, int y, int w, int h, uint16_t color) {
    gfx->fillRect(x, y, w, h, color);
}

void gfxDrawBitmap(Arduino_Canvas* gfx, int x, int y, const uint16_t* bitmap, int w, int h) {
    gfx->draw16bitRGBBitmap(x, y, const_cast<uint16_t*>(bitmap), w, h);
}

void gfxClearArea(Arduino_Canvas* gfx, int x, int y, int w, int h, uint16_t bgcolor) {
    gfx->fillRect(x, y, w, h, bgcolor);
}

void gfxClearScreen(Arduino_Canvas* gfx, uint16_t bgcolor) {
    gfx->fillScreen(bgcolor);
}

void gfxFlushScreen(Arduino_Canvas* gfx) {
    gfx->flush();
} 