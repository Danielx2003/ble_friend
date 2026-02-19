#include "esp_log.h"

#include "disc.h"
#include "ble.h"
#include "device.h"

#include <stdbool.h>

static const char *tag = "BLE_SCAN_APP";

void ble_store_config_init(void);

void on_connect(void *ctx)
{
  ESP_LOGI(tag, "Connected");

  handle_new_connection(ctx);
}

void on_disconnect(void *ctx)
{
  ESP_LOGI(tag, "Disconnected");
}

void on_ext_disc(void* ctx)
{
	ble_status_t status = handle_ext_disc(ctx);
}

void on_reset(ble_event_reset_t *reset)
{
  ESP_LOGE(tag, "Host reset. Reason=%d", reset->reason);
}

void on_sync(void)
{
  ble_disc_params_t params = {
    .filter_duplicates = 1,
    .passive = 1,
  };

  ble_event_cbs_t event_cbs = {
    .on_connect = on_connect,
    .on_disconnect = on_disconnect,
		.on_ext_disc = on_ext_disc
  };

  disc_set_event_handlers(&event_cbs);

  ble_status_t status = disc_start(&params, 0);
  if (status != BLE_SUCCESS)
  {
    ESP_LOGE(tag,
             "Failed to start discovery. Status=%d",
             status);
  }
}

void app_main(void)
{
  device_init();

  ble_callbacks_t callbacks = {
    .on_ready = on_sync,
    .on_reset = on_reset
  };

  ble_init(&callbacks);
  ble_start();
}
