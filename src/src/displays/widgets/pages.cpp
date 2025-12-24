#include "../dspcore.h"
#if DSP_MODEL!=DSP_DUMMY

#include "pages.h"
#include "widgets.h"

void Pager::begin(){

}

void Pager::loop(){
  for(const auto& p: _pages)
    if(p->isActive()) p->loop();
}

Page& Pager::addPage(Page* page, bool setNow){
  _pages.add(page);
  if(setNow) setPage(page);
  return *page;
}

bool Pager::removePage(Page* page){
  page->setActive(false);
  dsp.clearDsp();
  return _pages.remove(page);
}

void Pager::setPage(Page* page, bool black){
  for(const auto& p: _pages) p->setActive(false);
  dsp.clearDsp(black);
  page->setActive(true);
}

Page* Pager::getActivePage() const {
  for(const auto& p: _pages) {
    if(p->isActive()) return p;
  }
  return nullptr;
}


/*******************************************************/

Page::Page() : _widgets(LinkedList<Widget * >([](Widget * wd) { delete wd;})), _pages(LinkedList<Page*>([](Page* pg){ delete pg; })) {
  _active = false;
}

Page::~Page() {
  for (const auto& w : _widgets) removeWidget(w);
}

void Page::loop() {
  if(_active) for (const auto& w : _widgets) w->loop();
}

Widget& Page::addWidget(Widget* widget) {
  _widgets.add(widget);
  widget->setActive(_active, _active);
  return *widget;
}

bool Page::removeWidget(Widget* widget){
  widget->setActive(false, _active);
  return _widgets.remove(widget);
}

Page& Page::addPage(Page* page){
  _pages.add(page);
  return *page;
}

bool Page::removePage(Page* page){
  return _pages.remove(page);
}

void Page::setActive(bool act) {
  for(const auto& w: _widgets) w->setActive(act);
  for(const auto& p: _pages) p->setActive(act);
  _active = act;
}

bool Page::isActive() {
  return _active;
}

// Получить порядковый индекс ScrollWidget'а среди всех ScrollWidget'ов в этой странице
// Считает только ScrollWidget'ы, которые могут скроллиться (через canParticipateInScroll)
int16_t Page::getScrollWidgetIndex(void* widget) const {
  int16_t scrollIndex = -1;
  for (const auto& w : _widgets) {
    // Безопасная проверка типа через виртуальный метод isScrollWidget()
    if (!w || !w->isScrollWidget()) {
      continue; // Пропускаем виджеты, которые не являются ScrollWidget'ами
    }
    // Теперь безопасно приводим к ScrollWidget*
    ScrollWidget* sw = static_cast<ScrollWidget*>(w);
    if (sw) {
      // Проверяем, что виджет может скроллиться
      if (sw->canParticipateInScroll()) {
        scrollIndex++;
        if (sw == widget) {
          return scrollIndex;
        }
      }
    }
  }
  return -1; // Виджет не найден или не может скроллиться
}

// Получить количество ScrollWidget'ов, которые могут скроллиться
int16_t Page::getScrollableCount() {
  int16_t count = 0;
  for (const auto& w : _widgets) {
    // Безопасная проверка типа через виртуальный метод isScrollWidget()
    if (!w || !w->isScrollWidget()) {
      continue; // Пропускаем виджеты, которые не являются ScrollWidget'ами
    }
    // Теперь безопасно приводим к ScrollWidget*
    ScrollWidget* sw = static_cast<ScrollWidget*>(w);
    if (sw && sw->canParticipateInScroll()) {
      count++;
    }
  }
  return count;
}

#endif // #if DSP_MODEL!=DSP_DUMMY
