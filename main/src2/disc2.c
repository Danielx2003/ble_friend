#include "ble2.h"

#include "ble_worker2.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/util/util.h"
#include "esp_log.h"
#include "esp_central.h"

static const char* tag = "DISC";
static const ble_uuid128_t key_exchange_svr_uuid = BLE_UUID128_INIT(PUB_KEY_SERVICE_UUID);
static const ble_uuid128_t pub_key_chr_uuid = BLE_UUID128_INIT(PUB_KEY_CHAR_UUID);
static const ble_uuid128_t peer_pub_key_chr_uuid = BLE_UUID128_INIT(PEER_PUB_KEY_CHAR_WRITE_UUID);

uint8_t own_addr_type;

int on_read(uint16_t conn_handle,
            const struct ble_gatt_error *error,
            struct ble_gatt_attr *attr,
            void *arg)
{
//  const ble_uuid_any_t *uuid = (const ble_uuid_any_t *)arg;
//	crypto_key_t secret_key;
//	crypto_key_t keypair;
//
//	generate_keypair(CRYPTO_CURVE_X25519, &keypair);
//  if (error->status != 0)
//  {
//    ESP_LOGE(tag, "Read failed status=%d", error->status);
//    return 0;
//  }
//
//  if (ble_uuid_cmp(&uuid->u, &pub_key_chr_uuid.u) == 0)
//  {
//		crypto_key_t peer_pub_key = {
//			.type = KEY_TYPE_RAW,
//			.raw = {
//				.len = attr->om->om_len,
//			}
//		};
//		memcpy(peer_pub_key.raw.data, attr->om->om_data, 32);
//
//		crypto_status_t status = generate_secret(&keypair, &peer_pub_key, &secret_key);
//		if (status != CRYPTO_SUCCESS) {
//			ESP_LOGE(tag, "Failed to generate secret. status=%d",status);
//			return -1;
//		} else {
//			printf("SUccessfully generated secret key!\n");
//		}
//
//		// Write our public key to peer
//    const struct peer_chr *chr;
//    int rc;
//    struct os_mbuf *txom;
//
//		crypto_key_t pub_key;
//
//		status = export_public_key(&keypair, &pub_key);
//		if (status != CRYPTO_SUCCESS) {
//			printf("failed to export public key\n");
//			return -1;
//		}
//
//    const struct peer *peer = peer_find(conn_handle);
//    chr = peer_chr_find_uuid(
//				peer,
//				&key_exchange_svr_uuid.u,
//			  &peer_pub_key_chr_uuid.u);
//
//    if (chr == NULL)
//		{
//      printf("Error: Peer doesn't support the Alert "
//                  "Notification Control Point characteristic\n");
//			return -1;
//    }
//
//    txom = ble_hs_mbuf_from_flat(&pub_key.raw.data, pub_key.raw.len);
//    if (!txom) {
//      printf("Insufficient memory\n");
//			return -1;
//    }
//
//    rc = ble_gattc_write_long(conn_handle, chr->chr.val_handle, 0,
//                              txom, NULL, NULL);
//    if (rc != 0) {
//        printf("Error: Failed to write characteristic; rc=%d\n",
//                    rc);
//    	return -1;
//		}
//
//  }
//  else
//  {
//    ESP_LOGW(tag, "Unknown characteristic read");
//  }

  return 0;
}

void on_disc_complete(const struct peer *peer,
                      int status,
                      void *arg)
{
  if (status != 0)
  {
    ESP_LOGE(tag,
             "Service discovery failed status=%d",
             status);

    ble_gap_terminate(peer->conn_handle,
                      BLE_ERR_REM_USER_CONN_TERM);
    return;
  }

  const struct peer_chr *chr;
  int rc;

  chr = peer_chr_find_uuid(
    peer,
    &key_exchange_svr_uuid.u,
    &pub_key_chr_uuid.u);

  if (chr == NULL)
  {
    ESP_LOGE(tag,
             "Peer does not support characteristic");
    return;
  }

  rc = ble_gattc_read(peer->conn_handle,
                      chr->chr.val_handle,
                      on_read,
                     (void* )(&chr->chr.uuid));

  if (rc != 0)
  {
    ESP_LOGE(tag,
             "Failed to read characteristic rc=%d",
             rc);

    ble_gap_terminate(peer->conn_handle,
                      BLE_ERR_REM_USER_CONN_TERM);
  }
}

int disc_cb(struct ble_gap_event *event, void *arg)
{
  switch (event->type)
  {
    case BLE_GAP_EVENT_DISCONNECT:
      break;

    case BLE_GAP_EVENT_CONNECT:
      if (event->connect.status == 0)
			{
				printf("successfully connected. sending event\n");
				ble_work_item_t item = {
					.type = BLE_WORKER_EVENT_CONNECT,
				};
				item.context.connect.conn_handle = event->connect.conn_handle;

				xQueueSend(ble_worker_queue, &item, 0);	
			}
			else {
				printf("failed to connect. reason=%d\n", event->connect.status);
			}
      break;

    case BLE_GAP_EVENT_EXT_DISC:
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
      break;

    default:
      break;
  }

  return 0;
}

/* Public API */

ble_status_t handle_new_connection(ble_work_connect_t *connect)
{
  int rc;

  uint16_t conn = connect->conn_handle;

  ble_gattc_exchange_mtu(conn, NULL, NULL);
  ble_gap_security_initiate(conn);

  rc = peer_add(conn);
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

  disc_params.filter_duplicates = params->filter_duplicates;
  disc_params.passive = params->passive;
  disc_params.itvl = params->interval;

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
                    duration,
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

