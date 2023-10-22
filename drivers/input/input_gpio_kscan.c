/*
 * Copyright (c) 2023 Sophie Tyalie
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_kscan

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_kscan, CONFIG_INPUT_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

struct gpio_kscan_callback {
	uint32_t zephyr_code;
	int8_t pin_state;
};

struct gpio_kscan_key_config {
	/** GPIO specification from devicetree */
	struct gpio_dt_spec input;
	struct gpio_dt_spec output;
	/** Zephyr code from devicetree */
	uint32_t zephyr_code;
};
struct gpio_kscan_config {
	/** Debounce interval in milliseconds from devicetree */
	const int num_keys;
	const struct gpio_kscan_key_config *pin_cfg;
};

struct gpio_kscan_key_data {
	const struct device *dev;
	struct gpio_kscan_callback cb_data;
	struct k_work_delayable work;
	int8_t pin_state;
};

static int gpio_kscan_init(const struct device *dev)
{
	return 0;
}

#define GPIO_KEY_CFG_CHECK(node_id)								\
	BUILD_ASSERT(DT_NODE_HAS_PROP(node_id, zephyr_code),					\
		     "zephyr-code must be specified to use the input-gpio-keys driver");

#define GPIO_KSCAN_CFG_DEF(node_id)								\
	{											\
		.input = GPIO_DT_SPEC_GET(node_id, input_gpios),				\
		.output = GPIO_DT_SPEC_GET(node_id, output_gpios),				\
		.zephyr_code = DT_PROP(node_id, zephyr_code),					\
	}

#define GPIO_KSCAN_INIT(i)									\
	DT_INST_FOREACH_CHILD_STATUS_OKAY(i, GPIO_KEY_CFG_CHECK);				\
	static const struct gpio_kscan_key_config gpio_kscan_key_config_##i[] = {		\
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(i, GPIO_KSCAN_CFG_DEF, (,))};		\
	static const struct gpio_kscan_config gpio_kscan_config_##i = {				\
		.num_keys = ARRAY_SIZE(gpio_kscan_key_config_##i),				\
		.pin_cfg = gpio_kscan_key_config_##i,						\
	};											\
	static struct gpio_kscan_key_data							\
		gpio_kscan_pin_data_##i[ARRAY_SIZE(gpio_kscan_key_config_##i)];			\
	DEVICE_DT_INST_DEFINE(i, &gpio_kscan_init, NULL, gpio_kscan_pin_data_##i,		\
			      &gpio_kscan_config_##i, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,	\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_KSCAN_INIT)


BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one nuvoton,npcx-kbd compatible node can be supported");

