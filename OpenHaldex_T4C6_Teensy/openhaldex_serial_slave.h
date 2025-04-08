#ifndef OPENHALDEX_SERIAL_SLAVE
#define OPENHALDEX_SERIAL_SLAVE

#include <Arduino.h>

class openhaldexSerialSlave
{
  public:
    openhaldexSerialSlave();
    
    enum openhaldex_serial_slave_opcode_t
    {
      OPCODE_GET = 0x00,
      OPCODE_SET = 0x01,
      OPCODE_INCREMENT = 0x02,
      OPCODE_DECREMENT = 0x03,
    };
    
    enum openhaldex_serial_slave_subcode_t
    {
      SUBCODE_ALL = 0x00,
      SUBCODE_MODE = 0x01,
    };

    struct openhaldex_serial_slave_frame_t
    {
      openhaldex_serial_slave_opcode_t opcode;
      openhaldex_serial_slave_subcode_t subcode;
      uint8_t data[0xFF];
      uint8_t data_length;
    };

    openhaldex_serial_slave_frame_t* parseByte(uint8_t rx);

    void send_frame(Stream* stream, openhaldex_serial_slave_frame_t* frame);

  private:
    enum openhaldex_serial_slave_state_t
    {
      STATE_RX_START1,
      STATE_RX_START2,
      STATE_RX_OPCODE,
      STATE_RX_SUBCODE,
      STATE_RX_DATA_LENGTH,
      STATE_RX_DATA,
      STATE_RX_CHECKSUM,
      STATE_RX_END1,
      STATE_RX_END2,
      EMIT_MESSAGE
    };

    openhaldex_serial_slave_state_t state;

    unsigned long last_received_byte_ms;

    const char OPENHALDEX_MSG_START[3] = ">[";
    const char OPENHALDEX_MSG_END[3] = "]<";
    unsigned long RX_TIMEOUT_MS = 10;

    uint8_t received_data_index;
    openhaldex_serial_slave_frame_t current_frame;
    
    static uint8_t calculate_checksum(uint8_t opcode, uint8_t subcode, uint8_t *data, uint8_t data_length);
};

#endif
