/*
 * Copyright © 2014 Niels Ole Salscheider
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _WESTON_COLORCORRECTION_H_
#define _WESTON_COLORCORRECTION_H_

#include "compositor.h"
#include "cms-server-protocol.h"

#ifdef HAVE_LCMS
#include <lcms2.h>
#endif

struct weston_clut {
	unsigned points;
	char *data;
};

struct weston_colorspace {
	struct wl_list link;
	struct weston_compositor *compositor;

	int refcounted;
	int refcount;
	int input;

#ifdef HAVE_LCMS
	cmsHPROFILE lcms_handle;
#endif
	struct weston_clut clut;
};

int
weston_create_cms_interface(struct wl_display *display,
			    struct weston_compositor *ec);

int
weston_create_default_colorspaces(struct weston_compositor *ec);

void
weston_colorspace_destroy(struct weston_colorspace *colorspace, int force);

struct weston_colorspace *
weston_colorspace_from_fd(int fd, int input, struct weston_compositor *ec);

#endif
