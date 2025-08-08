/*
    MIT License - MPV MPRIS Bridge - Common Types and Definitions
*/

#ifndef MPV_MPRIS_TYPES_H
#define MPV_MPRIS_TYPES_H

#include <gio/gio.h>
#include <glib-unix.h>
#include <mpv/client.h>
#include <libavformat/avformat.h>
#include <inttypes.h>
#include <string.h>

#define CACHE_MAX_AGE_DAYS 15
#define SECONDS_PER_DAY 86400

extern const char *STATUS_PLAYING;
extern const char *STATUS_PAUSED;
extern const char *STATUS_STOPPED;
extern const char *LOOP_NONE;
extern const char *LOOP_TRACK;
extern const char *LOOP_PLAYLIST;

extern const char *youtube_url_pattern;
extern GRegex *youtube_url_regex;

extern const char *supported_extensions[];

extern const char art_files[][32];

extern const char *introspection_xml;

// Main user data structure
typedef struct UserData {
    mpv_handle *mpv;
    GMainLoop *loop;
    gint bus_id;
    GDBusConnection *connection;
    GDBusInterfaceInfo *root_interface_info;
    GDBusInterfaceInfo *player_interface_info;
    guint root_interface_id;
    guint player_interface_id;
    const char *status;
    const char *loop_status;
    gboolean shuffle;
    GHashTable *changed_properties;
    GVariant *metadata;
    gboolean seek_expected;
    gboolean idle;
    gboolean paused;

    // Cache fields
    char *cached_path;     // owned by mpv
    gchar *cached_art_url; // owned by glib
} UserData;

extern const char *STATUS_PLAYING;
extern const char *STATUS_PAUSED;
extern const char *STATUS_STOPPED;
extern const char *LOOP_NONE;
extern const char *LOOP_TRACK;
extern const char *LOOP_PLAYLIST;

extern const char *youtube_url_pattern;
extern GRegex *youtube_url_regex;

extern const char *supported_extensions[];

extern const char art_files[][32];

extern const char *introspection_xml;

extern GMutex metadata_mutex;

#endif // MPV_MPRIS_TYPES_H