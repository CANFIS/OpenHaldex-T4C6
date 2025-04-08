/*
  Base code by Adam Forbes (https://github.com/adamforbes92/OpenHaldex-T4)
  Base base code by Banging Donk (https://github.com/ABangingDonk/OpenHaldexT4)

  Modifications:
    -different pins used
    -removed standalone mode
    -removed Bluetooth by HC-05
    -removed interrupt for "Mode" button in favor of polling
    -fixed vehicle speed reading (using message 0x5A0 instead of 0x288)
    -added serial communication with ESP32-C6 for Wi-Fi

  TODOs:
    -CUSTOM mode by C6
*/

// Settings
#define HALDEX_GENERATION 4
#define SOFTWARE_VERSION 0x006C // 0.108
//#define BROADCAST_OPENHALDEX_ON_CAN // Comment out to disable

// Debug (comment out to disable)
//#define ENABLE_DEBUG
//#define DEBUG_HALDEXCAN_TRAFFIC
//#define DEBUG_CHASSISCAN_TRAFFIC

// Files
#include "openhaldex_definitions.h"
#include "openhaldex.h"
#include "openhaldex_gpio.h"
#include "openhaldex_c6_serial.h"
#include "openhaldex_can.h"
#include "openhaldex_calculations.h"
#include "openhaldex_eeprom.h"
#include "openhaldex_button.h"

// Timer used for repeating tasks
#include <arduino-timer.h>
auto timer = timer_create_default();

void setup()
{
#ifdef ENABLE_DEBUG
  Serial.begin(115200);
  DEBUG("OpenHaldex T4C6 - Teensy");
#endif

  DEBUG("Init CAN");
  init_CAN();

  DEBUG("Init EEPROM");
  init_EEPROM();

  // Update the values stored in EEPROM every 2.5 seconds.
  timer.every(2500, update_EEPROM);

#ifdef BROADCAST_OPENHALDEX_ON_CAN
  // Broadcast OpenHaldex data on CAN every 50ms.
  timer.every(50, broadcast_openhaldex_data);
#endif

  DEBUG("Init GPIO");
  init_GPIO();

  DEBUG("Init C6 serial");
  OPENHALDEX_C6_SERIAL_PORT.begin(OPENHALDEX_C6_SERIAL_BAUDRATE);
}

void loop()
{
  // Update the timer.
  timer.tick();

  // Update the "Mode" button and change the mode if it was pressed.
  poll_mode_button();

  // While bytes are available from the C6, read and parse them.
  while (OPENHALDEX_C6_SERIAL_PORT.available())
  {
    // Pointer to object into which full frames are emitted upon being received:
    static openhaldexSerialSlave::openhaldex_serial_slave_frame_t *received_frame;

    // Parse the current byte.
    // If a ful frame is emitted, the returned pointer will be non-NULL.
    if ((received_frame = ohSerial.parseByte(OPENHALDEX_C6_SERIAL_PORT.read())) != NULL)
    {
      // Parse the frame, and (always) send a response.
      openhaldex_c6_serial_handle_frame(received_frame);
    }
  }

  // Light up the LED according to the current mode.
  show_current_mode_LED();
}
