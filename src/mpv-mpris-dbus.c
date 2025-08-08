#include "mpv-mpris-types.h"
#include "mpv-mpris-dbus.h"

GDBusInterfaceVTable vtable_root = {
    method_call_root, get_property_root, set_property_root, {0}};

GDBusInterfaceVTable vtable_player = {
    method_call_player, get_property_player, set_property_player, {0}};

void method_call_root(G_GNUC_UNUSED GDBusConnection *connection,
                             G_GNUC_UNUSED const char *sender,
                             G_GNUC_UNUSED const char *object_path,
                             G_GNUC_UNUSED const char *interface_name,
                             const char *method_name,
                             G_GNUC_UNUSED GVariant *parameters,
                             GDBusMethodInvocation *invocation,
                             gpointer user_data)
{
    UserData *ud = (UserData *)user_data;
    if (g_strcmp0(method_name, "Quit") == 0)
    {
        const char *cmd[] = {"quit", NULL};
        mpv_command_async(ud->mpv, 0, cmd);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (g_strcmp0(method_name, "Raise") == 0)
    {
        // Can't raise
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else
    {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_UNKNOWN_METHOD,
                                              "Unknown method");
    }
}

GVariant *get_property_root(G_GNUC_UNUSED GDBusConnection *connection,
                                   G_GNUC_UNUSED const char *sender,
                                   G_GNUC_UNUSED const char *object_path,
                                   G_GNUC_UNUSED const char *interface_name,
                                   const char *property_name,
                                   G_GNUC_UNUSED GError **error,
                                   gpointer user_data)
{
    UserData *ud = (UserData *)user_data;
    GVariant *ret;

    if (g_strcmp0(property_name, "CanQuit") == 0)
    {
        ret = g_variant_new_boolean(TRUE);
    }
    else if (g_strcmp0(property_name, "Fullscreen") == 0)
    {
        int fullscreen;
        mpv_get_property(ud->mpv, "fullscreen", MPV_FORMAT_FLAG, &fullscreen);
        ret = g_variant_new_boolean(fullscreen);
    }
    else if (g_strcmp0(property_name, "CanSetFullscreen") == 0)
    {
        int can_fullscreen;
        mpv_get_property(ud->mpv, "vo-configured", MPV_FORMAT_FLAG, &can_fullscreen);
        ret = g_variant_new_boolean(can_fullscreen);
    }
    else if (g_strcmp0(property_name, "CanRaise") == 0)
    {
        ret = g_variant_new_boolean(FALSE);
    }
    else if (g_strcmp0(property_name, "HasTrackList") == 0)
    {
        ret = g_variant_new_boolean(FALSE);
    }
    else if (g_strcmp0(property_name, "Identity") == 0)
    {
        ret = g_variant_new_string("mpv");
    }
    else if (g_strcmp0(property_name, "DesktopEntry") == 0)
    {
        ret = g_variant_new_string("mpv");
    }
    else if (g_strcmp0(property_name, "SupportedUriSchemes") == 0)
    {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
        g_variant_builder_add(&builder, "s", "ftp");
        g_variant_builder_add(&builder, "s", "http");
        g_variant_builder_add(&builder, "s", "https");
        g_variant_builder_add(&builder, "s", "mms");
        g_variant_builder_add(&builder, "s", "rtmp");
        g_variant_builder_add(&builder, "s", "rtsp");
        g_variant_builder_add(&builder, "s", "sftp");
        g_variant_builder_add(&builder, "s", "smb");
        ret = g_variant_builder_end(&builder);
    }
    else if (g_strcmp0(property_name, "SupportedMimeTypes") == 0)
    {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
        g_variant_builder_add(&builder, "s", "application/ogg");
        g_variant_builder_add(&builder, "s", "audio/mpeg");
        // TODO add the rest
        ret = g_variant_builder_end(&builder);
    }
    else
    {
        ret = NULL;
        g_set_error(error, G_DBUS_ERROR,
                    G_DBUS_ERROR_UNKNOWN_PROPERTY,
                    "Unknown property %s", property_name);
    }

    return ret;
}

gboolean set_property_root(G_GNUC_UNUSED GDBusConnection *connection,
                                  G_GNUC_UNUSED const char *sender,
                                  G_GNUC_UNUSED const char *object_path,
                                  G_GNUC_UNUSED const char *interface_name,
                                  const char *property_name,
                                  GVariant *value,
                                  G_GNUC_UNUSED GError **error,
                                  gpointer user_data)
{
    UserData *ud = (UserData *)user_data;
    if (g_strcmp0(property_name, "Fullscreen") == 0)
    {
        int fullscreen;
        g_variant_get(value, "b", &fullscreen);
        mpv_set_property(ud->mpv, "fullscreen", MPV_FORMAT_FLAG, &fullscreen);
    }
    else
    {
        g_set_error(error, G_DBUS_ERROR,
                    G_DBUS_ERROR_UNKNOWN_PROPERTY,
                    "Cannot set property %s", property_name);
        return FALSE;
    }
    return TRUE;
}

void method_call_player(G_GNUC_UNUSED GDBusConnection *connection,
                               G_GNUC_UNUSED const char *sender,
                               G_GNUC_UNUSED const char *_object_path,
                               G_GNUC_UNUSED const char *interface_name,
                               const char *method_name,
                               G_GNUC_UNUSED GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data)
{
    UserData *ud = (UserData *)user_data;
    if (g_strcmp0(method_name, "Pause") == 0)
    {
        int paused = TRUE;
        mpv_set_property(ud->mpv, "pause", MPV_FORMAT_FLAG, &paused);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (g_strcmp0(method_name, "PlayPause") == 0)
    {
        int paused;
        if (ud->status == STATUS_PAUSED)
        {
            paused = FALSE;
        }
        else
        {
            paused = TRUE;
        }
        mpv_set_property(ud->mpv, "pause", MPV_FORMAT_FLAG, &paused);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (g_strcmp0(method_name, "Play") == 0)
    {
        int paused = FALSE;
        mpv_set_property(ud->mpv, "pause", MPV_FORMAT_FLAG, &paused);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (g_strcmp0(method_name, "Stop") == 0)
    {
        const char *cmd[] = {"stop", NULL};
        mpv_command_async(ud->mpv, 0, cmd);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (g_strcmp0(method_name, "Next") == 0)
    {
        const char *cmd[] = {"playlist_next", NULL};
        mpv_command_async(ud->mpv, 0, cmd);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (g_strcmp0(method_name, "Previous") == 0)
    {
        const char *cmd[] = {"playlist_prev", NULL};
        mpv_command_async(ud->mpv, 0, cmd);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (g_strcmp0(method_name, "Seek") == 0)
    {
        int64_t offset_us; // in microseconds
        char *offset_str;
        g_variant_get(parameters, "(x)", &offset_us);
        double offset_s = offset_us / 1000000.0;
        offset_str = g_strdup_printf("%f", offset_s);

        const char *cmd[] = {"seek", offset_str, NULL};
        mpv_command_async(ud->mpv, 0, cmd);
        g_dbus_method_invocation_return_value(invocation, NULL);
        g_free(offset_str);
    }
    else if (g_strcmp0(method_name, "SetPosition") == 0)
    {
        int64_t current_id;
        char *object_path;
        double new_position_s;
        int64_t new_position_us;

        mpv_get_property(ud->mpv, "playlist-pos", MPV_FORMAT_INT64, &current_id);
        g_variant_get(parameters, "(&ox)", &object_path, &new_position_us);
        new_position_s = ((float)new_position_us) / 1000000.0; // us -> s

        if (current_id == g_ascii_strtoll(object_path + 1, NULL, 10))
        {
            // Use MPV's seek command instead of setting time-pos property
            char *position_str = g_strdup_printf("%.6f", new_position_s);

            const char *cmd[] = {"seek", position_str, "absolute", "exact", NULL};
            mpv_command_async(ud->mpv, 0, cmd);

            g_free(position_str);
        }

        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if (g_strcmp0(method_name, "OpenUri") == 0)
    {
        char *uri;
        g_variant_get(parameters, "(&s)", &uri);
        const char *cmd[] = {"loadfile", uri, NULL};
        mpv_command_async(ud->mpv, 0, cmd);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else
    {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                                              G_DBUS_ERROR_UNKNOWN_METHOD,
                                              "Unknown method");
    }
}

GVariant *get_property_player(G_GNUC_UNUSED GDBusConnection *connection,
                                     G_GNUC_UNUSED const char *sender,
                                     G_GNUC_UNUSED const char *object_path,
                                     G_GNUC_UNUSED const char *interface_name,
                                     const char *property_name,
                                     GError **error,
                                     gpointer user_data)
{
    UserData *ud = (UserData *)user_data;
    GVariant *ret;
    if (g_strcmp0(property_name, "PlaybackStatus") == 0)
    {
        ret = g_variant_new_string(ud->status);
    }
    else if (g_strcmp0(property_name, "LoopStatus") == 0)
    {
        ret = g_variant_new_string(ud->loop_status);
    }
    else if (g_strcmp0(property_name, "Rate") == 0)
    {
        double rate;
        mpv_get_property(ud->mpv, "speed", MPV_FORMAT_DOUBLE, &rate);
        ret = g_variant_new_double(rate);
    }
    else if (g_strcmp0(property_name, "Shuffle") == 0)
    {
        int shuffle;
        mpv_get_property(ud->mpv, "shuffle", MPV_FORMAT_FLAG, &shuffle);
        ret = g_variant_new_boolean(shuffle);
    }
    else if (g_strcmp0(property_name, "Metadata") == 0)
    {
        if (!ud->metadata)
        {
            ud->metadata = create_metadata(ud);
        }
        // Increase reference count to prevent it from being freed after returning
        g_variant_ref(ud->metadata);
        ret = ud->metadata;
    }
    else if (g_strcmp0(property_name, "Volume") == 0)
    {
        double volume;
        mpv_get_property(ud->mpv, "volume", MPV_FORMAT_DOUBLE, &volume);
        volume /= 100;
        ret = g_variant_new_double(volume);
    }
    else if (g_strcmp0(property_name, "Position") == 0)
    {
        double position_s;
        int64_t position_us;
        mpv_get_property(ud->mpv, "time-pos", MPV_FORMAT_DOUBLE, &position_s);
        position_us = position_s * 1000000.0; // s -> us
        ret = g_variant_new_int64(position_us);
    }
    else if (g_strcmp0(property_name, "MinimumRate") == 0)
    {
        ret = g_variant_new_double(0.01);
    }
    else if (g_strcmp0(property_name, "MaximumRate") == 0)
    {
        ret = g_variant_new_double(100);
    }
    else if (g_strcmp0(property_name, "CanGoNext") == 0)
    {
        ret = g_variant_new_boolean(TRUE);
    }
    else if (g_strcmp0(property_name, "CanGoPrevious") == 0)
    {
        ret = g_variant_new_boolean(TRUE);
    }
    else if (g_strcmp0(property_name, "CanPlay") == 0)
    {
        ret = g_variant_new_boolean(TRUE);
    }
    else if (g_strcmp0(property_name, "CanPause") == 0)
    {
        ret = g_variant_new_boolean(TRUE);
    }
    else if (g_strcmp0(property_name, "CanSeek") == 0)
    {
        ret = g_variant_new_boolean(TRUE);
    }
    else if (g_strcmp0(property_name, "CanControl") == 0)
    {
        ret = g_variant_new_boolean(TRUE);
    }
    else
    {
        ret = NULL;
        g_set_error(error, G_DBUS_ERROR,
                    G_DBUS_ERROR_UNKNOWN_PROPERTY,
                    "Unknown property %s", property_name);
    }

    return ret;
}

gboolean set_property_player(G_GNUC_UNUSED GDBusConnection *connection,
                                    G_GNUC_UNUSED const char *sender,
                                    G_GNUC_UNUSED const char *object_path,
                                    G_GNUC_UNUSED const char *interface_name,
                                    const char *property_name,
                                    GVariant *value,
                                    G_GNUC_UNUSED GError **error,
                                    gpointer user_data)
{
    UserData *ud = (UserData *)user_data;
    if (g_strcmp0(property_name, "LoopStatus") == 0)
    {
        const char *status;
        int t = TRUE;
        int f = FALSE;
        status = g_variant_get_string(value, NULL);
        if (g_strcmp0(status, "Track") == 0)
        {
            mpv_set_property(ud->mpv, "loop-file", MPV_FORMAT_FLAG, &t);
            mpv_set_property(ud->mpv, "loop-playlist", MPV_FORMAT_FLAG, &f);
        }
        else if (g_strcmp0(status, "Playlist") == 0)
        {
            mpv_set_property(ud->mpv, "loop-file", MPV_FORMAT_FLAG, &f);
            mpv_set_property(ud->mpv, "loop-playlist", MPV_FORMAT_FLAG, &t);
        }
        else
        {
            mpv_set_property(ud->mpv, "loop-file", MPV_FORMAT_FLAG, &f);
            mpv_set_property(ud->mpv, "loop-playlist", MPV_FORMAT_FLAG, &f);
        }
    }
    else if (g_strcmp0(property_name, "Rate") == 0)
    {
        double rate = g_variant_get_double(value);
        mpv_set_property(ud->mpv, "speed", MPV_FORMAT_DOUBLE, &rate);
    }
    else if (g_strcmp0(property_name, "Shuffle") == 0)
    {
        int shuffle = g_variant_get_boolean(value);
        if (shuffle && !ud->shuffle)
        {
            const char *cmd[] = {"playlist-shuffle", NULL};
            mpv_command_async(ud->mpv, 0, cmd);
        }
        else if (!shuffle && ud->shuffle)
        {
            const char *cmd[] = {"playlist-unshuffle", NULL};
            mpv_command_async(ud->mpv, 0, cmd);
        }
        mpv_set_property(ud->mpv, "shuffle", MPV_FORMAT_FLAG, &shuffle);
    }
    else if (g_strcmp0(property_name, "Volume") == 0)
    {
        double volume = g_variant_get_double(value);
        volume *= 100;
        mpv_set_property(ud->mpv, "volume", MPV_FORMAT_DOUBLE, &volume);
    }
    else
    {
        g_set_error(error, G_DBUS_ERROR,
                    G_DBUS_ERROR_UNKNOWN_PROPERTY,
                    "Cannot set property %s", property_name);
        return FALSE;
    }

    return TRUE;
}

gboolean emit_property_changes(gpointer data)
{
    UserData *ud = (UserData *)data;
    GError *error = NULL;
    gpointer prop_name, prop_value;
    GHashTableIter iter;

    if (g_hash_table_size(ud->changed_properties) > 0)
    {
        GVariant *params;
        GVariantBuilder *properties = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
        GVariantBuilder *invalidated = g_variant_builder_new(G_VARIANT_TYPE("as"));
        g_hash_table_iter_init(&iter, ud->changed_properties);
        while (g_hash_table_iter_next(&iter, &prop_name, &prop_value))
        {
            if (prop_value)
            {
                g_variant_builder_add(properties, "{sv}", prop_name, prop_value);
            }
            else
            {
                g_variant_builder_add(invalidated, "s", prop_name);
            }
        }
        params = g_variant_new("(sa{sv}as)",
                               "org.mpris.MediaPlayer2.Player", properties, invalidated);
        g_variant_builder_unref(properties);
        g_variant_builder_unref(invalidated);

        g_dbus_connection_emit_signal(ud->connection, NULL,
                                      "/org/mpris/MediaPlayer2",
                                      "org.freedesktop.DBus.Properties",
                                      "PropertiesChanged",
                                      params, &error);
        if (error != NULL)
        {
            g_printerr("%s", error->message);
        }

        g_hash_table_remove_all(ud->changed_properties);
    }
    return TRUE;
}


void emit_seeked_signal(UserData *ud)
{
    GVariant *params;
    double position_s;
    int64_t position_us;
    GError *error = NULL;
    mpv_get_property(ud->mpv, "time-pos", MPV_FORMAT_DOUBLE, &position_s);
    position_us = position_s * 1000000.0; // s -> us
    params = g_variant_new("(x)", position_us);

    g_dbus_connection_emit_signal(ud->connection, NULL,
                                  "/org/mpris/MediaPlayer2",
                                  "org.mpris.MediaPlayer2.Player",
                                  "Seeked",
                                  params, &error);

    if (error != NULL)
    {
        g_printerr("%s", error->message);
    }
}

void on_bus_acquired(GDBusConnection *connection,
                            G_GNUC_UNUSED const char *name,
                            gpointer user_data)
{
    GError *error = NULL;
    UserData *ud = user_data;
    ud->connection = connection;

    ud->root_interface_id =
        g_dbus_connection_register_object(connection, "/org/mpris/MediaPlayer2",
                                          ud->root_interface_info,
                                          &vtable_root,
                                          user_data, NULL, &error);
    if (error != NULL)
    {
        g_printerr("Failed to register root interface: %s\n", error->message);
        g_error_free(error); // Free the error
        error = NULL;        // Reset to NULL
    }

    ud->player_interface_id =
        g_dbus_connection_register_object(connection, "/org/mpris/MediaPlayer2",
                                          ud->player_interface_info,
                                          &vtable_player,
                                          user_data, NULL, &error);
    if (error != NULL)
    {
        g_printerr("Failed to register player interface: %s\n", error->message);
        g_error_free(error);
    }
}

void on_name_lost(GDBusConnection *connection,
                         G_GNUC_UNUSED const char *_name,
                         gpointer user_data)
{
    if (connection)
    {
        UserData *ud = user_data;
        pid_t pid = getpid();
        char *name = g_strdup_printf("org.mpris.MediaPlayer2.mpv.instance%d", pid);
        ud->bus_id = g_bus_own_name(G_BUS_TYPE_SESSION,
                                    name,
                                    G_BUS_NAME_OWNER_FLAGS_NONE,
                                    NULL, NULL, NULL,
                                    &ud, NULL);
        g_free(name);
    }
}
