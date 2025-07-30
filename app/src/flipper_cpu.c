/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_vpr.h>

#define FLPR_SECURE_MODE

#if defined(FLPR_SECURE_MODE) && !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#include <hal/nrf_spu.h>
#endif

static __aligned(0x800) __used __attribute__((section("._flpr_code"))) const uint8_t flpr_firmware[] = {
#include <generated/remote.inc>
};

LOG_MODULE_REGISTER(flpr_cpu, 4);

static int flipper_cpu_launch(void)
{
	NRF_VPR_Type *flpr_cpu = (NRF_VPR_Type *)DT_REG_ADDR(DT_NODELABEL(cpuflpr_vpr));

#if defined(FLPR_SECURE_MODE)
	nrf_spu_periph_perm_secattr_set(NRF_SPU00, nrf_address_slave_get((uint32_t)flpr_cpu),
					true);
#endif

	nrf_vpr_initpc_set(flpr_cpu, (uint32_t)flpr_firmware);
	nrf_vpr_cpurun_set(flpr_cpu, true);

	return 0;
}

SYS_INIT(flipper_cpu_launch, POST_KERNEL, 0);
