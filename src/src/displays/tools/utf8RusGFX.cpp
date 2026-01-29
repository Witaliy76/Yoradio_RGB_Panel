#include "utf8RusGFX.h"
#include <string.h>
#include <ctype.h>
#include <Arduino.h>

char* utf8Rus(const char* str, bool uppercase) {
    // Ring-buffer на 3 буфера для безопасных множественных вызовов / Ring-buffer with 3 buffers for safe multiple calls
    static char buf[3][BUFLEN];
    static uint8_t idx = 0;
    char* strn = buf[idx = (idx + 1) % 3];
    int i = 0, j = 0;
    while (str[i] && j < BUFLEN - 1) {
        // UTF-8: специальные символы (кавычки и т.д.) / Special characters (quotes etc.)
        if ((uint8_t)str[i] == 0xC2 && str[i+1]) {
            uint8_t next = (uint8_t)str[i+1];
            if (next == 0xAB) { // « (U+00AB) - левая кавычка / left quote
                strn[j++] = '"';
                i += 2;
                continue;
            } else if (next == 0xBB) { // » (U+00BB) - правая кавычка / right quote
                strn[j++] = '"';
                i += 2;
                continue;
            } else if (next == 0xA0) { // NBSP (U+00A0) - неразрывный пробел / non-breaking space
                strn[j++] = ' ';
                i += 2;
                continue;
            }
            // Другие C2-символы: пропускаем оба байта без добавления символов / Other C2 chars: skip both bytes without adding to output
            i += 2;
            continue;
        }
        // UTF-8: кириллица
        if ((uint8_t)str[i] == 0xD0 && str[i+1]) {
            uint8_t next = (uint8_t)str[i+1];
            if (next >= 0x90 && next <= 0xBF) { // А-Я, а-п
                char ch = next + 0x30; // 0xC0–0xDF
                // Применяем uppercase для кириллицы если нужно
                if (uppercase) {
                    if ((unsigned char)ch >= 0xE0 && (unsigned char)ch <= 0xFF)
                        ch = ch - 0x20;
                }
                strn[j++] = ch;
                i += 2;
                continue;
            } else if (next == 0x81) { // Ё
                strn[j++] = 0xA8; // Ё в CP1251
                i += 2;
                continue;
            }
            // Другие D0-символы: пропускаем оба байта без добавления символов / Other D0 chars: skip both bytes
            i += 2;
            continue;
        } else if ((uint8_t)str[i] == 0xD1 && str[i+1]) {
            uint8_t next = (uint8_t)str[i+1];
            if (next >= 0x80 && next <= 0x8F) { // р-я
                char ch = next + 0x70; // 0xE0–0xEF
                // Применяем uppercase для кириллицы если нужно
                if (uppercase) {
                    if ((unsigned char)ch >= 0xE0 && (unsigned char)ch <= 0xFF)
                        ch = ch - 0x20;
                }
                strn[j++] = ch;
                i += 2;
                continue;
            } else if (next == 0x91) { // ё (U+0451)
                char ch = 0xB8; // ё в CP1251
                // Применяем uppercase для ё
                if (uppercase) {
                    ch = 0xA8; // Ё в верхнем регистре
                }
                strn[j++] = ch;
                i += 2;
                continue;
            }
            // Другие D1-символы: пропускаем оба байта без добавления символов / Other D1 chars: skip both bytes
            i += 2;
            continue;
        }
        // Не кириллица (ASCII и другие символы) — обрабатываем как есть
        char ch = str[i];
        if (uppercase) {
            // Английские буквы: строчные a-z → заглавные A-Z
            if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
            // Кириллица (строчные в CP1251): 0xE0-0xFF → заглавные (вычитание 0x20)
            else if ((unsigned char)ch >= 0xE0 && (unsigned char)ch <= 0xFF)
                ch = ch - 0x20;
            // ё → Ё
            else if ((unsigned char)ch == 0xB8)
                ch = 0xA8;
        }
        strn[j++] = ch;
        i++;
    }
    strn[j] = 0;
    return strn;
} 