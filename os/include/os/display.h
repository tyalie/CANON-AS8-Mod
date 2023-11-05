/*
 * Copyright (c) 2023 Sophie Tyalie
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include <sys/_stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>

static const struct device *display_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

union display_properties {
	struct {
		uint8_t width;
		uint8_t height;
		uint8_t deltaX;
		uint8_t deltaY;
	};
	struct {
		uint8_t dimensions[2];
		uint8_t offset[2];
	} raw;
};

const static union display_properties DISPLAY_PROPS = {.raw = {
	DT_PROP(DT_PATH(zephyr_user), real_display_dims),
	DT_PROP(DT_PATH(zephyr_user), real_display_offsets)}
};

