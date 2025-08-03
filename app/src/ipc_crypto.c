/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <stdlib.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include "ipc_endpoint.h"

#if defined(CONFIG_IPC_LORAWAN_CRYPTO_SERVER)
#elif defined(CONFIG_IPC_LORAWAN_CRYPTO_CLIENT)
#endif

#ifdef CONFIG_IPC_LORAWAN_CRYPTO_SERVER
#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#endif

LOG_MODULE_REGISTER(ipc_crypto, 4);

struct ipc_lorawan_crypto_set_key_data {
	uint16_t key_size;
	uint8_t key[];
};

struct ipc_lorawan_crypto_set_key_response_data {
	int rc;
};

struct ipc_lorawan_crypto_aes128_encrypt_data {
	uint8_t key_id;
	uint16_t data_size;
	uint8_t data[];
};

struct ipc_lorawan_crypto_aes128_encrypt_response_data {
	int rc;
	uint16_t data_size;
	uint8_t data[];
};

struct ipc_lorawan_crypto_cmac_aes128_verify_data {
	uint8_t key_id;
	uint16_t data_size;
	uint16_t signature_size;
	uint8_t data[]; //Data followed by signature
};

struct ipc_lorawan_crypto_cmac_aes128_verify_response_data {
	int rc;
};

/* Client -> server */
static int ipc_lorawan_crypto_callback_set_key(const uint8_t *message, uint16_t size, void *user_data);
static int ipc_lorawan_crypto_callback_aes128_ecb_encrypt(const uint8_t *message, uint16_t size, void *user_data);
static int ipc_lorawan_crypto_callback_aes128_ccm_encrypt(const uint8_t *message, uint16_t size, void *user_data);
static int ipc_lorawan_crypto_callback_cmac_aes128_encrypt(const uint8_t *message, uint16_t size, void *user_data);
static int ipc_lorawan_crypto_callback_cmac_aes128_verify(const uint8_t *message, uint16_t size, void *user_data);

#if defined(CONFIG_IPC_LORAWAN_CRYPTO_CLIENT)
static struct {
	struct k_sem busy;
	struct k_sem done;
	int rc;
	uint8_t *load_pointer;
	uint16_t load_size;
} ipc_lorawan_crypto_data;

static struct ipc_group ipc_group_set_key = {
	.callback = ipc_lorawan_crypto_callback_set_key,
	.opcode = IPC_OPCODE_CRYPTO_SET_KEY,
	.user_data = &ipc_lorawan_crypto_data,
};

static struct ipc_group ipc_group_aes128_ecb_encrypt = {
	.callback = ipc_lorawan_crypto_callback_aes128_ecb_encrypt,
	.opcode = IPC_OPCODE_CRYPTO_AES128_ECB_ENCRYPT,
	.user_data = &ipc_lorawan_crypto_data,
};

static struct ipc_group ipc_group_aes128_ccm_encrypt = {
	.callback = ipc_lorawan_crypto_callback_aes128_ccm_encrypt,
	.opcode = IPC_OPCODE_CRYPTO_AES128_CCM_ENCRYPT,
	.user_data = &ipc_lorawan_crypto_data,
};

static struct ipc_group ipc_group_cmac_aes128_encrypt = {
	.callback = ipc_lorawan_crypto_callback_cmac_aes128_encrypt,
	.opcode = IPC_OPCODE_CRYPTO_CMAC_AES128_ENCRYPT,
	.user_data = &ipc_lorawan_crypto_data,
};

static struct ipc_group ipc_group_cmac_aes128_verify = {
	.callback = ipc_lorawan_crypto_callback_cmac_aes128_verify,
	.opcode = IPC_OPCODE_CRYPTO_CMAC_AES128_VERIFY,
	.user_data = &ipc_lorawan_crypto_data,
};
#endif

#if defined(CONFIG_IPC_LORAWAN_CRYPTO_SERVER)
static struct ipc_group ipc_group_set_key = {
	.callback = ipc_lorawan_crypto_callback_set_key,
	.opcode = IPC_OPCODE_CRYPTO_SET_KEY,
};

static struct ipc_group ipc_group_aes128_ecb_encrypt = {
	.callback = ipc_lorawan_crypto_callback_aes128_ecb_encrypt,
	.opcode = IPC_OPCODE_CRYPTO_AES128_ECB_ENCRYPT,
};

