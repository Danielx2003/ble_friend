#include "ble2.h"
#include "ble_worker2.h"
#include "parser2.h"
#include "crypto2.h"

#include "esp_log.h"
#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/util/util.h"
#include "esp_log.h"
#include "esp_central.h"

#include <stdio.h>

static const ble_uuid128_t key_exchange_svr_uuid = BLE_UUID128_INIT(PUB_KEY_SERVICE_UUID);
static const ble_uuid128_t pub_key_chr_uuid = BLE_UUID128_INIT(PUB_KEY_CHAR_UUID);
static const ble_uuid128_t peer_pub_key_chr_uuid = BLE_UUID128_INIT(PEER_PUB_KEY_CHAR_WRITE_UUID);

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

static int on_read(uint16_t conn_handle,
                   const struct ble_gatt_error *error,
                   struct ble_gatt_attr *attr,
                   void *arg)
{
  ble_work_item_t item = {
      .type = BLE_WORKER_EVENT_READ_COMPLETE,
      .context.read_complete = {
          .conn_handle = conn_handle,
          .status      = error->status,
          .data_len    = 0,
      }
  };

  if (error->status == 0 && attr && attr->om)
  {
      uint16_t len = OS_MBUF_PKTLEN(attr->om);
      if (len > sizeof(item.context.read_complete.data))
      {
          ESP_LOGE(tag, "Read data too large: %d bytes", len);
          return BLE_ATT_ERR_INSUFFICIENT_RES;
      }
      os_mbuf_copydata(attr->om, 0, len, item.context.read_complete.data);
      item.context.read_complete.data_len = len;
  }
	
	// Do crypto work
	const struct peer_chr *chr;
	   int rc;
	   struct os_mbuf *txom;

	crypto_status_t status;
	crypto_key_t pub_key;
	crypto_key_t keypair;

	generate_keypair(CRYPTO_CURVE_X25519, &keypair);
  if (error->status != 0)
  {
    ESP_LOGE(tag, "Read failed status=%d", error->status);
    return 0;
  }

	status = export_public_key(&keypair, &pub_key);
	if (status != CRYPTO_SUCCESS) {
		printf("failed to export public key\n");
		return -1;
	}

	const struct peer *peer = peer_find(conn_handle);
	 chr = peer_chr_find_uuid(
			peer,
			&key_exchange_svr_uuid.u,
		  &peer_pub_key_chr_uuid.u);
	
	 if (chr == NULL)
	{
	   printf("Error: Peer doesn't support the Alert "
	               "Notification Control Point characteristic\n");
		return -1;
	 }
	
	 txom = ble_hs_mbuf_from_flat(&pub_key.raw.data, pub_key.raw.len);
	 if (!txom) {
	   printf("Insufficient memory\n");
		return -1;
	 }
	
	 rc = ble_gattc_write_long(conn_handle, chr->chr.val_handle, 0,
	                           txom, NULL, NULL);
	 if (rc != 0) {
	     printf("Error: Failed to write characteristic; rc=%d\n",
	                 rc);
	 	return -1;
	}
	
	xQueueSend(ble_worker_queue, &item, 0);
	return 0;
}

ble_status_t handle_read_complete(ble_work_read_complete_t *read)
{
    if (read->status != 0)
    {
        ESP_LOGE(tag, "Read failed status=%d", read->status);
        return BLE_FAIL;
    }

    if (read->data_len != 32)
    {
        ESP_LOGE(tag, "Unexpected key length: %d", read->data_len);
        return BLE_FAIL;
    }

    // Your crypto work here — runs safely in worker task, not NimBLE task
    // e.g. generate_keypair(), generate_secret(), export_public_key() ...

    ESP_LOGI(tag, "Received peer public key (%d bytes)", read->data_len);
		
		ESP_LOGI(tag, "Stack remaining: %u bytes",
		         uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t));

    return BLE_SUCCESS;
}

ble_status_t handle_disc_complete(ble_work_disc_complete_t *disc)
{
  if (disc->status != 0)
  {
      ESP_LOGE(tag, "Service discovery failed status=%d", disc->status);
      ble_gap_terminate(disc->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
      return BLE_FAIL;
  }

  const struct peer *peer = peer_find(disc->conn_handle);
  if (peer == NULL)
  {
      ESP_LOGE(tag, "Peer not found for conn_handle=%d", disc->conn_handle);
      return BLE_FAIL;
  }

  const struct peer_chr *chr = peer_chr_find_uuid(
      peer,
      &key_exchange_svr_uuid.u,
      &pub_key_chr_uuid.u);

  if (chr == NULL)
  {
      ESP_LOGE(tag, "Peer does not support pub key characteristic");
      ble_gap_terminate(disc->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
      return BLE_FAIL;
  }

  int rc = ble_gattc_read(disc->conn_handle,
                          chr->chr.val_handle,
                          on_read,
                          NULL);
  if (rc != 0)
  {
      ESP_LOGE(tag, "Failed to initiate read rc=%d", rc);
      ble_gap_terminate(disc->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
      return BLE_FAIL;
  }

  return BLE_SUCCESS;
}

ble_status_t handle_enc_change(ble_work_connect_t *connect)
{
	printf("upgraded connection. start discovery\n");
	discover_all_services(connect);
	return BLE_SUCCESS;
}

ble_status_t handle_on_connect(ble_work_item_t *connect)
{
	handle_new_connection(&(connect->context.connect));
	return BLE_SUCCESS;
}

ble_status_t handle_on_disconnect(ble_work_item_t *item)
{
	printf("disconnected\n");
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