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