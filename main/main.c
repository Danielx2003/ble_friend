#include "ble.h"
#include "crypto.h"
#include "device.h"
#include "crypto.h"
#include "parser.h"
#include "request.h"

#include "freertos/idf_additions.h"
#include "psa/crypto.h"

#include "esp_wifi.h"
#include <sys/time.h>

/* Static Variables */

static parser_action_table_t ble_actions = {
  .on_pairing = handle_pairing_msg,
  .on_paired = handle_paired_msg,
  .on_lost = handle_lost_msg
};

bool paired = false;

void app_main()
{
	if (!device_init())
	{
		return;
	}
	
	request_init();

	parser_init(&ble_actions);
	crypto_init();
	
	ble_start();
}
