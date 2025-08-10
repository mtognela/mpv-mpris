/*
    MIT License

    Copyright (c) 2025 Mattia Tognela

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef MPV_MPRIS_METADATA_H
#define MPV_MPRIS_METADATA_H

#include "mpv-mpris-types.h"

GVariant *create_metadata(UserData *ud);

gchar *string_to_utf8(gchar *maybe_utf8);
gchar *path_to_uri(mpv_handle *mpv, char *path);

void add_metadata_item_string(mpv_handle *mpv, GVariantDict *dict,
                             const char *property, const char *tag);

void add_metadata_item_int(mpv_handle *mpv, GVariantDict *dict,
                          const char *property, const char *tag);

void add_metadata_item_string_list(mpv_handle *mpv, GVariantDict *dict,
                                  const char *property, const char *tag);

void add_metadata_uri(mpv_handle *mpv, GVariantDict *dict);

void add_metadata_art(mpv_handle *mpv, GVariantDict *dict, UserData *ud);

void add_metadata_content_created(mpv_handle *mpv, GVariantDict *dict);

void wakeup_handler(void *fd);

GVariant *create_metadata(UserData *ud);

gchar *extract_embedded_art(AVFormatContext *context, const char *path);

#endif // MPV_MPRIS_METADATA_H
