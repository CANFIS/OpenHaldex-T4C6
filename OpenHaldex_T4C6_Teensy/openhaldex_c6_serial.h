#ifndef OPENHALDEX_C6_SERIAL
#define OPENHALDEX_C6_SERIAL

// Expected request lengths
#define OPENHALDEX_SERIAL_SET_MODE_LENGTH 1
#define OPENHALDEX_SERIAL_INCREMENT_MODE_LENGTH 0
#define OPENHALDEX_SERIAL_DECREMENT_MODE_LENGTH 0

// UART configuration
#define OPENHALDEX_C6_SERIAL_PORT Serial3
#define OPENHALDEX_C6_SERIAL_BAUDRATE 250000

// Class which handles serial communication with the C6
#include "openhaldex_serial_slave.h"
openhaldexSerialSlave ohSerial;

void openhaldex_c6_serial_handle_GET(openhaldexSerialSlave::openhaldex_serial_slave_frame_t *frame, openhaldexSerialSlave::openhaldex_serial_slave_frame_t *openhaldex_c6_serial_response_frame)
{
  switch (frame->subcode)
  {
    // Handle "GET-ALL":
    case openhaldexSerialSlave::SUBCODE_ALL:
      {
        openhaldex_c6_serial_response_frame->data_length = 7;
        openhaldex_c6_serial_response_frame->data[0] = (uint8_t)state.mode;
        openhaldex_c6_serial_response_frame->data[1] = (uint8_t)lock_target;
        openhaldex_c6_serial_response_frame->data[2] = received_haldex_engagement;
        openhaldex_c6_serial_response_frame->data[3] = (uint8_t)received_pedal_value;
        openhaldex_c6_serial_response_frame->data[4] = received_vehicle_speed;
        openhaldex_c6_serial_response_frame->data[5] = in_standalone_mode; // Always false, kept for compatibility
        openhaldex_c6_serial_response_frame->data[6] = state.mode_override;
      }
      break;

    default:
      break;
  }
}

void openhaldex_c6_serial_handle_SET(openhaldexSerialSlave::openhaldex_serial_slave_frame_t *frame, openhaldexSerialSlave::openhaldex_serial_slave_frame_t *openhaldex_c6_serial_response_frame)
{
  switch (frame->subcode)
  {
    // Handle "SET-MODE":
    case openhaldexSerialSlave::SUBCODE_MODE:
      {
        // Ensure the request has the correct length.
        if (frame->data_length == OPENHALDEX_SERIAL_SET_MODE_LENGTH)
        {
          // Determine which mode was requested (first byte of data).
          uint8_t requested_mode = (openhaldex_mode_t)frame->data[0];

          // If the requested mode is valid, apply it.
          if (requested_mode < (uint8_t)openhaldex_mode_t_MAX)
          {
            state.mode = (openhaldex_mode_t)requested_mode;
          }
          // For invalid values, fall back to STOCK mode.
          else
          {
            state.mode = MODE_STOCK;
          }

          // Prepare to send the current mode as a response.
          openhaldex_c6_serial_response_frame->data_length = 1;
          openhaldex_c6_serial_response_frame->data[0] = (uint8_t)state.mode;
        }
      }
      break;

    default:
      break;
  }
}

void openhaldex_c6_serial_handle_INCREMENT(openhaldexSerialSlave::openhaldex_serial_slave_frame_t *frame, openhaldexSerialSlave::openhaldex_serial_slave_frame_t *openhaldex_c6_serial_response_frame)
{
  switch (frame->subcode)
  {
    // Handle "INCREMENT-MODE":
    case openhaldexSerialSlave::SUBCODE_MODE:
      {
        // Ensure the request has the correct length.
        if (frame->data_length == OPENHALDEX_SERIAL_INCREMENT_MODE_LENGTH)
        {
          // Determine the next mode.
          uint8_t requested_mode = (uint8_t)state.mode + 1;

          // If the requested mode is valid, apply it.
          if (requested_mode < (uint8_t)MODE_CUSTOM)
          {
            state.mode = (openhaldex_mode_t)requested_mode;
          }
          // For invalid values, fall back to STOCK mode.
          else
          {
            state.mode = MODE_STOCK;
          }

          // Prepare to send the current mode as a response.
          openhaldex_c6_serial_response_frame->data_length = 1;
          openhaldex_c6_serial_response_frame ->data[0] = (uint8_t)state.mode;
        }
      }
      break;

    default:
      break;
  }
}

void openhaldex_c6_serial_handle_DECREMENT(openhaldexSerialSlave::openhaldex_serial_slave_frame_t *frame, openhaldexSerialSlave::openhaldex_serial_slave_frame_t *openhaldex_c6_serial_response_frame)
{
  switch (frame->subcode)
  {
    // Handle "DECREMENT-MODE":
    case openhaldexSerialSlave::SUBCODE_MODE:
      {
        // Ensure the request has the correct length.
        if (frame->data_length == OPENHALDEX_SERIAL_DECREMENT_MODE_LENGTH)
        {
          // Determine the previous mode.
          uint8_t requested_mode = (uint8_t)state.mode - 1;

          // If the requested mode is valid, apply it.
          if (requested_mode < (uint8_t)MODE_CUSTOM)
          {
            state.mode = (openhaldex_mode_t)requested_mode;
          }
          // For invalid values, fall back to the last mode before CUSTOM mode.
          else
          {
            state.mode = (openhaldex_mode_t)((uint8_t)((uint8_t)MODE_CUSTOM - 1));
          }

          // Prepare to send the current mode as a response.
          openhaldex_c6_serial_response_frame->data_length = 1;
          openhaldex_c6_serial_response_frame->data[0] = (uint8_t)state.mode;
        }
      }
      break;

    default:
      break;
  }
}

void openhaldex_c6_serial_handle_frame(openhaldexSerialSlave::openhaldex_serial_slave_frame_t *frame)
{
  // Object which holds a response to be sent:
  static openhaldexSerialSlave::openhaldex_serial_slave_frame_t openhaldex_c6_serial_response_frame;

  // Set the opcode and subcode in the response to be the same as in the request.
  openhaldex_c6_serial_response_frame.opcode = frame->opcode;
  openhaldex_c6_serial_response_frame.subcode = frame->subcode;

  // Prepare to send a frame without data for unknown opcodes/subcodes.
  openhaldex_c6_serial_response_frame.data_length = 0;

  // Edit the frame depending on the received opcode (and subcode).
  switch (frame->opcode)
  {
    case openhaldexSerialSlave::OPCODE_GET:
      openhaldex_c6_serial_handle_GET(frame, &openhaldex_c6_serial_response_frame);
      break;
    case openhaldexSerialSlave::OPCODE_SET:
      openhaldex_c6_serial_handle_SET(frame, &openhaldex_c6_serial_response_frame);
      break;
    case openhaldexSerialSlave::OPCODE_INCREMENT:
      openhaldex_c6_serial_handle_INCREMENT(frame, &openhaldex_c6_serial_response_frame);
      break;
    case openhaldexSerialSlave::OPCODE_DECREMENT:
      openhaldex_c6_serial_handle_DECREMENT(frame, &openhaldex_c6_serial_response_frame);
      break;
    default:
      break;
  }

  // Send the response (empty if the opcode/subcode/length were unknown/incorrect).
  ohSerial.send_frame(&OPENHALDEX_C6_SERIAL_PORT, &openhaldex_c6_serial_response_frame);
}

#endif
