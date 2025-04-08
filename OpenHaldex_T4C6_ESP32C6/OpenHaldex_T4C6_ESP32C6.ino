// Debug (comment out to disable)
//#define ENABLE_DEBUG           // Enables the "DEBUG()" macro, so that it's redirected to printf()
//#define TESTING_WITHOUT_TEENSY // Disables serial communication with the T4 in the broadcasting task and instead shows random values

//#define ENABLE_ACCESSORY_SYSTEM
/*
  The "Accessory system" is a wireless communication layer built on top of ESP-NOW.
  A device may be a master and/or a slave, by defining 0 or more masters/slaves to communicate with.
  Using this system, CANFIS, OpenHaldex-T4C6 and MQB2PQ are able to communicate when they are in range.
  Its implementation is currently not open-source, so it is excluded from the GitHub version.
  This means, compiling new firmware using the code from this repo removes support for the accessory system.
*/

// Rate at which to send new data to the webpage through SSEs or to the accessory system
#define OPENHALDEX_BROADCAST_RATE_MS 250

// Files
#include "definitions.h"
#include "openhaldex_serial.h"
#include "settings.h"
#ifdef ENABLE_ACCESSORY_SYSTEM
#include "accessory.h"
#endif
#include "wifi.h"
#include "hardware.h"
#include <esp_random.h>

// Connect to the configured Wi-Fi hotspot
bool connect_to_wifi_sta()
{
  // Stop the HTTP server if it's running.
  stop_http_server();

  // Store the current Wi-Fi channel, necessary for the accessory system, explained below.
#ifdef ENABLE_ACCESSORY_SYSTEM
  uint8_t wifi_previous_channel = 1;
  wifi_second_chan_t dummy;
  esp_wifi_get_channel(&wifi_previous_channel, &dummy);
#endif

  // Configure the Wi-Fi Station details.
  wifi_config_t sta_conf = {};
  strcpy((char *)sta_conf.sta.ssid, wifi_sta_ssid);
  strcpy((char *)sta_conf.sta.password, wifi_sta_pass);
  sta_conf.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
  esp_wifi_set_config(WIFI_IF_STA, &sta_conf);
  esp_wifi_set_mode(WIFI_MODE_STA);

  // Start trying to connect to the hotspot.
  DEBUG("Connecting to \"%s\", pass \"%s\"", sta_conf.sta.ssid, sta_conf.sta.password);
  xEventGroupClearBits(s_wifi_event_group, portMAX_EVENT_BITS);
  wifi_sta_connect_attempts = 1;
  esp_wifi_start();
  esp_wifi_connect();

  // Wait for a Wi-Fi event ("Connected" or "Not connected").
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdTRUE, pdFALSE,
                                         pdMS_TO_TICKS(30 * 1000));

  // If connected, start the "Dashboard" page.
  if (bits & WIFI_CONNECTED_BIT)
  {
#ifdef ENABLE_ACCESSORY_SYSTEM
    /*
      Connecting to the hotspot will more than likely change the Wi-Fi channel, which will disrupt ESP-NOW.
      To tell accessories about the new channel, disconnect from the hotspot first.
      It's very unlikely that the same hotspot will be on a different channel next time we connect.
    */

    DEBUG("Disconnecting from AP to tell accessories");
    esp_wifi_disconnect();

    DEBUG("STA: Telling accessories");
    accessory_change_wifi_channel(wifi_previous_channel);

    DEBUG("Reconnecting");
    xEventGroupClearBits(s_wifi_event_group, portMAX_EVENT_BITS);
    wifi_sta_connect_attempts = 1;
    esp_wifi_connect();
    bits = xEventGroupWaitBits(s_wifi_event_group,
                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                               pdTRUE, pdFALSE,
                               pdMS_TO_TICKS(30 * 1000));
#endif

    DEBUG("Starting Dashboard server");
    start_http_server(DASHBOARD_PAGE);
    return true;
  }
  else
  {
    DEBUG("Connection failed");
    xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
  }

  return false;
}

