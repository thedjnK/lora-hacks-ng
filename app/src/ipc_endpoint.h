/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#ifndef APP_IPC_ENDPOINT_H
#define APP_IPC_ENDPOINT_H

#include <stdint.h>

/** Setup IPC service */
int ipc_setup(void);

/** Wait for IPC service on remote end to be ready */
int ipc_wait_for_ready();

/** Send message over IPC */
int ipc_send_message(uint8_t opcode, uint16_t size, const uint8_t *message);

#endif /* APP_IPC_ENDPOINT_H */
