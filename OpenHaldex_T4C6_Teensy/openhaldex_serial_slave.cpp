#include "openhaldex_serial_slave.h"

// Constructor
openhaldexSerialSlave::openhaldexSerialSlave()
{
  // Initialize private members.
  state = STATE_RX_START1;
  last_received_byte_ms = 0;
}

// Parse a byte received via UART
openhaldexSerialSlave::openhaldex_serial_slave_frame_t* openhaldexSerialSlave::parseByte(uint8_t rx)
{
  // Get the current time, to check for timeouts.
  unsigned long current_ms = millis();
  if (current_ms - last_received_byte_ms >= RX_TIMEOUT_MS)
  {
    // If too much time has passed since the last byte, return to the idle state (looking for the first byte of the header).
    state = STATE_RX_START1;
  }
  last_received_byte_ms = current_ms;

  // State machine
  switch (state)
  {
    // Looking for first byte of header
    case STATE_RX_START1:
      {
        // If the byte matches what is expected, go to the next state.
        if (rx == OPENHALDEX_MSG_START[0])
        {
          state = STATE_RX_START2;
        }
      }
      break;

    // Looking for second byte of header
    case STATE_RX_START2:
      {
        // If the byte matches what is expected, go to the next state.
        if (rx == OPENHALDEX_MSG_START[1])
        {
          state = STATE_RX_OPCODE;
        }
        // Otherwise, return to the idle state.
        else
        {
          state = STATE_RX_START1;
        }
      }
      break;

    // Looking for opcode
    case STATE_RX_OPCODE:
      {
        // Store the value and go to the next state.
        current_frame.opcode = (openhaldex_serial_slave_opcode_t)rx;
        state = STATE_RX_SUBCODE;
      }
      break;

    // Looking for subcode
    case STATE_RX_SUBCODE:
      {
        // Store the value and go to the next state.
        current_frame.subcode = (openhaldex_serial_slave_subcode_t)rx;
        state = STATE_RX_DATA_LENGTH;
      }
      break;

    // Looking for length
    case STATE_RX_DATA_LENGTH:
      {
        // Store the value.
        current_frame.data_length = rx;

        // If the length is non-zero, expect to receive data next.
        if (current_frame.data_length)
        {
          received_data_index = 0;
          state = STATE_RX_DATA;
        }
        // Otherwise, expect to receive the checksum next.
        else
        {
          state = STATE_RX_CHECKSUM;
        }
      }
      break;

    // Looking for data
    case STATE_RX_DATA:
      {
        // Store the value and increment the index.
        current_frame.data[received_data_index] = rx;
        received_data_index++;

        // If all expected bytes were received, go to the next state.
        if (received_data_index >= current_frame.data_length)
        {
          state = STATE_RX_CHECKSUM;
        }
      }
      break;

    // Looking for checksum
    case STATE_RX_CHECKSUM:
      {
        // If the received checksum matches the checksum of the received data, go to the next state.
        if (rx == calculate_checksum(current_frame.opcode, current_frame.subcode, current_frame.data, current_frame.data_length))
        {
          state = STATE_RX_END1;
        }
        // Otherwise, return to the idle state.
        else
        {
          state = STATE_RX_START1;
        }
      }
      break;

    // Looking for first byte of footer
    case STATE_RX_END1:
      {
        // If the byte matches what is expected, go to the next state.
        if (rx == OPENHALDEX_MSG_END[0])
        {
          state = STATE_RX_END2;
        }
        // Otherwise, return to the idle state.
        else
        {
          state = STATE_RX_START1;
        }
      }
      break;

    // Looking for second byte of footer
    case STATE_RX_END2:
      {
        // Otherwise, return to the idle state.
        state = STATE_RX_START1;

        // If the byte matches what is expected, emit the frame that was received.
        if (rx == OPENHALDEX_MSG_END[1])
        {
          return &current_frame;
        }
      }
      break;

    default:
      break;
  }

  // When a frame is not emitted, return NULL.
  return NULL;
}

// Send a frame
void openhaldexSerialSlave::send_frame(Stream* stream, openhaldexSerialSlave::openhaldex_serial_slave_frame_t* frame)
{
  // Ensure the pointers are valid.
  if (!stream || !frame)
  {
    return;
  }

  // Send the header.
  stream->print(OPENHALDEX_MSG_START);

  // Send the opcode and subcode.
  stream->write(frame->opcode);
  stream->write(frame->subcode);

  // Send the data length and data bytes.
  stream->write(frame->data_length);
  for (uint8_t i = 0; i < frame->data_length; i++)
  {
    stream->write(frame->data[i]);
  }

  // Send the checksum.
  stream->write(calculate_checksum(frame->opcode, frame->subcode, frame->data, frame->data_length));

  // Send the footer.
  stream->print(OPENHALDEX_MSG_END);
}

// Calculate the checksum for a message
uint8_t openhaldexSerialSlave::calculate_checksum(uint8_t opcode, uint8_t subcode, uint8_t *data, uint8_t data_length)
{
  // Start the checksum from the opcode and subcode bytes.
  uint8_t checksum = 0x55 ^ opcode ^ subcode;
  
  // If the message contains data, XOR the checksum by each byte and, at the end, by the length.
  if (data)
  {
    for (uint8_t i = 0; i < data_length; i++)
    {
      checksum ^= data[i];
    }
    checksum ^= data_length;
  }
  return checksum;
}
