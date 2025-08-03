/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2025, Jamie M.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <psa/crypto.h>
#include <psa/crypto_extra.h>

#ifdef CONFIG_BUILD_WITH_TFM
#include <tfm_ns_interface.h>
#endif

#define APP_SUCCESS		(0)
#define APP_ERROR		(-1)
#define APP_SUCCESS_MESSAGE "Example finished successfully!"
#define APP_ERROR_MESSAGE "Example exited with error!"

#define PRINT_HEX(p_label, p_text, len)\
	({\
		LOG_INF("---- %s (len: %u): ----", p_label, len);\
		LOG_HEXDUMP_INF(p_text, len, "Content:");\
		LOG_INF("---- %s end  ----", p_label);\
	})

LOG_MODULE_REGISTER(aes_ecb, LOG_LEVEL_DBG);

/* ====================================================================== */
/* Global variables/defines for the AES-ECB-NO-PADDING example */

#define NRF_CRYPTO_EXAMPLE_AES_BLOCK_SIZE (16)

/* Below text is used as plaintext for encryption/decryption */
static uint8_t m_plain_text[] = {
	//test vector
	0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
};

#define NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE (sizeof(m_plain_text))

static uint8_t m_encrypted_text[NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE];
static uint8_t m_decrypted_text[NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE];

static psa_key_id_t key_id;
/* ====================================================================== */

int crypto_init(void)
{
	psa_status_t status;

	/* Initialize PSA Crypto */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS)
		return APP_ERROR;

	return APP_SUCCESS;
}

int crypto_finish(void)
{
	psa_status_t status;

	/* Destroy the key handle */
	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_destroy_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	return APP_SUCCESS;
}

int generate_key(void)
{
	psa_status_t status;

	LOG_INF("Generating random AES key...");

#if 0
	/* Configure the key attributes */
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 128);

	/* Generate a random key. The key is not exposed to the application,
	 * we can use it to encrypt/decrypt using the key handle
	 */
	status = psa_generate_key(&key_attributes, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_generate_key failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* After the key handle is acquired the attributes are not needed */
	psa_reset_key_attributes(&key_attributes);
#else
//test vector key
uint8_t vectortest[] = {
0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};
        psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
        psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECB_NO_PADDING);
        psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
        psa_set_key_bits(&key_attributes, 128);

status = psa_import_key(&key_attributes,
                            vectortest,
                            sizeof(vectortest),
                            &key_id);

        if (status != PSA_SUCCESS) {
                LOG_INF("psa_import_key failed! (Error: %d)", status);
                return APP_ERROR;
        }
#endif

	LOG_INF("AES key generated successfully!");

	return APP_SUCCESS;
}

int encrypt_ecb_aes(void)
{
	uint32_t olen;
	psa_status_t status;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	LOG_INF("Encrypting using AES ECB MODE...");

	/* Setup the encryption operation */
	status = psa_cipher_encrypt_setup(&operation, key_id, PSA_ALG_ECB_NO_PADDING);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_encrypt_setup failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Perform the encryption */
	status = psa_cipher_update(&operation, m_plain_text,
				   sizeof(m_plain_text), m_encrypted_text,
				   sizeof(m_encrypted_text), &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_update failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Finalize the encryption */
	status = psa_cipher_finish(&operation, m_encrypted_text + olen,
				   sizeof(m_encrypted_text) - olen,
				   &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_finish failed! (Error: %d)", status);
		return APP_ERROR;
	}

	LOG_INF("Encryption successful!");
	PRINT_HEX("Plaintext", m_plain_text, sizeof(m_plain_text));
	PRINT_HEX("Encrypted text", m_encrypted_text, sizeof(m_encrypted_text));

	/* Clean up cipher operation context */
	psa_cipher_abort(&operation);

	return APP_SUCCESS;
}

int decrypt_ecb_aes(void)
{
	uint32_t olen;
	psa_status_t status;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	LOG_INF("Decrypting using AES ECB MODE...");

	/* Setup the decryption operation */
	status = psa_cipher_decrypt_setup(&operation, key_id, PSA_ALG_ECB_NO_PADDING);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_decrypt_setup failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Perform the decryption */
	status = psa_cipher_update(&operation, m_encrypted_text,
				   sizeof(m_encrypted_text), m_decrypted_text,
				   sizeof(m_decrypted_text), &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_update failed! (Error: %d)", status);
		return APP_ERROR;
	}

	/* Finalize the decryption */
	status = psa_cipher_finish(&operation, m_decrypted_text + olen,
				   sizeof(m_decrypted_text) - olen,
				   &olen);
	if (status != PSA_SUCCESS) {
		LOG_INF("psa_cipher_finish failed! (Error: %d)", status);
		return APP_ERROR;
	}

	PRINT_HEX("Decrypted text", m_decrypted_text, sizeof(m_decrypted_text));

	/* Check the validity of the decryption */
	if (memcmp(m_decrypted_text,
				m_plain_text,
				NRF_CRYPTO_EXAMPLE_AES_MAX_TEXT_SIZE) != 0){

		LOG_INF("Error: Decrypted text doesn't match the plaintext");
		return APP_ERROR;
	}

	LOG_INF("Decryption successful!");

	/*  Clean up cipher operation context */
	psa_cipher_abort(&operation);

	return APP_SUCCESS;
}


int main(void)
{
	int status;

	LOG_INF("Starting AES-ECB-NO-PADDING example...");

	status = crypto_init();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = generate_key();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = encrypt_ecb_aes();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = decrypt_ecb_aes();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	status = crypto_finish();
	if (status != APP_SUCCESS) {
		LOG_INF(APP_ERROR_MESSAGE);
		return APP_ERROR;
	}

	LOG_INF(APP_SUCCESS_MESSAGE);

	return APP_SUCCESS;
}
