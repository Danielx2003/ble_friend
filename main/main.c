#include "ble2.h"
#include "crypto2.h"
#include "device2.h"
#include "parser2.h"
#include "request2.h"

static parser_action_table_t ble_actions = {
  .on_pairing = handle_pairing_msg,
  .on_paired = handle_paired_msg,
  .on_lost = handle_lost_msg
};

RTC_SLOW_ATTR struct timeval disc_start_time = {0};

#include "esp_wifi.h"
#include <sys/time.h>

void test_scan_nearby_aps()
{	
//	wifi_scan_config_t scan_config = {};  // Configuration for scanning
//	scan_config.ssid = NULL;  // Scan all networks (no specific SSID)
//	scan_config.bssid = NULL;  // Scan all BSSIDs
//	scan_config.channel = 0;  // Scan all channels
//	scan_config.show_hidden = true;  // Show hidden networksf

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
  struct timeval scan_enter_time;
  gettimeofday(&scan_enter_time, NULL);
	
	// Perform the Wi-Fi scan
	esp_wifi_scan_start(&scan_config, true);  // true = blocking scan
	
	struct timeval now;
	gettimeofday(&now, NULL);
	int duration = (now.tv_sec - scan_enter_time.tv_sec) * 1000 + (now.tv_usec - scan_enter_time.tv_usec) / 1000;
	printf("scan took: %d\n", duration);

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
	        if (j < 5) printf(":");  // Add colon between bytes
	    }

	    printf(" | Signal Strength (RSSI): ");
	    printf("%d\n", networks[i].rssi);
	}
}


void app_main()
{
	if (!device_init())
	{
		return;
	}
	
	request_init();
	get_device_location_from_bssid(NULL);
//	test_scan_nearby_aps();

	parser_init(&ble_actions);
	crypto_init();

	ble_init();
	ble_start();
	
	gettimeofday(&disc_start_time, NULL);
}
