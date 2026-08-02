#ifndef HW_SHARED_ESPNOW_PROTOCOL_H
#define HW_SHARED_ESPNOW_PROTOCOL_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <Arduino.h>
#include "shared_config.h"
#include "msg_type.h"
#include "sensor_type.h"
#include "common_util.h"

#ifndef LORA_DEVICE
#ifdef ESP32
#include <esp_now.h>
#else
#include <espnow.h>
#endif
#endif

#define ESPNOW_PROTOCOL_VERSION 1
#define ESPNOW_MAX_PAYLOAD 250
#define ESPNOW_SEQUENCE_MAX 65535
#define ESPNOW_HEADER_FIXED_SIZE (sizeof(espnow_header_t) - sizeof(((espnow_header_t*)0)->payload))

/* Unified message types in shared/src/msg_type.h */

typedef enum {
    HW_CHIP_UNKNOWN = 0xFF,
    HW_CHIP_ESP_1 = 0,   /* was HW_CHIP_ESP8266 */
    HW_CHIP_ESP_2 = 1,   /* was HW_CHIP_ESP32 */
} chip_type_t;

typedef enum {
    NAK_REASON_NONE = 0,
    NAK_REASON_NO_GATEWAY = 1,
    NAK_REASON_GATEWAY_LOST = 2,
    NAK_REASON_PAIRING_DISABLED = 3,
    NAK_REASON_FULL = 4,
} nak_reason_t;

typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t msg_type;
    uint16_t sequence;
    uint8_t sensor_mac[6];
    uint8_t sensor_type;
    uint8_t battery_pct;
    int16_t rssi;
    uint8_t payload_len;
    uint8_t payload[ESPNOW_MAX_PAYLOAD - 18];
} espnow_header_t;

typedef struct __attribute__((packed)) {
    float temperature;
    float humidity;
} payload_temp_hum_t;

typedef struct __attribute__((packed)) {
    uint8_t contact_state;
    uint8_t tamper;
} payload_contact_t;

typedef struct __attribute__((packed)) {
    uint8_t motion_state;
    uint8_t occupancy_duration;
} payload_motion_t;

typedef struct __attribute__((packed)) {
    uint16_t gas_level;
    uint8_t alarm;
} payload_gas_t;

typedef struct __attribute__((packed)) {
    float temperature;
    float humidity;
    uint16_t gas_level;
    uint8_t alarm;
} payload_dht_gas_t;

typedef struct __attribute__((packed)) {
    uint8_t rain_level;
    uint8_t rain_digital;
} payload_rain_t;

typedef struct __attribute__((packed)) {
    uint16_t level_pct;
    uint16_t distance_cm;
} payload_tank_t;

typedef struct __attribute__((packed)) {
    uint8_t state;
} payload_onoff_t;

typedef struct __attribute__((packed)) {
    uint16_t raw_adc;
    uint8_t moisture_pct;
} payload_soil_moisture_t;

typedef struct __attribute__((packed)) {
    uint8_t msg_type;
    uint16_t sequence;
    uint8_t sensor_mac[6];
    uint8_t status;
    uint8_t assigned_slot;
} espnow_ack_t;

typedef struct __attribute__((packed)) {
    uint8_t msg_type;
    uint16_t sequence;
    uint8_t sensor_mac[6];
    uint8_t gateway_mac[6];
    uint8_t status;
    uint16_t assigned_slot;
} espnow_pair_response_t;

typedef struct __attribute__((packed)) {
    uint8_t msg_type;
    uint16_t sequence;
    uint8_t target_mac[6];
    uint8_t command;
    char target_device_id[32];
} espnow_command_t;

typedef struct __attribute__((packed)) {
    uint8_t msg_type;
    uint16_t sequence;
    uint8_t target_mac[6];
} espnow_restart_t;

typedef struct __attribute__((packed)) {
    uint8_t msg_type;
    uint16_t sequence;
    uint8_t target_mac[6];
    uint8_t reason;
} espnow_nak_t;

/* Pair request sent by clients (ESP-NOW broadcast, msg_type=0x02).
   Layout mantido compatível com os clients em produção (campos gravados na
   ordem: msg_type, sequence, sensor_mac, sensor_type, firmware_version,
   device_name). client_chip é enviado como HW_CHIP_ESP_1 por clients legados.
   Regra 17: qualquer mudança nesta struct deve valer para gateway + clients. */
typedef struct __attribute__((packed)) {
    uint8_t  msg_type;
    uint16_t sequence;
    uint8_t  sensor_mac[6];
    uint8_t  sensor_type;
    uint8_t  firmware_version[4];
    char     device_name[32];
    uint8_t  client_chip;
} espnow_pair_request_t;

typedef struct __attribute__((packed)) {
    uint8_t msg_type;
    uint16_t sequence;
    uint8_t gateway_mac[6];
    uint32_t epoch_seconds;
} espnow_time_sync_t;

typedef struct __attribute__((packed)) {
    uint8_t msg_type;
    uint8_t gateway_mac[6];
    uint8_t fw_version[4];
} espnow_gw_announce_t;

typedef struct __attribute__((packed)) {
    uint8_t msg_type;
} espnow_gw_discover_t;

typedef struct __attribute__((packed)) {
    uint16_t received;
    uint16_t forwarded;
    uint8_t  client_count;
    uint8_t  channel;
    int16_t rssi;
    uint32_t uptime_s;
    uint16_t free_heap;
    uint8_t  ack_failures;
} payload_repeater_status_t;

#define PAIR_STATUS_OK 0
#define PAIR_STATUS_FULL 1
#define PAIR_STATUS_DENIED 2

// mac_to_str, mac_equal, mac_copy now live in common_util.h

// Envia via ESP-NOW e registra no console se foi BROADCAST ou UNICAST,
// incluindo o MAC de destino. Substitui chamadas diretas a esp_now_send()
// para centralizar o log (regra 17). Retorna true se enviado com sucesso.
// Usa Serial.printf para nao acoplar ao ConsoleOutput (evita conflito com
// console.h local dos clients); o telnet espelha Serial.
#ifndef LORA_DEVICE
static inline bool espnow_send_wrapper(const uint8_t *dst, const uint8_t *data,
                                       size_t len, const char *tag) {
    int ret = esp_now_send((uint8_t *)dst, (uint8_t *)data, len);
    if (ret != 0) {
        bool is_bcast = (dst[0] == 0xFF && dst[1] == 0xFF && dst[2] == 0xFF &&
                         dst[3] == 0xFF && dst[4] == 0xFF && dst[5] == 0xFF);
        char mac_str[18];
        if (is_bcast) {
            strcpy(mac_str, "FF:FF:FF:FF:FF:FF");
        } else {
            mac_to_str(dst, mac_str, sizeof(mac_str));
        }
        Serial.printf("[%s] ESP-NOW FAIL %s -> %s (%d bytes)\n",
                       tag ? tag : "espnow",
                       is_bcast ? "BROADCAST" : "UNICAST",
                       mac_str, (int)len);
    }
    return ret == 0;
}
#endif // !LORA_DEVICE

#endif
