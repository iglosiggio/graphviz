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

#include <assert.h>
#include <common/types.h>
#include <gvc/gvio.h>
#include <gvc/gvplugin_device.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

static void fix_colors(char *imagedata, size_t imagedata_size) {
  size_t i = 0;
  while (i < imagedata_size) {
    char blue = imagedata[i];
    char red = imagedata[i + 2];
    imagedata[i] = red;
    imagedata[i + 2] = blue;
    i += 4;
  }
}

static const char base64_alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

static size_t base64_encoded_size(size_t original_size) {
  return (original_size + 2) / 3 * 4;
}

static char *base64_encode(const char *data, size_t size) {
  size_t buf_i = 0;
  size_t data_i = 0;
  char *buf = malloc(base64_encoded_size(size));

  while (data_i < size) {
    char d0, d1, d2;
    int v;

    d0 = data[data_i];
    v = (d0 & 0xFC) >> 2; // 1111_1100
    buf[buf_i++] = base64_alphabet[v];

    d1 = data_i + 1 < size ? data[data_i + 1] : 0;
    v = (d0 & 0x03) << 4    // 0000_0011
        | (d1 & 0xF0) >> 4; // 1111_0000
    buf[buf_i++] = base64_alphabet[v];
    if (size <= data_i + 1)
      goto end;

    d2 = data_i + 2 < size ? data[data_i + 2] : 0;
    v = (d1 & 0x0F) << 2    // 0000_1111
        | (d2 & 0xC0) >> 6; // 1100_0000
    buf[buf_i++] = base64_alphabet[v];
    if (size <= data_i + 2)
      goto end;

    v = d2 & 0x3F; // 0011_1111
    buf[buf_i++] = base64_alphabet[v];

    data_i += 3;
  }

end:
  while (buf_i % 4 != 0)
    buf[buf_i++] = base64_alphabet[64];

  return buf;
}

static void kitty_write(char *data, size_t data_size, int width, int height,
                        bool is_compressed) {
  const size_t chunk_size = 4096;
  char *output = base64_encode(data, data_size);
  size_t offset = 0;
  size_t size = base64_encoded_size(data_size);

  while (offset < size) {
    int has_next_chunk = offset + chunk_size <= size;
    if (offset == 0)
      fprintf(stdout, "\033_Ga=T,f=32,s=%d,v=%d%s%s;", width, height,
              chunk_size < size ? ",m=1" : "", is_compressed ? ",o=z" : "");
    else
      fprintf(stdout, "\033_Gm=%d;", has_next_chunk);

    size_t this_chunk_size = has_next_chunk ? chunk_size : size - offset;
    fwrite(output + offset, this_chunk_size, 1, stdout);
    fprintf(stdout, "\033\\");
    offset += chunk_size;
  }
  fprintf(stdout, "\n");

  free(output);
}

static void kitty_format(GVJ_t *job) {
  char *imagedata = job->imagedata;
  size_t imagedata_size = job->width * job->height * 4;
  fix_colors(imagedata, imagedata_size);
  kitty_write(job->imagedata, imagedata_size, job->width, job->height, false);
}

static gvdevice_features_t device_features_kitty = {
    GVDEVICE_DOES_TRUECOLOR, /* flags */
    {0., 0.},                /* default margin - points */
    {0., 0.},                /* default page width, height - points */
    {96., 96.},              /* dpi */
};

static gvdevice_engine_t device_engine_kitty = {
    NULL,               /* kitty_initialize */
    kitty_format, NULL, /* kitty_finalize */
};

#ifdef HAVE_LIBZ
static int zlib_compress(char *source, size_t source_len, char **dest,
                         size_t *dest_len, int level) {
  int ret;

  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, level);
  if (ret != Z_OK)
    return ret;

  size_t dest_cap = deflateBound(&strm, source_len);
  *dest = malloc(dest_cap);
  if (*dest == NULL)
    return Z_MEM_ERROR;

  strm.avail_in = source_len;
  strm.next_in = (Bytef *)source;
  strm.next_out = (Bytef *)*dest;
  strm.avail_out = dest_cap;

  ret = deflate(&strm, Z_FINISH);
  assert(strm.avail_in == 0);
  assert(ret == Z_STREAM_END);

  *dest_len = dest_cap - strm.avail_out;

  (void)deflateEnd(&strm);
  return Z_OK;
}

static void zkitty_format(GVJ_t *job) {
  char *imagedata = job->imagedata;
  size_t imagedata_size = job->width * job->height * 4;
  fix_colors(imagedata, imagedata_size);

  char *zbuf;
  size_t zsize;
  zlib_compress(imagedata, imagedata_size, &zbuf, &zsize, -1);
  kitty_write(zbuf, zsize, job->width, job->height, true);
}

static gvdevice_features_t device_features_zkitty = {
    GVDEVICE_DOES_TRUECOLOR, /* flags */
    {0., 0.},                /* default margin - points */
    {0., 0.},                /* default page width, height - points */
    {96., 96.},              /* dpi */
};

static gvdevice_engine_t device_engine_zkitty = {
    NULL,                /* zkitty_initialize */
    zkitty_format, NULL, /* zkitty_finalize */
};
#endif

gvplugin_installed_t gvdevice_types_kitty[] = {
    {0, "kitty:cairo", 0, &device_engine_kitty, &device_features_kitty},
#ifdef HAVE_LIBZ
    {1, "kitty:cairo", 1, &device_engine_zkitty, &device_features_zkitty},
#endif
    {0, NULL, 0, NULL, NULL}};
