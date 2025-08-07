#ifndef METADATA_H
#define METADATA_H

#include <gio/gio.h>
#include <mpv/client.h>
#include <libavformat/avformat.h>

GVariant *create_metadata(UserData *ud);
void add_metadata_art(mpv_handle *mpv, GVariantDict *dict, UserData *ud);
gchar *try_get_local_art(mpv_handle *mpv, char *path);
gchar *try_get_embedded_art(char *path);
gchar *try_get_youtube_thumbnail(const char *path);
void cleanup_old_cache_files();

#endif