// Task which handles connecting to the configured Wi-Fi hotspot
void wifi_button_task(void *args)
{
  // Attempt to connect to the hotspot, 5 seconds after startup.
  vTaskDelay(pdMS_TO_TICKS(5 * 1000));
  if (connect_to_wifi_sta())
  {
    xEventGroupClearBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
  }

  TickType_t xLastWakeTime = xTaskGetTickCount();

  // Task loop:
  while (true)
  {
    // If Wi-Fi becomes disconnected (or didn't connect on the first try), start polling the "Wi-Fi" button.
    xEventGroupWaitBits(s_wifi_event_group,
                        WIFI_DISCONNECTED_BIT,
                        pdFALSE, pdFALSE,
                        portMAX_DELAY);

    // Update the button object.
    wifi_button.update(digitalRead(WIFI_BUTTON_PIN));

    // If the button was clicked/held, try to connect again.
    if (wifi_button.getChanged())
    {
      DEBUG("WiFi button pressed");
      if (connect_to_wifi_sta())
      {
        xEventGroupClearBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
      }
    }

    // Poll the button every 50ms.
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
  }
}

// Task which handles polling the T4 for data, and broadcasting it
void main_task(void *arg)
{
  // Objects for sending a "GET-ALL" command to the T4 and storing the response:
  openhaldexSerialMaster::openhaldex_serial_message_t openhaldex_transaction_tx, openhaldex_transaction_rx;
  openhaldex_transaction_tx.opcode = openhaldexSerialMaster::OPCODE_GET;
  openhaldex_transaction_tx.subcode = openhaldexSerialMaster::SUBCODE_ALL;
  openhaldex_transaction_tx.data_length = 0;
  int16_t transaction_ret = 0;

  // Objects for broadcasting the response on the accessory system:
#ifdef ENABLE_ACCESSORY_SYSTEM
  accessory_message_t openhaldex_accessory_message;
  openhaldex_accessory_message.source = ACCESSORY_SOURCE_SLAVE;
  openhaldex_accessory_message.device = &accy_supported_masters[0];
  openhaldex_accessory_message.data[0] = (uint8_t)openhaldex_transaction_tx.opcode;
  openhaldex_accessory_message.data[1] = (uint8_t)openhaldex_transaction_tx.subcode;
  openhaldex_accessory_message.data_length = 2 + OPENHALDEX_SERIAL_GET_ALL_LENGTH;
#endif

  // Buffer for broadcasting data to the "Dashboard" webpage using Server-Sent-Events:
  char sse_data_buf[64];

  TickType_t xLastWakeTime = xTaskGetTickCount();

  // Task loop:
  while (true)
  {
    // For testing without T4, the response is simulated by random bytes.
#ifdef TESTING_WITHOUT_TEENSY
    (void)transaction_ret;

    // Fill the response object with random bytes.
    openhaldex_transaction_rx.data[0] = esp_random() % (uint8_t)openhaldex_mode_t_MAX;
    for (uint8_t i = 1; i <= 4; i++)
    {
      openhaldex_transaction_rx.data[i] = esp_random();
    }

    // Broadcast the simulated response on the accessory system.
#ifdef ENABLE_ACCESSORY_SYSTEM
    memcpy(&openhaldex_accessory_message.data[2], openhaldex_transaction_rx.data, OPENHALDEX_SERIAL_GET_ALL_LENGTH);
    accessory_send(&openhaldex_accessory_message);
#endif

    // If the file descriptor for the websocket has been obtained, send the emulated bytes to the "Dashboard" webpage as an SSE.
    if (mySocketFD > 0)
    {
      sprintf(sse_data_buf, "%02X,%02X,%02X,%02X", openhaldex_transaction_rx.data[0], openhaldex_transaction_rx.data[1], openhaldex_transaction_rx.data[2], openhaldex_transaction_rx.data[4]);
      send_sse(sse_data_buf, (char*)"data");
    }

    // If the T4 is enabled, poll it for data.
#else

    // Send the request.
    transaction_ret = ohSerial.transaction(&openhaldex_transaction_tx, &openhaldex_transaction_rx);

    // Negative return-values represent errors.
    if (transaction_ret < 0)
    {
      openhaldexSerialMaster::read_error_t err = (openhaldexSerialMaster::read_error_t)(-transaction_ret);
      if (err != openhaldexSerialMaster::TIMEOUT_ERROR)
      {
        DEBUG("Error getting all data: %s", openhaldexSerialMaster::read_error_t_to_string(err));
      }
    }
    // Positive return-values represent the length of the received response.
    else
    {
      // Ensure the response has the correct length.
      if (transaction_ret == OPENHALDEX_SERIAL_GET_ALL_LENGTH)
      {
        // Broadcast the received response on the accessory system.
#ifdef ENABLE_ACCESSORY_SYSTEM
        memcpy(&openhaldex_accessory_message.data[2], openhaldex_transaction_rx.data, OPENHALDEX_SERIAL_GET_ALL_LENGTH);
        accessory_send(&openhaldex_accessory_message);
#endif

        // If the file descriptor for the websocket has been obtained, send the received bytes to the "Dashboard" webpage as an SSE.
        if (mySocketFD > 0)
        {
          sprintf(sse_data_buf, "%02X,%02X,%02X,%02X", openhaldex_transaction_rx.data[0], openhaldex_transaction_rx.data[1], openhaldex_transaction_rx.data[2], openhaldex_transaction_rx.data[4]);
          send_sse(sse_data_buf, (char*)"data");
        }
      }
      else
      {
        DEBUG("openhaldexSerialError GET-ALL: Wrong response length (%d)", transaction_ret);
      }
    }
#endif

    // Broadcast the data at the configured rate.
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(OPENHALDEX_BROADCAST_RATE_MS));
  }
}

