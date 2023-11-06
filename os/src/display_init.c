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

struct {
	lv_style_t pad_normal;
} my_theme_styles;

/**
 * The lvgl theme callback that is always run when an object
 * is in need for a bit of spice.
 */
static void new_theme_apply_cb(lv_theme_t *th, lv_obj_t *obj)
{
	LV_UNUSED(th);

	if(lv_obj_get_parent(obj) == NULL)
		return;

	if(lv_obj_check_type(obj, &lv_obj_class)) {
		lv_obj_add_style(obj, &my_theme_styles.pad_normal, 0);
	}
}

/**
* Custom lvgl theme builder
*
* Our screen is verry tiny, so there are a few things were the default
* themes assumes abilities or space that we don't have. Additionally there
* style choice we don't prefer.
*
* This method initializes the themes and set's up our lvgl theme callback.
*/
void lvgl_update_theme()
{
	// see `lv_theme_default.c` for inspiration

	// setup style
	lv_style_reset(&my_theme_styles.pad_normal);
	lv_style_set_pad_all(&my_theme_styles.pad_normal, 3);

	// set theme
	static lv_theme_t new_theme;
	lv_theme_t *act_theme = lv_disp_get_theme(NULL);
	new_theme = *act_theme;
	lv_theme_set_parent(&new_theme, act_theme);
	lv_theme_set_apply_cb(&new_theme, new_theme_apply_cb);

	lv_disp_set_theme(NULL, &new_theme);

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
	lvgl_update_theme();
	// initialize virtual screen which respects visible screen
	// offset and size
	static lv_style_t style;
	lv_style_init(&style);
	lv_style_set_pad_all(&style, 0);  // remove padding

	my_screen = lv_obj_create(lv_scr_act());
	lv_obj_add_style(my_screen, &style, 0);
	lv_obj_set_align(my_screen, LV_ALIGN_TOP_LEFT);
	lv_obj_set_size(my_screen, DISPLAY_PROPS.width, DISPLAY_PROPS.height);
	lv_obj_set_pos(my_screen, DISPLAY_PROPS.deltaX, DISPLAY_PROPS.deltaY);
#endif

	return 0;
}

SYS_INIT(os_display_init, APPLICATION, 99);
