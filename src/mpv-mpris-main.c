#include "mpv-mpris-types.h"
#include "mpv-mpris-artwork.h"
#include "mpv-mpris-metadata.h"
#include "mpv-mpris-dbus.h"
#include "mpv-mpris-events.h"

int mpv_open_cplugin(mpv_handle *mpv) 
{
    GMainContext *ctx = NULL;
    GMainLoop *loop = NULL;
    UserData ud = {0};
    GError *error = NULL;
    GDBusNodeInfo *introspection_data = NULL;
    int pipe[2] = {-1, -1};
    GSource *mpv_pipe_source = NULL;
    GSource *timeout_source = NULL;
    int ret = -1; // Default to error

    // Validate input
    if (!mpv) {
        g_printerr("MPV handle is NULL\n");
        return ret;
    }

    // Initialize context and loop
    ctx = g_main_context_new();
    if (!ctx) {
        g_printerr("Failed to create main context\n");
        goto cleanup;
    }

    loop = g_main_loop_new(ctx, FALSE);
    if (!loop) {
        g_printerr("Failed to create main loop\n");
        goto cleanup;
    }

    // Load D-Bus introspection data
    introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, &error);
    if (error != NULL) {
        g_printerr("Failed to parse introspection XML: %s\n", error->message);
        g_error_free(error);
        error = NULL;
        goto cleanup;
    }

    if (!introspection_data) {
        g_printerr("Failed to create D-Bus introspection data\n");
        goto cleanup;
    }

    // Setup interface info
    ud.root_interface_info = g_dbus_node_info_lookup_interface(introspection_data, 
                                "org.mpris.MediaPlayer2");
    ud.player_interface_info = g_dbus_node_info_lookup_interface(introspection_data, 
                                "org.mpris.MediaPlayer2.Player");
    
    if (!ud.root_interface_info || !ud.player_interface_info) {
        g_printerr("Failed to lookup D-Bus interfaces\n");
        goto cleanup;
    }

    // Initialize UserData
    ud.mpv = mpv;
    ud.loop = loop;
    ud.status = STATUS_STOPPED;
    ud.loop_status = LOOP_NONE;
    ud.changed_properties = g_hash_table_new(g_str_hash, g_str_equal);
    if (!ud.changed_properties) {
        g_printerr("Failed to create properties hash table\n");
        goto cleanup;
    }

    ud.seek_expected = FALSE;
    ud.idle = FALSE;
    ud.paused = FALSE;
    ud.shuffle = FALSE;

    // Register on D-Bus
    g_main_context_push_thread_default(ctx);
    ud.bus_id = g_bus_own_name(G_BUS_TYPE_SESSION,
                               "org.mpris.MediaPlayer2.mpv",
                               G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE,
                               on_bus_acquired,
                               NULL,
                               on_name_lost,
                               &ud, NULL);
    g_main_context_pop_thread_default(ctx);

    if (!ud.bus_id) {
        g_printerr("Failed to own D-Bus name\n");
        goto cleanup;
    }

    // Setup property observers
    if (mpv_observe_property(mpv, 0, "pause", MPV_FORMAT_FLAG) < 0 ||
        mpv_observe_property(mpv, 0, "idle-active", MPV_FORMAT_FLAG) < 0 ||
        mpv_observe_property(mpv, 0, "media-title", MPV_FORMAT_STRING) < 0 ||
        mpv_observe_property(mpv, 0, "speed", MPV_FORMAT_DOUBLE) < 0 ||
        mpv_observe_property(mpv, 0, "volume", MPV_FORMAT_DOUBLE) < 0 ||
        mpv_observe_property(mpv, 0, "loop-file", MPV_FORMAT_STRING) < 0 ||
        mpv_observe_property(mpv, 0, "loop-playlist", MPV_FORMAT_STRING) < 0 ||
        mpv_observe_property(mpv, 0, "duration", MPV_FORMAT_INT64) < 0 ||
        mpv_observe_property(mpv, 0, "shuffle", MPV_FORMAT_FLAG) < 0 ||
        mpv_observe_property(mpv, 0, "fullscreen", MPV_FORMAT_FLAG) < 0) {
        g_printerr("Failed to observe MPV properties\n");
        goto cleanup;
    }

    // Setup event pipe
    if (!g_unix_open_pipe(pipe, FD_CLOEXEC, &error)) {
        g_printerr("Failed to create pipe: %s\n", error->message);
        g_error_free(error);
        error = NULL;
        goto cleanup;
    }

    if (fcntl(pipe[0], F_SETFL, O_NONBLOCK) == -1) {
        g_printerr("Failed to set pipe non-blocking: %s\n", g_strerror(errno));
        goto cleanup;
    }

    mpv_set_wakeup_callback(mpv, wakeup_handler, &pipe[1]);

    // Create and attach pipe source
    mpv_pipe_source = g_unix_fd_source_new(pipe[0], G_IO_IN);
    if (!mpv_pipe_source) {
        g_printerr("Failed to create pipe source\n");
        goto cleanup;
    }

    g_source_set_callback(mpv_pipe_source, G_SOURCE_FUNC(event_handler), &ud, NULL);
    g_source_attach(mpv_pipe_source, ctx);

    // Create and attach timeout source
    timeout_source = g_timeout_source_new(100);
    if (!timeout_source) {
        g_printerr("Failed to create timeout source\n");
        goto cleanup;
    }

    g_source_set_callback(timeout_source, G_SOURCE_FUNC(emit_property_changes), &ud, NULL);
    g_source_attach(timeout_source, ctx);

    // Main loop - only reach here if everything succeeded
    ret = 0;
    g_main_loop_run(loop);

cleanup:
    // Cleanup in reverse order of initialization
    if (timeout_source) {
        g_source_unref(timeout_source);
    }

    if (mpv_pipe_source) {
        g_source_unref(mpv_pipe_source);
    }

    if (pipe[0] != -1) {
        close(pipe[0]);
    }
    if (pipe[1] != -1) {
        close(pipe[1]);
    }

    mpv_free(ud.cached_path);
    g_free(ud.cached_art_url);

    cleanup_old_cache_files();

    if (ud.connection) {
        if (ud.root_interface_id) {
            g_dbus_connection_unregister_object(ud.connection, ud.root_interface_id);
        }
        if (ud.player_interface_id) {
            g_dbus_connection_unregister_object(ud.connection, ud.player_interface_id);
        }
    }

    if (ud.bus_id) {
        g_bus_unown_name(ud.bus_id);
    }

    if (ud.changed_properties) {
        g_hash_table_unref(ud.changed_properties);
    }

    if (ud.metadata) {
        g_variant_unref(ud.metadata);
    }

    if (loop) {
        g_main_loop_unref(loop);
    }

    if (ctx) {
        g_main_context_unref(ctx);
    }

    if (introspection_data) {
        g_dbus_node_info_unref(introspection_data);
    }

    return ret;
}