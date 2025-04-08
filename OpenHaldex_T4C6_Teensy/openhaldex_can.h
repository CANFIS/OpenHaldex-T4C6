#ifndef OPENHALDEX_CAN_H
#define OPENHALDEX_CAN_H

#if (HALDEX_GENERATION == 1) // CAN IDs for Gen1
#define MOTOR1_ID 0x280
#define MOTOR2_ID 0x288
#define MOTOR3_ID 0x380
#define MOTOR5_ID 0x480
#define MOTOR6_ID 0x488
#define MOTOR7_ID 0x588
#define MOTORBREMS_ID 0x284
#define MOTOR_FLEX_ID 0x580
#define BRAKES1_ID 0x1A0
#define BRAKES2_ID 0x2A0
#define BRAKES3_ID 0x4A0
#define BRAKES5_ID 0x5A0
#define GRA_ID 0x38A
#define HALDEX_ID 0x2C0
#elif (HALDEX_GENERATION == 2) // CAN IDs for Gen2
#define MOTOR1_ID 0x280
#define MOTOR2_ID 0x288
#define MOTOR3_ID 0x380
#define MOTOR5_ID 0x480
#define MOTOR6_ID 0x488
#define MOTOR7_ID 0x588
#define MOTORBREMS_ID 0x284
#define MOTOR_FLEX_ID 0x580
#define BRAKES1_ID 0x1A0
#define BRAKES2_ID 0x5A0
#define BRAKES3_ID 0x4A0
#define BRAKES5_ID 0x5A0
#define GRA_ID 0x38A
#define HALDEX_ID 0x2C0
#elif (HALDEX_GENERATION == 4) // CAN IDs for Gen4
#define MOTOR1_ID 0x280
#define MOTOR2_ID 0x288
#define MOTOR3_ID 0x380
#define MOTOR5_ID 0x480
#define MOTOR6_ID 0x488
#define MOTOR7_ID 0x588
#define MOTORBREMS_ID 0x284
#define MOTOR_FLEX_ID 0x580
#define BRAKES1_ID 0x1A0
#define BRAKES2_ID 0x5A0
#define BRAKES3_ID 0x4A0
#define BRAKES5_ID 0x5A0
#define GRA_ID 0x38A
#define HALDEX_ID 0x2C0
#endif

// Custom CAN IDs
#define OPENHALDEX_BROADCAST_ID 0x7B0
#define OPENHALDEX_EXTERNAL_CONTROL_ID 0x7C0

// CAN library
#include <FlexCAN_T4.h>
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> HaldexCAN;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> ChassisCAN;

// Function prototypes
float get_lock_target_adjustment();
uint8_t get_lock_target_adjusted_value(uint8_t value, bool invert);
void get_lock_data(CAN_message_t &frame);

// Functions

