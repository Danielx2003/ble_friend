#pragma once

typedef struct {
	int reason;
} ble_event_reset_t;

typedef struct {
  void (*on_ready)(void);
  void (*on_reset)(ble_event_reset_t* reset);
} ble_callbacks_t;

int ble_init(const ble_callbacks_t* cbs);
int ble_start(void);