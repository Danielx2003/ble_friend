#include "ble_worker2.h"
#include "crypto2.h"
#include "crypto_worker2.h"
#include "freertos/idf_additions.h"
#include "psa/crypto_values.h"
#include "request2.h"

#include "esp_log.h"
#include "request_worker2.h"
#include <stddef.h>

/* Crypto Worker Task */

static int pass = 0;
static const char *tag = "CRYPTO";

void decrypt_loc_report(crypto_work_decrypt_loc_t *item)
{
  crypto_key_t finder_pub = {
    .type = KEY_TYPE_RAW
  };
  memcpy(finder_pub.raw.data, item->finder_key_raw, 32);
  finder_pub.raw.len = 32;

  crypto_key_t eph_priv;
  const uint8_t info[] = "eph_private";

  crypto_status_t status = derive_ephemeral_private_key(
    &master_secret,
    info, sizeof(info),
    &eph_priv);

  crypto_key_t owner_shared_secret;

  status = generate_secret(
    &eph_priv,
    &finder_pub,
    &owner_shared_secret);

  crypto_key_t owner_aes_key;
  derive_symmetric_aes_key_hkdf(
    &owner_shared_secret,
    NULL, 0,
    NULL, 0,
    &owner_aes_key);

  uint8_t decrypted[128];
  uint8_t nonce[12] = {0};
  size_t decrypted_len;

  status = psa_aead_decrypt(
    owner_aes_key.id,
    PSA_ALG_GCM,
    nonce, sizeof(nonce),
    NULL, 0,
    item->enc_loc, 24,
    decrypted, sizeof(decrypted),
    &decrypted_len);

  if (status != CRYPTO_SUCCESS) {
    ESP_LOGE(tag, "failed to decrypt location report\n");
    return;
  }

  int32_t *location = (int32_t *)decrypted;
  double lat = location[0] / 1e6;
  double lng = location[1] / 1e6;
//  printf("Result: Lat: %.6f, Lng: %.6f\n", lat, lng);
	printf("{\"lat\": %.6f, \"lon\": %.6f, \"timestamp\": 1776767745, \"device_id\": \"%s\"}\n",
	       lat, lng, device_uuid);
}

void handle_lost_msg_crypto(crypto_work_item_t *item)
{
  crypto_status_t status;
  crypto_key_t finder_keypair;

  status = generate_keypair(CRYPTO_CURVE_X25519, &finder_keypair);
  if (status != CRYPTO_SUCCESS) { return; }

  crypto_key_t finder_public_key;
  status = export_public_key(&finder_keypair, &finder_public_key, 32);
  if (status != CRYPTO_SUCCESS) { return; }

  crypto_key_t eph_pub_key = {
    .type = KEY_TYPE_RAW,
    .raw = {
      .len = item->context.lost_msg.mfg.payload_len
    }
  };
  memcpy(
    eph_pub_key.raw.data,
    item->context.lost_msg.mfg.payload,
    item->context.lost_msg.mfg.payload_len);

  crypto_key_t secret;

  status = generate_secret(&finder_keypair, &eph_pub_key, &secret);
  if (status != CRYPTO_SUCCESS) { return; }

  crypto_key_t aes_key;
  status = derive_symmetric_aes_key_hkdf(&secret, NULL, 0, NULL, 0, &aes_key);
  if (status != CRYPTO_SUCCESS) { return; }

  uint8_t location_enc[PSA_AEAD_ENCRYPT_OUTPUT_SIZE(
    PSA_KEY_TYPE_AES,
    PSA_ALG_GCM,
    sizeof(item->context.lost_msg.location))] = {0};
  uint8_t nonce[12] = {0};
  size_t ciphertext_len;

  status = psa_aead_encrypt(
    aes_key.id,
    PSA_ALG_GCM,
    nonce, sizeof(nonce),
    NULL, 0,
    (const uint8_t *)item->context.lost_msg.location,
    sizeof(item->context.lost_msg.location),
    location_enc, sizeof(location_enc),
    &ciphertext_len);

  if (status != PSA_SUCCESS) { return; }

  crypto_message_t msg = {
    .message = location_enc,
    .message_size = ciphertext_len
  };

  uint8_t signature[64];
  size_t signature_size;

  status = sign_message(&msg, signature, 64, &signature_size);
  if (status != CRYPTO_SUCCESS) { return; }

  request_lost_payload_t lost_payload;
  lost_payload.encryption_location_len = ciphertext_len;
  memcpy(lost_payload.device_id, device_uuid, sizeof(device_uuid));
  memcpy(lost_payload.encrypted_location, location_enc, ciphertext_len);
  memcpy(&lost_payload.finder_key_raw, &finder_public_key.raw.data, finder_public_key.raw.len);
  memcpy(&lost_payload.lost_eph_pub_key_raw, &eph_pub_key.raw.data, eph_pub_key.raw.len);
  memcpy(lost_payload.signature, signature, signature_size);

  request_work_item_t request_item = {
    .type = REQUEST_WORKER_EVENT_UPLOAD_LOST_LOCATION
  };
  memcpy(&request_item.lost_payload, &lost_payload, sizeof(request_lost_payload_t));

  xQueueSend(request_worker_queue, &request_item, 0);

  if (!paired) {
    psa_destroy_key(aes_key.id);
    psa_destroy_key(secret.id);
    psa_destroy_key(finder_keypair.id);
    return;
  }

  pass++;
  if (pass % 3 != 0) { return; }

  crypto_key_t eph_priv;
  const uint8_t info[] = "eph_private";

  status = derive_ephemeral_private_key(&master_secret, info, sizeof(info), &eph_priv);
  if (status != CRYPTO_SUCCESS) {
    ESP_LOGE(tag, "failed to derive eph priv key\n");
  }

  request_work_item_t request_item_2 = {
    .type = REQUEST_WORKER_EVENT_FETCH_LOST_DEVICE_LOCATION
  };
  request_location_for_eph_key_t eph_payload;
  crypto_key_t derived_eph_pub_key;

  status = derive_public_key(&master_secret, &derived_eph_pub_key);
  if (status != CRYPTO_SUCCESS) {
    ESP_LOGE(tag, "failed to derive pub key\n");
    return;
  }

  eph_payload.len = 32;
  memcpy(eph_payload.eph_pub_key, derived_eph_pub_key.raw.data, derived_eph_pub_key.raw.len);
  memcpy(&request_item_2.get_device_loc, &eph_payload, sizeof(eph_payload));

  xQueueSend(request_worker_queue, &request_item_2, 0);

  psa_destroy_key(aes_key.id);
  psa_destroy_key(secret.id);
  psa_destroy_key(finder_keypair.id);
}

