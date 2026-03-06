#include "ble_worker2.h"
#include "crypto2.h"
#include "crypto_worker2.h"
#include "request2.h"
#include "esp_log.h"

/* BLE Worker Task */

void handle_lost_msg_crypto(crypto_work_item_t *item)
{
	crypto_status_t status;
	crypto_key_t keypair;

	status = generate_keypair(CRYPTO_CURVE_X25519, &keypair);
	if (status != CRYPTO_SUCCESS) { return; }

	crypto_key_t eph_pub_key = {
		.type = KEY_TYPE_RAW,
		.raw = {
			.len = item->context.lost_msg.mfg.payload_len
		}
	};
	memcpy(
		eph_pub_key.raw.data,
		item->context.lost_msg.mfg.payload,
		item->context.lost_msg.mfg.payload_len
	);

	crypto_key_t secret;

	status = generate_secret(
		&keypair, 
		&eph_pub_key,
		&secret
	);
	if (status != CRYPTO_SUCCESS) { return; }

	crypto_key_t aes_key;
	status = derive_symmetric_aes_key_hkdf(
		&secret,
		NULL,
		NULL,
		&aes_key
	);
	if (status != CRYPTO_SUCCESS) { return; }
	
//	request_lost_payload_t payload = {
//		.finder_key = &eph_pub_key
//	};

	//	Send Upload Command
	request_work_item_t request_item = {
		.type = REQUEST_WORKER_EVENT_UPLOAD_LOST_LOCATION,
	};
	
	xQueueSend(request_worker_queue, &request_item, 0);
}

void handle_read_complete_crypto(crypto_work_item_t *item)
{
	crypto_key_t pub_key;
	crypto_key_t keypair;

	generate_keypair(CRYPTO_CURVE_X25519, &keypair);

	crypto_status_t status =
	    export_public_key(&keypair, &pub_key, 32);

	if (status != CRYPTO_SUCCESS) {
		ESP_LOGE("CRYPTO", "Failed to export public key. Status=%d", status);
	}

  ble_work_item_t ble_item;
  ble_item.type = BLE_WORKER_EVENT_READ_COMPLETE;
	memcpy(
		&ble_item.context.read_complete,
		&item->context.read_complete,
		sizeof(item->context.read_complete)
	);

  if (xQueueSend(ble_worker_queue, &ble_item, portMAX_DELAY) != pdPASS) {
      ESP_LOGE("CRYPTO", "Failed to send to BLE worker queue");
  }
	
	ble_item.type = BLE_WORKER_EVENT_WRITE_KEY_TO_PEER;
	ble_item.context.write_pub_key.conn_handle = item->context.read_complete.conn_handle;
	memcpy(
		ble_item.context.write_pub_key.pub_key,
		pub_key.raw.data,
		pub_key.raw.len
	);
	ble_item.context.write_pub_key.pub_key_len = pub_key.raw.len;

	if (xQueueSend(ble_worker_queue, &ble_item, portMAX_DELAY) != pdPASS) {
	    ESP_LOGE("CRYPTO", "Failed to send to BLE worker queue");
	}
}

void crypto_worker_task(void *param)
{
	printf("starting crypto task\n");
  crypto_work_item_t item;

  while(1) {
	  if(xQueueReceive(crypto_worker_queue, &item, portMAX_DELAY)) {
	    switch(item.type) {
				case CRYPTO_WORKER_EVENT_LOST_MSG:
					handle_lost_msg_crypto(&item);
					break;
				case CRYPTO_WORKER_EVENT_READ_COMPLETE:
					handle_read_complete_crypto(&item);
					break;
				default:
					break;
      }
    }
  }
}