static struct ipc_group ipc_group_aes128_ccm_encrypt = {
	.callback = ipc_lorawan_crypto_callback_aes128_ccm_encrypt,
	.opcode = IPC_OPCODE_CRYPTO_AES128_CCM_ENCRYPT,
};

static struct ipc_group ipc_group_cmac_aes128_encrypt = {
	.callback = ipc_lorawan_crypto_callback_cmac_aes128_encrypt,
	.opcode = IPC_OPCODE_CRYPTO_CMAC_AES128_ENCRYPT,
};

static struct ipc_group ipc_group_cmac_aes128_verify = {
	.callback = ipc_lorawan_crypto_callback_cmac_aes128_verify,
	.opcode = IPC_OPCODE_CRYPTO_CMAC_AES128_VERIFY,
};
#endif

#if defined(CONFIG_IPC_LORAWAN_CRYPTO_SERVER)
static psa_key_id_t magic_key_id;

static int encrypt_aes128(psa_key_id_t *key_id, uint8_t mode, uint8_t *data, uint16_t data_size, uint8_t *encrypted_data)
{
	/* ECB at present */
	uint32_t output_size;
	psa_status_t status;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	status = psa_cipher_encrypt_setup(&operation, *key_id, PSA_ALG_ECB_NO_PADDING);

	if (status != PSA_SUCCESS) {
		LOG_ERR("AES128 ECB setup failed: %d", status);
		return -EINVAL;
	}

	status = psa_cipher_update(&operation, data, data_size, encrypted_data, data_size, &output_size);

	if (status != PSA_SUCCESS) {
		LOG_ERR("AES128 ECB update failed: %d", status);
		return -EINVAL;
	}

	status = psa_cipher_finish(&operation, (encrypted_data + output_size), (data_size - output_size), &output_size);

	if (status != PSA_SUCCESS) {
		LOG_ERR("AES128 ECB finish failed: %d", status);
		return -EINVAL;
	}

LOG_HEXDUMP_ERR(data, data_size, "in");
LOG_HEXDUMP_ERR(encrypted_data, data_size, "out");

	psa_cipher_abort(&operation);

	return 0;
}

//TODO: this function is temporary and needs removing when KMU is used
static int crypto_cleanup(psa_key_id_t *key_id)
{
	psa_status_t status;

	status = psa_destroy_key(*key_id);

	if (status != PSA_SUCCESS) {
		LOG_ERR("Key removal failed: %d", status);
		return -EINVAL;
	}

	return 0;
}

//TODO: this function is temporary and needs removing when KMU is used
static int ipc_lorawan_crypto_callback_set_key(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	struct ipc_lorawan_crypto_set_key_data *setting = (struct ipc_lorawan_crypto_set_key_data *)message;
	struct ipc_lorawan_crypto_set_key_response_data data;
	psa_status_t status;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 128);
	status = psa_import_key(&attributes, setting->key, setting->key_size, &magic_key_id);

	if (status != PSA_SUCCESS) {
		LOG_ERR("Key import failed: %d", status);
		data.rc = -EINVAL;
	} else {
		data.rc = 0;
	}

	rc = ipc_send_message(IPC_OPCODE_CRYPTO_SET_KEY, sizeof(data), (uint8_t *)&data);

	return rc;
}

static int ipc_lorawan_crypto_callback_aes128_ecb_encrypt(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	int rc_cleanup;
	struct ipc_lorawan_crypto_aes128_encrypt_data *setting = (struct ipc_lorawan_crypto_aes128_encrypt_data *)message;
	struct ipc_lorawan_crypto_aes128_encrypt_response_data *data;
	uint16_t total_size = sizeof(struct ipc_lorawan_crypto_aes128_encrypt_response_data) + setting->data_size;

	data = (struct ipc_lorawan_crypto_aes128_encrypt_response_data *)malloc(total_size);

	rc = encrypt_aes128(&magic_key_id, 0, setting->data, setting->data_size, data->data);
	data->rc = rc;
LOG_ERR("encrypt: %d", rc);

	if (rc == 0) {
		data->data_size = setting->data_size;
	}

	rc_cleanup = crypto_cleanup(&magic_key_id);
LOG_ERR("finish: %d", rc_cleanup);

	rc = ipc_send_message(IPC_OPCODE_CRYPTO_AES128_ECB_ENCRYPT, total_size, (uint8_t *)data);
	free(data);

	return rc;
}

