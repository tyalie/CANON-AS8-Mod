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
	const int num_keys;
	const struct gpio_kscan_key_config *key_cfg;
};

struct gpio_kscan_key_data {
	int8_t pin_state;
};

struct gpio_kscan_data {
	const struct device *dev;
	struct gpio_kscan_key_data *key_data;
	struct k_work_delayable work;
};

static void gpio_kscan_read_matrix(const struct device *dev)
{
	struct gpio_kscan_data *data = dev->data;
	const struct gpio_kscan_config *cfg = dev->config;
	int new_pressed;

	for (int i = 0; i < cfg->num_keys; i++) {
		const struct gpio_kscan_key_config *key_cfg = &cfg->key_cfg[i];
		struct gpio_kscan_key_data *key_data = &data->key_data[i];

		gpio_pin_configure_dt(&key_cfg->input, GPIO_INPUT);
		gpio_pin_configure_dt(&key_cfg->output, GPIO_OUTPUT);

		gpio_pin_set_dt(&key_cfg->output, 1);

		new_pressed = gpio_pin_get_dt(&key_cfg->input);
		if (new_pressed != key_data->pin_state) {
			key_data->pin_state = new_pressed;
			LOG_DBG("Report event %s %d, code=%d", dev->name, new_pressed, key_cfg->zephyr_code);
			input_report_key(dev, key_cfg->zephyr_code, new_pressed, true, K_FOREVER);
		}

		gpio_pin_set_dt(&key_cfg->output, 0);
	}

	k_work_reschedule(&data->work, K_MSEC(10));
}

static void gpio_kscan_work_handler(struct k_work *work)
{
    struct k_work_delayable *dwork = CONTAINER_OF(work, struct k_work_delayable, work);
    struct gpio_kscan_data *data = CONTAINER_OF(dwork, struct gpio_kscan_data, work);
    gpio_kscan_read_matrix(data->dev);
}

static int gpio_kscan_init(const struct device *dev)
{
	struct gpio_kscan_data *data = dev->data;
	const struct gpio_kscan_config *cfg = dev->config;

	data->dev = dev;

	// check if used gpio systems are available
	for (int i = 0; i < cfg->num_keys; i++) {
		const struct gpio_kscan_key_config *key_cfg = &cfg->key_cfg[i];

		if (!gpio_is_ready_dt(&key_cfg->input)) {
			LOG_ERR("in: %s is not ready", key_cfg->input.port->name);
			return -ENODEV;
		}

		if (!gpio_is_ready_dt(&key_cfg->output)) {
			LOG_ERR("out: %s is not ready", key_cfg->output.port->name);
			return -ENODEV;
		}
	}

	// setup work
	k_work_init_delayable(&data->work, gpio_kscan_work_handler);
	gpio_kscan_read_matrix(dev);

	return 0;
}

#define GPIO_KSCAN_CFG_DEF(node_id)								\
	{											\
		.input = GPIO_DT_SPEC_GET(node_id, input_gpios),				\
		.output = GPIO_DT_SPEC_GET(node_id, output_gpios),				\
		.zephyr_code = DT_PROP(node_id, zephyr_code),					\
	}

#define GPIO_KSCAN_INIT(i)									\
	static const struct gpio_kscan_key_config gpio_kscan_key_config_##i[] = {		\
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(i, GPIO_KSCAN_CFG_DEF, (,))};		\
	static const struct gpio_kscan_config gpio_kscan_config_##i = {				\
		.num_keys = ARRAY_SIZE(gpio_kscan_key_config_##i),				\
		.key_cfg = gpio_kscan_key_config_##i,						\
	};											\
	static struct gpio_kscan_key_data							\
		gpio_kscan_key_data_##i[ARRAY_SIZE(gpio_kscan_key_config_##i)];			\
	static struct gpio_kscan_data gpio_kscan_data_##i = {					\
		.key_data = gpio_kscan_key_data_##i,						\
	};											\
	DEVICE_DT_INST_DEFINE(i, &gpio_kscan_init, NULL, &gpio_kscan_data_##i,			\
			      &gpio_kscan_config_##i, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,	\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_KSCAN_INIT)


BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "only one nuvoton,npcx-kbd compatible node can be supported");

