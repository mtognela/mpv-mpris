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

// Album art file patterns
extern const char art_files[][32];

// Supported image extensions
extern const char *supported_extensions[];

/**
 * Check if a file has a supported image extension
 * @param filename The filename to check
 * @return TRUE if the file has a supported image extension
 */
gboolean is_supported_image_file(const char *filename);

/**
 * Check if a filename matches known album art patterns
 * @param filename The filename to check
 * @return TRUE if the filename matches album art patterns
 */
gboolean is_art_file(const char *filename);

/**
 * Detect image format from binary data and return appropriate extension
 * @param data Binary image data
 * @param size Size of the image data
 * @return File extension string (e.g., ".jpg", ".png")
 */
const char *get_image_extension(const uint8_t *data, size_t size);

/**
 * Try to find local album art files in the same directory as the media file
 * @param mpv MPV handle
 * @param path Path to the media file
 * @return URI to the album art file, or NULL if not found
 */
gchar *try_get_local_art(mpv_handle *mpv, char *path);

/**
 * Try to get YouTube thumbnail URL from a YouTube URL
 * @param path YouTube URL
 * @return YouTube thumbnail URL, or NULL if not a YouTube URL
 */
gchar *try_get_youtube_thumbnail(const char *path);

/**
 * Try to extract embedded album art from media file
 * @param path Path to the media file
 * @return URI to the cached album art file, or NULL if not found
 */
gchar *try_get_embedded_art(char *path);

/**
 * Get the cache directory for album art
 * @return Path to cache directory, or NULL on error
 */
gchar *get_cache_dir(void);

/**
 * Generate cache filename for album art based on media path and image data
 * @param path Media file path
 * @param image_data Image binary data
 * @param image_size Size of image data
 * @return Generated filename for cache
 */
gchar *generate_cache_filename(const char *path, const uint8_t *image_data, size_t image_size);

/**
 * Extract embedded album art from AVFormatContext and save to cache
 * @param context AVFormatContext containing the media
 * @param media_path Path to the media file
 * @return URI to the cached album art, or NULL if not found
 */
gchar *extract_embedded_art(AVFormatContext *context, const char *media_path);

/**
 * Clean up old cache files based on CACHE_MAX_AGE_DAYS
 */
void cleanup_old_cache_files(void);

#endif // MPV_MPRIS_ARTWORK_H