#include "../dspcore.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "../tools/utf8RusGFX.h"
#ifdef __cplusplus
}
#endif
#if DSP_MODEL!=DSP_DUMMY

#include "widgets.h"
#include "../../core/player.h"    //  for VU widget
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>

#if DSP_MODEL==DSP_AXS15231B
#include "../tools/GFX_Canvas_screen.h"
extern Arduino_Canvas* gfx;
#endif

// === НАСТРАИВАЕМЫЕ ПАРАМЕТРЫ ДЛЯ ПОДБОРА ===
//НАСТРОЙКИ ШКАЛА VU-МЕТРА
const float redZonePos = 0.80f; // Доля шкалы, где начинается красная зона и деление 0 (подбирай от того что VU метр показывает!)
const int scaleHeightLong = 8; // длинные деления шкалы пикс
const int scaleHeightShort = 4; // короткие деления шкалы пикс
const int scaleThickness = 1; // толщина линии пикс
const int fontSize = 1; // минимальный шрифт
const int labelOffsetTop = -11;    // отступ верхней подписи от шкалы (отрицательное — вверх)
const int labelOffsetBottom = 4;   // отступ нижней подписи от шкалы (положительное — вниз)
const int labelLeftmostOffset = 9; // сдвиг самой левой подписи ('-30')
//VU-МЕТР EXPERIMENTAL
const int vuChannelGap = 48; // расстояние между каналами VU-метра в пикселях
static uint32_t holdTimeL = 1500; // время удержания пика для левого канала (мс, шаг 500 мс)
static uint32_t holdTimeR = 1500; // время удержания пика для правого канала (мс, шаг 500 мс)
// === КОНЕЦ НАСТРАИВАЕМЫХ ПАРАМЕТРОВ ===

/************************
      FILL WIDGET
 ************************/
void FillWidget::init(FillConfig conf, uint16_t bgcolor) {
  Widget::init(conf.widget, bgcolor, bgcolor);
  _width = conf.width;
  _height = conf.height;
  _lastDraw.valid = false;
}

void FillWidget::_fastFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!gfx) return;
    
    // Проверяем, нужно ли вообще что-то рисовать
    if (w <= 0 || h <= 0) return;
    
    // Проверяем, не рисуем ли мы то же самое
    if (_lastDraw.valid && 
        _lastDraw.x == x && 
        _lastDraw.y == y && 
        _lastDraw.w == w && 
        _lastDraw.h == h && 
        _lastDraw.color == color) {
        return;
    }
    
    // Используем оптимизированную отрисовку
    gfx->fillRect(x, y, w, h, color);
    
    // Сохраняем параметры последней отрисовки
    _lastDraw.x = x;
    _lastDraw.y = y;
    _lastDraw.w = w;
    _lastDraw.h = h;
    _lastDraw.color = color;
    _lastDraw.valid = true;
}

void FillWidget::_draw() {
    if(!_active || !gfx) return;
    _fastFillRect(_config.left, _config.top, _width, _height, _bgcolor);
}

void FillWidget::_clear() {
    if (!gfx) return;
    _fastFillRect(_config.left, _config.top, _width, _height, _bgcolor);
    _lastDraw.valid = false;
}

void FillWidget::setHeight(uint16_t newHeight) {
    _height = newHeight;
    _lastDraw.valid = false;
}

/************************
      TEXT WIDGET
 ************************/
TextWidget::TextCache* TextWidget::_textCache = nullptr;
size_t TextWidget::_cacheSize = 0;

