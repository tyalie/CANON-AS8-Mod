/*
 * Copyright (c) 2023 Sophie Tyalie
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os/display.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(as8_os, CONFIG_APP_LOG_LEVEL);

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

	if (cfb_framebuffer_init(display_dev)) {
		LOG_ERR("Framebuffer init failed!");
		return -EAGAIN;
	}


	cfb_framebuffer_clear(display_dev, true);

	display_blanking_off(display_dev);

	// because white and black is inverted
	cfb_framebuffer_invert(display_dev);

	return 0;
}

SYS_INIT(os_display_init, APPLICATION, 20);