void handle_read_complete_crypto(crypto_work_item_t *item)
{
  crypto_key_t peer_pub_key = {0};
  peer_pub_key.type = KEY_TYPE_RAW;
  memcpy(
    &peer_pub_key.raw,
    &item->context.read_complete.data,
    item->context.read_complete.data_len);
  peer_pub_key.raw.len = 32;

  crypto_status_t status = generate_secret(
    &device_private_key,
    &peer_pub_key,
    &master_secret);

  if (status != CRYPTO_SUCCESS) {
    ESP_LOGE(tag, "failed to generate master secret\n");
  } else {
    ESP_LOGI(tag, "generated master secret\n");
  }

  ble_work_item_t ble_item;
  ble_item.type = BLE_WORKER_EVENT_READ_COMPLETE;
  memcpy(
    &ble_item.context.read_complete,
    &item->context.read_complete,
    sizeof(item->context.read_complete));

  if (xQueueSend(ble_worker_queue, &ble_item, portMAX_DELAY) != pdPASS) {
    ESP_LOGE("CRYPTO", "Failed to send to BLE worker queue");
  }

  ble_item.type = BLE_WORKER_EVENT_WRITE_KEY_TO_PEER;
  ble_item.context.write_pub_key.conn_handle = item->context.read_complete.conn_handle;
  memcpy(
    ble_item.context.write_pub_key.pub_key,
    device_public_key.raw.data,
    device_public_key.raw.len);
  ble_item.context.write_pub_key.pub_key_len = device_public_key.raw.len;

  if (xQueueSend(ble_worker_queue, &ble_item, portMAX_DELAY) != pdPASS) {
    ESP_LOGE("CRYPTO", "Failed to send to BLE worker queue");
  }
}

void crypto_worker_task(void *param)
{
  crypto_status_t status;

  status = generate_ecdsa_keypair(&ecdsa_private_key);
  if (status != CRYPTO_SUCCESS) { ESP_LOGE(tag, "failed to generate keypair!\n"); }

  crypto_work_item_t item;

  while (1) {
    if (xQueueReceive(crypto_worker_queue, &item, portMAX_DELAY)) {
      switch (item.type) {
        case CRYPTO_WORKER_DECRYPT_LOC_REPORT:
          decrypt_loc_report(&item.context.decrypt_loc);
          break;
        case CRYPTO_WORKER_EVENT_LOST_MSG:
          handle_lost_msg_crypto(&item);
          break;
        case CRYPTO_WORKER_EVENT_READ_COMPLETE:
          handle_read_complete_crypto(&item);
          break;
        default:
          break;
      }
    }
  }
}