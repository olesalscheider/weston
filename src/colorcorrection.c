#include "colorcorrection.h"

#include <string.h>
#include <unistd.h>

static void
cms_colorspace_destroy(struct wl_client *client,
		       struct wl_resource *cms_colorspace)
{
	wl_resource_destroy(cms_colorspace);
}

static void
cms_colorspace_get_profile_fd(struct wl_client *client,
			      struct wl_resource *cms_colorspace)
{
	struct weston_colorspace *colorspace =
		wl_resource_get_user_data(cms_colorspace);
	char template[24];
	FILE *profilefile;
	int fd = -1;

	strncpy(template, "/tmp/weston-cms-XXXXXX", 24);
	fd = mkstemp(template);
	profilefile = fdopen(fd, "w");
	cmsSaveProfileToStream(colorspace->lcms_handle, profilefile);
	rewind(profilefile);

	wl_cms_colorspace_send_profile_data(cms_colorspace, fd);

	close(fd);
}

static const struct wl_cms_colorspace_interface cms_colorspace_interface = {
	cms_colorspace_destroy,
	cms_colorspace_get_profile_fd
};

static void
weston_free_clut(struct weston_clut *clut)
{
	clut->points = 0;
	free(clut->data);
	clut->data = NULL;
}

static int
weston_build_clut(struct weston_colorspace *colorspace)
{
	cmsHTRANSFORM xform;
	unsigned r, g, b;
	cmsHPROFILE input_profile, output_profile;
	cmsUInt8Number input_id[16];
	cmsUInt8Number output_id[16];
	struct weston_clut *clut = &colorspace->clut;
	struct weston_compositor *ec = colorspace->compositor;

	weston_free_clut(clut);

	if (colorspace->input) {
		input_profile = colorspace->lcms_handle;
		output_profile = ec->blending_colorspace->lcms_handle;
	} else {
		input_profile = ec->blending_colorspace->lcms_handle;
		output_profile = colorspace->lcms_handle;
	}

	/* Check if the input and output profiles are the same. We don't need a
	   LUT if they are. */
	cmsGetHeaderProfileID(input_profile, input_id);
	cmsGetHeaderProfileID(output_profile, output_id);
	if (memcmp(input_id, output_id, 16) == 0)
		return 1;

	/* Use the default number of grid points for now. We might want to
	   adjust this based on the profiles at some point. */
	clut->points = CLUT_DEFAULT_GRID_POINTS;

	xform = cmsCreateTransform(input_profile, TYPE_RGB_8, output_profile,
				   TYPE_RGB_8, INTENT_PERCEPTUAL, 0);
	if (!xform)
		return 0;

	clut->data = malloc(clut->points * clut->points * clut->points *
			    3 * sizeof(char));
	if (!clut->data) {
		cmsDeleteTransform(xform);
		return 0;
	}

	for (b = 0; b < clut->points; b++)
		for (g = 0; g < clut->points; g++)
			for (r = 0; r < clut->points; r++) {
				char in[3];
				unsigned entry = 3 * (r + clut->points *
						 (g + clut->points * b));
				const float step = 255.0 / (clut->points - 1);
				in[0] = (char) (r * step + 0.5);
				in[1] = (char) (g * step + 0.5);
				in[2] = (char) (b * step + 0.5);
				cmsDoTransform(xform, in,
					       &clut->data[entry], 1);
			}

	cmsDeleteTransform(xform);
	return 1;
}

void
weston_colorspace_destroy(struct weston_colorspace *colorspace, int force)
{
	if (!colorspace)
		return;
	if (!force && !colorspace->refcounted)
		return;

	colorspace->refcount--;
	if (colorspace->refcount > 0)
		return;

	wl_list_remove(&colorspace->link);

	cmsCloseProfile(colorspace->lcms_handle);
	weston_free_clut(&colorspace->clut);
	free(colorspace);
}

static void
weston_colorspace_resource_destroy(struct wl_resource *cms_colorspace)
{
	struct weston_colorspace *colorspace =
		wl_resource_get_user_data(cms_colorspace);
	weston_colorspace_destroy(colorspace, 0);
}

static void
cms_set_colorspace(struct wl_client *client, struct wl_resource *cms,
		   struct wl_resource *surface_resource,
		   struct wl_resource *colorspace_resource)
{
	struct weston_surface *surface =
		wl_resource_get_user_data(surface_resource);

	struct weston_colorspace *colorspace =
		wl_resource_get_user_data(colorspace_resource);
	surface->pending.colorspace = colorspace;
}

