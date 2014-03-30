/*
 * Copyright © 2008 Kristian Høgsberg
 * Copyright © 2009 Chris Wilson
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <cairo.h>
#include <assert.h>
#include <linux/input.h>
#include <sys/stat.h>

#include <wayland-client.h>

#include "window.h"
#include "../shared/cairo-util.h"

#include "cms-client-protocol.h"

const unsigned char icc_profile[798] =
{
    0x00, 0x00, 0x03, 0x1e,
    0x6c, 0x63, 0x6d, 0x73,
    0x02, 0x30, 0x00, 0x00,
    0x6d, 0x6e, 0x74, 0x72,
    0x52, 0x47, 0x42, 0x20,
    0x58, 0x59, 0x5a, 0x20,
    0x07, 0xd8, 0x00, 0x05,
    0x00, 0x19, 0x00, 0x05,
    0x00, 0x34, 0x00, 0x33,
    0x61, 0x63, 0x73, 0x70,
    0x41, 0x50, 0x50, 0x4c,
    0x00, 0x00, 0x00, 0x00,
    0x6c, 0x63, 0x6d, 0x73,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xf6, 0xd6,
    0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0xd3, 0x2d,
    0x6c, 0x63, 0x6d, 0x73,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0c,
    0x64, 0x6d, 0x6e, 0x64,
    0x00, 0x00, 0x01, 0x14,
    0x00, 0x00, 0x00, 0x6a,
    0x64, 0x65, 0x73, 0x63,
    0x00, 0x00, 0x01, 0x80,
    0x00, 0x00, 0x00, 0x66,
    0x64, 0x6d, 0x64, 0x64,
    0x00, 0x00, 0x01, 0xe8,
    0x00, 0x00, 0x00, 0x67,
    0x77, 0x74, 0x70, 0x74,
    0x00, 0x00, 0x02, 0x50,
    0x00, 0x00, 0x00, 0x14,
    0x72, 0x58, 0x59, 0x5a,
    0x00, 0x00, 0x02, 0x64,
    0x00, 0x00, 0x00, 0x14,
    0x62, 0x58, 0x59, 0x5a,
    0x00, 0x00, 0x02, 0x78,
    0x00, 0x00, 0x00, 0x14,
    0x67, 0x58, 0x59, 0x5a,
    0x00, 0x00, 0x02, 0x8c,
    0x00, 0x00, 0x00, 0x14,
    0x72, 0x54, 0x52, 0x43,
    0x00, 0x00, 0x02, 0xa0,
    0x00, 0x00, 0x00, 0x0e,
    0x67, 0x54, 0x52, 0x43,
    0x00, 0x00, 0x02, 0xb0,
    0x00, 0x00, 0x00, 0x0e,
    0x62, 0x54, 0x52, 0x43,
    0x00, 0x00, 0x02, 0xc0,
    0x00, 0x00, 0x00, 0x0e,
    0x63, 0x68, 0x72, 0x6d,
    0x00, 0x00, 0x02, 0xd0,
    0x00, 0x00, 0x00, 0x24,
    0x63, 0x70, 0x72, 0x74,
    0x00, 0x00, 0x02, 0xf4,
    0x00, 0x00, 0x00, 0x2a,
    0x64, 0x65, 0x73, 0x63,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10,
    0x28, 0x6c, 0x63, 0x6d,
    0x73, 0x20, 0x69, 0x6e,
    0x74, 0x65, 0x72, 0x6e,
    0x61, 0x6c, 0x29, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x64, 0x65, 0x73, 0x63,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0c,
    0x66, 0x61, 0x6b, 0x65,
    0x42, 0x52, 0x47, 0x2e,
    0x69, 0x63, 0x63, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x64, 0x65, 0x73, 0x63,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0d,
    0x72, 0x67, 0x62, 0x20,
    0x62, 0x75, 0x69, 0x6c,
    0x74, 0x2d, 0x69, 0x6e,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x58, 0x59, 0x5a, 0x20,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xf3, 0x4f,
    0x00, 0x01, 0x00, 0x00,
    0x00, 0x01, 0x16, 0xc2,
    0x58, 0x59, 0x5a, 0x20,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x24, 0x9e,
    0x00, 0x00, 0x0f, 0x84,
    0x00, 0x00, 0xb6, 0xc2,
    0x58, 0x59, 0x5a, 0x20,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x62, 0x97,
    0x00, 0x00, 0xb7, 0x88,
    0x00, 0x00, 0x18, 0xda,
    0x58, 0x59, 0x5a, 0x20,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x6f, 0xa0,
    0x00, 0x00, 0x38, 0xf5,
    0x00, 0x00, 0x03, 0x90,
    0x63, 0x75, 0x72, 0x76,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01,
    0x02, 0x33, 0x00, 0x00,
    0x63, 0x75, 0x72, 0x76,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01,
    0x02, 0x33, 0x00, 0x00,
    0x63, 0x75, 0x72, 0x76,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01,
    0x02, 0x33, 0x00, 0x00,
    0x63, 0x68, 0x72, 0x6d,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x03, 0x00, 0x00,
    0x00, 0x00, 0x26, 0x66,
    0x00, 0x00, 0x0f, 0x5c,
    0x00, 0x00, 0xa3, 0xd7,
    0x00, 0x00, 0x54, 0x7b,
    0x00, 0x00, 0x4c, 0xcd,
    0x00, 0x00, 0x99, 0x9a,
    0x74, 0x65, 0x78, 0x74,
    0x00, 0x00, 0x00, 0x00,
    0x32, 0x30, 0x30, 0x38,
    0x20, 0x4b, 0x61, 0x69,
    0x2d, 0x55, 0x77, 0x65,
    0x20, 0x42, 0x65, 0x68,
    0x72, 0x6d, 0x61, 0x6e,
    0x6e, 0x2c, 0x20, 0x75,
    0x73, 0x65, 0x20, 0x66,
    0x72, 0x65, 0x65, 0x6c,
    0x79, 0x00
};

struct image {
	struct window *window;
	struct widget *widget;
	struct display *display;
	char *filename;
	cairo_surface_t *image;
	int fullscreen;
	int *image_counter;
	int32_t width, height;

	struct {
		double x;
		double y;
	} pointer;
	bool button_pressed;

	bool initialized;
	cairo_matrix_t matrix;

	int color_mode;
	struct wl_cms *cms;
	struct wl_cms_colorspace *profile_colorspace;
	struct wl_cms_colorspace *srgb_colorspace;
	struct wl_cms_colorspace *blending_colorspace;
};

static double
get_scale(struct image *image)
{
	assert(image->matrix.xy == 0.0 &&
	       image->matrix.yx == 0.0 &&
	       image->matrix.xx == image->matrix.yy);
	return image->matrix.xx;
}

static void
clamp_view(struct image *image)
{
	struct rectangle allocation;
	double scale = get_scale(image);
	double sw, sh;

	sw = image->width * scale;
	sh = image->height * scale;
	widget_get_allocation(image->widget, &allocation);

	if (sw < allocation.width) {
		image->matrix.x0 =
			(allocation.width - image->width * scale) / 2;
	} else {
		if (image->matrix.x0 > 0.0)
			image->matrix.x0 = 0.0;
		if (sw + image->matrix.x0 < allocation.width)
			image->matrix.x0 = allocation.width - sw;
	}

	if (sh < allocation.width) {
		image->matrix.y0 =
			(allocation.height - image->height * scale) / 2;
	} else {
		if (image->matrix.y0 > 0.0)
			image->matrix.y0 = 0.0;
		if (sh + image->matrix.y0 < allocation.height)
			image->matrix.y0 = allocation.height - sh;
	}
}

static void
redraw_handler(struct widget *widget, void *data)
{
	struct image *image = data;
	struct rectangle allocation;
	cairo_t *cr;
	cairo_surface_t *surface;
	double width, height, doc_aspect, window_aspect, scale;
	cairo_matrix_t matrix;
	cairo_matrix_t translate;

	surface = window_get_surface(image->window);
	cr = cairo_create(surface);
	widget_get_allocation(image->widget, &allocation);
	cairo_rectangle(cr, allocation.x, allocation.y,
			allocation.width, allocation.height);
	cairo_clip(cr);
	cairo_push_group(cr);
	cairo_translate(cr, allocation.x, allocation.y);

	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_paint(cr);

	if (!image->initialized) {
		image->initialized = true;
		width = cairo_image_surface_get_width(image->image);
		height = cairo_image_surface_get_height(image->image);

		doc_aspect = width / height;
		window_aspect = (double) allocation.width / allocation.height;
		if (doc_aspect < window_aspect)
			scale = allocation.height / height;
		else
			scale = allocation.width / width;

		image->width = width;
		image->height = height;
		cairo_matrix_init_scale(&image->matrix, scale, scale);

		clamp_view(image);
	}

	matrix = image->matrix;
	cairo_matrix_init_translate(&translate, allocation.x, allocation.y);
	cairo_matrix_multiply(&matrix, &matrix, &translate);
	cairo_set_matrix(cr, &matrix);

	cairo_set_source_surface(cr, image->image, 0, 0);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_paint(cr);

	cairo_pop_group_to_source(cr);
	cairo_paint(cr);
	cairo_destroy(cr);

	cairo_surface_destroy(surface);
}

static void
resize_handler(struct widget *widget,
	       int32_t width, int32_t height, void *data)
{
	struct image *image = data;

	clamp_view(image);
}

static void
keyboard_focus_handler(struct window *window,
		       struct input *device, void *data)
{
	struct image *image = data;

	window_schedule_redraw(image->window);
}

static int
enter_handler(struct widget *widget,
	      struct input *input,
	      float x, float y, void *data)
{
	struct image *image = data;
	struct rectangle allocation;

	widget_get_allocation(image->widget, &allocation);
	x -= allocation.x;
	y -= allocation.y;

	image->pointer.x = x;
	image->pointer.y = y;

	return 1;
}

static void
move_viewport(struct image *image, double dx, double dy)
{
	double scale = get_scale(image);

	if (!image->initialized)
		return;

	cairo_matrix_translate(&image->matrix, -dx/scale, -dy/scale);
	clamp_view(image);

	window_schedule_redraw(image->window);
}

static int
motion_handler(struct widget *widget,
	       struct input *input, uint32_t time,
	       float x, float y, void *data)
{
	struct image *image = data;
	struct rectangle allocation;

	widget_get_allocation(image->widget, &allocation);
	x -= allocation.x;
	y -= allocation.y;

	if (image->button_pressed)
		move_viewport(image, image->pointer.x - x,
			      image->pointer.y - y);

	image->pointer.x = x;
	image->pointer.y = y;

	return image->button_pressed ? CURSOR_DRAGGING : CURSOR_LEFT_PTR;
}

static void
button_handler(struct widget *widget,
	       struct input *input, uint32_t time,
	       uint32_t button,
	       enum wl_pointer_button_state state,
	       void *data)
{
	struct image *image = data;

	if (button == BTN_LEFT) {
		image->button_pressed =
			state == WL_POINTER_BUTTON_STATE_PRESSED;

		if (state == WL_POINTER_BUTTON_STATE_PRESSED)
			input_set_pointer_image(input, CURSOR_DRAGGING);
		else
			input_set_pointer_image(input, CURSOR_LEFT_PTR);
	}
}

static void
zoom(struct image *image, double scale)
{
	double x = image->pointer.x;
	double y = image->pointer.y;
	cairo_matrix_t scale_matrix;

	if (!image->initialized)
		return;

	if (get_scale(image) * scale > 20.0 ||
	    get_scale(image) * scale < 0.02)
		return;

	cairo_matrix_init_identity(&scale_matrix);
	cairo_matrix_translate(&scale_matrix, x, y);
	cairo_matrix_scale(&scale_matrix, scale, scale);
	cairo_matrix_translate(&scale_matrix, -x, -y);

	cairo_matrix_multiply(&image->matrix, &image->matrix, &scale_matrix);
	clamp_view(image);
}

static void
key_handler(struct window *window, struct input *input, uint32_t time,
	    uint32_t key, uint32_t sym, enum wl_keyboard_key_state state,
	    void *data)
{
	struct image *image = data;

	if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
		return;

	switch (sym) {
	case XKB_KEY_minus:
		zoom(image, 0.8);
		window_schedule_redraw(image->window);
		break;
	case XKB_KEY_equal:
	case XKB_KEY_plus:
		zoom(image, 1.2);
		window_schedule_redraw(image->window);
		break;
	case XKB_KEY_1:
		image->matrix.xx = 1.0;
		image->matrix.xy = 0.0;
		image->matrix.yx = 0.0;
		image->matrix.yy = 1.0;
		clamp_view(image);
		window_schedule_redraw(image->window);
		break;
	case XKB_KEY_C:
		if (image->cms) {
			struct wl_surface *surface;
			surface = window_get_wl_surface(image->window);

			image->color_mode = (image->color_mode + 1) % 3;
			switch (image->color_mode) {
			case 0:
				wl_cms_set_colorspace(image->cms, surface,
					image->srgb_colorspace);
				break;
			case 1:
				wl_cms_set_colorspace(image->cms, surface,
					image->blending_colorspace);
				break;
			case 2:
				if (image->profile_colorspace)
					wl_cms_set_colorspace(image->cms,
						surface,
						image->profile_colorspace);
				break;
			}

			window_schedule_redraw(image->window);
		}
		break;
	}
}

static void
axis_handler(struct widget *widget, struct input *input, uint32_t time,
	     uint32_t axis, wl_fixed_t value, void *data)
{
	struct image *image = data;

	if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL &&
	    input_get_modifiers(input) == MOD_CONTROL_MASK) {
		/* set zoom level to 2% per 10 axis units */
		zoom(image, (1.0 - wl_fixed_to_double(value) / 500.0));

		window_schedule_redraw(image->window);
	} else if (input_get_modifiers(input) == 0) {
		if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
			move_viewport(image, 0, wl_fixed_to_double(value));
		else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
			move_viewport(image, wl_fixed_to_double(value), 0);
	}
}

