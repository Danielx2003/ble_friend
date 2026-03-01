#include "ble2.h"
#include "ble_worker2.h"
#include "parser2.h"
#include "crypto2.h"
#include "request2.h"

#include "esp_log.h"
#include "host/ble_gap.h"
#include "esp_central.h"

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
static int on_read(uint16_t conn_handle,
                   const struct ble_gatt_error *error,
                   struct ble_gatt_attr *attr,
                   void *arg);

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

/* GATT Callback */

static int on_read(uint16_t conn_handle,
                   const struct ble_gatt_error *error,
                   struct ble_gatt_attr *attr,
                   void *arg)
{
  ble_work_item_t item = {
    .type = BLE_WORKER_EVENT_READ_COMPLETE,
    .context.read_complete = {
      .conn_handle = conn_handle,
      .status = error->status,
      .data_len = 0,
    }
  };

  if (error->status == 0 && attr && attr->om) {
    uint16_t len = OS_MBUF_PKTLEN(attr->om);

    if (len > sizeof(item.context.read_complete.data)) {
      ESP_LOGE(tag, "Read data too large: %d bytes", len);
      return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    os_mbuf_copydata(
        attr->om,
        0,
        len,
        item.context.read_complete.data);

    item.context.read_complete.data_len = len;
  }

  xQueueSend(ble_worker_queue, &item, 0);
  return 0;
}

/* Protocol Message Handlers */

bool handle_pairing_msg(ble_work_msg_t *msg, mfg_data_t *mfg)
{
  disc_stop();
  start_connect(msg);

  return true;
}


bool handle_paired_msg(ble_work_msg_t *msg, mfg_data_t *mfg)
{
  printf("paired msg\n");
  return true;
}


bool handle_lost_msg(ble_work_msg_t *msg, mfg_data_t *mfg)
{
	crypto_status_t status;
	crypto_key_t keypair;

	status = generate_keypair(CRYPTO_CURVE_X25519, &keypair);
	if (status != CRYPTO_SUCCESS)
	{
		return false;
	}

	crypto_key_t eph_pub_key = {
		.type = KEY_TYPE_RAW,
		.raw = {
			.len = mfg->payload_len
		}
	};
	memcpy(eph_pub_key.raw.data, mfg->payload, mfg->payload_len);

	crypto_key_t secret;

	status = generate_secret(
		&keypair, 
		&eph_pub_key,
		&secret
	);
	if (status != CRYPTO_SUCCESS)
	{
		return false;
	}

	crypto_key_t aes_key;
	status = derive_symmetric_aes_key_hkdf(
		&secret,
		NULL,
		NULL,
		&aes_key
	);
	if (status != CRYPTO_SUCCESS)
	{
		return false;
	}

	request_lost_payload_t payload = {
		.finder_key = &eph_pub_key
	};

	upload_lost_details(&payload);
	
  return true;
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

  crypto_key_t pub_key;
  crypto_key_t keypair;

  generate_keypair(CRYPTO_CURVE_X25519, &keypair);

  crypto_status_t status =
      export_public_key(&keypair, &pub_key);

  if (status != CRYPTO_SUCCESS) {
		ESP_LOGE(tag, "Failed to export public key. Status=%d", status);
    return BLE_FAIL;
  }

  const struct peer *peer =
      peer_find(read->conn_handle);

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
          &pub_key.raw.data,
          pub_key.raw.len);

  if (!txom) {
    ESP_LOGE(tag, "Insufficient memory to create buffer");
    return BLE_FAIL;
  }

  int rc = ble_gattc_write_long(
      read->conn_handle,
      chr->chr.val_handle,
      0,
      txom,
      NULL,
      NULL);

  if (rc != 0) {
    ESP_LOGE(tag, "Failed to write characteristic; rc=%d\n", rc);
    return BLE_FAIL;
  }

  return BLE_SUCCESS;
}


ble_status_t handle_ext_disc(ble_work_item_t *item)
{
  parser_status_t status;
  mfg_data_t mfg;
  memset(&mfg, 0, sizeof(mfg));

  parser_result_t result = {
    .mfg = &mfg
  };

  status = parse_adv_data(
      item->context.msg.data,
      item->context.msg.len,
      &result);

  if (status != PARSER_SUCCESS) {
    return status;
  }

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


ble_status_t handle_on_disconnect()
{
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

  xTaskCreate(
      ble_worker_task,
      "ble_worker",
      16384,
      NULL,
      5,
      NULL);

  return BLE_SUCCESS;
}