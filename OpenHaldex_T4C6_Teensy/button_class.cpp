#include "button_class.h"

void button::setShortClickTime(unsigned long value)
{
  short_click_time = value;
}

void button::setLongClickTime(unsigned long value)
{
  long_click_time = value;
}

void button::update(bool current_state, unsigned long current_time_ms)
{
  if (paused)
  {
    return;
  }

  // Get the current millisecond.
  unsigned long current_time = current_time_ms;
  if (current_time_ms == 0)
  {
    current_time = millis();
  }

  // If the button has been released while waiting for a repeated longClick, stop waiting.
  if (repeating_long_click && !current_state)
  {
    repeating_long_click = false;
    click_counter = 0;
  }

  // released->pressed => increment counter, start timer
  // pressed->released => start timer
  if (current_state != last_state)
  {
    if (current_state)
    {
      click_counter++;
    }

    last_time = current_time;
    last_state = current_state;
  }
  else if (repeating_long_click && ((current_time - last_time) > long_click_time))
  {
    repeatLongClick();
  }

  // released, short click timer expired
  if (!changed && !repeating_long_click && !current_state && ((current_time - last_time) > short_click_time))
  {
    if ((clicks = click_counter))
    {
      changed = true;
      click_counter = 0;
    }
  }

  // pressed, long click timer expired
  if (!changed && current_state && ((current_time - last_time) > long_click_time))
  {
    if ((clicks = -click_counter))
    {
      changed = true;
      click_counter = 0;
    }
  }
}

void button::pause()
{
  paused = true;
}

void button::resume()
{
  paused = false;
}

bool button::getChanged()
{
  if (changed)
  {
    changed = false;
    return true;
  }
  else
  {
    return false;
  }
}

void button::setChanged()
{
  changed = true;
}

int8_t button::getClicks()
{
  return clicks;
}

void button::setClicks(int8_t value)
{
  clicks = value;
}

void button::repeatLongClick()
{
  repeating_long_click = true;

  // Make the update() function believe the button has been released momentarily.
  last_state = 0;
}

void button::stopLongClick()
{
  if (repeating_long_click)
  {
    repeating_long_click = false;

    click_counter = 0;
    last_state = 1;
    changed = false;
  }
}
