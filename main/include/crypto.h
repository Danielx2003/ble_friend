#pragma once

#include "psa/crypto.h"

#include <unistd.h>
#include <stdbool.h>

#define X25519_KEY_BITS 255
#define P256_KEY_BITS   256

typedef enum {
    CRYPTO_CURVE_X25519,
    CRYPTO_CURVE_P256
} crypto_curve_t;


//psa_status_t derive_public_key(
//	uint8_t master_secret[32],
//	uint8_t ephemeral_pub_key[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE],
//	size_t *ephemeral_pub_key_size
//);



/* Wrapper Implementation */

#define CRYPTO_BACKEND_KEY_HANDLE psa_key_id_t

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

crypto_status_t generate_keypair(
  crypto_curve_t curve,
	crypto_key_t *key
);

crypto_status_t generate_secret(
	crypto_key_t *priv_key,
	crypto_key_t *pub_key,
	uint8_t *raw_secret,
	size_t *secret_len
);

crypto_status_t convert_from_raw_to_id(crypto_key_t *key);
crypto_status_t convert_from_id_to_raw(crypto_key_t *key);