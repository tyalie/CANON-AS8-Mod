/*
 * Copyright (c) 2023 Sophie Tyalie
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os/display.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

#ifdef CONFIG_CHARACTER_FRAMEBUFFER
#include <zephyr/display/cfb.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(as8_os, CONFIG_APP_LOG_LEVEL);

#ifdef CONFIG_LVGL
lv_obj_t* my_screen;

lv_obj_t* get_screen() {
	return my_screen;
}
#endif

int os_display_init()
{
	if (!device_is_ready(display_dev)) {
		LOG_ERR("display %s device is not ready", display_dev->name);
		return -ENODEV;
	}

	if (display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO10)) {
		LOG_ERR("Failed to set required pixel format");
		return -EAGAIN;
	}

#ifdef CONFIG_CHARACTER_FRAMEBUFFER
	if (cfb_framebuffer_init(display_dev)) {
		LOG_ERR("Framebuffer init failed!");
		return -EAGAIN;
	}


	cfb_framebuffer_clear(display_dev, true);
#endif

	display_blanking_off(display_dev);

#ifdef CONFIG_CHARACTER_FRAMEBUFFER
	// because white and black is inverted
	cfb_framebuffer_invert(display_dev);
#endif

#ifdef CONFIG_LVGL
	my_screen = lv_obj_create(lv_scr_act());
	lv_obj_set_size(my_screen, DISPLAY_PROPS.width, DISPLAY_PROPS.height);
	lv_obj_set_pos(my_screen, DISPLAY_PROPS.deltaX, DISPLAY_PROPS.deltaY);
#endif

	return 0;
}

SYS_INIT(os_display_init, APPLICATION, 99);