char* TextWidget::_getCachedText(const char* txt) {
    TextCache* current = _textCache;
    while (current) {
        if (strcmp(current->key, txt) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return nullptr;
}

void TextWidget::_addToCache(const char* txt, char* value) {
    if (_cacheSize >= MAX_CACHE_SIZE) {
        // Удаляем самый старый элемент
        TextCache* old = _textCache;
        _textCache = _textCache->next;
        free(old->value);
        delete old;
        _cacheSize--;
    }
    
    // Добавляем новый элемент в начало списка
    TextCache* newCache = new TextCache(txt, value);
    newCache->next = _textCache;
    _textCache = newCache;
    _cacheSize++;
}

void TextWidget::_clearCache() {
    while (_textCache) {
        TextCache* current = _textCache;
        _textCache = _textCache->next;
        free(current->value);
        delete current;
    }
    _cacheSize = 0;
}

void TextWidget::init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) {
  Widget::init(wconf, fgcolor, bgcolor);
  _buffsize = buffsize;
  _text = (char *) malloc(sizeof(char) * _buffsize);
  memset(_text, 0, _buffsize);
  _oldtext = (char *) malloc(sizeof(char) * _buffsize);
  memset(_oldtext, 0, _buffsize);
  dsp.charSize(_config.textsize, _charWidth, _textheight);
  _textwidth = _oldtextwidth = _oldleft = 0;
  _uppercase = uppercase;
}

void TextWidget::setText(const char* txt) {
    if (!gfx) {
        Serial.println("[TextWidget] Waiting for gfx initialization...");
        delay(100);  // Даем время на инициализацию
        return;
    }
    
    // Проверяем, является ли это названием песни (длинная строка)
    if (strlen(txt) > 10) {
        // Очищаем старый кэш при смене контента
        static const char* lastContent = nullptr;
        if (lastContent != txt) {
            _clearCache();
            lastContent = txt;
        }
        
        char* cachedText = _getCachedText(txt);
        if (cachedText) {
            strlcpy(_text, cachedText, _buffsize);
        } else {
            char* converted = strdup(utf8Rus(txt, _uppercase));
            strlcpy(_text, converted, _buffsize);
            _addToCache(txt, converted);
        }
    } else {
        // Для коротких строк не используем кэш
        strlcpy(_text, utf8Rus(txt, _uppercase), _buffsize);
    }
    
    _textwidth = strlen(_text) * _charWidth;
    if (strcmp(_oldtext, _text) == 0) return;
    
    if (_active) {
        gfxFillRect(gfx, _oldleft == 0 ? _realLeft() : std::min(_oldleft, _realLeft()),  
                   _config.top, 
                   std::max(_oldtextwidth, _textwidth), 
                   _textheight, 
                   _bgcolor);
    }
    
    _oldtextwidth = _textwidth;
    _oldleft = _realLeft();
    if (_active) loop();
}

void TextWidget::setText(int val, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, val);
  setText(buf);
}

void TextWidget::setText(const char* txt, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, txt);
  setText(buf);
}

uint16_t TextWidget::_realLeft() {
  switch (_config.align) {
    case WA_CENTER: return (dsp.width() - _textwidth) / 2; break;
    case WA_RIGHT: return (dsp.width() - _textwidth - _config.left); break;
    default: return _config.left; break;
  }
}

void TextWidget::_draw() {
  if(!_active || !gfx) return;
  gfxDrawText(gfx, _realLeft(), _config.top, _text, _fgcolor, _bgcolor, _config.textsize);
  strlcpy(_oldtext, _text, _buffsize);
}

TextWidget::~TextWidget() {
    free(_text);
    free(_oldtext);
    // Очищаем кэш только если это последний экземпляр TextWidget
    static int widgetCount = 0;
    widgetCount++;
    if (widgetCount == 1) { // Если это последний TextWidget
        _clearCache();
    }
    widgetCount--;
}

/************************
      SCROLL WIDGET
 ************************/
ScrollWidget::ScrollWidget(const char* separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor) {
  init(separator, conf, fgcolor, bgcolor);
}

ScrollWidget::~ScrollWidget() {
  free(_sep);
  free(_window);
}

