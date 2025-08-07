#ifndef utf8RusGFX_h
#define  utf8RusGFX_h

#define BUFLEN 250

#ifdef __cplusplus
extern "C" {
#endif
// utf8Rus: перекодирует строку UTF-8 в однобайтовую кириллицу, при необходимости приводит к верхнему регистру (uppercase=true)
char* utf8Rus(const char* str, bool uppercase);
#ifdef __cplusplus
}
#endif

#endif
