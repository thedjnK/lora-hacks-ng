/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include "ipc_endpoint.h"
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, 4);

static uint8_t data[3];

int main(void)
{
	int rc;
	uint8_t up_value = 0;

	rc = ipc_setup();

	if (rc != 0) {
		return 0;
	}

	rc = ipc_wait_for_ready();

	if (rc != 0) {
		return 0;
	}

	while (1) {
		k_sleep(K_MSEC(2000));

		data[0] = up_value++;
		data[1] = up_value++;
		data[2] = up_value++;

		rc = ipc_send_message(0, sizeof(data), data);

		if (rc != 0) {
			LOG_ERR("Failed to send: %d", rc);
		}
	}

	return 0;
}
