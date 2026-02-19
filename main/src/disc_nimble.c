#include "disc.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "esp_log.h"
#include "esp_central.h"
#include "parser.h"
#include "crypto.h"
#include "psa/crypto_values.h"

#include <stdio.h>

static const char *tag = "BLE_DISC";

void ble_handle_lost(void *context, mfg_data_t *mfg);
void ble_handle_pairing(void *context, mfg_data_t *mfg);
void ble_handle_paired(void *context, mfg_data_t *mfg);
int disc_cb(struct ble_gap_event *event, void *arg);

static const ble_uuid128_t gatt_svr_svc_uuid =
  BLE_UUID128_INIT(PUB_KEY_SERVICE_UUID);

static const ble_uuid128_t gatt_svr_chr_uuid =
  BLE_UUID128_INIT(PUB_KEY_CHAR_UUID);

static parser_action_table_t ble_actions = {
  .on_pairing = ble_handle_pairing,
  .on_paired = ble_handle_paired,
  .on_lost = ble_handle_lost
};

static ble_event_cbs_t ble_event_cbs;
static uint8_t own_addr_type;

void ble_dispatch_event(ble_event_t ble_event,
                        struct ble_gap_event *event)
{
  switch (ble_event)
  {
		case BLE_DISCONNECT_EVENT:
      if (ble_event_cbs.on_disconnect)
			{
        ble_event_cbs.on_disconnect(event);
      }
			break;

    case BLE_CONNECT_EVENT:
      if (ble_event_cbs.on_connect)
			{
        ble_event_cbs.on_connect(event);
      }
			break;

		case BLE_EXT_DISC_EVENT:
			if (ble_event_cbs.on_ext_disc)
			{
			  ble_event_cbs.on_ext_disc(event);
			}
			break;

    default:
      break;
  }
}

void ble_handle_lost(void *context, mfg_data_t *mfg)
{
  struct ble_gap_event *event =
      (struct ble_gap_event *)context;
}

void ble_handle_pairing(void *context, mfg_data_t *mfg)
{
  int rc;
  struct ble_gap_event *event =
    (struct ble_gap_event *)context;

  rc = ble_gap_disc_cancel();
  if (rc != 0)
  {
    ESP_LOGE(tag, "Failed to cancel discovery rc=%d", rc);
    return;
  }

  rc = ble_hs_id_infer_auto(0, &own_addr_type);
  if (rc != 0)
  {
    ESP_LOGE(tag, "Failed to infer address type rc=%d", rc);
    return;
  }

  rc = ble_gap_connect(
    own_addr_type,
    &(event->ext_disc.addr),
    30000,
    NULL,
    disc_cb,
    NULL);

  ESP_LOGI(tag, "Attempted connection rc=%d", rc);
}

void ble_handle_paired(void *context, mfg_data_t *mfg)
{
  struct ble_gap_event *event =
    (struct ble_gap_event *)context;
}

int connect_cb(struct ble_gap_event *event, void *arg)
{
  ESP_LOGI(tag, "Connect callback event type=%d",
           event->type);
  return 0;
}

int on_read(uint16_t conn_handle,
            const struct ble_gatt_error *error,
            struct ble_gatt_attr *attr,
            void *arg)
{
  const ble_uuid_any_t *uuid = (const ble_uuid_any_t *)arg;
	psa_key_id_t secret_key;

  if (error->status != 0)
  {
    ESP_LOGE(tag, "Read failed status=%d", error->status);
    return 0;
  }

  if (ble_uuid_cmp(&uuid->u, &gatt_svr_chr_uuid.u) == 0)
  {
		psa_status_t status = generate_shared_secret_raw_bytes(
			NULL,
			attr->om->om_data,
			attr->om->om_len,
			256,
			&secret_key
		);

		if (status != PSA_SUCCESS) {
			ESP_LOGE(tag, "Failed to generate secret. status=%d",status);
		} else {
			printf("SUccessfully generated secret key!\n");
		}
  }
  else
  {
    ESP_LOGW(tag, "Unknown characteristic read");
  }

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
    &gatt_svr_svc_uuid.u,
    &gatt_svr_chr_uuid.u);

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

ble_status_t handle_new_connection(void *ctx)
{
  int rc;
  struct ble_gap_event *event =
    (struct ble_gap_event *)ctx;

  uint16_t conn = event->connect.conn_handle;

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

ble_status_t handle_ext_disc(void *ctx)
{
  struct ble_gap_event *event =
    (struct ble_gap_event *)ctx;

  parser_status_t status;
  mfg_data_t mfg;
  memset(&mfg, 0, sizeof(mfg));

  parser_result_t result;
  result.mfg = &mfg;

  status = parse_adv_data(event->ext_disc.data,
                          event->ext_disc.length_data,
                          &result);

  if (status != PARSER_SUCCESS)
    return status;

  result.action(event, result.mfg);

  return BLE_SUCCESS;
}

int disc_cb(struct ble_gap_event *event, void *arg)
{
  switch (event->type)
  {
    case BLE_GAP_EVENT_DISCONNECT:
      ble_dispatch_event(BLE_DISCONNECT_EVENT, event);
      break;

    case BLE_GAP_EVENT_CONNECT:
      if (event->connect.status == 0)
        ble_dispatch_event(BLE_CONNECT_EVENT, event);
      break;

    case BLE_GAP_EVENT_EXT_DISC:
      ble_dispatch_event(BLE_EXT_DISC_EVENT, event);
      break;

    default:
      break;
  }

  return 0;
}

ble_status_t disc_set_event_handlers(ble_event_cbs_t *cbs)
{
  ble_event_cbs.on_connect = cbs->on_connect;
  ble_event_cbs.on_disconnect = cbs->on_disconnect;
	ble_event_cbs.on_ext_disc = cbs->on_ext_disc;

  return BLE_SUCCESS;
}

ble_status_t disc_start(ble_disc_params_t *params,
                        uint32_t duration)
{
  int rc;

  parser_init(&ble_actions);

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