void setup()
{
#ifdef ENABLE_DEBUG
  Serial.begin(115200);
  DEBUG("OpenHaldex T4C6 - ESP32C6");
#endif

  // Initialize the hardware.
  setup_hardware();

  // Check if the "Wi-Fi" button is being held at power-on (to start a self-hosted Wi-Fi AP and enable the "Update" webpage).
  if (digitalRead(WIFI_BUTTON_PIN))
  {
    DEBUG("Entering Update mode");

    // Configure the Wi-Fi Access Point details.
    wifi_config_t ap_conf = {};
    strcpy((char *)ap_conf.ap.ssid, wifi_ap_ssid);
    strcpy((char *)ap_conf.ap.password, wifi_ap_pass);
    ap_conf.ap.channel = 1;
    ap_conf.ap.max_connection = 4;
    ap_conf.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    ap_conf.ap.pmf_cfg.required = true;
    esp_wifi_set_config(WIFI_IF_AP, &ap_conf);
    esp_wifi_set_mode(WIFI_MODE_AP);

    // Try to start Wi-Fi.
    if (esp_wifi_start() == ESP_OK)
    {
      // If Wi-Fi started, start the HTTP server in "Update" mode.
      start_http_server(UPDATE_PAGE);

      DEBUG("Started AP and Update server, deleting main task");
      vTaskDelete(NULL);
    }
    else
    {
      DEBUG("Failed to start AP, continuing");
    }
  }
  else
  {
    DEBUG("No Update mode");
  }

  // Read the settings stored in the file on LittleFS.
  if (!read_settings())
  {
    DEBUG("Error reading settings");
  }

#ifdef ENABLE_ACCESSORY_SYSTEM
  DEBUG("setup_accessory_system()");
  setup_accessory_system();
#endif

  // Start the task responsible for polling the T4 and broadcasting the data to the "Dashboard" webpage and the accessory system.
  xTaskCreate(main_task, "main", 4096, NULL, 4, NULL);

  // If the Wi-Fi hotspot details have been configured, start the task responsible for connecting to it.
  xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECTED_BIT);
  if (strlen(wifi_sta_ssid) >= 1 && strlen(wifi_sta_pass) >= 8)
  {
    DEBUG("Creating wifi_button_task");
    xTaskCreate(wifi_button_task, "wButton", 4096, NULL, 3, NULL);
  }
  else
  {
    DEBUG("No WiFi configured");
  }

  DEBUG("Setup done");
}

void loop()
{
  // Delete the loop's task.
  vTaskDelete(NULL);
}
