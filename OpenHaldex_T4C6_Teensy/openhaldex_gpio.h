#ifndef OPENHALDEX_GPIO_H
#define OPENHALDEX_GPIO_H

// Pin assignments
#define GPIO_MODE_BUTTON 12
#define GPIO_LED_R 11
#define GPIO_LED_G 10
#define GPIO_LED_B 9

// Functions

// Unused
void blinkLED(int duration, int flashes, uint8_t R, uint8_t G, uint8_t B)
{
  for (int i = 0; i < flashes; i++)
  {
    analogWrite(GPIO_LED_R, R);
    analogWrite(GPIO_LED_G, G);
    analogWrite(GPIO_LED_B, B);
    delay(duration);
    analogWrite(GPIO_LED_R, 0);
    analogWrite(GPIO_LED_G, 0);
    analogWrite(GPIO_LED_B, 0);
    delay(duration);
  }
}

void init_GPIO()
{
  // Disable unwanted Teensy optionals.
  CCM_ANALOG_PLL_AUDIO |= CCM_ANALOG_PLL_AUDIO_POWERDOWN;
  CCM_ANALOG_PLL_VIDEO |= CCM_ANALOG_PLL_VIDEO_POWERDOWN;
  CCM_ANALOG_PLL_ENET |= CCM_ANALOG_PLL_ENET_POWERDOWN;

  // Turn off the power LED.
  pinMode(LED_BUILTIN, OUTPUT);

  // Configure the RGB LED pins as outputs.
  pinMode(GPIO_LED_R, OUTPUT);
  pinMode(GPIO_LED_G, OUTPUT);
  pinMode(GPIO_LED_B, OUTPUT);
  
  // Configure the "Mode" button pin as input.
  pinMode(GPIO_MODE_BUTTON, INPUT);

  // Attach an interrupt to the Mode button.
  //attachInterrupt(GPIO_MODE_BUTTON, mode_button_ISR, HIGH);
}

void show_current_mode_LED()
{
  switch (state.mode)
  {
    case MODE_STOCK:
      analogWrite(GPIO_LED_R, 20);
      analogWrite(GPIO_LED_G, 0);
      analogWrite(GPIO_LED_B, 0);
      break;
    case MODE_FWD:
      analogWrite(GPIO_LED_R, 0);
      analogWrite(GPIO_LED_G, 20);
      analogWrite(GPIO_LED_B, 0);
      break;
    case MODE_5050:
      analogWrite(GPIO_LED_R, 0);
      analogWrite(GPIO_LED_G, 0);
      analogWrite(GPIO_LED_B, 20);
      break;
    case MODE_7525:
      analogWrite(GPIO_LED_R, 20);
      analogWrite(GPIO_LED_G, 20);
      analogWrite(GPIO_LED_B, 20);
      break;
    case MODE_CUSTOM:
      analogWrite(GPIO_LED_R, 20);
      analogWrite(GPIO_LED_G, 0);
      analogWrite(GPIO_LED_B, 20);
      break;
    default:
      break;
  }
}

#endif
