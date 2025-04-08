#ifndef HARDWARE_H
#define HARDWARE_H

// This class handles debouncing and detecting multi-clicks/holds for buttons:
#include "button_class.h"
button wifi_button; // Object for the "Wi-Fi" button

#ifndef TESTING_WITHOUT_TEENSY
// Default pin for the "Wi-Fi" button
#define WIFI_BUTTON_PIN 15
#else
// Different pins, more accessible for me for testing
#define WIFI_BUTTON_PIN 12
#define WIFI_BUTTON_3V3_PIN 14
#endif

void configure_gpio()
{
#ifndef TESTING_WITHOUT_TEENSY
  pinMode(WIFI_BUTTON_PIN, INPUT);
#else
  pinMode(WIFI_BUTTON_PIN, INPUT_PULLDOWN);
  pinMode(WIFI_BUTTON_3V3_PIN, OUTPUT);
  digitalWrite(WIFI_BUTTON_3V3_PIN, HIGH);
#endif
}

void configure_serial()
{
  OPENHALDEX_event_group = xEventGroupCreate();
  OPENHALDEX_SERIAL_PORT.begin(OPENHALDEX_SERIAL_BAUDRATE, SERIAL_8N1, OPENHALDEX_SERIAL_RX, OPENHALDEX_SERIAL_TX);
  OPENHALDEX_SERIAL_PORT.setRxTimeout(OPENHALDEX_SERIAL_TIMEOUT_SYMBOLS);
  OPENHALDEX_SERIAL_PORT.onReceive(openhaldexSerial_onReceiveFunction, true);
}

void setup_hardware()
{
  configure_gpio();
  configure_littlefs();
  configure_serial();
  configure_wifi();
}

#endif
