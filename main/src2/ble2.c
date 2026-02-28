#include "ble2.h"
#include "ble_worker2.h"
#include "parser2.h"

#include "esp_log.h"
#include "host/ble_gap.h"

#include <stdio.h>

QueueHandle_t ble_worker_queue = NULL;
void ble_store_config_init(void);
static const char *tag = "BLE_2";

bool handle_pairing_msg(ble_work_msg_t *msg, mfg_data_t *mfg)
{
	disc_stop();
	start_connect(msg);

	printf("BLE worker stack free: %u bytes\n",
	       uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t));
	return true;
}

bool handle_paired_msg(ble_work_msg_t *msg, mfg_data_t *mfg)
{
	printf("paired msg\n");
	return true;
}

bool handle_lost_msg(ble_work_msg_t *msg, mfg_data_t *mfg)
{
	printf("lost msg\n");
	return true;
}

void handle_on_sync()
{
	printf("synced!\n");
  ble_disc_params_t params = {
    .filter_duplicates = 1,
    .passive = 1,
  };

  ble_status_t status = disc_start(&params, 0);
  if (status != BLE_SUCCESS)
  {
    ESP_LOGE(tag,
             "Failed to start discovery. Status=%d",
             status);
  }
}

ble_status_t handle_ext_disc(ble_work_item_t *item)
{
  parser_status_t status;
  mfg_data_t mfg;
  memset(&mfg, 0, sizeof(mfg));

  parser_result_t result;
  result.mfg = &mfg;

  status = parse_adv_data(item->context.msg.data,
                          item->context.msg.len,
                          &result);

  if (status != PARSER_SUCCESS)
    return status;

  result.action(&(item->context.msg), result.mfg);
  return BLE_SUCCESS;
}

ble_status_t handle_on_connect(ble_work_item_t *connect)
{
	printf("connected to\n");
	return BLE_SUCCESS;
}

ble_status_t handle_on_disconnect(ble_work_item_t *item)
{
	ble_disc_params_t params = {
	  .filter_duplicates = 1,
	  .passive = 1,
	};

	ble_status_t status = disc_start(&params, 0);
	return BLE_SUCCESS;
}

/* Public API */

ble_status_t ble_init()
{	
	return BLE_SUCCESS;
}

ble_status_t ble_start()
{
	ble_worker_queue = xQueueCreate(BLE_QUEUE_LEN, sizeof(ble_work_item_t));
	if (!ble_worker_queue) {
	    ESP_LOGE(tag, "Failed to create BLE worker queue");
	    return BLE_ERR_NO_MEMORY;
	}

	xTaskCreate(
	    ble_worker_task,
	    "ble_worker",
	    8192,
	    NULL,
	    5,
	    NULL
	);
	
	printf("returned from main\n");

	return BLE_SUCCESS;
}