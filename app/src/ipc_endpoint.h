/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#ifndef APP_IPC_ENDPOINT_H
#define APP_IPC_ENDPOINT_H

#include <stdint.h>
#include <zephyr/sys/slist.h>

enum ipc_opcode {
	/* Settings */
	/* | Client -> server */
	IPC_OPCODE_SETTINGS_SAVE,
	IPC_OPCODE_SETTINGS_LOAD,
	IPC_OPCODE_SETTINGS_COMMIT,
	IPC_OPCODE_SETTINGS_TREE_COUNT,
	IPC_OPCODE_SETTINGS_TREE_LOAD,

	/* | Server -> client */
	IPC_OPCODE_SETTINGS_BOOT_LOAD,

	/* Crypto */
	/* | Client -> server */
	IPC_OPCODE_CRYPTO_SET_KEY,
	IPC_OPCODE_CRYPTO_AES128_ECB_ENCRYPT,
	IPC_OPCODE_CRYPTO_AES128_CCM_ENCRYPT,
	IPC_OPCODE_CRYPTO_CMAC_AES128_ENCRYPT,
	IPC_OPCODE_CRYPTO_CMAC_AES128_VERIFY,
};

typedef int (*ipc_callback_fn)(const uint8_t *message, uint16_t size, void *user_data);

struct ipc_group {
	sys_snode_t node;
	ipc_callback_fn callback;
	uint8_t opcode;
	void *user_data;
};

/*
save: name, value, size
load: name -> value, size
*/

/** Setup IPC service */
int ipc_setup(void);

/** Wait for IPC service on remote end to be ready */
int ipc_wait_for_ready();

/** Send message over IPC */
int ipc_send_message(uint8_t opcode, uint16_t size, const uint8_t *message);

/** Register IPC callback handler for an opcode */
void ipc_register(struct ipc_group *group);

/** Unregister IPC callback handler for an opcode */
void ipc_unregister(struct ipc_group *group);

#endif /* APP_IPC_ENDPOINT_H */
