#ifndef OPENHALDEX_SERIAL_MASTER_H
#define OPENHALDEX_SERIAL_MASTER_H

#include <Arduino.h>

#define OPENHALDEX_SERIAL_MAX_PAYLOAD_SIZE 0xFF

class openhaldexSerialMaster
{
  public:
    typedef void (*clear_rx_t)(void);
    typedef void (*clear_tx_t)(void);
    typedef bool (*rx_t)(size_t *received_length, uint8_t *buffer, size_t buffer_size, TickType_t timeout_ticks);
    typedef void (*tx_t)(uint8_t);
    
    openhaldexSerialMaster(clear_rx_t clear_rx_function, clear_tx_t clear_tx_function, rx_t rx_function, tx_t tx_function);
    
    enum read_error_t
    {
      NO_ERROR,
      PARAMETER_ERROR,
      TIMEOUT_ERROR,
      SIZE_ERROR,
      FORMAT_ERROR,
      DATA_ERROR,
      CHECKSUM_ERROR
    };
    
    static const char *read_error_t_to_string(read_error_t err);
    
    enum openhaldex_serial_master_opcode_t
    {
      OPCODE_GET = 0x00,
      OPCODE_SET = 0x01,
      OPCODE_INCREMENT = 0x02,
      OPCODE_DECREMENT = 0x03,
    };
    
    enum openhaldex_serial_master_subcode_t
    {
      SUBCODE_ALL = 0x00,
      SUBCODE_MODE = 0x01,
    };

    struct openhaldex_serial_message_t
    {
      openhaldex_serial_master_opcode_t opcode;
      openhaldex_serial_master_subcode_t subcode;
      uint8_t data[OPENHALDEX_SERIAL_MAX_PAYLOAD_SIZE];
      uint8_t data_length;
    };

    int16_t transaction(openhaldex_serial_message_t *message_to_send, openhaldex_serial_message_t *received_message);

  private:
    static const uint8_t OPENHALDEX_MSG_START[3];
    static const uint8_t OPENHALDEX_MSG_END[3];
    static const unsigned long OPENHALDEX_RESPONSE_TIMEOUT_MS = 10;

    static const uint8_t OPENHALDEX_OPCODE_GET = 0x00;
    static const uint8_t OPENHALDEX_SUBCODE_GETALL = 0x00;

    clear_rx_t clear_rx_function;
    clear_tx_t clear_tx_function;
    rx_t rx_function;
    tx_t tx_function;

    SemaphoreHandle_t interface_mutex;

    uint8_t calculate_checksum(uint8_t opcode, uint8_t subcode, uint8_t *data = NULL, size_t data_length = 0);
    void write_request(uint8_t opcode, uint8_t subcode, uint8_t *data = NULL, size_t data_length = 0);
    read_error_t read_response(uint8_t opcode, uint8_t subcode, size_t *received_bytes, uint8_t *buffer, size_t buffer_size, TickType_t timeout_ticks = portMAX_DELAY);
};

#endif
