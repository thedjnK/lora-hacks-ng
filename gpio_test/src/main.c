/*
 * Copyright (c) 2025 Jamie M.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <app_version.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(gpio_test);

#include "common.h"

struct gpio_ports_t {
	const struct device *device;
	uint32_t pin;
	const char *name;
};

/*
bl652 | bl54l15
------+--------
8     | 0.00
7     | 0.01
6     | 0.02
27    | 0.03
19    | 0.04

9     | 1.02
10    | 1.03
2     | 1.04
3     | 1.05
4     | 1.06
5     | 1.07
17    | 1.08
15    | 1.09
13    | 1.10
28    | 1.11
29    | 1.12
30    | 1.13
31    | 1.14

18    | 2.00
16    | 2.01
//20    | 2.02 (under board)
11    | 2.03
14    | 2.04
12    | 2.05
25    | 2.06
//22    | 2.07 (under board)
23    | 2.08
24    | 2.09
26    | 2.10
*/

#define GPIO_0 DT_NODELABEL(gpio0)
#define GPIO_1 DT_NODELABEL(gpio1)
#define GPIO_2 DT_NODELABEL(gpio2)

struct gpio_ports_t gpio_ports[] = {
	/* p0.xx */
	{
		.device = DEVICE_DT_GET(GPIO_0),
		.pin = 0,
		.name = "p0.00",
	},
	{
		.device = DEVICE_DT_GET(GPIO_0),
		.pin = 1,
		.name = "p0.01",
	},
	{
		.device = DEVICE_DT_GET(GPIO_0),
		.pin = 2,
		.name = "p0.02",
	},
	{
		.device = DEVICE_DT_GET(GPIO_0),
		.pin = 3,
		.name = "p0.03",
	},
	{
		.device = DEVICE_DT_GET(GPIO_0),
		.pin = 4,
		.name = "p0.04",
	},

	/* p1.xx */
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 2,
		.name = "p1.02",
	},
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 3,
		.name = "p1.03",
	},
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 4,
		.name = "p1.04",
	},
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 5,
		.name = "p1.05",
	},
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 6,
		.name = "p1.06",
	},
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 7,
		.name = "p1.07",
	},
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 8,
		.name = "p1.08",
	},
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 9,
		.name = "p1.09",
	},
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 10,
		.name = "p1.10",
	},
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 11,
		.name = "p1.11",
	},
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 12,
		.name = "p1.12",
	},
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 13,
		.name = "p1.13",
	},
	{
		.device = DEVICE_DT_GET(GPIO_1),
		.pin = 14,
		.name = "p1.14",
	},

	/* p2.xx */
	{
		.device = DEVICE_DT_GET(GPIO_2),
		.pin = 0,
		.name = "p2.00",
	},
	{
		.device = DEVICE_DT_GET(GPIO_2),
		.pin = 1,
		.name = "p2.01",
	},
	{
		.device = DEVICE_DT_GET(GPIO_2),
		.pin = 2,
		.name = "p2.02",
	},
	{
		.device = DEVICE_DT_GET(GPIO_2),
		.pin = 3,
		.name = "p2.03",
	},
	{
		.device = DEVICE_DT_GET(GPIO_2),
		.pin = 4,
		.name = "p2.04",
	},
	{
		.device = DEVICE_DT_GET(GPIO_2),
		.pin = 5,
		.name = "p2.05",
	},
	{
		.device = DEVICE_DT_GET(GPIO_2),
		.pin = 6,
		.name = "p2.06",
	},
	{
		.device = DEVICE_DT_GET(GPIO_2),
		.pin = 7,
		.name = "p2.07",
	},
	{
		.device = DEVICE_DT_GET(GPIO_2),
		.pin = 8,
		.name = "p2.08",
	},
	{
		.device = DEVICE_DT_GET(GPIO_2),
		.pin = 9,
		.name = "p2.09",
	},
	{
		.device = DEVICE_DT_GET(GPIO_2),
		.pin = 10,
		.name = "p2.10",
	}
};

static uint8_t last_port = ARRAY_SIZE(gpio_ports);

int main(void)
{
#ifdef CONFIG_MCUMGR_TRANSPORT_BT
	start_smp_bluetooth_adverts();
#endif

	return 0;
}

static void configure_gpio(const struct shell *sh, uint8_t next_port)
{
	int rc;

	if (last_port < ARRAY_SIZE(gpio_ports)) {
		rc = gpio_pin_configure(gpio_ports[last_port].device, gpio_ports[last_port].pin, GPIO_INPUT);

		if (rc) {
			shell_error(sh, "GPIO configuration failed (%s): %d", gpio_ports[last_port].name, rc);
		}
	}

	rc = gpio_pin_configure(gpio_ports[next_port].device, gpio_ports[next_port].pin, (GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH));

	if (rc) {
		shell_error(sh, "GPIO configuration failed (%s): %d", gpio_ports[next_port].name, rc);
	}

	last_port = next_port;
}

static int app_current_handler(const struct shell *sh, size_t argc, char **argv)
{
	if (last_port == ARRAY_SIZE(gpio_ports)) {
		shell_print(sh, "Inactive");
	} else {
		shell_print(sh, "Current GPIO under test: %s", gpio_ports[last_port].name);
	}

	return 0;
}

static int app_next_handler(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t next_port;

	if (last_port >= ARRAY_SIZE(gpio_ports)) {
		next_port = 0;
	} else {
		next_port = last_port + 1;
	}

	configure_gpio(sh, next_port);
	return app_current_handler(sh, argc, argv);
}

static int app_previous_handler(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t next_port;

	if (last_port == ARRAY_SIZE(gpio_ports)) {
		next_port = ARRAY_SIZE(gpio_ports) - 1;
	} else {
		next_port = last_port - 1;
	}

	configure_gpio(sh, next_port);
	return app_current_handler(sh, argc, argv);
}

static int app_version_handler(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "Version: %s", APP_VERSION_TWEAK_STRING);
	return 0;
}

static int app_reboot_handler(const struct shell *sh, size_t argc, char **argv)
{
        sys_reboot(SYS_REBOOT_COLD);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(app_cmd,
	/* Command handlers */
	SHELL_CMD(next, NULL, "Test next GPIO", app_next_handler),
	SHELL_CMD(previous, NULL, "Test previous GPIO", app_previous_handler),
	SHELL_CMD(current, NULL, "Show current GPIO under test", app_current_handler),
	SHELL_CMD(version, NULL, "Show application version", app_version_handler),
	SHELL_CMD(reboot, NULL, "Reboot device", app_reboot_handler),

	/* Array terminator. */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(app, &app_cmd, "App commands", NULL);