void ScrollWidget::init(const char* separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor) {
  TextWidget::init(conf.widget, conf.buffsize, conf.uppercase, fgcolor, bgcolor);
  _sep = (char *) malloc(sizeof(char) * 4);
  memset(_sep, 0, 4);
  snprintf(_sep, 4, " %.*s ", 1, separator);
  _x = conf.widget.left;
  _fx = conf.widget.left;
  _startscrolldelay = conf.startscrolldelay;
  _scrolldelta = conf.scrolldelta;
  _scrolltime = conf.scrolltime;
  dsp.charSize(_config.textsize, _charWidth, _textheight);
  _sepwidth = strlen(_sep) * _charWidth;
  _width = conf.width;
  _backMove.width = _width;
  _window = (char *) malloc(sizeof(char) * (MAX_WIDTH / _charWidth + 1));
  memset(_window, 0, (MAX_WIDTH / _charWidth + 1));
  _doscroll = false;
}

void ScrollWidget::_setTextParams() {
  // Не требуется для Canvas API
}

bool ScrollWidget::_checkIsScrollNeeded() {
  return _textwidth > _width;
}

void ScrollWidget::setText(const char* txt) {
  if (!gfx) return;
  strlcpy(_text, utf8Rus(txt, _uppercase), _buffsize - 1);
  if (strcmp(_oldtext, _text) == 0) return;
  _textwidth = strlen(_text) * _charWidth;
  _x = _config.left;
  _fx = _config.left;
  _doscroll = _checkIsScrollNeeded();
  if (dsp.getScrollId() == this) dsp.setScrollId(NULL);
  _scrolldelay = millis();
  if (_active) {
    if (_doscroll) {
        gfxFillRect(gfx, _config.left,  _config.top, _width, _textheight, _bgcolor);
        snprintf(_window, _width / _charWidth + 1, "%s", _text); //TODO: прокрутка
        gfxDrawText(gfx, _config.left, _config.top, _window, _fgcolor, _bgcolor, _config.textsize);
    } else {
      gfxFillRect(gfx, _config.left, _config.top, _width, _textheight, _bgcolor);
      gfxDrawText(gfx, _realLeft(), _config.top, _text, _fgcolor, _bgcolor, _config.textsize);
    }
    strlcpy(_oldtext, _text, _buffsize);
  }
}

void ScrollWidget::setText(const char* txt, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, txt);
  setText(buf);
}

void ScrollWidget::loop() {
  if(_locked) return;
  if (!_doscroll || _config.textsize == 0 || (dsp.getScrollId() != NULL && dsp.getScrollId() != this)) return;
  if (_checkDelay(_fx == _config.left ? _startscrolldelay : _scrolltime, _scrolldelay)) {
    _fx -= _scrolldelta * 0.1f;
    if (-_fx > _textwidth + _sepwidth - _config.left) {
      _fx = _config.left;
      dsp.setScrollId(NULL);
    } else {
      dsp.setScrollId(this);
    }
    if (_active) _draw();
  }
}

void ScrollWidget::_clear(){
  if (!gfx) return;
  gfxFillRect(gfx, _config.left, _config.top, _width, _textheight, _bgcolor);
}

void ScrollWidget::_draw() {
  if(!_active || _locked || !gfx) return;
  
  // Сначала очищаем всю область виджета
  gfxFillRect(gfx, _config.left, _config.top, _width, _textheight, _bgcolor);
  
  if (_doscroll) {
    uint16_t _newx = _config.left - _fx;
    const char* _cursor = _text + _newx / _charWidth;
    uint16_t hiddenChars = _cursor - _text;
    if (hiddenChars < strlen(_text)) {
      snprintf(_window, _width / _charWidth + 1, "%s%s%s", _cursor, _sep, _text);
    } else {
      const char* _scursor = _sep + (_cursor - (_text + strlen(_text)));
      snprintf(_window, _width / _charWidth + 1, "%s%s", _scursor, _text);
    }
    int16_t drawX = _fx + hiddenChars * _charWidth;
    if (drawX < _config.left) drawX = _config.left;
    gfxDrawText(gfx, drawX, _config.top, _window, _fgcolor, _bgcolor, _config.textsize);
  } else {
    gfxDrawText(gfx, _realLeft(), _config.top, _text, _fgcolor, _bgcolor, _config.textsize);
  }
}

