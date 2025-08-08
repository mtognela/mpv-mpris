#include "mpv-mpris-types.h"
#include "mpv-mpris-dbus.h"
#include "mpv-mpris-metadata.h"

/**
 * Set the playback status based on current state
 * @param ud User data structure
 * @return GVariant containing the playback status string
 */
GVariant *set_playback_status(UserData *ud)
{
    if (ud->idle)
    {
        ud->status = STATUS_STOPPED;
    }
    else if (ud->paused)
    {
        ud->status = STATUS_PAUSED;
    }
    else
    {
        ud->status = STATUS_PLAYING;
    }
    return g_variant_new_string(ud->status);
}

/**
 * Set status to stopped and emit property change
 * @param ud User data structure
 */
void set_stopped_status(UserData *ud)
{
    const char *prop_name = "PlaybackStatus";
    GVariant *prop_value = g_variant_new_string(STATUS_STOPPED);

    ud->status = STATUS_STOPPED;

    g_hash_table_insert(ud->changed_properties,
                        (gpointer)prop_name, prop_value);

    emit_property_changes(ud);
}

/**
 * Handle MPV property changes and update MPRIS properties
 * @param name Property name that changed
 * @param data New property value
 * @param ud User data structure
 */
void handle_property_change(const char *name, void *data, UserData *ud)
{
    const char *prop_name = NULL;
    GVariant *prop_value = NULL;
    if (g_strcmp0(name, "pause") == 0)
    {
        ud->paused = *(int *)data;
        prop_name = "PlaybackStatus";
        prop_value = set_playback_status(ud);
    }
    else if (g_strcmp0(name, "idle-active") == 0)
    {
        ud->idle = *(int *)data;
        prop_name = "PlaybackStatus";
        prop_value = set_playback_status(ud);
    }
    else if (g_strcmp0(name, "media-title") == 0 ||
             g_strcmp0(name, "duration") == 0)
    {
        // Free existing metadata object
        if (ud->metadata)
        {
            g_variant_unref(ud->metadata);
        }
        ud->metadata = create_metadata(ud);
        prop_name = "Metadata";
        prop_value = ud->metadata;
    }
    else if (g_strcmp0(name, "speed") == 0)
    {
        double *rate = data;
        prop_name = "Rate";
        prop_value = g_variant_new_double(*rate);
    }
    else if (g_strcmp0(name, "volume") == 0)
    {
        double *volume = data;
        *volume /= 100;
        prop_name = "Volume";
        prop_value = g_variant_new_double(*volume);
    }
    else if (g_strcmp0(name, "loop-file") == 0)
    {
        char *status = *(char **)data;
        if (g_strcmp0(status, "no") != 0)
        {
            ud->loop_status = LOOP_TRACK;
        }
        else
        {
            char *playlist_status;
            mpv_get_property(ud->mpv, "loop-playlist", MPV_FORMAT_STRING, &playlist_status);
            if (g_strcmp0(playlist_status, "no") != 0)
            {
                ud->loop_status = LOOP_PLAYLIST;
            }
            else
            {
                ud->loop_status = LOOP_NONE;
            }
            mpv_free(playlist_status);
        }
        prop_name = "LoopStatus";
        prop_value = g_variant_new_string(ud->loop_status);
    }
    else if (g_strcmp0(name, "loop-playlist") == 0)
    {
        char *status = *(char **)data;
        if (g_strcmp0(status, "no") != 0)
        {
            ud->loop_status = LOOP_PLAYLIST;
        }
        else
        {
            char *file_status;
            mpv_get_property(ud->mpv, "loop-file", MPV_FORMAT_STRING, &file_status);
            if (g_strcmp0(file_status, "no") != 0)
            {
                ud->loop_status = LOOP_TRACK;
            }
            else
            {
                ud->loop_status = LOOP_NONE;
            }
            mpv_free(file_status);
        }
        prop_name = "LoopStatus";
        prop_value = g_variant_new_string(ud->loop_status);
    }
    else if (g_strcmp0(name, "shuffle") == 0)
    {
        int shuffle = *(int *)data;
        ud->shuffle = shuffle;
        prop_name = "Shuffle";
        prop_value = g_variant_new_boolean(shuffle);
    }
    else if (g_strcmp0(name, "fullscreen") == 0)
    {
        gboolean *status = data;
        prop_name = "Fullscreen";
        prop_value = g_variant_new_boolean(*status);
    }

    if (prop_name)
    {
        if (prop_value)
        {
            g_variant_ref(prop_value);
        }
        g_hash_table_insert(ud->changed_properties,
                            (gpointer)prop_name, prop_value);
    }
}

/**
 * Handle MPV events and update MPRIS state accordingly
 * @param fd File descriptor for MPV event notifications
 * @param condition GIO condition flags
 * @param data User data structure
 * @return TRUE to continue watching events
 */
gboolean event_handler(int fd, G_GNUC_UNUSED GIOCondition condition, gpointer data)
{
    UserData *ud = data;
    gboolean has_event = TRUE;

    // Discard data in pipe
    char unused[16];
    while (read(fd, unused, sizeof(unused)) > 0)
        ;

    while (has_event)
    {
        mpv_event *event = mpv_wait_event(ud->mpv, 0);
        switch (event->event_id)
        {
        case MPV_EVENT_NONE:
            has_event = FALSE;
            break;
        case MPV_EVENT_SHUTDOWN:
            set_stopped_status(ud);
            g_main_loop_quit(ud->loop);
            break;
        case MPV_EVENT_PROPERTY_CHANGE:
        {
            mpv_event_property *prop_event = (mpv_event_property *)event->data;
            handle_property_change(prop_event->name, prop_event->data, ud);
        }
        break;
        case MPV_EVENT_SEEK:
            ud->seek_expected = TRUE;
            break;
        case MPV_EVENT_PLAYBACK_RESTART:
        {
            if (ud->seek_expected)
            {
                emit_seeked_signal(ud);
                ud->seek_expected = FALSE;
            }
        }
        break;
        default:
            break;
        }
    }

    return TRUE;
}

/**
 * MPV wakeup callback to notify main loop of events
 * @param fd Pointer to file descriptor to write to
 */
void wakeup_handler(void *fd)
{
    (void)!write(*((int *)fd), "0", 1);
}
