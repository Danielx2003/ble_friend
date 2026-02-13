#include "disc.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/util/util.h"
#include "esp_log.h"

#include <stdio.h>

static const char* tag = "BLE_DISC";

ble_status_t disc_start(
	ble_disc_params_t* params,
  uint32_t duration
)
{
  int rc;
  uint8_t own_addr_type;

  struct ble_gap_disc_params disc_params;
  memset(&disc_params, 0, sizeof(disc_params));

  disc_params.filter_duplicates = params->filter_duplicates;
  disc_params.passive = params->passive;
  disc_params.itvl = params->interval;
 
  rc = ble_hs_id_infer_auto(0, &own_addr_type);
  if (rc != 0) { ESP_LOGE(tag, "Could not infer address type\n"); return rc; }
  
  duration = duration == 0 ? BLE_HS_FOREVER : duration;

  rc = ble_gap_disc(own_addr_type, duration, &disc_params, NULL, NULL);
  if (rc != 0) { ESP_LOGE(tag, "Failed to start discovery rc=%d\n", rc); return rc; }

  ESP_LOGI(tag, "Started Discovery\n");
  
  return BLE_SUCCESS;
}
