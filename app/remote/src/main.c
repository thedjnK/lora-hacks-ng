/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>

//#include <hal/nrf_vpr_csr.h>
//#include <hal/nrf_vpr_csr_vio.h>
//#include <hal/nrf_vpr_csr_vtim.h>
//#include <hal/nrf_timer.h>
//#include <haly/nrfy_gpio.h>

//#define HRT_IRQ_PRIORITY    2
//#define HRT_VEVIF_IDX_READ  17
//#define HRT_VEVIF_IDX_WRITE 18

//#define VEVIF_IRQN(vevif)   VEVIF_IRQN_1(vevif)
//#define VEVIF_IRQN_1(vevif) VPRCLIC_##vevif##_IRQn

BUILD_ASSERT(CONFIG_HPF_MSPI_MAX_RESPONSE_SIZE > 0, "Response max size should be greater that 0");

static volatile uint8_t response_buffer[CONFIG_HPF_MSPI_MAX_RESPONSE_SIZE];

#define HPF_MSPI_EP_BOUNDED 0

static struct ipc_ept ep;
static atomic_t ipc_atomic_sem = ATOMIC_INIT(0);
#if defined(CONFIG_HPF_MSPI_FAULT_TIMER)
static NRF_TIMER_Type *fault_timer;
#endif
//static volatile uint32_t *cpuflpr_error_ctx_ptr =
//	(uint32_t *)DT_REG_ADDR(DT_NODELABEL(cpuflpr_error_code));

static void ep_bound(void *priv)
{
	atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_EP_BOUNDED);
}

