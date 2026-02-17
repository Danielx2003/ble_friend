#include "disc.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "esp_log.h"
#include "esp_central.h"

#include "parser.h"

#include <stdio.h>

static const char* tag = "BLE_DISC";

void ble_handle_lost(void* context, mfg_data_t* mfg);
void ble_handle_pairing(void* contexnt, mfg_data_t* mfg);
void ble_handle_paired(void* context, mfg_data_t* mfg);
int disc_cb(struct ble_gap_event *event, void *arg);

static const ble_uuid128_t gatt_svr_svc_uuid = BLE_UUID128_INIT(0x87, 0xb0, 0x8f, 0x1d, 0xb3, 0x5f, 0xa5, 0xa1, 0x1d, 0x4c, 0xe3, 0x77, 0x76, 0x1b, 0xc4, 0xb0);
static const ble_uuid128_t gatt_svr_chr_uuid = BLE_UUID128_INIT(0x00, 0x1b, 0x15, 0x3a, 0xbc, 0x4c, 0xac, 0x94, 0xaf, 0x49, 0x32, 0xb9, 0x86, 0xd0, 0x60, 0x46);

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
	int rc;
	
	printf("\n\nHanlde Pairing Msg\n\n");
	struct ble_gap_event* event = (struct ble_gap_event*)context;
	
	rc = ble_gap_disc_cancel();
	if (rc != 0) { printf("Failed to cancel\n"); return; }
	printf("Cancelled! rc=%d\n", rc);

	rc = ble_hs_id_infer_auto(0, &own_addr_type);
	if (rc != 0) { return; }

 rc = ble_gap_connect(
		own_addr_type, 
		&(event->ext_disc.addr),
		30000,
		NULL,
		disc_cb, NULL);
		
	printf("Tried to connect to device: Rc=%d\n", rc);
}

void ble_handle_paired(void* context, mfg_data_t* mfg)
{
	struct ble_gap_event* event = (struct ble_gap_event*)context;
	printf("Event Type: %d\n", event->type);
}

int connect_cb(struct ble_gap_event *event, void *arg)
{
	printf("Type: %d\n", event->type);
	return 0;
}

int on_read(uint16_t conn_handle,
                const struct ble_gatt_error *error,
                struct ble_gatt_attr *attr,
                void *arg)
{
	printf("Length: %d\nChar Read: %s\n",attr->om->om_len, (char *)attr->om->om_data);
	return 0;
}

void on_disc_complete(const struct peer* peer, int status, void* arg)
{
  if (status != 0)
  {
    ESP_LOGE(tag, "Service discovery failed. status=%d\n", status);
    ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    return;
  }

  // Read the characteristic
  const struct peer_chr* chr;
  int rc;

  chr = peer_chr_find_uuid(
		peer,
		&gatt_svr_svc_uuid.u,
    &gatt_svr_chr_uuid.u);

  if (chr == NULL) { ESP_LOGE(tag, "Peer doesnt support this char\n");  return; }
	printf("Char supported!\n");

    rc = ble_gattc_read(peer->conn_handle, chr->chr.val_handle,
                        on_read, NULL);
    if (rc != 0) {
        printf("Error: Failed to read characteristic; rc=%d\n",
                    rc);
        goto err;
    }

    return;
err:
    /* Terminate the connection. */
    ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}

int disc_cb(struct ble_gap_event *event, void *arg)
{
	parser_status_t status;
	mfg_data_t mfg;
	memset(&mfg, 0, sizeof(mfg));
	parser_result_t result;
	int rc;
	struct ble_gap_conn_desc desc;
	
	result.mfg = &mfg;
	
	switch(event->type)
	{
		case BLE_GAP_EVENT_DISC_COMPLETE:
			printf("Discovery Complete?\n");			
			break;
		case BLE_GAP_EVENT_DISCONNECT:
			printf("Disconnected\n");
			break;
		case BLE_GAP_EVENT_CONNECT:
			if (event->connect.status == 0)
			{
				printf("Connection Handle: %d Status: %d\n", event->connect.conn_handle, event->connect.status);

				ble_gattc_exchange_mtu(event->connect.conn_handle, NULL, NULL);
				ble_gap_security_initiate(event->connect.conn_handle);

				rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
				assert(rc == 0);

				struct peer *peer;

				rc = peer_add(event->connect.conn_handle);
				if (rc != 0) {
				    printf("Failed to add peer; rc=%d\n", rc);
				    return 0;
				}
				rc = peer_disc_all(event->connect.conn_handle,
				             on_disc_complete, NULL);
				if (rc != 0) { printf("Failed to discover services: rc=%d\n", rc); return 0; }
			}
			else {
				printf("Error: Connection failed; status=%d\n",
				            event->connect.status);
			}
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
