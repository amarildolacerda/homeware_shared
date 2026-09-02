#ifndef HW_SHARED_CONFIG_H
#define HW_SHARED_CONFIG_H

#include <Arduino.h>

// Watchdog estavel: tempo saudavel continuo para (re)armar o timer de restart
// (ver common_watchdog.h). Devices podem sobrescrever no proprio config.h.
#ifndef WATCHDOG_STABLE_RESET_MS
#define WATCHDOG_STABLE_RESET_MS 60000
#endif

// Configuracao minima compartilhada entre gateway e clients para o
// myWiFiManager e espnow_protocol. Mantenha IGUAL em todos os devices.

#define WIFI_MODE_DHCP 0
#define WIFI_MODE_STATIC 1

#define EEPROM_SIZE 512
#define EEPROM_WIFI_SSID_OFFSET 0
#define EEPROM_WIFI_SSID_SIZE 33
#define EEPROM_WIFI_PASS_OFFSET (EEPROM_WIFI_SSID_OFFSET + EEPROM_WIFI_SSID_SIZE)
#define EEPROM_WIFI_PASS_SIZE 65
#define EEPROM_WIFI_MODE_OFFSET (EEPROM_WIFI_PASS_OFFSET + EEPROM_WIFI_PASS_SIZE)
#define EEPROM_WIFI_MODE_SIZE 1
#define EEPROM_WIFI_IP_OFFSET (EEPROM_WIFI_MODE_OFFSET + EEPROM_WIFI_MODE_SIZE)
#define EEPROM_WIFI_IP_SIZE 16
#define EEPROM_WIFI_GW_OFFSET (EEPROM_WIFI_IP_OFFSET + EEPROM_WIFI_IP_SIZE)
#define EEPROM_WIFI_GW_SIZE 16
#define EEPROM_WIFI_MASK_OFFSET (EEPROM_WIFI_GW_OFFSET + EEPROM_WIFI_GW_SIZE)
#define EEPROM_WIFI_MASK_SIZE 16
#define EEPROM_WIFI_DNS_OFFSET (EEPROM_WIFI_MASK_OFFSET + EEPROM_WIFI_MASK_SIZE)
#define EEPROM_WIFI_DNS_SIZE 16
#define EEPROM_WIFI_CHANNEL_OFFSET (EEPROM_WIFI_DNS_OFFSET + EEPROM_WIFI_DNS_SIZE)
#define EEPROM_WIFI_CHANNEL_SIZE 1

// EEPROM layout shared by all clients (common_espnow.h):
//   [0]    = MAGIC 0xAA (gateway MAC present)
//   [1..6] = gateway MAC
//   [10]   = marker 0xFF (device name present)
//   [11..58] = device name (max 48 bytes)
// Clients define additional device-specific EEPROM data after offset 60+.

#define WIFI_CONFIG_PORTAL_SSID "ESPNOW_Setup"
#define WIFI_CONFIG_PORTAL_PASS "password123"

#define ESP_NOW_CHANNEL 1

#ifndef FW_VERSION
#define FW_VERSION "v1.2.24"
#endif

// TCP node defaults
#ifndef HUB_IP_DEFAULT
#define HUB_IP_DEFAULT "192.168.1.14"
#endif
#ifndef HUB_PORT
#define HUB_PORT 80
#endif

#endif
