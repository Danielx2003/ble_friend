#include "ble_worker2.h"
#include "crypto2.h"
#include "crypto_worker2.h"
#include "freertos/idf_additions.h"
#include "psa/crypto_values.h"
#include "request2.h"

#include "esp_log.h"
#include <stddef.h>

/* Crypto Worker Task */

void handle_lost_msg_crypto(crypto_work_item_t *item)
{
	crypto_status_t status;
	crypto_key_t finder_keypair;

	status = generate_keypair(CRYPTO_CURVE_X25519, &finder_keypair); // Replace with actual private key later
	if (status != CRYPTO_SUCCESS) { return; }
	
	crypto_key_t finder_public_key;
	status = export_public_key(
		&finder_keypair,
		&finder_public_key,
		32
	);
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
		&finder_keypair, 
		&eph_pub_key,
		&secret
	);
	if (status != CRYPTO_SUCCESS) { return; }

	crypto_key_t aes_key;
	status = derive_symmetric_aes_key_hkdf(
		&secret,
		NULL,
		0,
		NULL,
		0,
		&aes_key
	);
	if (status != CRYPTO_SUCCESS) { return; }
	
	// Encrypt location
	uint8_t location_plaintext[] = {0x04, 0x05}; // Replace with a get_location function
	uint8_t location_enc[128];
	uint8_t nonce[12] = {0};
	size_t ciphertext_len;

	status = psa_aead_encrypt(
	  aes_key.id,
	  PSA_ALG_GCM,
	  nonce, sizeof(nonce),
	  NULL, 0,
	  location_plaintext, sizeof(location_plaintext),
	  location_enc, sizeof(location_enc),
	  &ciphertext_len
	);
	if (status != PSA_SUCCESS) {  return; }
	
	// Sign whole payload
	crypto_message_t msg = {
		.message = location_enc,
		.message_size = ciphertext_len
	};

	uint8_t signature[64];
	size_t signature_size;

	status = sign_message(
		&ecdsa_private_key,
		&msg,
		signature,
		64,
		&signature_size
	);
	if (status != CRYPTO_SUCCESS) { return; }

	//	Send Upload Command
	
	request_lost_payload_t lost_payload;
	lost_payload.encryption_location_len = ciphertext_len;
	memcpy(lost_payload.device_id, device_uuid, sizeof(device_uuid));
	memcpy(lost_payload.encrypted_location, location_enc, ciphertext_len);
	memcpy(&lost_payload.finder_key_raw, &finder_public_key.raw.data, finder_public_key.raw.len);
	memcpy(&lost_payload.lost_eph_pub_key_raw, &eph_pub_key.raw.data, eph_pub_key.raw.len);
	memcpy(lost_payload.signature, signature, signature_size);
	
	request_work_item_t request_item = {
		.type = REQUEST_WORKER_EVENT_UPLOAD_LOST_LOCATION
	};
	memcpy(&request_item.lost_payload, &lost_payload, sizeof(request_lost_payload_t));
	
	
	xQueueSend(request_worker_queue, &request_item, 0);

	psa_destroy_key(aes_key.id);
	psa_destroy_key(secret.id);
	psa_destroy_key(finder_keypair.id);
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
	crypto_status_t status;
	
	status = generate_ecdsa_keypair(&ecdsa_private_key);
	if (status != CRYPTO_SUCCESS) { printf("failed to generate keypair!\n"); }
  crypto_work_item_t item;

  while(1) {
	  if(xQueueReceive(crypto_worker_queue, &item, portMAX_DELAY)) {
			vTaskDelay(pdMS_TO_TICKS(10000));
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