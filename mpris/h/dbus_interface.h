#ifndef DBUS_INTERFACE_H
#define DBUS_INTERFACE_H

#include <gio/gio.h>
#include <mpv/client.h>

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
    char *cached_path;
    gchar *cached_art_url;
} UserData;

extern const char *STATUS_PLAYING;
extern const char *STATUS_PAUSED;
extern const char *STATUS_STOPPED;
extern const char *LOOP_NONE;
extern const char *LOOP_TRACK;
extern const char *LOOP_PLAYLIST;

gboolean init_dbus_interface(UserData *ud, GError **error);
gboolean setup_mpv_properties(mpv_handle *mpv);
void cleanup_resources(UserData *ud, GMainContext *ctx, GMainLoop *loop, 
                      int pipe[2], GSource *mpv_pipe_source, GSource *timeout_source);

#endif