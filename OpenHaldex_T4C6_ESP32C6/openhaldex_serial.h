#ifndef OPENHALDEX_SERIAL_H
#define OPENHALDEX_SERIAL_H

// Expected response lengths
#define OPENHALDEX_SERIAL_GET_ALL_LENGTH 7
#define OPENHALDEX_SERIAL_GET_MODE_LENGTH 1
#define OPENHALDEX_SERIAL_SET_MODE_LENGTH 1
#define OPENHALDEX_SERIAL_INCREMENT_MODE_LENGTH 0
#define OPENHALDEX_SERIAL_DECREMENT_MODE_LENGTH 0

// UART configuration
#define OPENHALDEX_SERIAL_PORT Serial1
#define OPENHALDEX_SERIAL_BAUDRATE 250000
#define OPENHALDEX_SERIAL_RX 2
#define OPENHALDEX_SERIAL_TX 3
#define OPENHALDEX_SERIAL_TIMEOUT_SYMBOLS 2

// EventGroup used for detecting new data on the UART
EventGroupHandle_t OPENHALDEX_event_group;
#define OPENHALDEX_event_group_UART_RX_BIT (1 << 0)

// Callback triggered when new data is received on the UART
void openhaldexSerial_onReceiveFunction()
{
  // Set the bit in the event group.
  xEventGroupSetBits(OPENHALDEX_event_group, OPENHALDEX_event_group_UART_RX_BIT);
}

// Callback used for clearing the UART's RX buffer
void clear_rx_function()
{
  // Clear the RX buffer.
  while (OPENHALDEX_SERIAL_PORT.available())
  {
    OPENHALDEX_SERIAL_PORT.read();
  }

  // Clear the bit in the event group.
  xEventGroupClearBits(OPENHALDEX_event_group, OPENHALDEX_event_group_UART_RX_BIT);
}

// Callback used for clearing the UART's TX buffer
void clear_tx_function()
{
  // Wait for the bytes to be sent.
  OPENHALDEX_SERIAL_PORT.flush();
}

// Callback used for receiving data from the UART
bool rx_function(size_t *received_length, uint8_t *buffer, size_t buffer_size, TickType_t timeout_ticks)
{
  // Wait for data to become available, for at most the timeout time.
  EventBits_t bits = xEventGroupWaitBits(OPENHALDEX_event_group, OPENHALDEX_event_group_UART_RX_BIT, pdTRUE, pdFALSE, timeout_ticks);
  if (!(bits & OPENHALDEX_event_group_UART_RX_BIT))
  {
    return false;
  }

  // Determine how many bytes to read.
  *received_length = OPENHALDEX_SERIAL_PORT.available();
  size_t bytes_to_copy = *received_length;
  if (*received_length > buffer_size)
  {
    bytes_to_copy = buffer_size;
  }

  // Read the bytes.
  OPENHALDEX_SERIAL_PORT.read(buffer, bytes_to_copy);
  return true;
}

// Callback used for sending data on the UART
void tx_function(uint8_t data)
{
  // Send the bytes.
  OPENHALDEX_SERIAL_PORT.write(data);
}

// Class which handles serial communication with the T4
#include "openhaldex_serial_master.h"
openhaldexSerialMaster ohSerial(clear_rx_function, clear_tx_function, rx_function, tx_function);

#endif
