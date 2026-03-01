#pragma once

#include "psa/crypto.h"

#include <unistd.h>
#include <stdbool.h>

#define X25519_KEY_BITS 255
#define P256_KEY_BITS   256

#define CRYPTO_BACKEND_KEY_HANDLE psa_key_id_t

void curve25519_clamp(uint8_t k[32]);

typedef enum {
    CRYPTO_CURVE_X25519,
    CRYPTO_CURVE_P256
} crypto_curve_t;



typedef enum {
	CRYPTO_SUCCESS,
	CRYPTO_ERR_INVALID_ARGS,
	CRYPTO_ERR_INVALID_HANDLE,
	CRYPTO_ERR_UNKNOWN
} crypto_status_t;

typedef CRYPTO_BACKEND_KEY_HANDLE crypto_key_handle_t;

typedef enum {
  KEY_TYPE_ID,
  KEY_TYPE_RAW
} key_type_t;

typedef struct {
	key_type_t type;
	union {
	  crypto_key_handle_t id;
	  struct {
	    uint8_t data[32];
	    size_t len;
	  } raw;
	};
} crypto_key_t;

crypto_status_t crypto_init();

crypto_status_t generate_keypair(
  crypto_curve_t curve,
	crypto_key_t *key
);

crypto_status_t generate_secret(
	crypto_key_t *priv_key,
	crypto_key_t *pub_key,
	crypto_key_t *secret
);

crypto_status_t derive_public_key(
	crypto_key_t *secret,
	crypto_key_t *public_key
);

crypto_status_t derive_ephemeral_private_key(
    crypto_key_t *secret,
    const uint8_t *info,
    size_t info_len,
    crypto_key_t *private_key
);

crypto_status_t derive_symmetric_aes_key_hkdf(
	crypto_key_t *secret,
	uint8_t *salt,
	uint8_t *info,
	crypto_key_t *aes_key
);

crypto_status_t export_public_key(
	crypto_key_t *keypair,
	crypto_key_t *public_key
);

crypto_status_t convert_from_raw_to_id(crypto_key_t *key);
crypto_status_t convert_from_id_to_raw(crypto_key_t *key);

