#include "openhaldex_serial_master.h"

// Constructor
openhaldexSerialMaster::openhaldexSerialMaster(clear_rx_t clear_rx_function, clear_tx_t clear_tx_function, rx_t rx_function, tx_t tx_function)
{
  // Initialize private members.
  this->clear_rx_function = clear_rx_function;
  this->clear_tx_function = clear_tx_function;
  this->rx_function = rx_function;
  this->tx_function = tx_function;
  interface_mutex = xSemaphoreCreateMutex();
}

// Send a request and store the response
int16_t openhaldexSerialMaster::transaction(openhaldex_serial_message_t *message_to_send, openhaldex_serial_message_t *received_message)
{
  // Ensure the provided pointers are valid.
  if (!message_to_send || !received_message)
  {
    return -(PARAMETER_ERROR);
  }

  // Take the mutex.
  xSemaphoreTake(interface_mutex, portMAX_DELAY);

  // Send the request.
  write_request(message_to_send->opcode, message_to_send->subcode, message_to_send->data, message_to_send->data_length);

  // Read the response.
  size_t received_bytes = 0;
  read_error_t ret = read_response(message_to_send->opcode, message_to_send->subcode, &received_bytes, received_message->data, OPENHALDEX_SERIAL_MAX_PAYLOAD_SIZE, pdMS_TO_TICKS(OPENHALDEX_RESPONSE_TIMEOUT_MS));

  // Release the mutex.
  xSemaphoreGive(interface_mutex);

  // Return any errors.
  if (ret != NO_ERROR)
  {
    return -ret;
  }

  // If no error occurred, return the received length.
  received_message->data_length = received_bytes;
  return received_bytes;
}

// Convert an error enum value to string
const char* openhaldexSerialMaster::read_error_t_to_string(read_error_t err)
{
  switch (err)
  {
    case NO_ERROR:
      return "NO_ERROR";
    case PARAMETER_ERROR:
      return "PARAMETER_ERROR";
    case TIMEOUT_ERROR:
      return "TIMEOUT_ERROR";
    case SIZE_ERROR:
      return "SIZE_ERROR";
    case FORMAT_ERROR:
      return "FORMAT_ERROR";
    case DATA_ERROR:
      return "DATA_ERROR";
    case CHECKSUM_ERROR:
      return "CHECKSUM_ERROR";
  }
  return "?";
}

// Calculate the checksum for a message
uint8_t openhaldexSerialMaster::calculate_checksum(uint8_t opcode, uint8_t subcode, uint8_t *data, size_t data_length)
{
  // Start the checksum from the opcode and subcode bytes.
  uint8_t checksum = 0x55 ^ opcode ^ subcode;

  // If the message contains data, XOR the checksum by each byte and, at the end, by the length.
  if (data)
  {
    for (size_t i = 0; i < data_length; i++)
    {
      checksum ^= data[i];
    }
    checksum ^= data_length;
  }
  return checksum;
}

// Send a request
void openhaldexSerialMaster::write_request(uint8_t opcode, uint8_t subcode, uint8_t *data, size_t data_length)
{
  // Clear the RX buffer.
  clear_rx_function();

  // Transmit the header, opcode and subcode.
  tx_function(OPENHALDEX_MSG_START[0]);
  tx_function(OPENHALDEX_MSG_START[1]);
  tx_function(opcode);
  tx_function(subcode);

  // If the message contains data, transmit the length and each byte.
  if (data)
  {
    // If the maximum length is exceeded, transmit 0 bytes.
    if (data_length > OPENHALDEX_SERIAL_MAX_PAYLOAD_SIZE)
    {
      data_length = 0;
    }

    // Send the length.
    tx_function(data_length);

    // Send the bytes.
    for (size_t i = 0; i < data_length; i++)
    {
      tx_function(data[i]);
    }
  }
  else
  {
    // If the message doesn't contain data, transmit a 0 as the length.
    tx_function(0);
  }

  // Transmit the checksum and footer.
  tx_function(calculate_checksum(opcode, subcode, data, data_length));
  tx_function(OPENHALDEX_MSG_END[0]);
  tx_function(OPENHALDEX_MSG_END[1]);

  // Wait for the bytes to be sent.
  clear_tx_function();
}

// Read a response
openhaldexSerialMaster::read_error_t openhaldexSerialMaster::read_response(uint8_t opcode, uint8_t subcode, size_t *received_bytes, uint8_t *buffer, size_t buffer_size, TickType_t timeout_ticks)
{
  // If the length pointer is valid, store a 0 in case an error occurrs.
  if (received_bytes)
  {
    *received_bytes = 0;
  }

  // Ensure the buffer is valid.
  if (!buffer || !buffer_size)
  {
    return PARAMETER_ERROR;
  }

  // Try to receive from the UART.
  size_t avail = 0;
  if (!rx_function(&avail, buffer, buffer_size, timeout_ticks))
  {
    return TIMEOUT_ERROR;
  }

  // Ensure the response fits in the buffer.
  if (avail > buffer_size)
  {
    return SIZE_ERROR;
  }

  // Ensure the header, footer and length are valid.
  if (
    buffer[0] != OPENHALDEX_MSG_START[0] ||
    buffer[1] != OPENHALDEX_MSG_START[1] ||
    buffer[avail - 2] != OPENHALDEX_MSG_END[0] ||
    buffer[avail - 1] != OPENHALDEX_MSG_END[1] ||
    avail < 8 // START0, START1, OPCODE, SUBCODE, DATALENGTH, CHECKSUM, END0, END1
  )
  {
    return FORMAT_ERROR;
  }

  // Ensure the received opcode and subcode match what was expected.
  if (buffer[2] != opcode || buffer[3] != subcode)
  {
    return DATA_ERROR;
  }

  // Ensure all data bytes were received.
  uint8_t data_length = buffer[4];
  if (data_length != (avail - 8))
  {
    return SIZE_ERROR;
  }

  // Get a pointer to the start of the actual data.
  uint8_t *data = &buffer[5];

  // Calculate the expected checksum.
  uint8_t checksum = calculate_checksum(opcode, subcode, data, data_length);

  // Ensure the received checksum matches what was expected.
  if (data[data_length] != checksum)
  {
    return CHECKSUM_ERROR;
  }

  // Copy the data into the buffer.
  memcpy(buffer, data, data_length);
  
  // If the length pointer is valid, store the received message length.
  if (received_bytes)
  {
    *received_bytes = data_length;
  }
  return NO_ERROR;
}

const uint8_t openhaldexSerialMaster::OPENHALDEX_MSG_START[] = ">["; // Header
const uint8_t openhaldexSerialMaster::OPENHALDEX_MSG_END[] = "]<"; // Footer
