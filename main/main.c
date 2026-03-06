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

void app_main()
{
	if (!device_init())
	{
		return;
	}
	
	request_init();

	parser_init(&ble_actions);
	crypto_init();
	
	ble_init();
	ble_start();
}
