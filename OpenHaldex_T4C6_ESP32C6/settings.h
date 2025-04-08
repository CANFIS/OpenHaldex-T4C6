#ifndef SETTINGS_H
#define SETTINGS_H

// If the format of the "settings" file changes, this version must be changed (incremented) in order to reset the file and avoid misinterpreting the data.
#define SETTINGS_VERSION 0x0001

// LittleFS
#include "lfs.h"
#include "file_operations.h"
const char settings_file_name[] = "/littlefs/settings.txt";
FILE *settings_file;

// Strings to read the configured hotspot details into
char wifi_sta_ssid[32] = "";
char wifi_sta_pass[64] = "";

// Function prototypes
bool read_settings();
bool write_settings();
void reset_settings();

// Try to read settings.
// If the file doesn't exist, create it and write default values.
// If the file exists but contains invalid data, write default values.
// If the file contains valid data, read the settings.
bool read_settings()
{
  DEBUG("Reading settings");

  // Check if the settings file exists.
  struct stat st;
  if (stat(settings_file_name, &st))
  {
    DEBUG("No settings file");

    // Set the variables to their default values.
    reset_settings();

    // Create the file by writing the default values.
    return write_settings();
  }
  // If it exists, check the version stored in it.
  else
  {
    // Open the file and read the settings.
    settings_file = fopen(settings_file_name, "r");
    if (settings_file)
    {
      // Get the footer (always 0x55AA).
      fseek(settings_file, -2, SEEK_END);
      uint16_t footer = read16(settings_file);

      // Get the version.
      fseek(settings_file, 0, SEEK_SET);
      uint16_t version = read16(settings_file);

      // Check the values.
      if (!(version == SETTINGS_VERSION && footer == 0x55AA))
      {
        DEBUG("Invalid settings version or footer");

        // Set the variables to their default values.
        reset_settings();

        // Create the file by writing the default values.
        return write_settings();
      }
      else
      {
        if (!readCharArray(settings_file, wifi_sta_ssid, sizeof(wifi_sta_ssid)) ||
            !readCharArray(settings_file, wifi_sta_pass, sizeof(wifi_sta_pass)) || (strlen(wifi_sta_pass) != 0 && strlen(wifi_sta_pass) < 8))
        {
          DEBUG("Invalid hotspot details in settings");

          // Set the variables to their default values.
          reset_settings();

          // Create the file by writing the default values.
          return write_settings();
        }

        //uint8_t test_setting1 = read8(settings_file);
        //uint16_t test_setting2 = read16(settings_file);
        //uint32_t test_setting3 = read32(settings_file);
        //uint64_t test_setting4 = read64(settings_file);
        
        return true;
      }
    }
    else
    {
      return false;
    }
  }

  return false;
}

bool write_settings()
{
  DEBUG("Writing settings");

  // Try to open the settings file for writing.
  settings_file = fopen(settings_file_name, "w");
  if (!settings_file)
  {
    DEBUG("Error opening settings file");
    return false;
  }

  // If the hotspot's password is invalid, clear both strings.
  if (strlen(wifi_sta_pass) < 8)
  {
    strcpy(wifi_sta_ssid, "");
    strcpy(wifi_sta_pass, "");
  }

  // Rewrite the entire file (version, settings, footer).
  write16(settings_file, SETTINGS_VERSION);
  writeCharArray(settings_file, wifi_sta_ssid);
  writeCharArray(settings_file, wifi_sta_pass);
  write16(settings_file, 0x55AA);

  // Close the file.
  close_file(&settings_file);
  return true;
}

void reset_settings()
{
  // Store default values for all supported settings.
  strcpy(wifi_sta_ssid, "");
  strcpy(wifi_sta_pass, "");
}

#endif
