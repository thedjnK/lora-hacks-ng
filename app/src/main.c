/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
//#include <zephyr/drivers/mspi.h>
//#include <zephyr/drivers/pinctrl.h>
//#include <zephyr/drivers/counter.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/pm/device.h>

#if !defined(CONFIG_MULTITHREADING)
#include <zephyr/sys/atomic.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, 4);

#include <hal/nrf_gpio.h>

#define MSPI_HPF_NODE		     DT_DRV_INST(0)
#define MAX_TX_MSG_SIZE		     (DT_REG_SIZE(DT_NODELABEL(sram_tx)))
#define MAX_RX_MSG_SIZE		     (DT_REG_SIZE(DT_NODELABEL(sram_rx)))
#define IPC_TIMEOUT_MS		     100
#define EP_SEND_TIMEOUT_MS	     10

static struct ipc_ept ep;
static size_t ipc_received;
static uint8_t *ipc_receive_buffer;
//static volatile uint32_t *cpuflpr_error_ctx_ptr =
//	(uint32_t *)DT_REG_ADDR(DT_NODELABEL(cpuflpr_error_code));

#if defined(CONFIG_MULTITHREADING)
static K_SEM_DEFINE(ipc_sem, 0, 1);
#else
static atomic_t ipc_atomic_sem = ATOMIC_INIT(0);
#endif

static void ep_recv(const void *data, size_t len, void *priv);

static void ep_bound(void *priv)
{
	ipc_received = 0;
#if defined(CONFIG_MULTITHREADING)
	k_sem_give(&ipc_sem);
#else
	atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_EP_BOUNDED);
#endif
	LOG_DBG("Ep bounded");
}

static struct ipc_ept_cfg ep_cfg = {
	.cb = {.bound = ep_bound, .received = ep_recv},
};

const char *z_riscv_mcause_str(uint32_t cause)
{
	static const char *const mcause_str[17] = {
		[0] = "Instruction address misaligned",
		[1] = "Instruction Access fault",
		[2] = "Illegal instruction",
		[3] = "Breakpoint",
		[4] = "Load address misaligned",
		[5] = "Load access fault",
		[6] = "Store/AMO address misaligned",
		[7] = "Store/AMO access fault",
		[8] = "Environment call from U-mode",
		[9] = "Environment call from S-mode",
		[10] = "Unknown",
		[11] = "Environment call from M-mode",
		[12] = "Instruction page fault",
		[13] = "Load page fault",
		[14] = "Unknown",
		[15] = "Store/AMO page fault",
		[16] = "Unknown",
	};

	return mcause_str[MIN(cause, ARRAY_SIZE(mcause_str) - 1)];
}

/**
 * @brief IPC receive callback function.
 *
 * This function is called by the IPC stack when a message is received from the
 * other core. The function checks the opcode of the received message and takes
 * appropriate action.
 *
 * @param data Pointer to the received message.
 * @param len Length of the received message.
 */
static void ep_recv(const void *data, size_t len, void *priv)
{
#if 0
	hpf_mspi_flpr_response_msg_t *response = (hpf_mspi_flpr_response_msg_t *)data;

	switch (response->opcode) {
#if defined(CONFIG_MSPI_HPF_FAULT_TIMER)
	case HPF_MSPI_CONFIG_TIMER_PTR: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem);
#else
		atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_CONFIG_TIMER_PTR);
#endif
		break;
	}
#endif
	case HPF_MSPI_CONFIG_PINS: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_cfg);
#else
		atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_CONFIG_PINS);
#endif
		break;
	}
	case HPF_MSPI_CONFIG_DEV: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_cfg);
#else
		atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_CONFIG_DEV);
#endif
		break;
	}
	case HPF_MSPI_CONFIG_XFER: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_cfg);
#else
		atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_CONFIG_XFER);
#endif
		break;
	}
	case HPF_MSPI_TX: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_xfer);
#else
		atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_TX);
#endif
		break;
	}
	case HPF_MSPI_TXRX: {
		if (len > 0) {
			ipc_received = len - sizeof(hpf_mspi_opcode_t);
			ipc_receive_buffer = (uint8_t *)&response->data;
		}
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_xfer);
#else
		atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_TXRX);
#endif
		break;
	}
	case HPF_MSPI_HPF_APP_HARD_FAULT: {

		const uint32_t mcause_exc_mask = 0xfff;
		volatile uint32_t cause = cpuflpr_error_ctx_ptr[0];
		volatile uint32_t pc = cpuflpr_error_ctx_ptr[1];
		volatile uint32_t bad_addr = cpuflpr_error_ctx_ptr[2];
		volatile uint32_t *ctx = (volatile uint32_t *)cpuflpr_error_ctx_ptr[3];

		LOG_ERR(">>> HPF APP FATAL ERROR: %s", z_riscv_mcause_str(cause & mcause_exc_mask));
		LOG_ERR("Faulting instruction address (mepc): 0x%08x", pc);
		LOG_ERR("mcause: 0x%08x, mtval: 0x%08x, ra: 0x%08x", cause, bad_addr, ctx[0]);
		LOG_ERR("    t0: 0x%08x,    t1: 0x%08x, t2: 0x%08x", ctx[1], ctx[2], ctx[3]);

		LOG_ERR("HPF application halted...");
		break;
	}
	default: {
		LOG_ERR("Invalid response opcode: %d", response->opcode);
		break;
	}
	}
