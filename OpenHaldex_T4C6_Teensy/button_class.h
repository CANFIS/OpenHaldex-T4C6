#ifndef BUTTON_CLASS_H
#define BUTTON_CLASS_H

#include <Arduino.h>

class button
{
  public:
    void setShortClickTime(unsigned long value);
    void setLongClickTime(unsigned long value);
    void update(bool current_state, unsigned long current_time_ms = 0);
    void pause();
    void resume();
    bool getChanged();
    void setChanged();
    int8_t getClicks();
    void setClicks(int8_t value);
    void repeatLongClick();
    void stopLongClick();
    
  private:
    unsigned long short_click_time = 50, long_click_time = 500, last_time;
    bool last_state, changed, paused, repeating_long_click;
    int8_t clicks, click_counter;
};

#endif
