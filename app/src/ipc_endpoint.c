/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ipc_endpoint, 4);

#define IPC_MESSAGE_OVERHEAD 3
#define IPC_MESSAGE_DATA_SIZE 64

struct ipc_payload {
	uint8_t opcode;
	uint16_t size;
	uint8_t data[IPC_MESSAGE_DATA_SIZE];
};

static void ipc_endpoint_bound(void *priv);
static void ipc_endpoint_receive(const void *data, size_t len, void *priv);

static struct ipc_payload data_payload;
static struct ipc_ept ipc_endpoint;
static K_SEM_DEFINE(ipc_bound_sem, 0, 1);
static K_SEM_DEFINE(ipc_receive_sem, 0, 1);

static struct ipc_ept_cfg ipc_endpoint_config = {
	.name = "ep0",
	.cb = {
		.bound    = ipc_endpoint_bound,
		.received = ipc_endpoint_receive,
	},
};

static void ipc_endpoint_bound(void *priv)
{
	k_sem_give(&ipc_bound_sem);
}

static void ipc_endpoint_receive(const void *data, size_t len, void *priv)
{
	struct ipc_payload *values = (struct ipc_payload *)data;

	if (len > sizeof(struct ipc_payload)) {
		printk("invalid len: %d", len);
	} else if (len != (values->size + 2)) {
		printk("len mismatch: %d vs %d", len, (values->size + 2));
	} else {
		printk("got message! %d, %d, %d, %d, %d\n", values->opcode, values->size, values->data[0], values->data[1], values->data[2]);
	}

	k_sem_give(&ipc_receive_sem);
}

int ipc_setup(void)
{
	int rc;
	const struct device *ipc_device = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	rc = ipc_service_open_instance(ipc_device);

	if (rc < 0 && rc != -EALREADY) {
		LOG_ERR("IPC service open fail: %d", rc);
		return rc;
	}

	rc = ipc_service_register_endpoint(ipc_device, &ipc_endpoint, &ipc_endpoint_config);

	if (rc < 0) {
		LOG_ERR("IPC service register fail: %d", rc);
		return rc;
	}

	return 0;
}

int ipc_wait_for_ready()
{
	k_sem_take(&ipc_bound_sem, K_FOREVER);
	return 0;
}

int ipc_send_message(uint8_t opcode, uint16_t size, const uint8_t *message)
{
	int rc;

	if (size > sizeof(struct ipc_payload)) {
		return -1;
	}

	data_payload.opcode = opcode;
	data_payload.size = size;
	memcpy(data_payload.data, message, size);

	rc = ipc_service_send(&ipc_endpoint, &data_payload, (size + IPC_MESSAGE_OVERHEAD));

	return rc;
}
