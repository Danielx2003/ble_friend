#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "host/ble_gap.h"

typedef enum {
  BLE_SUCCESS
} ble_status_t;

typedef struct {
  uint8_t filter_duplicates:1;
  uint8_t passive:1;
  uint16_t interval;
} ble_disc_params_t;

ble_status_t disc_start(
    ble_disc_params_t* params,
    uint32_t duration
);