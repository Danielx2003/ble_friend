#include "disc.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/util/util.h"
#include "esp_log.h"
#include "parser.h"

#include <stdio.h>

static const char* tag = "BLE_DISC";

void ble_handle_lost(void* context, mfg_data_t* mfg);
void ble_handle_pairing(void* contexnt, mfg_data_t* mfg);
void ble_handle_paired(void* context, mfg_data_t* mfg);

static parser_action_table_t ble_actions = {
    .on_pairing = ble_handle_pairing,
    .on_paired  = ble_handle_paired,
    .on_lost    = ble_handle_lost
};

static uint8_t own_addr_type;

void ble_handle_lost(void* context, mfg_data_t* mfg)
{
  printf("\nHandle Lost Msg\n");
	struct ble_gap_event* event = (struct ble_gap_event*)context;
	printf("Event Type: %d\n", event->type);
}

void ble_handle_pairing(void* context, mfg_data_t* mfg)
{
	printf("\n\nHanlde Pairing Msg\n\n");
	struct ble_gap_event* event = (struct ble_gap_event*)context;
	
//	struct ble_gap_conn_params params;
//	memset(&params, 0, sizeof(params));
//	
	printf("Expected: %d. Actual: %d\n", event->ext_disc.length_data, mfg->payload_len);
//	
//	for (int i=0; i<event->ext_disc.length_data; i++)
//	{
//		printf("%02x:", event->ext_disc.data[i]);
//	}
//	
//	printf("----\n\n");
//	
	for (int i=0; i<mfg->payload_len; i++)
	{
		printf("%02x:", mfg->payload[i]);
	}
	
	printf("----\n\n");


//	int rc= ble_gap_connect(
//		own_addr_type, 
//		&(event->ext_disc.addr),
//		0,
//		&params,
//		NULL, NULL);
//	
//	printf("Tried to connect to device: Rc=%d", rc);
}

void ble_handle_paired(void* context, mfg_data_t* mfg)
{
	struct ble_gap_event* event = (struct ble_gap_event*)context;
	printf("Event Type: %d\n", event->type);
}

int disc_cb(struct ble_gap_event *event, void *arg)
{
	parser_status_t status;
	mfg_data_t mfg;
	memset(&mfg, 0, sizeof(mfg));
	parser_result_t result;
	
	result.mfg = &mfg;
	
//	memset(&result, 0, sizeof(result));
//	memset(result.mfg, 0, sizeof(*result.mfg));
//	result.mfg->payload_len = 0;
	
	switch(event->type)
	{
		case BLE_GAP_EVENT_DISC_COMPLETE:
			printf("Connection Complete?\n");
			break;

		default:
			break;
	}
	
	status = parse_adv_data(event->ext_disc.data, event->ext_disc.length_data, &result);
	if (status != PARSER_SUCCESS) { return status; }
	
	result.action(event, result.mfg);
	
	return 0;
}

ble_status_t disc_start(
  ble_disc_params_t* params,
  uint32_t duration
)
{
  int rc;
	
	parser_init(&ble_actions);
  
  rc = ble_hs_util_ensure_addr(0);
  if (rc != 0) { ESP_LOGE(tag, "No valid Address available. rc=%d\n", rc); return rc; }

  struct ble_gap_disc_params disc_params;
  memset(&disc_params, 0, sizeof(disc_params));

  disc_params.filter_duplicates = params->filter_duplicates;
  disc_params.passive = params->passive;
  disc_params.itvl = params->interval;
 
  rc = ble_hs_id_infer_auto(0, &own_addr_type);
  if (rc != 0) { ESP_LOGE(tag, "Could not infer address type\n"); return rc; }
  
  duration = duration == 0 ? BLE_HS_FOREVER : duration;

  rc = ble_gap_disc(own_addr_type, duration, &disc_params, disc_cb, NULL);
  if (rc != 0) { ESP_LOGE(tag, "Failed to start discovery rc=%d\n", rc); return rc; }

  ESP_LOGI(tag, "Started Discovery\n");
  
  return BLE_SUCCESS;
}
