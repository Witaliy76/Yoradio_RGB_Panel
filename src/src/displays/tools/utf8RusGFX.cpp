#include "utf8RusGFX.h"
#include <string.h>
#include <ctype.h>
#include <Arduino.h>

char* utf8Rus(const char* str, bool uppercase) {
    static char strn[BUFLEN];
    int i = 0, j = 0;
    while (str[i] && j < BUFLEN - 1) {
        // UTF-8: кириллица
        if ((uint8_t)str[i] == 0xD0 && str[i+1]) {
            uint8_t next = (uint8_t)str[i+1];
            if (next >= 0x90 && next <= 0xBF) { // А-Я, а-п
                char ch = next + 0x30; // 0xC0–0xDF
                strn[j++] = ch;
                i += 2;
                continue;
            } else if (next == 0x81) { // Ё
                strn[j++] = 0xA8;
                i += 2;
                continue;
            }
        } else if ((uint8_t)str[i] == 0xD1 && str[i+1]) {
            uint8_t next = (uint8_t)str[i+1];
            if (next >= 0x80 && next <= 0x8F) { // р-я
                char ch = next + 0x70; // 0xE0–0xEF
                strn[j++] = ch;
                i += 2;
                continue;
            } else if (next == 0x91) { // ё
                strn[j++] = 0xB8;
                i += 2;
                continue;
            }
        }
        // Не кириллица — копируем как есть
        char ch = str[i];
        if (uppercase) {
            // латиница
            if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
            // кириллица (строчные)
            else if ((unsigned char)ch >= 0xE0 && (unsigned char)ch <= 0xFF)
                ch = ch - 0x20;
            // ё
            else if ((unsigned char)ch == 0xB8)
                ch = 0xA8;
        }
        strn[j++] = ch;
        i++;
    }
    strn[j] = 0;
    return strn;
} 