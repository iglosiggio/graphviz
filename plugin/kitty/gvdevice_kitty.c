/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <gvc/gvplugin_device.h>

#include <cairo.h>

static const char base64_alphabet[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

static void fix_colors(char* imagedata, size_t imagedata_size) {
	size_t i = 0;
	while (i < imagedata_size) {
		char blue = imagedata[i+0];
		char red = imagedata[i+2];
		imagedata[i+0] = red;
		imagedata[i+2] = blue;
		i += 4;
	}
}

static size_t base64_encoded_size(size_t original_size) {
	return (original_size + 2) / 3 * 4;
}

static char* base64_encode(const char* data, size_t size) {
	size_t buf_i = 0;
	size_t data_i = 0;
	char* buf = malloc(base64_encoded_size(size));

	while (data_i < size) {
		char v, d0, d1, d2;

		d0 = data[data_i];
		v = (d0 & 0xFC) >> 2; // 1111_1100
		buf[buf_i++] = base64_alphabet[v];

		d1 = data_i+1 < size ? data[data_i+1] : 0;
		v = (d0 & 0x03) << 4  // 0000_0011
		  | (d1 & 0xF0) >> 4; // 1111_0000
		buf[buf_i++] = base64_alphabet[v];
		if (size <= data_i+1) goto end;

		d2 = data_i+2 < size ? data[data_i+2] : 0;
		v = (d1 & 0x0F) << 2  // 0000_1111
		  | (d2 & 0xC0) >> 6; // 1100_0000
		buf[buf_i++] = base64_alphabet[v];
		if (size <= data_i+2) goto end;

		v = d2 & 0x3F; // 0011_1111
		buf[buf_i++] = base64_alphabet[v];

		data_i += 3;
	}

end:
	while (buf_i % 4 != 0)
		buf[buf_i++] = base64_alphabet[64];

	return buf;
}
static void kitty_initialize(GVJ_t* job)
{
	job->output_data = malloc(4096);
}

static void kitty_finalize(GVJ_t* job)
{
	char* imagedata = job->imagedata;
	size_t imagedata_size = job->width * job->height * 4;
	fix_colors(imagedata, imagedata_size);

	const size_t chunk_size = 4096;
	char* output = base64_encode(imagedata, imagedata_size);
	size_t offset = 0;
	size_t size = base64_encoded_size(imagedata_size);

	while (offset < size) {
		int has_next_chunk = offset + chunk_size <= size;
		if (size < chunk_size)
			fprintf(stdout, "\033_Ga=T,f=32,s=%d,v=%d;", job->width, job->height);
		else if (offset == 0)
			fprintf(stdout, "\033_Ga=T,f=32,s=%d,v=%d,m=1;", job->width, job->height);
		else
			fprintf(stdout, "\033_Gm=%d;", has_next_chunk);

		size_t this_chunk_size = has_next_chunk ? chunk_size : size - offset;
		fwrite(output + offset, this_chunk_size, 1, stdout);
		fprintf(stdout, "\033\\");
		offset += chunk_size;
	}

	free(output);
}


static gvdevice_features_t device_features_kitty = {
    GVDEVICE_DOES_TRUECOLOR,    /* flags */
    {0.,0.},                    /* default margin - points */
    {0.,0.},                    /* default page width, height - points */
    {96.,96.},                  /* dpi */
};

static gvdevice_engine_t device_engine_kitty = {
    kitty_initialize,	/* kitty_initialize */
    NULL,               /* kitty_format */
    kitty_finalize,	/* kitty_finalize */
};

gvplugin_installed_t gvdevice_types_kitty[] = {
#ifdef CAIRO_HAS_PNG_FUNCTIONS
    {0, "kitty:cairo", 0, &device_engine_kitty, &device_features_kitty},
#endif
    {0, NULL, 0, NULL, NULL}
};
