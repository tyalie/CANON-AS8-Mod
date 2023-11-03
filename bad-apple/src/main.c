/*
 * Copyright (c) 2023 Sophie Tyalie
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys_clock.h"
#include <zephyr/sys/util.h>
#include <stdio.h>
#include <sys/_stdint.h>
#include <zephyr/kernel.h>
#include <sys/types.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/input/input.h>

#include <zephyr/logging/log.h>

#include <heatshrink_common.h>
#include <heatshrink_decoder.h>

#include <os/display.h>
#include <zephyr/drivers/display.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

static const off_t storage_offset = FIXED_PARTITION_OFFSET(storage_partition);
static const struct device *flash_device = FIXED_PARTITION_DEVICE(storage_partition);

#define INPUT_BUFFER_SIZE KB(2)

uint8_t input_buffer[INPUT_BUFFER_SIZE];

uint8_t display_buffer[128 * 64/8];
uint8_t output_buffer[ARRAY_SIZE(display_buffer)];
bool pending_reset = false;

void key_callback(struct input_event *evt)
{
	pending_reset = true;
}

INPUT_CALLBACK_DEFINE(NULL, key_callback);


void process_frame(uint8_t *bit_buffer)
{
	struct display_buffer_descriptor desc = {
		.buf_size = sizeof(display_buffer),
		.pitch = 128, .width = 128, .height = 64
	};

	/*
	 * currently pixels are htiled, but need to be vtiled, order is still y * width + x
	 * in order to read each pixel from input do
	 *
	 * fb[x,y] = in[x/8 + width/8*y] & BIT(x%8) >> x%8;
	 *
	 * in order to convert normal framebuffer to output do
	 * out[x + width*(y/8)] & BIT(y%8) >> y%8 = fb[x,y]
	*/

	memset(display_buffer, 0, sizeof(display_buffer));
	for (uint8_t x = 0; x < 128; x++) {
		for (uint8_t y = 0; y < 64; y++) {
			bool px = (bit_buffer[x/8 + 128/8*y] & BIT(7-x%8)) >> (7-x%8);
			display_buffer[x + 128*(y/8)] |= px << y%8;
		}
	}

	display_write(display_dev, 0, 0, &desc, display_buffer);
}

int main()
{
	off_t address = storage_offset;
	heatshrink_decoder *decoder;
	int64_t last_frame_ts = k_uptime_get();
	int err = 0;

	k_sleep(K_SECONDS(1));

	if (!device_is_ready(flash_device)) {
		LOG_ERR("%s: device not ready.", flash_device->name);
		return 0;
	}

	decoder = heatshrink_decoder_alloc(INPUT_BUFFER_SIZE, CONFIG_HEATSHRINK_WINDOW_SZ2, CONFIG_HEATSHRINK_LOOKAHEAD_SZ2);

	size_t out_offset = 0, framecnt = 0, in_idx = 0;
	size_t act_size;

	while (true) {
		if (in_idx >= CONFIG_VIDEO_FILE_SIZE)
			pending_reset = true;

		if (pending_reset) {
			in_idx = out_offset = framecnt = 0;
			heatshrink_decoder_reset(decoder);
			pending_reset = false;
		}

		// 1. read data from flash
		flash_read(flash_device, storage_offset + in_idx, input_buffer, sizeof(input_buffer));
		// 2. decompress data and increase idx accordingly (sink)
		heatshrink_decoder_sink(decoder, input_buffer, sizeof(input_buffer), &act_size);
		in_idx += act_size;

		if (act_size > 0)
			LOG_DBG("new location %u", in_idx);

		// 3. display data (poll)
		heatshrink_decoder_poll(decoder, output_buffer + out_offset, sizeof(output_buffer) - out_offset, &act_size);
		out_offset = (out_offset + act_size) % sizeof(output_buffer);

		if (out_offset == 0) {
			k_timeout_t new_timeout = K_TIMEOUT_ABS_MS(last_frame_ts + 1000/30 + 4);
			last_frame_ts = k_uptime_get();
			k_sleep(new_timeout);
			process_frame(output_buffer);
			framecnt++;
		}

		if (framecnt % 10 == 0)
			LOG_DBG("Cur frame: %u", framecnt);
	}

	return -err;
}
