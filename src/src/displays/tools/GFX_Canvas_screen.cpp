#include "GFX_Canvas_screen.h"
#include <stdio.h>
#include <string.h>

// Объявление функции для установки флага dirty (определена в display.cpp)
extern void markFrameDirty();

// Helper: применяет uppercase к 1-byte строке (ASCII + CP1251 кириллица)
// Не вызывает utf8Rus(), работает только с уже сконвертированными строками
// Использует ring-buffer из 3 буферов для безопасности при множественных вызовах в одном кадре
static char* uppercase1b(const char* str, bool uppercase) {
    static char buf[3][256]; // Ring-buffer: 3 буфера для безопасных множественных вызовов
    static uint8_t idx = 0; // Индекс текущего буфера в ring-buffer
    // Размер 256 достаточен, т.к. входная строка уже обрезана buffsize виджетов (обычно < 250)
    
    // Переключаемся на следующий буфер в ring-buffer (циклически)
    idx = (idx + 1) % 3;
    char* strn = buf[idx];
    
    int i = 0;
    while (str[i] && i < (int)sizeof(buf[0]) - 1) {
        char ch = str[i];
        if (uppercase) {
            // Английские буквы: строчные a-z -> заглавные A-Z
            if (ch >= 'a' && ch <= 'z') {
                ch = ch - 'a' + 'A';
            }
            // Кириллица (строчные в CP1251): 0xE0-0xFF -> заглавные (вычитание 0x20)
            // Примечание: символ "ё" (0xB8) не попадает в этот диапазон, обрабатывается отдельно
            else if ((unsigned char)ch >= 0xE0 && (unsigned char)ch <= 0xFF) {
                ch = ch - 0x20;
            }
            // ё -> Ё (0xB8 -> 0xA8)
            else if ((unsigned char)ch == 0xB8) {
                ch = 0xA8;
            }
        }
        // При uppercase == false символ "ё" (0xB8) проходит как есть без изменений
        strn[i++] = ch;
    }
    strn[i] = 0;
    return strn;
}

// UTF-8 input: конвертирует через utf8Rus() и печатает
void gfxDrawTextUtf8(Arduino_Canvas* gfx, int x, int y, const char* text, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font, bool uppercase) {
    markFrameDirty(); // Отмечаем кадр как грязный перед рисованием
    if (font) gfx->setFont(font);
    else gfx->setFont();
    gfx->setTextColor(color, bgcolor);
    gfx->setTextSize(size);
    gfx->setCursor(x, y);
    gfx->print(utf8Rus(text, uppercase));
}

// 1-byte input: печатает уже сконвертированную строку без повторной конвертации
// Применяет uppercase если нужно (для ASCII и CP1251 кириллицы)
void gfxDrawText1b(Arduino_Canvas* gfx, int x, int y, const char* text, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font, bool uppercase) {
    markFrameDirty(); // Отмечаем кадр как грязный перед рисованием
    if (font) gfx->setFont(font);
    else gfx->setFont();
    gfx->setTextColor(color, bgcolor);
    gfx->setTextSize(size);
    gfx->setCursor(x, y);
    // Применяем uppercase к 1-byte строке если нужно (без вызова utf8Rus)
    // uppercase1b() всегда возвращает буфер, даже если uppercase==false (для безопасности)
    gfx->print(uppercase1b(text, uppercase));
}

// Legacy wrapper: вызывает gfxDrawTextUtf8() для обратной совместимости
void gfxDrawText(Arduino_Canvas* gfx, int x, int y, const char* text, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font, bool uppercase) {
    gfxDrawTextUtf8(gfx, x, y, text, color, bgcolor, size, font, uppercase);
}

void gfxDrawNumber(Arduino_Canvas* gfx, int x, int y, int num, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", num);
    gfxDrawText(gfx, x, y, buf, color, bgcolor, size, font, false);
}

void gfxDrawFormatted(Arduino_Canvas* gfx, int x, int y, const char* fmt, uint16_t color, uint16_t bgcolor, uint8_t size, const GFXfont* font, ...) {
    char buf[64];
    va_list args;
    va_start(args, font);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    gfxDrawText(gfx, x, y, buf, color, bgcolor, size, font, false);
}

void gfxDrawPixel(Arduino_Canvas* gfx, int x, int y, uint16_t color) {
    markFrameDirty(); // Отмечаем кадр как грязный перед рисованием
    gfx->drawPixel(x, y, color);
}

void gfxDrawLine(Arduino_Canvas* gfx, int x0, int y0, int x1, int y1, uint16_t color) {
    markFrameDirty(); // Отмечаем кадр как грязный перед рисованием
    gfx->drawLine(x0, y0, x1, y1, color);
}

void gfxDrawRect(Arduino_Canvas* gfx, int x, int y, int w, int h, uint16_t color) {
    markFrameDirty(); // Отмечаем кадр как грязный перед рисованием
    gfx->drawRect(x, y, w, h, color);
}

void gfxFillRect(Arduino_Canvas* gfx, int x, int y, int w, int h, uint16_t color) {
    markFrameDirty(); // Отмечаем кадр как грязный перед рисованием
    gfx->fillRect(x, y, w, h, color);
}

void gfxDrawBitmap(Arduino_Canvas* gfx, int x, int y, const uint16_t* bitmap, int w, int h) {
    markFrameDirty(); // Отмечаем кадр как грязный перед рисованием
    gfx->draw16bitRGBBitmap(x, y, const_cast<uint16_t*>(bitmap), w, h);
}

void gfxClearArea(Arduino_Canvas* gfx, int x, int y, int w, int h, uint16_t bgcolor) {
    markFrameDirty(); // Отмечаем кадр как грязный перед рисованием
    gfx->fillRect(x, y, w, h, bgcolor);
}

void gfxClearScreen(Arduino_Canvas* gfx, uint16_t bgcolor) {
    markFrameDirty(); // Отмечаем кадр как грязный перед рисованием
    gfx->fillScreen(bgcolor);
}

void gfxFlushScreen(Arduino_Canvas* gfx) {
    gfx->flush();
} 