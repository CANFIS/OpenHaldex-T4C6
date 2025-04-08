#ifndef HTTP_SERVER_HANDLER_H
#define HTTP_SERVER_HANDLER_H

// Provide favicon
#include "HTML/favicon_ico_gz.h"
esp_err_t send_favicon(httpd_req_t *req)
{
  httpd_resp_set_type(req, "image/x-icon");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_send(req, favicon_ico_gz, favicon_ico_gz_len);
  return ESP_OK;
}

// Provide jQuery
#include "HTML/jquery_min_js.h"
esp_err_t send_jquery_min_js(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/javascript");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_send(req, jquery_min_js_v3_7_1_gz, jquery_min_js_v3_7_1_gz_len);
  return ESP_OK;
}

// Provide background image
#include "HTML/background_jpeg_gz.h"
esp_err_t send_background_image(httpd_req_t *req)
{
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_send(req, background_jpeg_gz, background_jpeg_gz_len);
  return ESP_OK;
}

// Provide logo image
#include "HTML/logo_png_gz.h"
esp_err_t send_logo_image(httpd_req_t *req)
{
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  httpd_resp_send(req, logo_png_gz, logo_png_gz_len);
  return ESP_OK;
}

// Process index.html
#include "html_template_processing.h"
char *myProcessor(const char *str, size_t ln, char *processed_string)
{
  // Return an empty string by default.
  strcpy(processed_string, "");

  // Handle the {{TEST}} template.
  if (strncmp(str, "TEST", ln) == 0)
  {
    strcpy(processed_string, "Testing");
  }

  // Return the same string as the one received as a parameter.
  return processed_string;
}

// Provide index.html for Update
#include "HTML/update_index_html.h"
esp_err_t send_update_index_html(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send_with_template_processing(req, update_index_html, sizeof(update_index_html), myProcessor);
  return ESP_OK;
}

// Provide index.html for Dashboard
#include "HTML/dashboard_index_html.h"
esp_err_t send_dashboard_index_html(httpd_req_t *req)
{
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send_with_template_processing(req, dashboard_index_html, sizeof(dashboard_index_html), myProcessor);
  return ESP_OK;
}

// Convert special characters from the HTTP query string (such as "%20") to the correct characters
void decode_http_query_string(char *dest, char* src)
{
  size_t len = strlen(src);
  size_t dest_idx = 0;
  for (size_t i = 0; i < len; i++)
  {
    if (src[i] != '%')
    {
      dest[dest_idx++] = src[i];
    }
    else
    {
      char hex_buf[3] = "";
      hex_buf[0] = src[i + 1];
      hex_buf[1] = src[i + 2];

      uint8_t character = 0;
      sscanf(hex_buf, "%hhX", &character);

      dest[dest_idx++] = character;

      i += 2;
    }
  }
}

// Store the details of the Wi-Fi hotspot
esp_err_t set_wifi_settings(httpd_req_t *req)
{
  DEBUG("Received Wi-Fi hotspot details");

  // Buffer to hold the HTTP query:
  static char query[128];

  // Calculate how many bytes to read, and ensure the string is properly terminated.
  size_t recv_size = (req->content_len < (sizeof(query) - 1)) ? req->content_len : (sizeof(query) - 1);
  query[recv_size] = '\0';

  // Try to read the query string.
  int ret = httpd_req_recv(req, query, recv_size);
  if (ret <= 0)
  {
    if (ret == HTTPD_SOCK_ERR_TIMEOUT)
    {
      httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, NULL);
    }

    return ESP_FAIL;
  }

  // The hotspot's SSID and password will be searched for in the query string.
  bool found_ssid = false, found_pass = false;

  // Special characters will appear like "%20" which takes 3 characters.
  static char ssid[31 * 3 + 1] = {0}, pass[63 * 3 + 1] = {0};

  // Tokenize the query string.
  char *token = strtok(query, "&");
  char *var = NULL;
  while (token)
  {
    // Get the SSID.
    if (!found_ssid && (var = strstr(token, "ssid=")))
    {
      strncpy(ssid, strchr(var, '=') + 1, sizeof(ssid) - 1);
      found_ssid = true;
    }

    // Get the password.
    if (!found_pass && (var = strstr(token, "pass=")))
    {
      strncpy(pass, strchr(var, '=') + 1, sizeof(pass) - 1);
      found_pass = true;
    }

    // Get the next token.
    token = strtok(NULL, "&");
  }

  // The SSID and password must have been found.
  if (!found_ssid || !found_pass)
  {
    // Send a negative response.
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
    return ESP_FAIL;
  }

  // Fix the foreign characters in the strings.
  decode_http_query_string(wifi_sta_ssid, ssid);
  decode_http_query_string(wifi_sta_pass, pass);
  DEBUG("SSID: \"%s\", Pass: \"%s\"", wifi_sta_ssid, wifi_sta_pass);

  // Store the details.
  write_settings();

  // Send a positive response.
  httpd_resp_set_status(req, HTTPD_200);
  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

