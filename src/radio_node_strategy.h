// shared/src/radio_node_strategy.h
// Compile-time radio strategy selection for nodes — single place for the #ifdef decision.
// Each node just includes this and uses NodeRadioType.
#ifndef HW_SHARED_RADIO_NODE_STRATEGY_H
#define HW_SHARED_RADIO_NODE_STRATEGY_H

#ifdef TCP_ENABLED
#include "tcp_node_protocol.h"
typedef TcpNodeProtocol NodeRadioType;
#elif defined(ESPNOW_ENABLED)
#include "espnow_node_protocol.h"
typedef EspnowNodeProtocol NodeRadioType;
#elif defined(LORA_ENABLED)
#include "lora_node_protocol.h"
typedef LoraNodeProtocol NodeRadioType;
#else
#error "Define one of: TCP_ENABLED, ESPNOW_ENABLED, LORA_ENABLED"
#endif

#endif