#endif

	LOG_HEXDUMP_DBG((uint8_t *)data, len, "Received msg:");
}

/**
 * @brief Send data to the FLPR core using the IPC service, and wait for FLPR response.
 *
 * @param opcode The configuration packet opcode to send.
 * @param data The data to send.
 * @param len The length of the data to send.
 *
 * @return 0 on success, negative errno code on failure.
 */
static int send_data(uint8_t opcode, const void *data, size_t len)
{
	LOG_DBG("Sending msg with opcode: %d", (uint8_t)opcode);

#if 0
	int rc;
#ifdef CONFIG_MSPI_HPF_IPC_NO_COPY
	(void)len;
	void *data_ptr = (void *)data;
#endif

#if defined(CONFIG_SYS_CLOCK_EXISTS)
	uint32_t start = k_uptime_get_32();
#else
	uint32_t repeat = EP_SEND_TIMEOUT_MS;
#endif
#if !defined(CONFIG_MULTITHREADING)
	atomic_clear_bit(&ipc_atomic_sem, opcode);
#endif

	do {
#ifdef CONFIG_MSPI_HPF_IPC_NO_COPY
		rc = ipc_service_send(&ep, &data_ptr, sizeof(void *));
#else
		rc = ipc_service_send(&ep, data, len);
#endif
#if defined(CONFIG_SYS_CLOCK_EXISTS)
		if ((k_uptime_get_32() - start) > EP_SEND_TIMEOUT_MS) {
#else
		repeat--;
		if ((rc < 0) && (repeat == 0)) {
#endif
			break;
		};
	} while (rc == -ENOMEM); /* No space in the buffer. Retry. */

	if (rc < 0) {
		LOG_ERR("Data transfer failed: %d", rc);
		return rc;
	}

	rc = hpf_mspi_wait_for_response(opcode, IPC_TIMEOUT_MS);
	if (rc < 0) {
		LOG_ERR("Data transfer: %d response timeout: %d!", opcode, rc);
	}

	return rc;
#endif
}

#if defined(CONFIG_MSPI_HPF_FAULT_TIMER)
static void flpr_fault_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	LOG_ERR("HPF fault detected.");
}
#endif

/**
 * @brief Initialize the MSPI HPF driver.
 *
 * This function initializes the MSPI HPF driver. It is responsible for
 * setting up the hardware and registering the IPC endpoint for the
 * driver.
 *
 * @param dev Pointer to the device structure for the MSPI HPF driver.
 *
 * @retval 0 If successful.
 * @retval -errno If an error occurs.
 */
int main()
{
	int ret;
	const struct device *ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

#if defined(CONFIG_MSPI_HPF_FAULT_TIMER)
	const struct device *const flpr_fault_timer = DEVICE_DT_GET(DT_NODELABEL(fault_timer));
	const struct counter_top_cfg top_cfg = {
		.callback = flpr_fault_handler,
		.user_data = NULL,
		.flags = 0,
		.ticks = counter_us_to_ticks(flpr_fault_timer, CONFIG_MSPI_HPF_FAULT_TIMEOUT)
	};
#endif

	ret = ipc_service_open_instance(ipc_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("ipc_service_open_instance() failure");
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc_instance, &ep, &ep_cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint() failure");
		return ret;
	}

	/* Wait for ep to be bounded */
#if defined(CONFIG_MULTITHREADING)
	k_sem_take(&ipc_sem, K_FOREVER);
#else
	while (!atomic_test_and_clear_bit(&ipc_atomic_sem, HPF_MSPI_EP_BOUNDED)) {
	}
#endif

#if defined(CONFIG_MSPI_HPF_FAULT_TIMER)
	/* Configure timer as HPF `watchdog` */
	if (!device_is_ready(flpr_fault_timer)) {
		LOG_ERR("FLPR timer not ready");
		return -1;
	}

	ret = counter_set_top_value(flpr_fault_timer, &top_cfg);
	if (ret < 0) {
		LOG_ERR("counter_set_top_value() failure");
		return ret;
	}

	/* Send timer address to FLPR */
	hpf_mspi_flpr_timer_msg_t timer_data = {
		.opcode = HPF_MSPI_CONFIG_TIMER_PTR,
		.timer_ptr = (NRF_TIMER_Type *)DT_REG_ADDR(DT_NODELABEL(fault_timer)),
	};

	ret = send_data(HPF_MSPI_CONFIG_TIMER_PTR, (const void *)&timer_data.opcode,
			sizeof(hpf_mspi_flpr_timer_msg_t));
	if (ret < 0) {
		LOG_ERR("Send timer configuration failure");
		return ret;
	}

	ret = counter_start(flpr_fault_timer);
	if (ret < 0) {
		LOG_ERR("counter_start() failure");
		return ret;
	}
#endif

	return ret;
}
