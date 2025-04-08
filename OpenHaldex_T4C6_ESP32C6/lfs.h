#ifndef LFS_H
#define LFS_H

// The ESP-IDF version of the LittleFS library is used.
#include <esp_littlefs.h>

esp_vfs_littlefs_conf_t littlefs_conf;
void configure_littlefs()
{
  littlefs_conf.base_path = "/littlefs";
  littlefs_conf.partition_label = "littlefs";
  littlefs_conf.format_if_mount_failed = true;
  littlefs_conf.dont_mount = false;

  ESP_ERROR_CHECK(esp_vfs_littlefs_register(&littlefs_conf));
}

#endif
