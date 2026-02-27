#pragma once

#include "ble_worker2.h"

typedef enum {
	BLE_SUCCESS,
	BLE_ERR_NO_MEMORY
} ble_status_t;

typedef struct {
	int reason;
} ble_event_reset_t;

typedef struct {
  void (*on_ready)(void);
  void (*on_reset)(ble_event_reset_t* reset);
} ble_callbacks_t;


/* Public API */

/*
Initialise necessary callbacks, BLE services etc
*/
ble_status_t ble_init();


/*
Start the BLE Worker Task
*/
ble_status_t ble_start(void);