static int ipc_lorawan_crypto_callback_aes128_ccm_encrypt(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	struct ipc_lorawan_crypto_aes128_encrypt_data *setting = (struct ipc_lorawan_crypto_aes128_encrypt_data *)message;
	struct ipc_lorawan_crypto_aes128_encrypt_response_data data;

LOG_ERR("abc1: %d", rc);
data.rc = rc;

	rc = ipc_send_message(IPC_OPCODE_CRYPTO_AES128_CCM_ENCRYPT, sizeof(data), (uint8_t *)&data);

	return rc;
}

static int ipc_lorawan_crypto_callback_cmac_aes128_encrypt(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	struct ipc_lorawan_crypto_aes128_encrypt_data *setting = (struct ipc_lorawan_crypto_aes128_encrypt_data *)message;
	struct ipc_lorawan_crypto_aes128_encrypt_response_data data;

LOG_ERR("abc2: %d", rc);
data.rc = rc;

	rc = ipc_send_message(IPC_OPCODE_CRYPTO_CMAC_AES128_ENCRYPT, sizeof(data), (uint8_t *)&data);

	return rc;
}

static int ipc_lorawan_crypto_callback_cmac_aes128_verify(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	struct ipc_lorawan_crypto_cmac_aes128_verify_data *setting = (struct ipc_lorawan_crypto_cmac_aes128_verify_data *)message;
	struct ipc_lorawan_crypto_cmac_aes128_verify_response_data data;

LOG_ERR("abc3: %d", rc);
data.rc = rc;

	rc = ipc_send_message(IPC_OPCODE_CRYPTO_CMAC_AES128_VERIFY, sizeof(data), (uint8_t *)&data);

	return rc;
}
#endif

#if defined(CONFIG_IPC_LORAWAN_CRYPTO_CLIENT)
static int ipc_lorawan_crypto_callback_set_key(const uint8_t *message, uint16_t size, void *user_data)
{
//get error, give sem
	struct ipc_lorawan_crypto_set_key_response_data *data = (struct ipc_lorawan_crypto_set_key_response_data *)message;

	ipc_lorawan_crypto_data.rc = data->rc;
LOG_ERR("abc4: %d", data->rc);

	k_sem_give(&ipc_lorawan_crypto_data.done);

	return 0;
}

static int ipc_lorawan_crypto_callback_aes128_ecb_encrypt(const uint8_t *message, uint16_t size, void *user_data)
{
//get error, give sem
	struct ipc_lorawan_crypto_aes128_encrypt_response_data *data = (struct ipc_lorawan_crypto_aes128_encrypt_response_data *)message;

//check length/null
LOG_ERR("rc? %d, len? %d, ", data->rc, data->data_size);
//LOG_HEXDUMP_ERR(data->data, data->data_size, "tmp");
	if (data->rc == 0) {
//LOG_ERR("move to %p", ipc_lorawan_crypto_data.load_pointer);
		memcpy(ipc_lorawan_crypto_data.load_pointer, data->data, data->data_size);
	}

	ipc_lorawan_crypto_data.load_pointer = NULL;
	ipc_lorawan_crypto_data.load_size = data->data_size;
	ipc_lorawan_crypto_data.rc = data->rc;
//LOG_ERR("abc5: %d", data->rc);

	k_sem_give(&ipc_lorawan_crypto_data.done);

	return 0;
}

static int ipc_lorawan_crypto_callback_aes128_ccm_encrypt(const uint8_t *message, uint16_t size, void *user_data)
{
//get error, give sem
	struct ipc_lorawan_crypto_aes128_encrypt_response_data *data = (struct ipc_lorawan_crypto_aes128_encrypt_response_data *)message;

	ipc_lorawan_crypto_data.rc = data->rc;
//LOG_ERR("abc6: %d", data->rc);

	k_sem_give(&ipc_lorawan_crypto_data.done);

	return 0;
}

