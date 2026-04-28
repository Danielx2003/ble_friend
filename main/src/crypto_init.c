#include "crypto.h"
#include "crypto_worker.h"

#include "request_worker.h"
#include "request.h"

#include "esp_log.h"

static request_ecdsa_response_t response;

void register_device()
{
	crypto_status_t status = generate_ecdsa_keypair(&ecdsa_private_key);
	if (status != CRYPTO_SUCCESS) { ESP_LOGE("main", "failed to generate ecdsa keypair"); return; }

	crypto_key_t key_out;
	
	status = export_ecdsa_public_key(
		&ecdsa_private_key,
		&key_out
	);
	if (status != CRYPTO_SUCCESS) { ESP_LOGE("main", "failed to export ecdsa keypair"); return; }

	status = import_ecdsa_key(&key_out, &ecdsa_public_key);
	if (status != CRYPTO_SUCCESS) { ESP_LOGE("main", "failed to import ecdsa keypair"); return; }
	
	request_ecdsa_payload_t payload = {
		.ecdsa_public_key = &ecdsa_public_key
	};

	request_status_t req_status = send_ecdsa_public_key(&payload, &response);
	if (req_status != REQUEST_SUCCESS) { return; }
	
	uint8_t arr[2] = {0x04, 0x05};
	crypto_message_t msg = {
	   .message = arr,
	   .message_size = 2
	 };

	 uint8_t signature[64];
	 size_t signature_size;

	 status = sign_message(&msg, signature, 64, &signature_size);
	 if (status != CRYPTO_SUCCESS) { printf("sign message failed\n"); return; }
	
	 printf("---- \n  successfully signed message \n ----\n");
	 
	 ESP_LOGI("CRYPTO_INIT", "private key id: %lu", (uint32_t)ecdsa_private_key.id);
	 ESP_LOGI("CRYPTO_INIT", "public key id:  %lu", (uint32_t)ecdsa_public_key.id);
	 ESP_LOGI("CRYPTO_INIT", "public key type: %d", ecdsa_public_key.type);
}


/* Public API */

crypto_status_t crypto_init(void)
{
  psa_status_t status = psa_crypto_init();

  generate_keypair(CRYPTO_CURVE_X25519, &device_private_key);
  crypto_status_t s = export_public_key(&device_private_key, &device_public_key, 32);
	
	register_device();

  if (s != CRYPTO_SUCCESS) {
    ESP_LOGE("CRYPTO_INIT", "failed to setup crypto\n");
    return CRYPTO_ERR_UNKNOWN;
  }

  crypto_worker_queue = xQueueCreate(128, sizeof(crypto_work_item_t));
  if (!crypto_worker_queue) { return CRYPTO_ERR_UNKNOWN; }

  xTaskCreatePinnedToCore(
    crypto_worker_task,
    "crypto_worker",
    8192,
    NULL,
    15,
    NULL,
    1);

  return PSA_SUCCESS;
}
