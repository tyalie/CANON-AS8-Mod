/*
 * Copyright (c) 2023 Sophie Tyalie
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/display/cfb.h>
#include <zephyr/input/input.h>

#include <os/display.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

static uint32_t button_hit = 0;

void key_callback(struct input_event *evt)
{
  if (evt->value)
    button_hit++;
}

INPUT_CALLBACK_DEFINE(NULL, key_callback);


int main(void)
{
  struct cfb_position start = {0,0}, end;
  size_t b = 0;
  char text[30];

  int8_t deltaX = 1, deltaY = 1;

  while (1) {
    start.x += deltaX;
    start.y += deltaY;

    end.x = start.x + 10;
    end.y = start.y + 10;

    if ((start.x + deltaX) < 0 || (end.x + deltaX) >= DISPLAY_PROPS.width)
      deltaX *= -1;

    if ((start.y + deltaY) < 0 || (end.y + deltaY) >= DISPLAY_PROPS.height)
      deltaY *= -1;


    sprintf(text, "B[%u] | b[%d]", b/10, button_hit);

    cfb_framebuffer_clear(display_dev, false);
    cfb_draw_text(display_dev, text, 0,0);
    cfb_invert_area(display_dev, start.x, start.y, end.x-start.x, end.y-start.y);
    cfb_framebuffer_finalize(display_dev);

    k_sleep(K_MSEC(10));

    if (b % 10)
      display_set_contrast(display_dev, b/10);
    b = (b + 1) % (128 * 10);
  }

  return 0;
}
