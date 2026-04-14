#include "ble2.h"
#include "crypto2.h"
#include "device2.h"
#include "crypto2.h"
#include "parser2.h"
#include "psa/crypto.h"
#include "request2.h"

#include "esp_wifi.h"
#include <sys/time.h>

static parser_action_table_t ble_actions = {
  .on_pairing = handle_pairing_msg,
  .on_paired = handle_paired_msg,
  .on_lost = handle_lost_msg
};

struct timeval disc_start_time = {0};
bool paired = false;

static request_ecdsa_response_t response;

void test_scan_nearby_aps()
{	
	wifi_scan_config_t scan_config = {
	    .ssid = NULL,
	    .bssid = NULL,
	    .channel = 0, // or rotate channels for faster scans
//	    .show_hidden = false,
	    .scan_type = WIFI_SCAN_TYPE_ACTIVE,
//	    .scan_time.active = {
//	        .min = 30,
//	        .max = 60
//	    }
	};

	// Perform the Wi-Fi scan
	esp_wifi_scan_start(&scan_config, true);  // true = blocking scan

	// Get the results of the scan
	uint16_t num_networks = 0;
	esp_wifi_scan_get_ap_num(&num_networks);  // Get the number of networks found
	
	wifi_ap_record_t* networks = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * num_networks);
	esp_wifi_scan_get_ap_records(&num_networks, networks);

	printf("number found: %u\n", num_networks);
	
	for (int i = 0; i < num_networks; i++) {
	    printf("%d: ", i + 1);
	    printf("%s | BSSID: ", (char*)networks[i].ssid);

	    for (int j = 0; j < 6; j++) {
	        printf("%02X", networks[i].bssid[j]);
	        if (j < 5) printf(":");
	    }

	    printf(" | Signal Strength (RSSI): ");
	    printf("%d\n", networks[i].rssi);
	}
}


void register_device()
{
	printf("register device\n");
	crypto_status_t status = generate_ecdsa_keypair(&ecdsa_private_key);
	if (status != CRYPTO_SUCCESS) { return; }

	crypto_key_t key_out;
	
	status = export_ecdsa_public_key(
		&ecdsa_private_key,
		&key_out
	);
	if (status != CRYPTO_SUCCESS) { return; }

	status = import_ecdsa_key(&key_out, &ecdsa_public_key);

	if (status != CRYPTO_SUCCESS) { return; }

	request_ecdsa_payload_t payload = {
		.ecdsa_public_key = &ecdsa_public_key
	};

	request_status_t req_status = send_ecdsa_public_key(&payload, &response);
	if (req_status != REQUEST_SUCCESS) { return; }
}

void app_main()
{
	if (!device_init())
	{
		return;
	}
	
	request_init();
//	get_device_location_from_bssid(NULL);
//	test_scan_nearby_aps();

	parser_init(&ble_actions);
	crypto_init();
	register_device();
	
	ble_init();
	ble_start();

//	gettimeofday(&disc_start_time, NULL);
}
