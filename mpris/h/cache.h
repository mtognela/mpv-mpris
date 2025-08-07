#ifndef CACHE_H
#define CACHE_H

#include <gio/gio.h>
#include <libavformat/avformat.h>

#define CACHE_MAX_AGE_DAYS 15 
#define SECONDS_PER_DAY 86400

static gchar *get_cache_dir();
gchar* extract_embedded_art(AVFormatContext *context, const char *media_path);

#endif