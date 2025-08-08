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

#ifndef MPV_MPRIS_ARTWORK_H
#define MPV_MPRIS_ARTWORK_H

#include "mpv-mpris-types.h"

// Forward declaration
gchar *path_to_uri(mpv_handle *mpv, char *path);

// Album art file patterns
extern const char art_files[][32];

// Supported image extensions
extern const char *supported_extensions[];

gboolean is_supported_image_file(const char *filename);

gchar *path_to_uri(mpv_handle *mpv, char *path);

gchar* extract_embedded_art(AVFormatContext *context, const char *media_path);

gboolean is_art_file(const char *filename);

const char *get_image_extension(const uint8_t *data, size_t size);

gchar *try_get_local_art(mpv_handle *mpv, char *path);

gchar *get_cache_dir(void);

gchar *generate_cache_filename(const char *path, const uint8_t *image_data, size_t image_size);

void cleanup_old_cache_files(void);

#endif // MPV_MPRIS_ARTWORK_H