// The handle and file descriptor of the websocket will be stored, for sending Server-Sent-Events.
httpd_handle_t mySocketHD;
int mySocketFD;

// When the "Dasboard" webpage is entered/refreshed, the eventListener is added and this function is triggered
esp_err_t handle_sse(httpd_req_t *req)
{
  // Store the handle and file descriptor.
  mySocketHD = req->handle;
  mySocketFD = httpd_req_to_sockfd(req);

  // Send the SSE response to configure this function.
  const static char sse_resp[] = "HTTP/1.1 200 OK\r\nCache-Control: no-store\r\nContent-Type: text/event-stream\r\n\r\nretry: 20000\r\n\r\n";
  httpd_socket_send(mySocketHD, mySocketFD, sse_resp, sizeof(sse_resp) - 1, 0);

  DEBUG("Sent SSE configuration");
  return ESP_OK;
}

// Set the OpenHaldex mode when one of the buttons on the Dashboard is pressed
esp_err_t handle_set_mode(httpd_req_t *req)
{
  // Buffer to hold the HTTP query:
  static char query[8];

  // Calculate how many bytes to read, and ensure the string is properly terminated.
  size_t recv_size = (req->content_len < (sizeof(query) - 1)) ? req->content_len : (sizeof(query) - 1);
  query[recv_size] = '\0';

  // Try to read the query string.
  int ret = httpd_req_recv(req, query, recv_size);
  if (ret <= 0)
  {
    if (ret == HTTPD_SOCK_ERR_TIMEOUT)
    {
      httpd_resp_send_408(req);
    }
    return ESP_FAIL;
  }

  // The query only contains a number; read it as a byte.
  uint8_t requested_mode = 0;
  sscanf(query, "%hhu", &requested_mode);
  DEBUG("Requested mode: %s", get_openhaldex_mode_string((openhaldex_mode_t)requested_mode));

  // Objects for sending a "SET-MODE" command to the T4 and storing the response:
  openhaldexSerialMaster::openhaldex_serial_message_t openhaldex_transaction_tx, openhaldex_transaction_rx;
  openhaldex_transaction_tx.opcode = openhaldexSerialMaster::OPCODE_SET;
  openhaldex_transaction_tx.subcode = openhaldexSerialMaster::SUBCODE_MODE;
  openhaldex_transaction_tx.data[0] = requested_mode;
  openhaldex_transaction_tx.data_length = OPENHALDEX_SERIAL_SET_MODE_LENGTH;

  // Send the request.
  int16_t transaction_ret = ohSerial.transaction(&openhaldex_transaction_tx, &openhaldex_transaction_rx);

  // Negative return-values represent errors.
  if (transaction_ret < 0)
  {
    openhaldexSerialMaster::read_error_t err = (openhaldexSerialMaster::read_error_t)(-transaction_ret);
    DEBUG("Error setting mode: %s", openhaldexSerialMaster::read_error_t_to_string(err));
    (void)err;
  }
  // Positive return-values represent the length of the received response.
  else
  {
    // Ensure the response has the correct length.
    if (transaction_ret == OPENHALDEX_SERIAL_GET_MODE_LENGTH)
    {
      // The response contains the currently set mode, check if it matches what was requested.
      if (openhaldex_transaction_rx.data[0] != requested_mode)
      {
        DEBUG("openhaldexSerialError SET-MODE: mode not set, still %s", get_openhaldex_mode_string((openhaldex_mode_t)openhaldex_transaction_rx.data[0]));
      }
    }
    else
    {
      DEBUG("openhaldexSerialError SET-MODE: Wrong response length (%d)", transaction_ret);
    }
  }

  // Send a positive response.
  httpd_resp_set_status(req, HTTPD_200);
  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

// Strings to be sent in the HTTP response for uploading firmware
const char HTTP_system_error_string[] = "System error.";
const char HTTP_file_receive_error_string[] = "Failed to receive file.";
const char HTTP_file_store_error_string[] = "Failed to write file to storage.";
const char HTTP_done_string[] = "Update complete. Please press OK.";

// Handle firmware updates
esp_err_t upload_firmware(httpd_req_t *req)
{
  // Find an unused OTA partition.
  const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);

  // Couldn't find partition.
  if (!partition)
  {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, HTTP_system_error_string);

    // handle_end_update will take care of restarting after the alert message is closed.
    return ESP_OK;
  }

  // Erase the selected partition.
  esp_ota_handle_t update_handle;
  esp_err_t ret;
  if ((ret = esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &update_handle)))
  {
    // Couldn't erase partition.
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, HTTP_system_error_string);

    // handle_end_update will take care of restarting after the alert message is closed.
    return ESP_OK;
  }

  // Get data until there is no more left.
  size_t remaining = req->content_len;
  while (remaining)
  {
    // Put data in the buffer.
    int received = httpd_req_recv(req, file_scratch, ((remaining < sizeof(file_scratch)) ? remaining : sizeof(file_scratch)));

    // Handle receiving errors.
    if (received <= 0)
    {
      // Retry.
      if (received == HTTPD_SOCK_ERR_TIMEOUT)
      {
        continue;
      }

      // Couldn't receive data.
      esp_ota_abort(update_handle);
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, HTTP_file_receive_error_string);

      // handle_end_update will take care of restarting after the alert message is closed.
      return ESP_OK;
    }

    // Write the received data.
    ret = esp_ota_write(update_handle, file_scratch, received);

    // Couldn't write data.
    if (ret)
    {
      esp_ota_abort(update_handle);
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, HTTP_file_store_error_string);

      // handle_end_update will take care of restarting after the alert message is closed.
      return ESP_OK;
    }

    // Calculate how much is left to receive.
    remaining -= received;
  }

  // End the OTA update.
  ret = esp_ota_end(update_handle);

  // Couldn't end the update.
  if (ret)
  {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, HTTP_system_error_string);

    // handle_end_update will take care of restarting after the alert message is closed.
    return ESP_OK;
  }

  // Set the boot partition.
  ret = esp_ota_set_boot_partition(partition);

  // Update done.
  httpd_resp_set_status(req, HTTPD_200);
  httpd_resp_send(req, HTTP_done_string, HTTPD_RESP_USE_STRLEN);

  // handle_end_update will take care of restarting after the alert message is closed.
  return ESP_OK;
}

// Restart the ESP32 when the user closes the alert message
esp_err_t handle_end_update(httpd_req_t *req)
{
  DEBUG("Ending update");
  esp_restart();
  return ESP_OK;
}

// Handle the "Factory reset" button
esp_err_t handle_factory_reset(httpd_req_t *req)
{
  DEBUG("Formatting filesystem");
  esp_littlefs_format("littlefs");
  esp_restart();
  return ESP_OK;
}

// Provide the current IP address, to be displayed on the Dashboard
uint32_t sta_ip;
esp_err_t provide_ip(httpd_req_t *req)
{
  // Put the IP in a string.
  static char ip_buf[16];
  sprintf(ip_buf, IPSTR, IP2STR((esp_ip4_addr_t*)&sta_ip));

  // Send the IP as a string in the positive response.
  DEBUG("Giving IP: \"%s\"", ip_buf);
  httpd_resp_set_status(req, HTTPD_200);
  httpd_resp_send(req, ip_buf, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

#endif
