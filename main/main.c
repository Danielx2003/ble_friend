#include "ble2.h"
#include "crypto2.h"
#include "device2.h"
#include "crypto2.h"
#include "hal/gpio_types.h"
#include "parser2.h"
#include "request2.h"

#include <sys/time.h>
#include "driver/gpio.h"

#define LOG_LOCAL_LEVEL ESP_LOG_INFO

RTC_SLOW_ATTR struct timeval disc_start_time = {0};

static parser_action_table_t ble_actions = {
  .on_pairing = handle_pairing_msg,
  .on_paired = handle_paired_msg,
  .on_lost = handle_lost_msg
};

//static request_ecdsa_response_t response;
//static crypto_key_t ecdsa_private_key;
//static crypto_key_t key_out;
//
//void register_device()
//{
//	crypto_status_t status = generate_ecdsa_keypair(&ecdsa_private_key);
//	if (status != CRYPTO_SUCCESS) { return; }
//
//	crypto_key_t public_key;
//	status = export_ecdsa_public_key(
//		&ecdsa_private_key,
//		&public_key
//	);
//	if (status != CRYPTO_SUCCESS) { return; }
//
//	status = import_ecdsa_key(&public_key, &key_out);
//
//	if (status != CRYPTO_SUCCESS) { return; }
//
//	request_ecdsa_payload_t payload = {
//		.ecdsa_public_key = &key_out
//	};
//
//	request_status_t req_status = send_ecdsa_public_key(&payload, &response);
//	if (req_status != REQUEST_SUCCESS) { return; }
//}
//
//void test_upload_location_report()
//{
//	/* Temp logic to generate finders keypair */
//	crypto_key_t finder_priv_key;
//	crypto_status_t status = generate_keypair(CRYPTO_CURVE_X25519, &finder_priv_key);
//	if (status != CRYPTO_SUCCESS)
//	{
//		printf("failed to generate keypair\n");
//		return;
//	}
//
//	crypto_key_t finder_public_key;
//	status = export_public_key(&finder_priv_key, &finder_public_key, 32);
//	if (status != CRYPTO_SUCCESS)
//	{
//		printf("failed to export publoc key\n");
//		return;
//	}
//
//	request_lost_payload_t payload = {
//		.finder_key = &finder_public_key
//	};
//	
//	uint8_t message[] = { 0x45, 0x46 };
//	
//	crypto_message_t msg = {
//		.message = message,
//		.message_size = sizeof(message)
//	};
//
//	uint8_t signature[64];
//	size_t signature_size;
//	
//	status = sign_message(
//		&ecdsa_private_key,
//		&msg,
//		signature,
//		64,
//		&signature_size
//	);
//	memcpy(payload.device_id, response.uuid, sizeof(response.uuid));
//	memcpy(payload.location, msg.message, msg.message_size);
//	memcpy(payload.signature, signature, signature_size);
//
//	upload_lost_details(&payload);
//}

void app_main()
{
	if (!device_init())
	{
		return;
	}

	request_init();

	parser_init(&ble_actions);
	crypto_init();
	
//	register_device();
//
//	ble_init();
//	ble_start();
//
//	gettimeofday(&disc_start_time, NULL);
}
