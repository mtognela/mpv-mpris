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

#include "mpv-mpris-artwork.h"
#include "mpv-mpris-dbus.h"
#include "mpv-mpris-events.h"
#include "mpv-mpris-metadata.h"
#include "mpv-mpris-types.h"

// Plugin entry point
int mpv_open_cplugin(mpv_handle *mpv)
{
    GMainContext *ctx;
    GMainLoop *loop;
    UserData ud = {0};
    GError *error = NULL;
    GDBusNodeInfo *introspection_data = NULL;
    int pipe[2];
    GSource *mpv_pipe_source;
    GSource *timeout_source;

    if (!mpv) {
        g_printerr("MPV handle is NULL\n");
        return -1;
    }

    ctx = g_main_context_new();
    if (!ctx) {
        g_printerr("Failed to create main context\n");
        return -1;
    }

    loop = g_main_loop_new(ctx, FALSE);
    if (!loop) {
        g_printerr("Failed to create main loop\n");
        g_main_context_unref(ctx);
        return -1;
    }

    // Load introspection data and split into separate interfaces
    introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, &error);
    if (error != NULL)
    {
        g_printerr("%s", error->message);
    }
    ud.root_interface_info =
        g_dbus_node_info_lookup_interface(introspection_data, "org.mpris.MediaPlayer2");
    ud.player_interface_info =
        g_dbus_node_info_lookup_interface(introspection_data, "org.mpris.MediaPlayer2.Player");

    ud.mpv = mpv;
    ud.loop = loop;
    ud.status = STATUS_STOPPED;
    ud.loop_status = LOOP_NONE;
    ud.changed_properties = g_hash_table_new(g_str_hash, g_str_equal);
    ud.seek_expected = FALSE;
    ud.idle = FALSE;
    ud.paused = FALSE;
    ud.shuffle = FALSE;

    g_main_context_push_thread_default(ctx);
    ud.bus_id = g_bus_own_name(G_BUS_TYPE_SESSION,
                               "org.mpris.MediaPlayer2.mpv",
                               G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE,
                               on_bus_acquired,
                               NULL,
                               on_name_lost,
                               &ud, NULL);
    g_main_context_pop_thread_default(ctx);

    // Receive event for property changes
    mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "idle-active", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "media-title", MPV_FORMAT_STRING);
    mpv_observe_property(mpv, 0, "speed", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(mpv, 0, "loop-file", MPV_FORMAT_STRING);
    mpv_observe_property(mpv, 0, "loop-playlist", MPV_FORMAT_STRING);
    mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_INT64);
    mpv_observe_property(mpv, 0, "shuffle", MPV_FORMAT_FLAG);
    mpv_observe_property(mpv, 0, "fullscreen", MPV_FORMAT_FLAG);

    // Run callback whenever there are events
    g_unix_open_pipe(pipe, 0, &error);
    if (error != NULL)
    {
        g_printerr("%s", error->message);
    }
    fcntl(pipe[0], F_SETFL, O_NONBLOCK);
    mpv_set_wakeup_callback(mpv, wakeup_handler, &pipe[1]);
    mpv_pipe_source = g_unix_fd_source_new(pipe[0], G_IO_IN);
    g_source_set_callback(mpv_pipe_source,
                          G_SOURCE_FUNC(event_handler),
                          &ud,
                          NULL);
    g_source_attach(mpv_pipe_source, ctx);

    // Emit any new property changes every 100ms
    timeout_source = g_timeout_source_new(100);
    g_source_set_callback(timeout_source,
                          G_SOURCE_FUNC(emit_property_changes),
                          &ud,
                          NULL);
    g_source_attach(timeout_source, ctx);

    g_main_loop_run(loop);

    mpv_free(ud.cached_path);
    g_free(ud.cached_art_url);

    cleanup_old_cache_files();

    g_source_unref(mpv_pipe_source);
    g_source_unref(timeout_source);

    g_dbus_connection_unregister_object(ud.connection, ud.root_interface_id);
    g_dbus_connection_unregister_object(ud.connection, ud.player_interface_id);

    g_bus_unown_name(ud.bus_id);
    g_main_loop_unref(loop);
    g_main_context_unref(ctx);
    g_dbus_node_info_unref(introspection_data);

    return 0;
}
