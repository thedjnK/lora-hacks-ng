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
#include "ipc_endpoint.h"

LOG_MODULE_REGISTER(ipc_endpoint, 4);

#define IPC_MESSAGE_OVERHEAD (data_payload.data - &data_payload.opcode)
#define IPC_MESSAGE_DATA_SIZE 64

struct ipc_payload {
	uint8_t opcode;
	uint16_t size;
	uint8_t data[IPC_MESSAGE_DATA_SIZE];
};

static void ipc_endpoint_bound(void *priv);
static void ipc_endpoint_receive(const void *data, size_t len, void *priv);

static sys_slist_t ipc_registered_handlers = SYS_SLIST_STATIC_INIT(&ipc_registered_handlers);
static struct ipc_payload data_payload;
static struct ipc_ept ipc_endpoint;
static K_SEM_DEFINE(ipc_bound_sem, 0, 1);
static K_SEM_DEFINE(ipc_receive_sem, 0, 1);

static struct ipc_ept_cfg ipc_endpoint_config = {
	.name = "ep0",
	.cb = {
		.bound = ipc_endpoint_bound,
		.received = ipc_endpoint_receive,
	},
};

static void ipc_endpoint_bound(void *priv)
{
	k_sem_give(&ipc_bound_sem);
}

static void ipc_endpoint_receive(const void *data, size_t len, void *priv)
{
	sys_snode_t *snp, *sns;
	struct ipc_payload *values = (struct ipc_payload *)data;

	if (len > sizeof(struct ipc_payload)) {
		printk("invalid len: %d", len);
	} else if (len != (values->size + IPC_MESSAGE_OVERHEAD)) {
		printk("len mismatch: %d vs %d", len, (values->size + IPC_MESSAGE_OVERHEAD));
	} else {
		printk("got message! %d, %d, %d, %d, %d\n", values->opcode, values->size, values->data[0], values->data[1], values->data[2]);
LOG_HEXDUMP_ERR(data, len, "DAT");
	}

	SYS_SLIST_FOR_EACH_NODE_SAFE(&ipc_registered_handlers, snp, sns) {
		struct ipc_group *group = CONTAINER_OF(snp, struct ipc_group, node);

		if (group->opcode == values->opcode) {
			(void)group->callback(&values->data[0], values->size, group->user_data);
			break;
		}
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
LOG_HEXDUMP_ERR(message, size, "out");
	memcpy(data_payload.data, message, size);
LOG_HEXDUMP_ERR(&data_payload, (size + IPC_MESSAGE_OVERHEAD), "out2");

	rc = ipc_service_send(&ipc_endpoint, &data_payload, (size + IPC_MESSAGE_OVERHEAD));

	return rc;
}

void ipc_register(struct ipc_group *group)
{
	sys_slist_append(&ipc_registered_handlers, &group->node);
}

void ipc_unregister(struct ipc_group *group)
{
	(void)sys_slist_find_and_remove(&ipc_registered_handlers, &group->node);
}