void ScrollWidget::_calcX() {
  if (!_doscroll || _config.textsize == 0) return;
  _x -= _scrolldelta;
  if (-_x > _textwidth + _sepwidth - _config.left) {
    _x = _config.left;
  }
}

bool ScrollWidget::_checkDelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

void ScrollWidget::_reset(){
  _x = _config.left;
  _scrolldelay = millis();
  _doscroll = _checkIsScrollNeeded();
}

/************************
      SLIDER WIDGET
 ************************/
void SliderWidget::init(FillConfig conf, uint16_t fgcolor, uint16_t bgcolor, uint32_t maxval, uint16_t oucolor) {
  Widget::init(conf.widget, fgcolor, bgcolor);
  _width = conf.width; _height = conf.height; _outlined = conf.outlined; _oucolor = oucolor, _max = maxval;
  _oldvalwidth = _value = 0;
}

void SliderWidget::setValue(uint32_t val) {
  _value = val;
  if (_active && !_locked) _drawslider();
}

void SliderWidget::_drawslider() {
  if (!gfx) return;
  uint16_t valwidth = map(_value, 0, _max, 0, _width - _outlined * 2);
  if (_oldvalwidth == valwidth) return;
  gfxFillRect(gfx, _config.left + _outlined + std::min(valwidth, _oldvalwidth), _config.top + _outlined, std::abs((int)_oldvalwidth - (int)valwidth), _height - _outlined * 2, _oldvalwidth > valwidth ? _bgcolor : _fgcolor);
  _oldvalwidth = valwidth;
}

void SliderWidget::_draw() {
  if(_locked || !gfx) return;
  _clear();
  if(!_active) return;
  if (_outlined) gfxDrawRect(gfx, _config.left, _config.top, _width, _height, _oucolor);
  uint16_t valwidth = map(_value, 0, _max, 0, _width - _outlined * 2);
  gfxFillRect(gfx, _config.left + _outlined, _config.top + _outlined, valwidth, _height - _outlined * 2, _fgcolor);
}

void SliderWidget::_clear() {
  if (!gfx) return;
  gfxFillRect(gfx, _config.left, _config.top, _width, _height, _bgcolor);
}
void SliderWidget::_reset() {
  _oldvalwidth = 0;
}
/************************
      VU WIDGET
 ************************/
VuWidget::~VuWidget() {
  // canvas больше не нужен
}

void VuWidget::init(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumincolor, uint16_t bgcolor) {
  Widget::init(wconf, bgcolor, bgcolor);
  _vumaxcolor = vumaxcolor;
  _vumincolor = vumincolor;
  _bands = bands;
}

