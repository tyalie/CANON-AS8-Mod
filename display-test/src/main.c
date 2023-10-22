/*
 * Copyright (c) 2023 Sophie Tyalie
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gpio_reg.h"
#include "zephyr/dt-bindings/gpio/gpio.h"
#include "zephyr/sys/util.h"
#include <stdio.h>
#include <sys/_stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <hal/gpio_hal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

static const struct device *dev =
  DEVICE_DT_GET(DT_PROP(DT_PATH(zephyr_user), display));

static const uint8_t disp_dimension[2] = DT_PROP(DT_PATH(zephyr_user), real_display_dims);
static const uint8_t disp_offset[2] = DT_PROP(DT_PATH(zephyr_user), real_display_offsets);


void display_init()
{
  if (!device_is_ready(dev)) {
    LOG_ERR("display not ready");
    return;
  }

  if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10)) {
    LOG_ERR("Failed to set required pixel format");
    return;
  }

  if (cfb_framebuffer_init(dev)) {
    LOG_ERR("Framebuffer init failed!");
    return;
  }

  cfb_framebuffer_clear(dev, true);

  display_blanking_off(dev);

  cfb_framebuffer_invert(dev);
}

int main(void)
{
  //k_sleep(K_SECONDS(2));

  display_init();

  struct cfb_position start = {0,0}, end;
  uint8_t width, height;
  size_t b = 0;
  char text[10];

  width = disp_dimension[0];
  height = disp_dimension[1];

  int8_t deltaX = 4, deltaY = 5;

  while (1) {
    start.x += deltaX;
    start.y += deltaY;

    end.x = start.x + 10;
    end.y = start.y + 10;

    if ((start.x + deltaX) < 0 || (end.x + deltaX) >= width)
      deltaX *= -1;

    if ((start.y + deltaY) < 0 || (end.y + deltaY) >= height)
      deltaY *= -1;


    sprintf(text, "B: %lu", b/10);

    cfb_framebuffer_clear(dev, false);
    cfb_draw_text(dev, text, 0,0);
    cfb_draw_rect(dev, &start, &end);
    cfb_framebuffer_finalize(dev);

    k_sleep(K_MSEC(10));

    if (b % 10)
      display_set_contrast(dev, b/10);
    b = (b + 1) % (128 * 10);
  }

  return 0;
}