static void ep_recv(const void *data, size_t len, void *priv)
{
#ifdef CONFIG_HPF_MSPI_IPC_NO_COPY
	data = *(void **)data;
#endif

	(void)priv;
	(void)len;
	uint8_t opcode = *(uint8_t *)data;
	uint32_t num_bytes = 0;

#if 0
#if defined(CONFIG_HPF_MSPI_FAULT_TIMER)
	if (fault_timer != NULL) {
		nrf_timer_task_trigger(fault_timer, NRF_TIMER_TASK_START);
	}
#endif

	switch (opcode) {
#if defined(CONFIG_HPF_MSPI_FAULT_TIMER)
	case HPF_MSPI_CONFIG_TIMER_PTR: {
		const hpf_mspi_flpr_timer_msg_t *timer_data =
			(const hpf_mspi_flpr_timer_msg_t *)data;

		fault_timer = timer_data->timer_ptr;
		break;
	}
#endif
	case HPF_MSPI_CONFIG_PINS: {
		hpf_mspi_pinctrl_soc_pin_msg_t *pins_cfg = (hpf_mspi_pinctrl_soc_pin_msg_t *)data;

		config_pins(pins_cfg);
		break;
	}
	case HPF_MSPI_CONFIG_DEV: {
		hpf_mspi_dev_config_msg_t *dev_config = (hpf_mspi_dev_config_msg_t *)data;

		NRFX_ASSERT(dev_config->device_index < DEVICES_MAX);
		NRFX_ASSERT(dev_config->dev_config.io_mode < SUPPORTED_IO_MODES_COUNT);
		NRFX_ASSERT((dev_config->dev_config.cpp == MSPI_CPP_MODE_0) ||
			    (dev_config->dev_config.cpp == MSPI_CPP_MODE_3));
		NRFX_ASSERT(dev_config->dev_config.ce_index < ce_vios_count);
		NRFX_ASSERT(dev_config->dev_config.ce_polarity <= MSPI_CE_ACTIVE_HIGH);

		hpf_mspi_devices[dev_config->device_index] = dev_config->dev_config;

		/* Configure CE pin. */
		if (hpf_mspi_devices[dev_config->device_index].ce_polarity == MSPI_CE_ACTIVE_LOW) {
			nrf_vpr_csr_vio_out_or_set(
				BIT(ce_vios[hpf_mspi_devices[dev_config->device_index].ce_index]));
		} else {
			nrf_vpr_csr_vio_out_clear_set(
				BIT(ce_vios[hpf_mspi_devices[dev_config->device_index].ce_index]));
		}

		if (dev_config->dev_config.io_mode == MSPI_IO_MODE_SINGLE) {
			if (data_vios[DATA_LINE_INDEX(NRF_FUN_HPF_MSPI_DQ2)] != DATA_PIN_UNUSED &&
			    data_vios[DATA_LINE_INDEX(NRF_FUN_HPF_MSPI_DQ3)] != DATA_PIN_UNUSED) {
				nrf_vpr_csr_vio_out_or_set(
					BIT(data_vios[DATA_LINE_INDEX(NRF_FUN_HPF_MSPI_DQ2)]));
				nrf_vpr_csr_vio_out_or_set(
					BIT(data_vios[DATA_LINE_INDEX(NRF_FUN_HPF_MSPI_DQ3)]));
			}
		} else {
			nrf_vpr_csr_vio_out_clear_set(
				BIT(data_vios[DATA_LINE_INDEX(NRF_FUN_HPF_MSPI_DQ2)]));
			nrf_vpr_csr_vio_out_clear_set(
				BIT(data_vios[DATA_LINE_INDEX(NRF_FUN_HPF_MSPI_DQ3)]));
		}

		break;
	}
	case HPF_MSPI_CONFIG_XFER: {
		hpf_mspi_xfer_config_msg_t *xfer_config = (hpf_mspi_xfer_config_msg_t *)data;

		NRFX_ASSERT(xfer_config->xfer_config.device_index < DEVICES_MAX);
		/* Check if device was configured. */
		NRFX_ASSERT(hpf_mspi_devices[xfer_config->xfer_config.device_index].ce_index <
			    ce_vios_count);
		NRFX_ASSERT(xfer_config->xfer_config.command_length <= sizeof(uint32_t));
		NRFX_ASSERT(xfer_config->xfer_config.address_length <= sizeof(uint32_t));
		NRFX_ASSERT(xfer_config->xfer_config.tx_dummy == 0 ||
			    xfer_config->xfer_config.command_length != 0 ||
			    xfer_config->xfer_config.address_length != 0);

#ifdef CONFIG_HPF_MSPI_IPC_NO_COPY
		hpf_mspi_xfer_config_ptr = &xfer_config->xfer_config;
#else
		hpf_mspi_xfer_config = xfer_config->xfer_config;
#endif
		configure_clock(hpf_mspi_devices[hpf_mspi_xfer_config_ptr->device_index].cpp);

		/* Tune up pad bias for frequencies above 32MHz */
		if (hpf_mspi_devices[hpf_mspi_xfer_config_ptr->device_index].cnt0_value <=
		    STD_PAD_BIAS_CNT0_THRESHOLD) {
			NRF_GPIOHSPADCTRL->BIAS = PAD_BIAS_VALUE;
		}

		break;
	}
	case HPF_MSPI_TX:
		hpf_mspi_xfer_packet_msg_t *packet = (hpf_mspi_xfer_packet_msg_t *)data;

		xfer_execute(packet, NULL);
		break;
	case HPF_MSPI_TXRX: {
		hpf_mspi_xfer_packet_msg_t *packet = (hpf_mspi_xfer_packet_msg_t *)data;
		if (packet->num_bytes > 0) {
#ifdef CONFIG_HPF_MSPI_IPC_NO_COPY
			xfer_execute(packet, packet->data);
#else
			NRFX_ASSERT(packet->num_bytes <=
				    CONFIG_HPF_MSPI_MAX_RESPONSE_SIZE - sizeof(hpf_mspi_opcode_t));
			num_bytes = packet->num_bytes;
			xfer_execute(packet, response_buffer + sizeof(hpf_mspi_opcode_t));
#endif
		}
		break;
	}
	default:
		opcode = HPF_MSPI_WRONG_OPCODE;
		break;
	}

	response_buffer[0] = opcode;
	ipc_service_send(&ep, (const void *)response_buffer,
			 sizeof(hpf_mspi_opcode_t) + num_bytes);

#if defined(CONFIG_HPF_MSPI_FAULT_TIMER)
	if (fault_timer != NULL) {
		nrf_timer_task_trigger(fault_timer, NRF_TIMER_TASK_CLEAR);
		nrf_timer_task_trigger(fault_timer, NRF_TIMER_TASK_STOP);
	}
#endif
#endif
}

static const struct ipc_ept_cfg ep_cfg = {
	.cb = {.bound = ep_bound, .received = ep_recv},
};

static int backend_init(void)
{
	int ret = 0;
	const struct device *ipc0_instance;
	volatile uint32_t delay = 0;

#if !defined(CONFIG_SYS_CLOCK_EXISTS)
	/* Wait a little bit for IPC service to be ready on APP side. */
	while (delay < 6000) {
		delay++;
	}
#endif

	ipc0_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));

	ret = ipc_service_open_instance(ipc0_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc0_instance, &ep, &ep_cfg);
	if (ret < 0) {
		return ret;
	}

	/* Wait for endpoint to be bound. */
	while (!atomic_test_and_clear_bit(&ipc_atomic_sem, HPF_MSPI_EP_BOUNDED)) {
	}

	return 0;
}

#if 0
__attribute__((interrupt)) void hrt_handler_read(void)
{
	hrt_read(&xfer_params);
}

