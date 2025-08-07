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

/**
 * Convert a potentially non-UTF8 string to valid UTF8
 * @param maybe_utf8 Input string that may or may not be UTF8
 * @return Valid UTF8 string (must be freed with g_free)
 */
gchar *string_to_utf8(gchar *maybe_utf8);

/**
 * Convert a file path to a URI, handling both absolute and relative paths
 * @param mpv MPV handle for getting working directory
 * @param path File path to convert
 * @return URI string (must be freed with g_free), or NULL on error
 */
gchar *path_to_uri(mpv_handle *mpv, char *path);

/**
 * Add a string metadata item to the variant dictionary
 * @param mpv MPV handle
 * @param dict Variant dictionary to add to
 * @param property MPV property name
 * @param tag MPRIS metadata tag name
 */
void add_metadata_item_string(mpv_handle *mpv, GVariantDict *dict,
                             const char *property, const char *tag);

/**
 * Add an integer metadata item to the variant dictionary
 * @param mpv MPV handle
 * @param dict Variant dictionary to add to
 * @param property MPV property name
 * @param tag MPRIS metadata tag name
 */
void add_metadata_item_int(mpv_handle *mpv, GVariantDict *dict,
                          const char *property, const char *tag);

/**
 * Add a string list metadata item to the variant dictionary
 * @param mpv MPV handle
 * @param dict Variant dictionary to add to
 * @param property MPV property name
 * @param tag MPRIS metadata tag name
 */
void add_metadata_item_string_list(mpv_handle *mpv, GVariantDict *dict,
                                  const char *property, const char *tag);

/**
 * Add URI metadata to the variant dictionary
 * @param mpv MPV handle
 * @param dict Variant dictionary to add to
 */
void add_metadata_uri(mpv_handle *mpv, GVariantDict *dict);

/**
 * Add album art metadata to the variant dictionary
 * @param mpv MPV handle
 * @param dict Variant dictionary to add to
 * @param ud User data containing cached art information
 */
void add_metadata_art(mpv_handle *mpv, GVariantDict *dict, UserData *ud);

/**
 * Add content creation date metadata to the variant dictionary
 * @param mpv MPV handle
 * @param dict Variant dictionary to add to
 */
void add_metadata_content_created(mpv_handle *mpv, GVariantDict *dict);

/**
 * Create complete MPRIS metadata variant
 * @param ud User data structure
 * @return GVariant containing all metadata (caller must unref)
 */
GVariant *create_metadata(UserData *ud);

#endif // MPV_MPRIS_METADATA_H