void VuWidget::_draw() {
  if(!_active || _locked || !gfx) return;
  static float measL = 0, measR = 0;
  static uint8_t peakHoldL = 0, peakHoldR = 0;
  static uint32_t peakHoldTimeL = 0, peakHoldTimeR = 0;
  uint8_t numBands = _bands.perheight;
  uint16_t vulevel = player.get_VUlevel(numBands);
  uint8_t activeL = (vulevel >> 8) & 0xFF;
  uint8_t activeR = vulevel & 0xFF;

  // --- Peak Hold ---
  // Усиленная логика: если fading peakHold > 0 и сигнал доходит до этого или выше сегмента, сбрасываем холд и таймер
  if (peakHoldL > 0 && activeL >= peakHoldL) {
    peakHoldL = activeL;
    peakHoldTimeL = millis();
  } else if (activeL > 0 && activeL > peakHoldL) {
    peakHoldL = activeL;
    peakHoldTimeL = millis();
  } else if (millis() - peakHoldTimeL > holdTimeL && peakHoldL > 0) {
    peakHoldL = 0;
  }
  if (peakHoldR > 0 && activeR >= peakHoldR) {
    peakHoldR = activeR;
    peakHoldTimeR = millis();
  } else if (activeR > 0 && activeR > peakHoldR) {
    peakHoldR = activeR;
    peakHoldTimeR = millis();
  } else if (millis() - peakHoldTimeR > holdTimeR && peakHoldR > 0) {
    peakHoldR = 0;
  }
  // --- конец Peak Hold ---
  // Если основной VU-метр догнал fading peak hold — сбросить fading
  if (measL >= peakHoldL) peakHoldL = measL;
  if (measR >= peakHoldR) peakHoldR = measR;

  // Левый канал
  if (activeL == 0) {
    measL = 0;
  } else if (activeL >= measL) {
    measL = activeL;
  } else {
    if (measL > _bands.fadespeed) measL -= _bands.fadespeed;
    else measL = 0;
    if (measL < activeL) measL = activeL;
  }
  // Правый канал
  if (activeR == 0) {
    measR = 0;
  } else if (activeR >= measR) {
    measR = activeR;
  } else {
    if (measR > _bands.fadespeed) measR -= _bands.fadespeed;
    else measR = 0;
    if (measR < activeR) measR = activeR;
  }

  // Очистка области
  gfxFillRect(gfx, _config.left, _config.top, _bands.width, _bands.height*2 + _bands.vspace + vuChannelGap, _bgcolor);

  // Левая и правая полосы (сегменты)
  uint8_t redBands = std::max(3, numBands / 4);
  for (uint8_t i = 0; i < numBands; ++i) {
    uint16_t colorL = (i >= numBands - redBands) ? _vumaxcolor : _vumincolor;
    uint16_t colorR = colorL;
    int x = _config.left + i * (_bands.width - (numBands-1)*_bands.space) / numBands + i * _bands.space;
    int yL = _config.top;
    int yR = _config.top + _bands.height + vuChannelGap; // расстояние между каналами регулируется константой
    gfxFillRect(gfx, x, yL, (_bands.width - (numBands-1)*_bands.space) / numBands, _bands.height, i < measL ? colorL : _bgcolor);
    gfxFillRect(gfx, x, yR, (_bands.width - (numBands-1)*_bands.space) / numBands, _bands.height, i < measR ? colorR : _bgcolor);
  }
  // --- Peak Hold визуализация ---
  uint16_t peakColorL = (peakHoldL >= numBands - redBands + 1) ? _vumaxcolor : _vumincolor;
  uint16_t peakColorR = (peakHoldR >= numBands - redBands + 1) ? _vumaxcolor : _vumincolor;

  // Fading (затухание)
  float fadeL = 1.0f - float(millis() - peakHoldTimeL) / holdTimeL;
  if (fadeL < 0.2f) fadeL = 0.2f;
  float fadeR = 1.0f - float(millis() - peakHoldTimeR) / holdTimeR;
  if (fadeR < 0.2f) fadeR = 0.2f;

  auto fadeColor = [](uint16_t color, float fade) -> uint16_t {
    uint8_t r = ((color >> 11) & 0x1F) * fade;
    uint8_t g = ((color >> 5) & 0x3F) * fade;
    uint8_t b = (color & 0x1F) * fade;
    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
  };

  uint16_t fadedPeakColorL = fadeColor(peakColorL, fadeL);
  uint16_t fadedPeakColorR = fadeColor(peakColorR, fadeR);

  if (peakHoldL > 0 && peakHoldL <= numBands) {
    int x = _config.left + (peakHoldL-1) * ((_bands.width - (numBands-1)*_bands.space) / numBands + _bands.space);
    int yL = _config.top;
    gfxFillRect(gfx, x, yL, (_bands.width - (numBands-1)*_bands.space) / numBands, _bands.height, fadedPeakColorL);
  }
  if (peakHoldR > 0 && peakHoldR <= numBands) {
    int x = _config.left + (peakHoldR-1) * ((_bands.width - (numBands-1)*_bands.space) / numBands + _bands.space);
    int yR = _config.top + _bands.height + vuChannelGap;
    gfxFillRect(gfx, x, yR, (_bands.width - (numBands-1)*_bands.space) / numBands, _bands.height, fadedPeakColorR);
  }

  // === НАСТРОЙКА ПАРАМЕТРОВ VU-МЕТРА ===
  const int scaleLength = _bands.width -10;    // Общая длина ШКАЛЫ в пикселях (максимальная длина виджета + 1)
  int scaleX0 = _config.left; // начальная позиция шкалы (левый край)
  int scaleX1 = scaleX0 + scaleLength; // расчетная конечная позиция шкалы (правый край)
  int redX = scaleX0 + redZonePos * scaleLength; // расчетная позиция начала красной зоны (деление 0)
  // ===== ДОБАВЛЕНО: ОТРИСОВКА ШКАЛЫ МЕЖДУ КАНАЛАМИ  =====
  // Параметры шкалы
  // Подписи и позиции (в процентах от длины шкалы)
  struct Mark {
    int value; // значение для подписи
    float pos; // позиция (0..1)
    const char* label;
  };
  // --- Новая логика построения шкалы с двумя зонами ---
  // Метки для зелёной зоны
  const Mark marksGreen[] = {
    { -30, 0.00f, "-30" },
    { -20, 0.18f, "-20" },
    { -10, 0.36f, "-10" },
    {  -5, 0.54f,  "-5" },
    {  -1, 0.72f,  "-1" }
  };
  const int numGreenMarks = sizeof(marksGreen)/sizeof(marksGreen[0]);
  // Метки для красной зоны
  const Mark marksRed[] = {
    { 0, 0.0f, "0" },
    { 1, 0.5f, "+1" },
    { 3, 1.0f, "+3" }
  };
  const int numRedMarks = sizeof(marksRed)/sizeof(marksRed[0]);

  // --- Зелёная часть шкалы ---
  for (int m = 0; m < numGreenMarks; ++m) {
    float frac = marksGreen[m].pos; // логарифмически или вручную подобрано
    int x = scaleX0 + frac * (redX - scaleX0);
    int h = scaleHeightLong;
    uint16_t color = _vumincolor;
    // Деление
    for(int i = 0; i < scaleThickness; i++) {
      gfxDrawLine(gfx, x+i, _config.top + _bands.height + (vuChannelGap / 2) - h/2, x+i, _config.top + _bands.height + (vuChannelGap / 2) + h/2, color);
    }
    // Подписи
    int textY = _config.top + _bands.height + (vuChannelGap / 2) + h/2 + labelOffsetBottom;
    int textX = x - strlen(marksGreen[m].label)*3;
    int textYtop = _config.top + _bands.height + (vuChannelGap / 2) - h/2 + labelOffsetTop;
    int textXtop = textX;
    if (strcmp(marksGreen[m].label, "-30") == 0) {
      textX += labelLeftmostOffset;
      textXtop += labelLeftmostOffset;
    }
    gfxDrawText(gfx, textX, textY, marksGreen[m].label, color, _bgcolor, fontSize);
    gfxDrawText(gfx, textXtop, textYtop, marksGreen[m].label, color, _bgcolor, fontSize);
  }
  // --- Красная часть шкалы ---
  for (int m = 0; m < numRedMarks; ++m) {
    float frac = marksRed[m].pos;
    int x = redX + frac * (scaleX1 - redX);
    int h = scaleHeightLong;
    uint16_t color = _vumaxcolor;
    // Деление
    for(int i = 0; i < scaleThickness; i++) {
      gfxDrawLine(gfx, x+i, _config.top + _bands.height + (vuChannelGap / 2) - h/2, x+i, _config.top + _bands.height + (vuChannelGap / 2) + h/2, color);
    }
    // Подписи
    int textY = _config.top + _bands.height + (vuChannelGap / 2) + h/2 + labelOffsetBottom;
    int textX = x - strlen(marksRed[m].label)*3;
    int textYtop = _config.top + _bands.height + (vuChannelGap / 2) - h/2 + labelOffsetTop;
    int textXtop = textX;
    gfxDrawText(gfx, textX, textY, marksRed[m].label, color, _bgcolor, fontSize);
    gfxDrawText(gfx, textXtop, textYtop, marksRed[m].label, color, _bgcolor, fontSize);
  }
  // --- Короткие деления (зелёная зона) ---
  for (int i = 0; i < 20; ++i) {
    float frac = (float)i / 20.0f;
    int x = scaleX0 + frac * (redX - scaleX0);
    // Проверяем, не совпадает ли с длинным делением
    bool isLong = false;
    for (int m = 0; m < numGreenMarks; ++m) {
      if (fabs(frac - marksGreen[m].pos) < 0.025f) { isLong = true; break; }
    }
    if (!isLong) {
      int h = scaleHeightShort;
      for(int j = 0; j < scaleThickness; j++) {
        gfxDrawLine(gfx, x+j, _config.top + _bands.height + (vuChannelGap / 2) - h/2, x+j, _config.top + _bands.height + (vuChannelGap / 2) + h/2, _vumincolor);
      }
    }
  }
  // --- Короткие деления (красная зона) ---
  // Только по одному между длинными делениями
  for (int m = 0; m < numRedMarks - 1; ++m) {
    float frac1 = marksRed[m].pos;
    float frac2 = marksRed[m+1].pos;
    float midFrac = (frac1 + frac2) / 2.0f;
    int x = redX + midFrac * (scaleX1 - redX);
    int h = scaleHeightShort;
    for(int i = 0; i < scaleThickness; i++) {
      gfxDrawLine(gfx, x+i, _config.top + _bands.height + (vuChannelGap / 2) - h/2, x+i, _config.top + _bands.height + (vuChannelGap / 2) + h/2, _vumaxcolor);
    }
  }
}

