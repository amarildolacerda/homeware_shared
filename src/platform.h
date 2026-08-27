#ifndef PLATFORM_H
#define PLATFORM_H

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32)
  #include <WiFi.h>
  #include <esp_now.h>
  #include <Update.h>
  #define PLATFORM_PREFIX "agri"
  #if defined(ASYNC_WEB_ENABLED)
    #include <ESPAsyncWebServer.h>
    typedef AsyncWebServer MyWebServer;
  #else
    #include <WebServer.h>
    typedef WebServer MyWebServer;
  #endif

  static inline uint32_t chip_id() {
      return (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF);
  }

  // chip_type_t (espnow_protocol.h): HW_CHIP_ESP_2 = ESP32
  static inline uint8_t hw_chip_type() { return 1; }

  static inline bool espnow_add_peer_wrapper(const uint8_t *mac, int channel) {
      esp_now_del_peer((uint8_t*)mac);
      esp_now_peer_info_t peer = {};
      memcpy(peer.peer_addr, mac, 6);
      peer.channel = channel;
      peer.ifidx = WIFI_IF_STA;
      peer.encrypt = false;
      return esp_now_add_peer(&peer) == ESP_OK;
  }
#else
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266HTTPClient.h>
  #include <WiFiClient.h>
  #include <espnow.h>
  #include <Updater.h>
  #define PLATFORM_PREFIX "Ag"
  typedef ESP8266WebServer MyWebServer;

  static inline uint32_t chip_id() {
      return ESP.getChipId();
  }

  // chip_type_t (espnow_protocol.h): HW_CHIP_ESP_1 = ESP8266
  static inline uint8_t hw_chip_type() { return 0; }

  static inline bool espnow_add_peer_wrapper(const uint8_t *mac, int channel) {
      esp_now_del_peer((uint8_t*)mac);
      int ret = esp_now_add_peer((uint8_t*)mac, ESP_NOW_ROLE_COMBO, channel, NULL, 0);
      return ret == 0;
  }
#endif

#endif