static int ipc_lorawan_crypto_callback_cmac_aes128_encrypt(const uint8_t *message, uint16_t size, void *user_data)
{
//get error, give sem
	struct ipc_lorawan_crypto_aes128_encrypt_response_data *data = (struct ipc_lorawan_crypto_aes128_encrypt_response_data *)message;

	ipc_lorawan_crypto_data.rc = data->rc;
LOG_ERR("abc7: %d", data->rc);

	k_sem_give(&ipc_lorawan_crypto_data.done);

	return 0;
}

static int ipc_lorawan_crypto_callback_cmac_aes128_verify(const uint8_t *message, uint16_t size, void *user_data)
{
//get error, give sem
	struct ipc_lorawan_crypto_cmac_aes128_verify_response_data *data = (struct ipc_lorawan_crypto_cmac_aes128_verify_response_data *)message;

	ipc_lorawan_crypto_data.rc = data->rc;
LOG_ERR("abc8: %d", data->rc);

	k_sem_give(&ipc_lorawan_crypto_data.done);

	return 0;
}

int ipc_lorawan_crypto_set_key(uint8_t *key, uint16_t key_size)
{
	int rc;
	struct ipc_lorawan_crypto_set_key_data *data;
	uint16_t total_size = sizeof(struct ipc_lorawan_crypto_set_key_data) + key_size;

	rc = k_sem_take(&ipc_lorawan_crypto_data.busy, K_FOREVER);

	data = (struct ipc_lorawan_crypto_set_key_data *)malloc(total_size);
	data->key_size = key_size;
	memcpy(data->key, key, key_size);

	rc = ipc_send_message(IPC_OPCODE_CRYPTO_SET_KEY, total_size, (uint8_t *)data);
	free(data);

//check length?
	if (rc < 0) {
		goto finish;
	}

	rc = k_sem_take(&ipc_lorawan_crypto_data.done, K_FOREVER);

	if (rc == 0) {
		rc = ipc_lorawan_crypto_data.rc;
	}

finish:
	k_sem_give(&ipc_lorawan_crypto_data.busy);
	return rc;
}

int ipc_lorawan_crypto_aes128_ecb_encrypt(uint8_t key_id, uint8_t *data, uint16_t data_size, uint8_t *encrypted_data)
{
	int rc;
	struct ipc_lorawan_crypto_aes128_encrypt_data *internal_data;
	uint16_t total_size = sizeof(struct ipc_lorawan_crypto_aes128_encrypt_data) + data_size;

	rc = k_sem_take(&ipc_lorawan_crypto_data.busy, K_FOREVER);

	internal_data = (struct ipc_lorawan_crypto_aes128_encrypt_data *)malloc(total_size);
	internal_data->key_id = key_id;
	internal_data->data_size = data_size;
	memcpy(internal_data->data, data, data_size);

	ipc_lorawan_crypto_data.load_pointer = encrypted_data;
	ipc_lorawan_crypto_data.load_size = data_size;

	rc = ipc_send_message(IPC_OPCODE_CRYPTO_AES128_ECB_ENCRYPT, total_size, (uint8_t *)internal_data);
	free(internal_data);

//check length?
	if (rc < 0) {
		goto finish;
	}

	rc = k_sem_take(&ipc_lorawan_crypto_data.done, K_FOREVER);

	if (rc == 0) {
		rc = ipc_lorawan_crypto_data.rc;
	}

//LOG_HEXDUMP_ERR(data, data_size, "in");
//LOG_HEXDUMP_ERR(encrypted_data, data_size, "out");

finish:
	k_sem_give(&ipc_lorawan_crypto_data.busy);
	return rc;
}
#endif

static int ipc_lorawan_crypto_register(void)
{
#if defined(CONFIG_IPC_LORAWAN_CRYPTO_CLIENT)
	k_sem_init(&ipc_lorawan_crypto_data.busy, 1, 1);
	k_sem_init(&ipc_lorawan_crypto_data.done, 0, 1);
	ipc_lorawan_crypto_data.load_pointer = NULL;
	ipc_lorawan_crypto_data.load_size = 0;
#endif

	ipc_register(&ipc_group_set_key);
	ipc_register(&ipc_group_aes128_ecb_encrypt);
	ipc_register(&ipc_group_aes128_ccm_encrypt);
	ipc_register(&ipc_group_cmac_aes128_encrypt);
	ipc_register(&ipc_group_cmac_aes128_verify);

	return 0;
}

SYS_INIT(ipc_lorawan_crypto_register, APPLICATION, 0);