void VuWidget::loop(){
  if(_active || !_locked) _draw();
}

void VuWidget::_clear(){
  if (!gfx) return;
  const int vuChannelGap = 48;
  gfxFillRect(gfx, _config.left, _config.top, _bands.width, _bands.height*2 + _bands.vspace + vuChannelGap, _bgcolor);
}
/************************
      NUM WIDGET
 ************************/
void NumWidget::init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) {
  Widget::init(wconf, fgcolor, bgcolor);
  _buffsize = buffsize;
  _text = (char *) malloc(sizeof(char) * _buffsize);
  memset(_text, 0, _buffsize);
  _oldtext = (char *) malloc(sizeof(char) * _buffsize);
  memset(_oldtext, 0, _buffsize);
  _textwidth = _oldtextwidth = _oldleft = 0;
  _uppercase = uppercase;
  dsp.charSize(wconf.textsize, _charWidth, _textheight);
}

void NumWidget::setText(const char* txt) {
  if (!gfx) return;
  strlcpy(_text, txt, _buffsize);
  _getBounds();
  if (strcmp(_oldtext, _text) == 0) return;
  uint16_t realth = _textheight;
  if (_active) gfxFillRect(gfx, _oldleft == 0 ? _realLeft() : std::min(_oldleft, _realLeft()),  _config.top-_textheight+1, std::max(_oldtextwidth, _textwidth), realth, _bgcolor);
  _oldtextwidth = _textwidth;
  _oldleft = _realLeft();
  if (_active) loop();
}

