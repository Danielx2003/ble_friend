#include "ble_callbacks.h"
#include "disc.h"
#include "crypto.h"
#include "request.h"

#include <stdio.h>
#include <string.h>

bool on_pairing_msg(void *ctx, mfg_data_t *mfg)
{
	disc_stop();
	start_connect(ctx);
	return true;
}

bool on_lost_msg(void *ctx, mfg_data_t *mfg)
{
	printf("received lost msg\n");
	crypto_status_t status;
	crypto_key_t keypair;

	status = generate_keypair(CRYPTO_CURVE_X25519, &keypair);
	if (status != CRYPTO_SUCCESS)
	{
		return false;
	}

	/*
	- Convert Advertised Bytes to Key
	- Do ECDH
	- Derive AES Key
	- Upload to server via POST request	
	*/

	crypto_key_t eph_pub_key = {
		.type = KEY_TYPE_RAW,
		.raw = {
			.len = mfg->payload_len
		}
	};
	memcpy(eph_pub_key.raw.data, mfg->payload, mfg->payload_len);

	crypto_key_t secret;

	status = generate_secret(
		&keypair, 
		&eph_pub_key,
		&secret
	);
	if (status != CRYPTO_SUCCESS)
	{
		return false;
	}

	crypto_key_t aes_key;
	status = derive_symmetric_aes_key_hkdf(
		&secret,
		NULL,
		NULL,
		&aes_key
	);
	if (status != CRYPTO_SUCCESS)
	{
		return false;
	}
	
	printf("sending lost details\n");
	upload_lost_details(NULL);

	return true;
}

bool on_paired_msg(void *ctx, mfg_data_t *mfg)
{
	printf("paired msg receiev\n");
	return true;
}