__attribute__((interrupt)) void hrt_handler_write(void)
{
	hrt_write(&xfer_params);
}
#endif

#if 0
/**
 * @brief Trap handler for HPF application.
 *
 * @details
 * This function is called on unhandled CPU exceptions. It's a good place to
 * handle critical errors and notify the core that the HPF application has
 * crashed.
 *
 * @param mcause  - cause of the exception (from mcause register)
 * @param mepc    - address of the instruction that caused the exception (from mepc register)
 * @param mtval   - additional value (e.g. bad address)
 * @param context - pointer to the saved context (only some registers are saved - ra, t0, t1, t2)
 */
void trap_handler(uint32_t mcause, uint32_t mepc, uint32_t mtval, void *context)
{
	const uint8_t fault_opcode = HPF_MSPI_HPF_APP_HARD_FAULT;

	/* It can be distinguish whether the exception was caused by an interrupt or an error:
	 * On RV32, bit 31 of the mcause register indicates whether the event is an interrupt.
	 */
	if (mcause & 0x80000000) {
		/* Interrupt – can be handled or ignored */
	} else {
		/* Exception – critical error */
	}

	cpuflpr_error_ctx_ptr[0] = mcause;
	cpuflpr_error_ctx_ptr[1] = mepc;
	cpuflpr_error_ctx_ptr[2] = mtval;
	cpuflpr_error_ctx_ptr[3] = (uint32_t)context;

	ipc_service_send(&ep, &fault_opcode, sizeof(fault_opcode));

	while (1) {
		/* Loop forever */
	}
}

/* The trap_entry function is the entry point for exception handling.
 * The naked attribute prevents the compiler from generating an automatic prologue/epilogue.
 */
__attribute__((naked)) void trap_entry(void)
{
	__asm__ volatile(
		/* Reserve space on the stack:
		 * 16 bytes for 4 registers context (ra, t0, t1, t2).
		 */
		"addi sp, sp, -16\n"
		"sw   ra, 12(sp)\n"
		"sw   t0, 8(sp)\n"
		"sw   t1, 4(sp)\n"
		"sw   t2, 0(sp)\n"

		/* Read CSR: mcause, mepc, mtval */
		"csrr t0, mcause\n" /* t0 = mcause */
		"csrr t1, mepc\n"   /* t1 = mepc   */
		"csrr t2, mtval\n"  /* t2 = mtval  */

		/* Prepare arguments for trap_handler function:
		 * a0 = mcause (t0), a1 = mepc (t1), a2 = mtval (t2), a3 = sp (pointer on context).
		 */
		"mv   a0, t0\n"
		"mv   a1, t1\n"
		"mv   a2, t2\n"
		"mv   a3, sp\n"

		"call trap_handler\n"

		/* Restore registers values */
		"lw   ra, 12(sp)\n"
		"lw   t0, 8(sp)\n"
		"lw   t1, 4(sp)\n"
		"lw   t2, 0(sp)\n"
		"addi sp, sp, 16\n"

		"mret\n");
}

void init_trap_handler(void)
{
	/* Write the address of the trap_entry function into the mtvec CSR.
	 * Note: On RV32, the address must be aligned to 4 bytes.
	 */
	uintptr_t trap_entry_addr = (uintptr_t)&trap_entry;

	__asm__ volatile("csrw mtvec, %0\n"
			 : /* no outs */
			 : "r"(trap_entry_addr));
}
#endif

#if defined(CONFIG_ASSERT)
#ifdef CONFIG_ASSERT_NO_FILE_INFO
void assert_post_action(void)
#else
void assert_post_action(const char *file, unsigned int line)
#endif
{
#ifndef CONFIG_ASSERT_NO_FILE_INFO
	ARG_UNUSED(file);
	ARG_UNUSED(line);
#endif

	/* force send trap_handler signal */
	trap_entry();
}
#endif

int main(void)
{
	int ret = 0;

//	init_trap_handler();

	ret = backend_init();
	if (ret < 0) {
		return 0;
	}

//	IRQ_DIRECT_CONNECT(HRT_VEVIF_IDX_READ, HRT_IRQ_PRIORITY, hrt_handler_read, 0);
//	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_READ), true);

//	IRQ_DIRECT_CONNECT(HRT_VEVIF_IDX_WRITE, HRT_IRQ_PRIORITY, hrt_handler_write, 0);
//	nrf_vpr_clic_int_enable_set(NRF_VPRCLIC, VEVIF_IRQN(HRT_VEVIF_IDX_WRITE), true);

//	nrf_vpr_csr_rtperiph_enable_set(true);

	while (true) {
		k_cpu_idle();
	}

	return 0;
}
