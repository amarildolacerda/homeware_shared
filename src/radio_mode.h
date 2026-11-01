// shared/src/radio_mode.h
// Protocolo radio node -> hub escolhido em runtime (ESP-NOW ou TCP/HTTP).
//
// Em builds com RADIO_SELECT_ENABLED (os dois protocolos compilados) o modo
// fica na EEPROM (EEPROM_RADIO_MODE_OFFSET) e pode ser trocado pelo dashboard
// (Configurações). Sem esse flag o modo é o do build (RADIO_MODE_DEFAULT).
#ifndef HW_SHARED_RADIO_MODE_H
#define HW_SHARED_RADIO_MODE_H

#include <stdint.h>

enum RadioMode : uint8_t {
    RADIO_MODE_ESPNOW = 0,
    RADIO_MODE_TCP    = 1,
};

// Modo padrão quando a EEPROM ainda não tem valor válido.
#ifndef RADIO_MODE_DEFAULT
  #if defined(TCP_ENABLED)
    #define RADIO_MODE_DEFAULT RADIO_MODE_TCP
  #else
    #define RADIO_MODE_DEFAULT RADIO_MODE_ESPNOW
  #endif
#endif

// Texto usado na API/dashboard ("espnow" | "tcp").
const char* radio_mode_id(uint8_t mode);
// Texto legível ("ESP-NOW" | "TCP (HTTP)").
const char* radio_mode_label(uint8_t mode);
// Converte id/texto para modo. Retorna false se desconhecido.
bool radio_mode_parse(const char* text, uint8_t* out);

// true se o protocolo foi compilado neste firmware.
bool radio_mode_compiled(uint8_t mode);

// Modo persistido na EEPROM (fallback: RADIO_MODE_DEFAULT).
uint8_t radio_mode_load();
// Persiste o modo (ignora modo não compilado).
void radio_mode_save(uint8_t mode);

// Modo em uso agora (RAM). Resolvido da EEPROM no primeiro acesso.
uint8_t radio_mode_active();
void radio_mode_set_active(uint8_t mode);

#endif
