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

gchar *try_get_youtube_thumbnail(const char *url);
gchar *try_get_embedded_art(const char *path);
gchar *try_get_local_art_enhanced(mpv_handle *mpv, const char *path);


gboolean event_handler(int fd, GIOCondition condition, gpointer data);
void wakeup_handler(void *fd);

GVariant *create_metadata(UserData *ud);

gchar *extract_embedded_art(AVFormatContext *context, const char *path);
gboolean is_art_file(const char *filename);

#endif // MPV_MPRIS_METADATA_H