static void
cms_colorspace_from_fd(struct wl_client *client, struct wl_resource *cms,
		       int fd, uint32_t id)
{
	struct weston_compositor *compositor =
		wl_resource_get_user_data(cms);

	struct weston_colorspace *colorspace;
	struct wl_resource *colorspace_resource;

	colorspace = weston_colorspace_from_fd(fd, 1, compositor);
	if (colorspace == NULL) {
		wl_resource_post_error(cms,
				       WL_CMS_ERROR_INVALID_PROFILE,
				       "the passed ICC profile is not valid");
		return;
	}

	colorspace_resource = wl_resource_create(client,
						 &wl_cms_colorspace_interface,
						 1, id);
	if (colorspace_resource == NULL) {
		weston_colorspace_destroy(colorspace, 0);
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(colorspace_resource,
				       &cms_colorspace_interface, colorspace,
				       weston_colorspace_resource_destroy);
}

static void
cms_output_colorspace(struct wl_client *client, struct wl_resource *cms,
		      struct wl_resource *output_resource, uint32_t id)
{
	struct weston_output *output =
		wl_resource_get_user_data(output_resource);

	struct weston_colorspace *colorspace;
	struct wl_resource *colorspace_resource;

	colorspace = output->colorspace;
	colorspace->refcount++;

	colorspace_resource = wl_resource_create(client,
						 &wl_cms_colorspace_interface,
						 1, id);
	if (colorspace_resource == NULL) {
		weston_colorspace_destroy(colorspace, 0);
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(colorspace_resource,
				       &cms_colorspace_interface, colorspace,
				       weston_colorspace_resource_destroy);
}

static void
cms_srgb_colorspace(struct wl_client *client, struct wl_resource *cms,
		    uint32_t id)
{
	struct weston_compositor *compositor =
		wl_resource_get_user_data(cms);
	struct weston_colorspace *colorspace;
	struct wl_resource *colorspace_resource;

	colorspace = compositor->srgb_colorspace;

	colorspace_resource = wl_resource_create(client,
						 &wl_cms_colorspace_interface,
						 1, id);
	if (colorspace_resource == NULL) {
		weston_colorspace_destroy(colorspace, 0);
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(colorspace_resource,
				       &cms_colorspace_interface, colorspace,
				       weston_colorspace_resource_destroy);
}

static void
cms_blending_colorspace(struct wl_client *client, struct wl_resource *cms,
			uint32_t id)
{
	struct weston_compositor *compositor =
		wl_resource_get_user_data(cms);
	struct weston_colorspace *colorspace;
	struct wl_resource *colorspace_resource;

	colorspace = compositor->blending_colorspace;

	colorspace_resource = wl_resource_create(client,
						 &wl_cms_colorspace_interface,
						 1, id);
	if (colorspace_resource == NULL) {
		weston_colorspace_destroy(colorspace, 0);
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(colorspace_resource,
				       &cms_colorspace_interface, colorspace,
				       weston_colorspace_resource_destroy);
}

static const struct wl_cms_interface cms_interface = {
	cms_set_colorspace,
	cms_colorspace_from_fd,
	cms_output_colorspace,
	cms_srgb_colorspace,
	cms_blending_colorspace
};

static void
unbind_cms_resource(struct wl_resource *resource)
{
	wl_list_remove(wl_resource_get_link(resource));
}

static void
bind_cms(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct weston_compositor *compositor = data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_cms_interface, 1, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_list_insert(&compositor->cms_list, wl_resource_get_link(resource));
	wl_resource_set_implementation(resource, &cms_interface, data,
				       unbind_cms_resource);
}

int
weston_create_cms_interface(struct wl_display *display,
			    struct weston_compositor *ec)
{
	if (!wl_global_create(display, &wl_cms_interface, 1,
			      ec, bind_cms))
		return 0;
	return 1;
}

static struct weston_colorspace *
weston_find_colorspace(struct wl_list *cs_list, cmsHPROFILE lcms_handle)
{
	cmsUInt8Number input_profile_id[17];
	cmsUInt8Number cmp_profile_id[17];
	struct weston_colorspace *colorspace;

	input_profile_id[16] = '\0';
	cmp_profile_id[16] = '\0';
	cmsGetHeaderProfileID(lcms_handle, input_profile_id);

	wl_list_for_each(colorspace, cs_list, link) {
		cmsGetHeaderProfileID(colorspace->lcms_handle, cmp_profile_id);
		if (memcmp(input_profile_id, cmp_profile_id, 16) == 0)
			return colorspace;
	}
	return NULL;
}

struct weston_colorspace *
weston_colorspace_from_fd(int fd, int input, struct weston_compositor *ec)
{
	FILE *profile;
	cmsHPROFILE lcms_handle;
	struct weston_colorspace *colorspace;

	profile = fdopen(fd, "r");
	if (!profile)
		goto err_0;

	lcms_handle = cmsOpenProfileFromStream(profile, "r");
	if (!lcms_handle)
		goto err_0;

	if (!cmsMD5computeID(lcms_handle))
		goto err_1;

	if (input)
		colorspace = weston_find_colorspace(&ec->input_colorspaces,
						    lcms_handle);
	else
		colorspace = weston_find_colorspace(&ec->output_colorspaces,
						    lcms_handle);
	if (colorspace) {
		colorspace->refcount++;

		cmsCloseProfile(lcms_handle);
		return colorspace;
	}

	colorspace = calloc(1, sizeof(struct weston_colorspace));
	if (!colorspace)
		goto err_1;

	colorspace->refcounted = 1;
	colorspace->refcount = 1;
	colorspace->input = input;
	colorspace->lcms_handle = lcms_handle;
	colorspace->compositor = ec;

	if (!weston_build_clut(colorspace))
		goto err_2;

	wl_list_insert(&ec->input_colorspaces, &colorspace->link);

	return colorspace;

err_2:
	free(colorspace);
err_1:
	cmsCloseProfile(lcms_handle);
	return NULL;
err_0:
	close(fd);
	return NULL;
}

int
weston_create_default_colorspaces(struct weston_compositor *ec)
{
	ec->srgb_output_colorspace =
		calloc(1, sizeof(struct weston_colorspace));
	if (!ec->srgb_output_colorspace)
		goto err_0;

	ec->srgb_colorspace = calloc(1, sizeof(struct weston_colorspace));
	if (!ec->srgb_colorspace)
		goto err_1;

	ec->blending_colorspace = calloc(1, sizeof(struct weston_colorspace));
	if (!ec->blending_colorspace)
		goto err_2;

	ec->srgb_output_colorspace->input = 0;
	ec->srgb_output_colorspace->compositor = ec;
	ec->srgb_output_colorspace->refcounted = 0;
	ec->srgb_output_colorspace->refcount = 0;
	ec->srgb_colorspace->input = 1;
	ec->srgb_colorspace->compositor = ec;
	ec->srgb_colorspace->refcounted = 0;
	ec->srgb_colorspace->refcount = 0;
	ec->blending_colorspace->input = 1;
	ec->blending_colorspace->compositor = ec;
	ec->blending_colorspace->refcounted = 0;
	ec->blending_colorspace->refcount = 0;

	ec->srgb_output_colorspace->lcms_handle = cmsCreate_sRGBProfile();
	ec->srgb_colorspace->lcms_handle = cmsCreate_sRGBProfile();
	/* TODO: We should really use a linear blending space. But this would
	 * cause additional banding since we only use an 8 bit LUT for now. */
	ec->blending_colorspace->lcms_handle = cmsCreate_sRGBProfile();

	if (!cmsMD5computeID(ec->srgb_output_colorspace->lcms_handle))
		goto err_3;

	if (!cmsMD5computeID(ec->srgb_colorspace->lcms_handle))
		goto err_3;

	if (!cmsMD5computeID(ec->blending_colorspace->lcms_handle))
		goto err_3;

	if (!weston_build_clut(ec->srgb_output_colorspace)) {
		weston_log("fatal: failed to build output SRGB CLUT\n");
		goto err_3;
	}
	if (!weston_build_clut(ec->srgb_colorspace)) {
		weston_log("fatal: failed to build SRGB CLUT\n");
		goto err_3;
	}
	/* This is unnecessary but will initialize everything with 0 */
	if (!weston_build_clut(ec->blending_colorspace)) {
		weston_log("fatal: failed to build blending CLUT\n");
		goto err_3;
	}

	if (!cmsMD5computeID(ec->srgb_colorspace->lcms_handle)) {
		weston_log("fatal: failed to compute MD5 sum of SRGB profile\n");
		goto err_3;
	}
	if (!cmsMD5computeID(ec->blending_colorspace->lcms_handle)) {
		weston_log("fatal: failed to compute MD5 sum of blending profile\n");
		goto err_3;
	}

	wl_list_insert(&ec->output_colorspaces,
		       &ec->srgb_output_colorspace->link);
	wl_list_insert(&ec->input_colorspaces, &ec->srgb_colorspace->link);
	wl_list_insert(&ec->input_colorspaces, &ec->blending_colorspace->link);

	return 1;

err_3:
	free(ec->blending_colorspace);
err_2:
	free(ec->srgb_colorspace);
err_1:
	free(ec->srgb_output_colorspace);
err_0:
	return 0;
}
