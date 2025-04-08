#ifndef OPENHALDEX_BUTTON_H
#define OPENHALDEX_BUTTON_H

// This class handles debouncing and detecting multi-clicks/holds for buttons:
#include "button_class.h"
button mode_button; // Object for the "Mode" button

// Functions

void poll_mode_button()
{
  // Update the button object with the current state of the "Mode" button.
  mode_button.update(digitalRead(GPIO_MODE_BUTTON));

  // Change the mode if the button was clicked/held/etc.
  if (!mode_button.getChanged())
  {
    return;
  }

  DEBUG("Mode button pressed");

  // mode_override is disabled when switching modes with the button.
  state.mode_override = false;

  // Determine the next mode in the sequence.
  uint8_t next_mode = (uint8_t)state.mode + 1;

  // If the next mode is valid, change the current mode to it.
  if (next_mode < (uint8_t)openhaldex_mode_t_MAX - 1)
  {
    state.mode = (openhaldex_mode_t)next_mode;
  }
  // On overflow, start over from MODE_STOCK.
  else
  {
    state.mode = MODE_STOCK;
  }

  // Only allow CUSTOM mode after receiving custom lockpoint data (TODO).
  if (state.mode == MODE_CUSTOM)
  {
    if (!custom_mode_available)
    {
      state.mode = MODE_STOCK;
    }
  }

  DEBUG("Goto mode: %s", get_openhaldex_mode_string(state.mode));
}

#endif
