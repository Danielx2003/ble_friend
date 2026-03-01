//#include "esp_log.h"
//
//#include "disc.h"
//#include "ble.h"
//#include "device.h"
//#include "parser.h"
//#include "crypto.h"
//
//#include <stdbool.h>
//#include <stdio.h>
//
///* Static Variables */
//
//static const char *tag = "BLE_SCAN_APP";
//crypto_key_t keypair;
//
//static parser_action_table_t ble_actions = {
//  .on_pairing = on_pairing_msg,
//  .on_paired = on_paired_msg,
//  .on_lost = on_lost_msg
//};
//
///* Function Definitions */
//
//void ble_store_config_init(void);
//
///* Callbacks */
//
//void on_connect(void *ctx)
//{
//  ESP_LOGI(tag, "Connected");
//
//  handle_new_connection(ctx);
//}
//
//void on_disconnect(void *ctx)
//{
//	ble_disc_params_t params = {
//	  .filter_duplicates = 1,
//	  .passive = 1,
//	};
//
//	ble_status_t status = disc_start(&params, 0);
//	printf("status: %d\n", status);
//}
//
//void on_ext_disc(void* ctx)
//{
//	ble_status_t status = handle_ext_disc(ctx);
//}
//
//void on_reset(ble_event_reset_t *reset)
//{
//  ESP_LOGE(tag, "Host reset. Reason=%d", reset->reason);
//}
//
//void on_sync(void)
//{
//  ble_disc_params_t params = {
//    .filter_duplicates = 1,
//    .passive = 1,
//  };
//
//  ble_event_cbs_t event_cbs = {
//    .on_connect = on_connect,
//    .on_disconnect = on_disconnect,
//		.on_ext_disc = on_ext_disc,
//  };
//
//  disc_set_event_handlers(&event_cbs);
//
//  ble_status_t status = disc_start(&params, 0);
//  if (status != BLE_SUCCESS)
//  {
//    ESP_LOGE(tag,
//             "Failed to start discovery. Status=%d",
//             status);
//  }
//}
//
///* Main */
//
//void app_main(void)
//{
//  if (!device_init())
//	{
//		ESP_LOGE(tag, "Failed to initialise device.");
//		return;
//	}
//
//  ble_callbacks_t callbacks = {
//    .on_ready = on_sync,
//    .on_reset = on_reset
//  };
//
//	parser_init(&ble_actions);
//	crypto_init();
//
//  ble_init(&callbacks);
//  ble_start();
//}

#include "ble2.h"
#include "crypto2.h"
#include "device2.h"
#include "parser2.h"

static parser_action_table_t ble_actions = {
  .on_pairing = handle_pairing_msg,
  .on_paired = handle_paired_msg,
  .on_lost = handle_lost_msg
};

void app_main()
{
	device_init();

	parser_init(&ble_actions);
	crypto_init();
	
	ble_init();
	ble_start();
}
