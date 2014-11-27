#include "colorcorrection.h"

/* These stubs are used if weston is built without LCMS support */

int
weston_create_cms_interface(struct wl_display *display,
			    struct weston_compositor *ec)
{
	return 0;
}

int
weston_create_default_colorspaces(struct weston_compositor *ec)
{
	return 1;
}

void
weston_destroy_default_colorspaces(struct weston_compositor *ec)
{
}

void
weston_colorspace_destroy(struct weston_colorspace *colorspace, int force)
{
}

struct weston_colorspace *
weston_colorspace_from_fd(int fd, int input, struct weston_compositor *ec)
{
	return NULL;
}
