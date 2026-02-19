#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "host/ble_gap.h"

#define PUB_KEY_SERVICE_UUID 0x87, 0xb0, 0x8f, 0x1d, 0xb3, 0x5f, 0xa5, 0xa1, 0x1d, 0x4c, 0xe3, 0x77, 0x76, 0x1b, 0xc4, 0xb0
#define PUB_KEY_CHAR_UUID 0x00, 0x1b, 0x15, 0x3a, 0xbc, 0x4c, 0xac, 0x94, 0xaf, 0x49, 0x32, 0xb9, 0x86, 0xd0, 0x60, 0x46

typedef enum {
  BLE_SUCCESS
} ble_status_t;

typedef enum {
	BLE_DISCONNECT_EVENT,
	BLE_CONNECT_EVENT,
	BLE_EXT_DISC_EVENT
} ble_event_t;

typedef struct {
	void (*on_connect)(void* ctx);
	void (*on_disconnect)(void* ctx);
	void (*on_ext_disc)(void* ctx);
} ble_event_cbs_t;

ble_status_t disc_set_event_handlers(ble_event_cbs_t* cbs);

typedef struct {
  uint8_t filter_duplicates:1;
  uint8_t passive:1;
  uint16_t interval;
} ble_disc_params_t;

ble_status_t disc_start(ble_disc_params_t* params, uint32_t duration);
ble_status_t handle_new_connection(void* ctx);
ble_status_t handle_ext_disc(void* ctx);
