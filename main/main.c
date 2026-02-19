#include "esp_log.h"

#include "disc.h"
#include "ble.h"
#include "device.h"
#include "parser.h"

#include <stdbool.h>

/* Static Variables */

static const char *tag = "BLE_SCAN_APP";

void on_pairing_msg(void *ctx, mfg_data_t *mfg);
void on_lost_msg(void *ctx, mfg_data_t *mfg);
void on_paired_msg(void *ctx, mfg_data_t *mfg);

static parser_action_table_t ble_actions = {
  .on_pairing = on_pairing_msg,
  .on_paired = on_paired_msg,
  .on_lost = on_lost_msg
};

/* Function Definitions */

void ble_store_config_init(void);

/* Callbacks */

void on_connect(void *ctx)
{
  ESP_LOGI(tag, "Connected");

  handle_new_connection(ctx);
}

void on_disconnect(void *ctx)
{
  ESP_LOGI(tag, "Disconnected");
}

void on_pairing_msg(void *ctx, mfg_data_t *mfg)
{
	disc_stop();
	start_connect(ctx);
}

void on_lost_msg(void *ctx, mfg_data_t *mfg)
{
	printf("lost msg receiev\n");
}

void on_paired_msg(void *ctx, mfg_data_t *mfg)
{
	printf("paired msg receiev\n");
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
		.on_ext_disc = on_ext_disc,
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

/* Main */

void app_main(void)
{
  device_init();

  ble_callbacks_t callbacks = {
    .on_ready = on_sync,
    .on_reset = on_reset
  };

	parser_init(&ble_actions);

  ble_init(&callbacks);
  ble_start();
}