void onHaldexRX(const CAN_message_t &frame)
{
  // A frame was received from the Haldex module.
#ifdef DEBUG_HALDEXCAN_TRAFFIC
  DEBUG_("[HDX RX] %03X: ", frame.id);
  for (uint8_t i = 0; i < frame.len; i++)
  {
    DEBUG_("%02X ", frame.buf[i]);
  }
  DEBUG("");
#endif
  
  // Check if the ID corresponds to the "Haldex status" message.
  if (frame.id == HALDEX_ID)
  {
#ifdef DEBUG_HALDEXCAN_TRAFFIC
    DEBUG("    BIN haldexState: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(frame.buf[0]));
    DEBUG("    Raw haldexEngagement: %d", frame.buf[1]);
    DEBUG("    Mapped haldexEngagement: %d", map(frame.buf[1], 128, 198, 0, 100));
#endif
    
    // Extract data from the frame.
    received_haldex_state = frame.buf[0];
    received_haldex_engagement = frame.buf[1];
    
    // Decode the state byte.
    received_report_clutch1 = (received_haldex_state & (1 << 0));
    received_temp_protection = (received_haldex_state & (1 << 1));
    received_report_clutch2 = (received_haldex_state & (1 << 2));
    received_coupling_open = (received_haldex_state & (1 << 3));
    received_speed_limit = (received_haldex_state & (1 << 6));
  }

  // Build a frame for transmitting to Chassis CAN.
  CAN_message_t frame_out;
  frame_out.id = frame.id;
  frame_out.flags = frame.flags;
  frame_out.len = frame.len;
  memcpy(frame_out.buf, frame.buf, frame.len);

  // Forward whatever is received from Haldex to Chassis.
  if (!ChassisCAN.write(frame_out))
  {
    DEBUG("Chassis CAN TX Fail!");
    ChassisCAN.mailboxStatus();
  }
}

void onChassisRX(const CAN_message_t &frame)
{
  // A frame was received from the Chassis bus.
#ifdef DEBUG_CHASSISCAN_TRAFFIC
  DEBUG_("[CHS RX] %03X: ", frame.id);
  for (uint8_t i = 0; i < frame.len; i++)
  {
    DEBUG_("%02X ", frame.buf[i]);
  }
  DEBUG("");
#endif

  // Build a frame for transmitting to Haldex CAN.
  CAN_message_t frame_out;
  frame_out.id = frame.id;
  frame_out.flags = frame.flags;
  frame_out.len = frame.len;
  memcpy(frame_out.buf, frame.buf, frame.len);

  // Extract data from the frame.
  switch (frame.id)
  {
    case MOTOR1_ID:
      received_pedal_value = frame.buf[5] * 0.4; // wpedv_w
      break;

    case BRAKES5_ID:
      received_vehicle_speed = (uint8_t)((double)((frame.buf[2] << 8) | frame.buf[1]) / 148.0);
      break;

    case OPENHALDEX_EXTERNAL_CONTROL_ID:
      // If the requested mode is valid, apply it.
      if (frame.buf[0] < (uint8_t)openhaldex_mode_t_MAX && frame.buf[0] != (uint8_t)MODE_CUSTOM)
      {
        state.mode = (openhaldex_mode_t)frame.buf[0];
      }
      break;
  }

  // Edit the CAN frame, if not in STOCK mode (otherwise, the original frame is already copied in the new buffer).
  if (state.mode != MODE_STOCK)
  {
    // Edit the frame.
    if (HALDEX_GENERATION == 1 || HALDEX_GENERATION == 2 || HALDEX_GENERATION == 4)
    {
      get_lock_data(frame_out);
    }
    
    // For other generations, frames are not edited yet.
  }

  // Send the (edited) frame from Chassis to Haldex.
  if (!HaldexCAN.write(frame_out))
  {
    DEBUG("Haldex CAN TX Fail!");
    HaldexCAN.mailboxStatus();
  }
}

void init_CAN()
{
  // Initialize the Haldex CAN bus.
  HaldexCAN.begin();
  HaldexCAN.setClock(CLK_60MHz);
  HaldexCAN.setBaudRate(500000);
  HaldexCAN.setMaxMB(16);
  HaldexCAN.onReceive(onHaldexRX);
  HaldexCAN.enableFIFO();
  HaldexCAN.enableFIFOInterrupt();
#ifdef ENABLE_DEBUG
  HaldexCAN.mailboxStatus();
#endif

  // Initialize the Chassis CAN bus.
  ChassisCAN.begin();
  ChassisCAN.setClock(CLK_60MHz);
  ChassisCAN.setBaudRate(500000);
  ChassisCAN.setMaxMB(16);
  ChassisCAN.onReceive(onChassisRX);
  ChassisCAN.enableFIFO();
  ChassisCAN.enableFIFOInterrupt();
#ifdef ENABLE_DEBUG
  ChassisCAN.mailboxStatus();
#endif
}

bool broadcast_openhaldex_data(void *params)
{
  // Construct the frame.
  CAN_message_t broadcast_frame;
  broadcast_frame.id = OPENHALDEX_BROADCAST_ID;
  broadcast_frame.len = 8;
  broadcast_frame.buf[0] = 0x01;
  broadcast_frame.buf[1] = in_standalone_mode; // Always false, kept for compatibility
  broadcast_frame.buf[2] = received_haldex_engagement;
  broadcast_frame.buf[3] = (uint8_t)lock_target;
  broadcast_frame.buf[4] = received_vehicle_speed;
  broadcast_frame.buf[5] = state.mode_override;
  broadcast_frame.buf[6] = (uint8_t)state.mode;
  broadcast_frame.buf[7] = (uint8_t)received_pedal_value;

  // Send the frame on Chassis CAN.
  if (!ChassisCAN.write(broadcast_frame))
  {
    DEBUG("Chassis CAN TX Fail! (broadcast)");
    ChassisCAN.mailboxStatus();
  }
  return true;
}

#endif
