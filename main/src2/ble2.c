#include "ble2.h"
#include "ble_worker2.h"
#include "crypto2.h"
#include "crypto_worker2.h"
#include "parser2.h"
#include "crypto2.h"
#include "request2.h"
#include "request_worker2.h"

#include "esp_log.h"
#include "host/ble_gap.h"
#include "esp_central.h"
#include "freertos/idf_additions.h"
#include <sys/time.h>

#include <stdio.h>

/* Static + Global Variables */

static const char *tag = "BLE_2";

static const ble_uuid128_t key_exchange_svr_uuid =
    BLE_UUID128_INIT(PUB_KEY_SERVICE_UUID);
static const ble_uuid128_t pub_key_chr_uuid =
    BLE_UUID128_INIT(PUB_KEY_CHAR_UUID);
static const ble_uuid128_t peer_pub_key_chr_uuid =
    BLE_UUID128_INIT(PEER_PUB_KEY_CHAR_WRITE_UUID);

QueueHandle_t ble_worker_queue = NULL;

/* Function Declarations */

void ble_store_config_init(void);

/* NimBLE Stack Callbacks */
									 
void handle_on_sync(void)
{
  ble_disc_params_t params = {
    .filter_duplicates = 1,
    .passive = 1,
  };

  ble_status_t status = disc_start(&params, 0);
  if (status != BLE_SUCCESS) {
    ESP_LOGE(tag, "Failed to start discovery. Status=%d", status);
  }
}


/* Protocol Message Handlers */

void handle_pairing_msg(ble_work_msg_t *msg, mfg_data_t *mfg)
{
	printf("pairing msg receive\n");
  disc_stop();
  start_connect(msg);
}


void handle_paired_msg(ble_work_msg_t *msg, mfg_data_t *mfg)
{
  printf("paired msg\n");
}

int payloads_received = 0;


void handle_lost_msg(ble_work_msg_t *msg, mfg_data_t *mfg)
{
	printf("lost msg discovered\n");
	crypto_work_item_t crypto_item = {
		.type = CRYPTO_WORKER_EVENT_LOST_MSG,
	};
	
	xQueueSend(crypto_worker_queue, &crypto_item, 0);
}

/* Event Handlers */

ble_status_t handle_read_complete(ble_work_read_complete_t *read)
{
  if (read->status != 0) {
    ESP_LOGE(tag, "Read failed status=%d", read->status);
    return BLE_FAIL;
  }

  if (read->data_len != 32) {
    ESP_LOGE(tag, "Unexpected key length: %d", read->data_len);
    return BLE_FAIL;
  }

  ESP_LOGI(tag, "Received peer public key (%d bytes)", read->data_len);

  return BLE_SUCCESS;
}

ble_status_t write_key_to_peer(ble_work_write_key_t *item)
{
	const struct peer *peer =
	      peer_find(item->conn_handle);

  const struct peer_chr *chr =
      peer_chr_find_uuid(
          peer,
          &key_exchange_svr_uuid.u,
          &peer_pub_key_chr_uuid.u);

  if (chr == NULL) {
    ESP_LOGE(tag, "Peer does not support key write characteristic");
    return BLE_FAIL;
  }

  struct os_mbuf *txom =
      ble_hs_mbuf_from_flat(
				item->pub_key,
				item->pub_key_len);

  if (!txom) {
    ESP_LOGE(tag, "Insufficient memory to create buffer");
    return BLE_FAIL;
  }

  int rc = ble_gattc_write_long(
      item->conn_handle,
      chr->chr.val_handle,
      0,
      txom,
      NULL,
      NULL);

  if (rc != 0) {
    ESP_LOGE(tag, "Failed to write characteristic; rc=%d\n", rc);
    return BLE_FAIL;
  }
	
	ESP_LOGE(tag, "Wrote public key to other device\n");

	return BLE_SUCCESS;
}

#include "esp_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

ble_status_t handle_ext_disc(ble_work_item_t *item)
{
  parser_status_t status;
  mfg_data_t mfg;

  parser_result_t result = {
    .mfg = &mfg
  };

  status = parse_adv_data(
		item->context.msg.data,
			item->context.msg.len,
			&result
	);

	if (status != CRYPTO_SUCCESS) { return status; }

  result.action(&item->context.msg, result.mfg);
	
  return BLE_SUCCESS;
}


ble_status_t handle_disc_complete(ble_work_disc_complete_t *disc)
{
  if (disc->status != 0) {
    ESP_LOGE(tag, "Service discovery failed status=%d", disc->status);
    ble_gap_terminate(disc->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    return BLE_FAIL;
  }

  const struct peer *peer = peer_find(disc->conn_handle);
  if (peer == NULL) {
    ESP_LOGE(tag, "Peer not found for conn_handle=%d", disc->conn_handle);
    return BLE_FAIL;
  }

  const struct peer_chr *chr = peer_chr_find_uuid(
      peer,
      &key_exchange_svr_uuid.u,
      &pub_key_chr_uuid.u);

  if (chr == NULL) {
    ESP_LOGE(tag, "Peer does not support pub key characteristic");
    ble_gap_terminate(disc->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    return BLE_FAIL;
  }

  int rc = ble_gattc_read(
      disc->conn_handle,
      chr->chr.val_handle,
      on_read,
      NULL);

  if (rc != 0) {
    ESP_LOGE(tag, "Failed to initiate read rc=%d", rc);
    ble_gap_terminate(disc->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    return BLE_FAIL;
  }

  return BLE_SUCCESS;
}


ble_status_t handle_on_connect(ble_work_item_t *item)
{
  handle_new_connection(&item->context.connect);
  return BLE_SUCCESS;
}

ble_status_t handle_enc_change(ble_work_connect_t *connect)
{
  discover_all_services(connect);
  return BLE_SUCCESS;
}


ble_status_t handle_on_disconnect(ble_work_disconnect_t *disconnect)
{
	int rc = peer_delete(disconnect->conn_handle);
	if (rc != 0)
	{
		ESP_LOGE(tag,
		         "Failed to delete peer rc=%d",
		         rc);
		return 0;
	}	

  ble_disc_params_t params = {
    .filter_duplicates = 1,
    .passive = 1,
  };

  ble_status_t status = disc_start(&params, 0);
  if (status != BLE_SUCCESS) {
    ESP_LOGE(tag, "Failed to start discovery. Status=%d", status);
    return status;
  }

  return BLE_SUCCESS;
}


/* Public API */

ble_status_t ble_init(void)
{
  return BLE_SUCCESS;
}

ble_status_t ble_start(void)
{
  ble_worker_queue =
      xQueueCreate(BLE_QUEUE_LEN, sizeof(ble_work_item_t));

  if (!ble_worker_queue) {
    ESP_LOGE(tag, "Failed to create BLE worker queue");
    return BLE_ERR_NO_MEMORY;
  }
	
	xTaskCreatePinnedToCore(
    ble_worker_task,
    "ble_worker",
    16384,
    NULL,
    20,
    NULL,
		0
	);

  return BLE_SUCCESS;
}