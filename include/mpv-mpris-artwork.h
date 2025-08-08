/*
    MIT License - MPV MPRIS Bridge - Album Artwork Functions
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
 * Clean up old cache files based on CACHE_MAX_AGE_DAYS
 */
void cleanup_old_cache_files(void);

#endif // MPV_MPRIS_ARTWORK_H