void NumWidget::setText(int val, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, val);
  setText(buf);
}

void NumWidget::_getBounds() {
  _textwidth= dsp.textWidth(_text);
}

void NumWidget::_draw() {
  if(!_active || !gfx) return;
  _clear();
  gfxDrawText(gfx, _realLeft(), _config.top, _text, _fgcolor, _bgcolor, _config.textsize, &DS_DIGI56pt7b);
  strlcpy(_oldtext, _text, _buffsize);
}

void NumWidget::_clear() {
    if (!gfx) return;
    uint16_t maxWidth = dsp.textWidth("888");
    uint16_t left = (dsp.width() - maxWidth) / 2;
    uint16_t clearHeight = 80; // вручную подобранная высота
    uint16_t top = _config.top - clearHeight ;
    gfxFillRect(gfx, left, top, maxWidth, clearHeight, _bgcolor);
}

/**************************
      PROGRESS WIDGET
 **************************/
void ProgressWidget::_progress() {
  if (!gfx) {
    Serial.println("[ProgressWidget] gfx is nullptr!");
    return;
  }
  
  char buf[_width + 1];
  snprintf(buf, _width, "%*s%.*s%*s", _pg <= _barwidth ? 0 : _pg - _barwidth, "", _pg <= _barwidth ? _pg : 5, ".....", _width - _pg, "");
  _pg++; if (_pg >= _width + _barwidth) _pg = 0;
  
  // Очищаем предыдущий текст
  _clear();
  
  // Рисуем новый текст
  gfxDrawText(gfx, _realLeft(), _config.top, buf, _fgcolor, _bgcolor, _config.textsize);
}

