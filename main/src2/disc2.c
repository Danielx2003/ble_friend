#include "ble2.h"

#include "ble_worker2.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/util/util.h"
#include "esp_log.h"
#include "esp_central.h"

/* Static + Global Variables */

static const char* tag = "DISC";
uint8_t own_addr_type;

/* GAP Event Callbacks */

int on_read(uint16_t conn_handle,
                   const struct ble_gatt_error *error,
                   struct ble_gatt_attr *attr,
                   void *arg)
{
  ble_work_item_t item = {
    .type = BLE_WORKER_EVENT_READ_COMPLETE,
    .context.read_complete = {
      .conn_handle = conn_handle,
      .status = error->status,
      .data_len = 0,
    }
  };

  if (error->status == 0 && attr && attr->om) {
    uint16_t len = OS_MBUF_PKTLEN(attr->om);

    if (len > sizeof(item.context.read_complete.data)) {
      ESP_LOGE(tag, "Read data too large: %d bytes", len);
      return BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    os_mbuf_copydata(
        attr->om,
        0,
        len,
        item.context.read_complete.data);

    item.context.read_complete.data_len = len;
  }

  xQueueSend(ble_worker_queue, &item, 0);
  return 0;
}


void on_disc_complete(const struct peer *peer,
                             int status,
                             void *arg)
{
	ble_work_item_t item = {
    .type = BLE_WORKER_EVENT_DISC_COMPLETE,
    .context.disc_complete = {
      .conn_handle = peer->conn_handle,
      .status      = status,
    }
  };

  xQueueSend(ble_worker_queue, &item, 0);
}


int disc_cb(struct ble_gap_event *event, void *arg)
{	
  switch (event->type)
  {
		case BLE_GAP_EVENT_ENC_CHANGE:
			if (event->enc_change.status == 0)
			{
				ble_work_item_t item = {
					.type = BLE_WORKER_EVENT_ENC_CHANGE,
				};
				item.context.connect.conn_handle = event->connect.conn_handle;

				xQueueSend(ble_worker_queue, &item, 0);	
			}
			break;

    case BLE_GAP_EVENT_CONNECT:
      if (event->connect.status == 0)
			{
				uint16_t conn = event->connect.conn_handle;

				int rc = ble_gap_security_initiate(conn);

				if (rc != 0) {
				    printf("security initiate failed rc=%d\n", rc);
				}

				ble_work_item_t item = {
				    .type = BLE_WORKER_EVENT_CONNECT
				};
				item.context.connect.conn_handle = conn;

				xQueueSend(ble_worker_queue, &item, 0);
			} else {
				printf("failed to connect. reason=%d\n", event->connect.status);
			}
      break;

    case BLE_GAP_EVENT_EXT_DISC:
			{
				ble_work_item_t item = {
					.type = BLE_WORKER_EVENT_EXT_DISC,
				};

				item.context.msg.len = event->ext_disc.length_data;
				memcpy(item.context.msg.data,
				       event->ext_disc.data,
				       event->ext_disc.length_data);
				memcpy(
					&(item.context.msg.pairing.addr),
					&(event->ext_disc.addr),
					sizeof(event->ext_disc.addr)
				);

				xQueueSend(ble_worker_queue, &item, 0);
			}
      break;
    
		case BLE_GAP_EVENT_DISCONNECT:
			{
				ble_work_item_t item = {
					.type = BLE_WORKER_EVENT_DISCONNECT,
				};

				xQueueSend(ble_worker_queue, &item, 0);	
			}
			break;

		case BLE_GAP_EVENT_DISC_COMPLETE:
			printf("discovery complete\n");
			printf("Total Payloads: %d\n", payloads_received);
			break;
    default:
      break;
  }

  return 0;
}

/* Public API */

ble_status_t discover_all_services(ble_work_connect_t *connect)
{
	uint16_t conn = connect->conn_handle;
	
	int rc = peer_add(conn);
	 if (rc != 0)
	 {
	   ESP_LOGE(tag,
	            "Failed to add peer rc=%d",
	            rc);
	   return 0;
	 }

	 rc = peer_disc_all(conn,
	                    on_disc_complete,
	                    NULL);
	 if (rc != 0)
	 {
	   ESP_LOGE(tag,
	            "Failed to discover services rc=%d",
	            rc);
	   return 0;
	 }
	 
	 return BLE_SUCCESS;
}


ble_status_t handle_new_connection(ble_work_connect_t *connect)
{
  uint16_t conn = connect->conn_handle;

  if (ble_gattc_exchange_mtu(conn, NULL, NULL) != 0)
	{
		ESP_LOGE(tag, "failed to exchange MTU");
	}
  
	if (ble_gap_security_initiate(conn) != 0)
	{
		ESP_LOGE(tag, "failed to upgrade connection");
	}

  return BLE_SUCCESS;
}


ble_status_t start_connect(ble_work_msg_t *msg)
{
	int rc;
	rc = ble_hs_id_infer_auto(0, &own_addr_type);
	if (rc != 0)
	{
	  ESP_LOGE(tag, "Failed to infer address type rc=%d", rc);
	  return -1;
	}

	rc = ble_gap_connect(
	  own_addr_type,
	  &(msg->pairing.addr),
	  30000,
	  NULL,
	  disc_cb,
	  NULL);

	ESP_LOGI(tag, "Attempted connection rc=%d", rc);

	return BLE_SUCCESS;
}

ble_status_t disc_start(ble_disc_params_t *params,
                        uint32_t duration)
{
  int rc;

  rc = ble_hs_util_ensure_addr(0);
  if (rc != 0)
  {
    ESP_LOGE(tag,
             "No valid address available rc=%d",
             rc);
    return rc;
  }

  struct ble_gap_disc_params disc_params;
  memset(&disc_params, 0, sizeof(disc_params));

//  disc_params.filter_duplicates = params->filter_duplicates;
	disc_params.filter_duplicates = 0;
  disc_params.passive = params->passive;
  disc_params.itvl = BLE_GAP_SCAN_ITVL_MS(100);
	disc_params.window = BLE_GAP_SCAN_WIN_MS(10);
	
	// BLE_GAP_SCAN_SLOW_INTERVAL1 -> Terrible for discovery

  rc = ble_hs_id_infer_auto(0, &own_addr_type);
  if (rc != 0)
  {
    ESP_LOGE(tag,
             "Could not infer address type rc=%d",
             rc);
    return rc;
  }

  duration = (duration == 0) ? BLE_HS_FOREVER : duration;

  rc = ble_gap_disc(own_addr_type,
                    60000, // 2 mins = 120000, 1 min = 60000
                    &disc_params,
                    disc_cb,
                    NULL);

  if (rc != 0)
  {
    ESP_LOGE(tag,
             "Failed to start discovery rc=%d",
             rc);
    return rc;
  }

  ESP_LOGI(tag, "Discovery started");

  return BLE_SUCCESS;
}

ble_status_t disc_stop()
{
	int rc = ble_gap_disc_cancel();
	if (rc != 0)
	{
	  ESP_LOGE(tag, "Failed to cancel discovery rc=%d", rc);
	  return -1;
	}
	
	printf("stopped discovery\n");

	return BLE_SUCCESS;
}
