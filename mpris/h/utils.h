#ifndef UTILS_H
#define UTILS_H

#include <gio/gio.h>
#include <mpv/client.h>

gchar *string_to_utf8(gchar *maybe_utf8);
gchar *path_to_uri(mpv_handle *mpv, char *path);
gboolean is_supported_image_file(const char *filename);
gboolean is_art_file(const char *filename);
const char* get_image_extension(const uint8_t *data, size_t size);
gchar* generate_cache_filename(const char *path, const uint8_t *image_data, size_t image_size);

#endif