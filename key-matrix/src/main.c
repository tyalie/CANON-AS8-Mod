/*
 * Copyright (c) 2023 Sophie Tyalie
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gpio_reg.h"
#include "zephyr/dt-bindings/gpio/gpio.h"
#include "zephyr/sys/util.h"
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <hal/gpio_hal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

static const struct gpio_dt_spec keyboard_pins[] = {
  DT_FOREACH_PROP_ELEM_SEP(
    DT_PATH(zephyr_user), keyboard_gpios,
    GPIO_DT_SPEC_GET_BY_IDX, (,)
  )
};

int main(void)
{
  printf("Zephyr Example Application\n");

  int ret;
  volatile size_t pin = 7;


  for (size_t i = 0; i < ARRAY_SIZE(keyboard_pins); i++)
    gpio_pin_configure_dt(&keyboard_pins[i], GPIO_DISCONNECTED);

  while (1) {
    gpio_pin_configure_dt(&keyboard_pins[pin], GPIO_INPUT | GPIO_PULL_DOWN);

    for (size_t i = 0; i < ARRAY_SIZE(keyboard_pins); i++) {
      if (i == pin)
        continue;

      gpio_pin_configure_dt(&keyboard_pins[i], GPIO_OUTPUT_HIGH);

      ret = gpio_pin_get_dt(&keyboard_pins[pin]);
      if (ret == 1)
        printf("found match: pin %zu | output %zu\n", pin, i);
      else if (ret < 0)
        printf("Readout return was: %d", ret);

      gpio_pin_configure_dt(&keyboard_pins[i], GPIO_DISCONNECTED);
    }

    gpio_pin_configure_dt(&keyboard_pins[pin], GPIO_DISCONNECTED);

    pin = ((pin+1) % ARRAY_SIZE(keyboard_pins));

    if (pin == 0) {
      k_sleep(K_MSEC(100));
      printf("\033[2J-- iteration complete --\n\n");
    }
  }

  return 0;
}
