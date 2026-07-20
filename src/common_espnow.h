#ifndef HW_SHARED_ESPNOW_CLIENT_H
#define HW_SHARED_ESPNOW_CLIENT_H

#include <Arduino.h>
#include <EEPROM.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <esp_now.h>
#else
#include <ESP8266WiFi.h>
#include <espnow.h>
#endif

// EEPROM layout shared by all clients:
//   [0]    = MAGIC 0xAA
//   [1..6] = gateway MAC (6 bytes)
//   [10]   = marker 0xFF
//   [11..58] = device name (max 48 bytes, null-terminated)
#define EEPROM_CLIENT_DATA_ADDR 0
#define EEPROM_CLIENT_MAGIC 0xAA
#define EEPROM_GATEWAY_MAC_OFFSET 1
#define EEPROM_GATEWAY_MAC_SIZE 6
#define EEPROM_NAME_MARKER_OFFSET 10
#define EEPROM_NAME_DATA_OFFSET 11
#define EEPROM_NAME_MAX 48
#define EEPROM_CLIENT_DATA_SIZE 128

static inline bool espnow_client_init(const char *tag)
{
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    if (esp_now_init() != 0)
    {
        Serial.printf("[%s] ESP-NOW init failed\n", tag ? tag : "client");
        return false;
    }
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    Serial.printf("[%s] ESP-NOW initialized\n", tag ? tag : "client");
    return true;
}

static inline bool espnow_client_add_peer(const uint8_t *mac, const char *tag)
{
    esp_now_del_peer((uint8_t *)mac);
    int ch = WiFi.channel();
    if (ch < 1 || ch > 13)
        ch = 1;
    int ret = esp_now_add_peer((uint8_t *)mac, ESP_NOW_ROLE_COMBO, ch, NULL, 0);
    if (ret != 0)
    {
        char mac_str[18];
        mac_to_str(mac, mac_str, sizeof(mac_str));
        Serial.printf("[%s] Failed to add peer %s: %d\n", tag ? tag : "client", mac_str, ret);
    }
    return ret == 0;
}

static inline void espnow_save_gateway_mac(const uint8_t *mac, const char *tag)
{
    EEPROM.begin(EEPROM_CLIENT_DATA_SIZE);
    EEPROM.write(EEPROM_CLIENT_DATA_ADDR, EEPROM_CLIENT_MAGIC);
    for (int i = 0; i < EEPROM_GATEWAY_MAC_SIZE; i++)
        EEPROM.write(EEPROM_GATEWAY_MAC_OFFSET + i, mac[i]);
    EEPROM.commit();
    EEPROM.end();
    char mac_str[18];
    mac_to_str(mac, mac_str, sizeof(mac_str));
    Serial.printf("[%s] Gateway MAC saved: %s\n", tag ? tag : "client", mac_str);
}

static inline bool espnow_load_gateway_mac(uint8_t *mac_out, const char *tag)
{
    EEPROM.begin(EEPROM_CLIENT_DATA_SIZE);
    uint8_t marker = EEPROM.read(EEPROM_CLIENT_DATA_ADDR);
    if (marker == EEPROM_CLIENT_MAGIC)
    {
        for (int i = 0; i < EEPROM_GATEWAY_MAC_SIZE; i++)
            mac_out[i] = EEPROM.read(EEPROM_GATEWAY_MAC_OFFSET + i);
        EEPROM.end();
        char mac_str[18];
        mac_to_str(mac_out, mac_str, sizeof(mac_str));
        Serial.printf("[%s] Loaded gateway MAC: %s\n", tag ? tag : "client", mac_str);
        return true;
    }
    EEPROM.end();
    return false;
}

static inline bool espnow_is_valid_name(const char *s)
{
    if (!s || s[0] == '\0')
        return false;
    for (int i = 0; s[i]; i++)
    {
        char c = s[i];
        if (c < 32 || c > 126)
            return false;
    }
    return true;
}

static inline void espnow_save_device_name(const char *name)
{
    EEPROM.begin(EEPROM_CLIENT_DATA_SIZE);
    EEPROM.write(EEPROM_NAME_MARKER_OFFSET, 0xFF);
    for (int i = 0; i < EEPROM_NAME_MAX - 1; i++)
    {
        EEPROM.write(EEPROM_NAME_DATA_OFFSET + i, name[i]);
        if (name[i] == '\0')
            break;
    }
    EEPROM.write(EEPROM_NAME_DATA_OFFSET + EEPROM_NAME_MAX - 1, '\0');
    EEPROM.commit();
    EEPROM.end();
}

static inline bool espnow_load_device_name(char *name_out, size_t max_len)
{
    EEPROM.begin(EEPROM_CLIENT_DATA_SIZE);
    uint8_t marker = EEPROM.read(EEPROM_NAME_MARKER_OFFSET);
    bool found = false;
    if (marker == 0xFF)
    {
        char buf[EEPROM_NAME_MAX];
        for (int i = 0; i < EEPROM_NAME_MAX - 1; i++)
        {
            buf[i] = EEPROM.read(EEPROM_NAME_DATA_OFFSET + i);
            if (buf[i] == '\0')
                break;
        }
        buf[EEPROM_NAME_MAX - 1] = '\0';
        if (espnow_is_valid_name(buf))
        {
            size_t slen = strlen(buf);
            size_t copy_len = (slen < max_len - 1) ? slen : max_len - 1;
            memcpy(name_out, buf, copy_len);
            name_out[copy_len] = '\0';
            found = true;
        }
    }
    EEPROM.end();
    return found;
}

static inline bool mac_parse(const char *str, uint8_t *mac)
{
    int vals[6];
    if (sscanf(str, "%x:%x:%x:%x:%x:%x",
               &vals[0], &vals[1], &vals[2],
               &vals[3], &vals[4], &vals[5]) != 6)
        return false;
    for (int i = 0; i < 6; i++)
        mac[i] = (uint8_t)vals[i];
    return true;
}

#endif