static void
fullscreen_handler(struct window *window, void *data)
{
	struct image *image = data;

	image->fullscreen ^= 1;
	window_set_fullscreen(window, image->fullscreen);
}

static void
close_handler(void *data)
{
	struct image *image = data;

	*image->image_counter -= 1;

	if (*image->image_counter == 0)
		display_exit(image->display);

	widget_destroy(image->widget);
	window_destroy(image->window);

	free(image);
}

static void
global_handler(struct display *display, uint32_t name,
	       const char *interface, uint32_t version, void *data)
{
	struct image *image = data;

	if (strcmp(interface, "wl_cms") == 0) {
		image->cms =
			display_bind(display, name,
				     &wl_cms_interface, 1);
	}
}

static struct image *
image_create(struct display *display, const char *filename,
	     int *image_counter)
{
	struct image *image;
	char *b, *copy, title[512];;
	int fd;
	FILE *profilefile;
	char template[24];

	image = zalloc(sizeof *image);
	if (image == NULL)
		return image;

	copy = strdup(filename);
	b = basename(copy);
	snprintf(title, sizeof title, "Wayland Image - %s", b);
	free(copy);

	image->filename = strdup(filename);
	image->image = load_cairo_surface(filename);

	if (!image->image) {
		free(image->filename);
		free(image);
		return NULL;
	}

	image->window = window_create(display);
	image->widget = window_frame_create(image->window, image);
	window_set_title(image->window, title);
	image->display = display;
	image->image_counter = image_counter;
	*image_counter += 1;
	image->initialized = false;

	window_set_user_data(image->window, image);
	widget_set_redraw_handler(image->widget, redraw_handler);
	widget_set_resize_handler(image->widget, resize_handler);
	window_set_keyboard_focus_handler(image->window,
					  keyboard_focus_handler);
	window_set_fullscreen_handler(image->window, fullscreen_handler);
	window_set_close_handler(image->window, close_handler);

	widget_set_enter_handler(image->widget, enter_handler);
	widget_set_motion_handler(image->widget, motion_handler);
	widget_set_button_handler(image->widget, button_handler);
	widget_set_axis_handler(image->widget, axis_handler);
	window_set_key_handler(image->window, key_handler);
	widget_schedule_resize(image->widget, 500, 400);

	display_set_user_data(display, image);
	display_set_global_handler(display, global_handler);

	if (image->cms) {
		image->color_mode = 0;
		image->srgb_colorspace = wl_cms_srgb_colorspace(image->cms);
		image->blending_colorspace =
			wl_cms_blending_colorspace(image->cms);

		image->profile_colorspace = NULL;
		strncpy(template, "/tmp/weston-cms-XXXXXX", sizeof(template));
		fd = mkstemp(template);
		if (fd == -1)
			return image;
		unlink(template);
		profilefile = fdopen(fd, "w");
		if (!profilefile)
			return image;

		if (fwrite(icc_profile, sizeof(icc_profile), 1, profilefile) != 1) {
			close(fd);
			return image;
		}
		rewind(profilefile);
		image->profile_colorspace =
			wl_cms_colorspace_from_fd(image->cms, fd);
		close(fd);
	}

	return image;
}

int
main(int argc, char *argv[])
{
	struct display *d;
	int i;
	int image_counter = 0;

	if (argc <= 1 || argv[1][0]=='-') {
		printf("Usage: %s image...\n", argv[0]);
		return 1;
	}

	d = display_create(&argc, argv);
	if (d == NULL) {
		fprintf(stderr, "failed to create display: %m\n");
		return -1;
	}

	for (i = 1; i < argc; i++)
		image_create(d, argv[i], &image_counter);

	if (image_counter > 0)
		display_run(d);

	display_destroy(d);

	return 0;
}
