#ifndef HW_SHARED_MY_WIFI_MANAGER_H
#define HW_SHARED_MY_WIFI_MANAGER_H

#include <Arduino.h>
#include "shared_config.h"

#if defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
  #include <WiFiManager.h>
#endif

typedef enum {
    WIFI_STATE_DISCONNECTED = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_PORTAL,
} wifi_state_t;

bool mywifi_begin(bool force_portal);
bool mywifi_portal(char *name_buf, size_t name_size, void (*on_name)(const char*));
void mywifi_loop();
wifi_state_t mywifi_state();
const char* mywifi_ssid();
void mywifi_espnow_mac(uint8_t *out);

#endif
