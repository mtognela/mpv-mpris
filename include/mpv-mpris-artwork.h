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