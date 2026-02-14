#include "ble.h"

#include "esp_log.h"
#include "nvs_flash.h"
/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "host/ble_gap.h"
#include <stdbool.h>
#include <string.h>

static ble_callbacks_t app_callbacks;
void ble_store_config_init(void);

static void nimble_on_reset(int reason)
{
    if (app_callbacks.on_reset) {
		
		ble_event_reset_t reset;
		memset(&reset, 0, sizeof(reset));
		reset.reason = reason;
		
        app_callbacks.on_reset(&reset);
    }
}

static void nimble_on_sync(void)
{
    if (app_callbacks.on_ready) {
        app_callbacks.on_ready();
    }
}

int ble_init(const ble_callbacks_t* callbacks)
{
  esp_err_t ret;

  app_callbacks = *callbacks;

  ret = nimble_port_init();
  if (ret != 0) {
      return -1;
  }

  ble_hs_cfg.reset_cb = nimble_on_reset;
  ble_hs_cfg.sync_cb  = nimble_on_sync;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  ble_store_config_init();

  return 0;
}

void ble_host_task(void* param)
{
  nimble_port_run();
  nimble_port_freertos_deinit();
}

int ble_start(void)
{
    nimble_port_freertos_init(ble_host_task);
    return 0;
}

void device_init()
{
	/* Initialize NVS — it is used to store PHY calibration data */
	esp_err_t ret = nvs_flash_init();
	if  (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	    ESP_ERROR_CHECK(nvs_flash_erase());
	    ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
}