bool ProgressWidget::_checkDelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

void ProgressWidget::loop() {
  if (!gfx) {
    Serial.println("[ProgressWidget] gfx is nullptr in loop!");
    return;
  }
  
  if (_checkDelay(_speed, _scrolldelay)) {
    _progress();
  }
}

void ProgressWidget::_clear() {
  if (!gfx) return;
  gfxFillRect(gfx, _config.left, _config.top, _width * _config.textsize, _config.textsize * 8, _bgcolor);
}

/**************************
      CLOCK WIDGET
 **************************/
void ClockWidget::draw(){
  if(!_active || !gfx) return;
  dsp.printClock(_config.top, _config.left, _config.textsize, false);
}

void ClockWidget::_draw(){
  if(!_active || !gfx) return;
  dsp.printClock(_config.top, _config.left, _config.textsize, true);
}

void ClockWidget::_clear(){
  if (!gfx) return;
  dsp.clearClock();
}

/**************************
      BITRATE WIDGET
 **************************/
void BitrateWidget::init(BitrateConfig bconf, uint16_t fgcolor, uint16_t bgcolor){
  Widget::init(bconf.widget, fgcolor, bgcolor);
  _dimension = bconf.dimension;
  _bitrate = 0;
  _format = BF_UNCNOWN;
  dsp.charSize(bconf.widget.textsize, _charWidth, _textheight);
  memset(_buf, 0, 6);
}

void BitrateWidget::setBitrate(uint16_t bitrate){
  _bitrate = bitrate;
  _draw();
}

void BitrateWidget::setFormat(BitrateFormat format){
  _format = format;
  _draw();
}

void BitrateWidget::_draw(){
  _clear();
  if(!_active || _format == BF_UNCNOWN || !gfx) return;

  gfxDrawRect(gfx, _config.left, _config.top, _dimension, _dimension, _fgcolor);
  gfxFillRect(gfx, _config.left, _config.top + _dimension/2+1, _dimension, _dimension/2-1, _fgcolor);

  if(_bitrate < 1000) {
    snprintf(_buf, 6, "%d", (int)_bitrate);
  } else {
    float _br = (float)_bitrate / 1000;
    snprintf(_buf, 6, "%.1f", _br);
  }

  gfxDrawText(gfx, _config.left + _dimension/2 - _charWidth*strlen(_buf)/2 + 1, _config.top + _dimension/4 - _textheight/2 + 2, _buf, _fgcolor, _bgcolor, _config.textsize);

  const char* fmt = nullptr;
  switch(_format){
    case BF_MP3:  fmt = "mp3"; break;
    case BF_AAC:  fmt = "aac"; break;
    case BF_FLAC: fmt = "flc"; break;
    case BF_WAV:  fmt = "wav"; break;
    case BF_OGG:  fmt = "ogg"; break;
    case BF_VOR:  fmt = "vor"; break;
    case BF_OPU:  fmt = "opu"; break;
    default:     fmt = ""; break;
  }
  gfxDrawText(gfx, _config.left + _dimension/2 - _charWidth*3/2 + 1, _config.top + _dimension - _dimension/4 - _textheight/2, fmt, _bgcolor, _fgcolor, _config.textsize);
}

void BitrateWidget::_clear() {
  if (!gfx) return;
  gfxFillRect(gfx, _config.left, _config.top, _dimension*1.08, _dimension, _bgcolor);
}

#endif // #if DSP_MODEL!=DSP_DUMMY
