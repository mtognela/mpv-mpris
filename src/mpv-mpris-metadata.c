#include "mpv-mpris-types.h"

static gchar *string_to_utf8(gchar *maybe_utf8)
{
    gchar *attempted_validation;
    attempted_validation = g_utf8_make_valid(maybe_utf8, -1);

    if (g_utf8_validate(attempted_validation, -1, NULL))
    {
        return attempted_validation;
    }
    else
    {
        g_free(attempted_validation);
        return g_strdup("<invalid utf8>");
    }
}

static gchar *path_to_uri(mpv_handle *mpv, char *path)
{
    if (!path)
    {
        return NULL;
    }

    gchar *uri = NULL;

#if GLIB_CHECK_VERSION(2, 58, 0)
    char *working_dir = mpv_get_property_string(mpv, "working-directory");
    if (!working_dir)
    {
        g_warning("Failed to get working directory");
        return NULL;
    }

    gchar *canonical = g_canonicalize_filename(path, working_dir);
    mpv_free(working_dir);

    if (!canonical)
    {
        g_warning("Failed to canonicalize path");
        return NULL;
    }

    uri = g_filename_to_uri(canonical, NULL, NULL);
    g_free(canonical);
#else
    // for compatibility with older versions of glib
    if (g_path_is_absolute(path))
    {
        uri = g_filename_to_uri(path, NULL, NULL);
        if (!uri)
        {
            g_warning("Failed to convert absolute path to URI: %s", path);
        }
    }
    else
    {
        char *working_dir = NULL;
        gchar *absolute = NULL;
        GError *error = NULL;

        working_dir = mpv_get_property_string(mpv, "working-directory");
        if (!working_dir)
        {
            g_warning("Failed to get working directory");
            goto legacy_cleanup;
        }

        absolute = g_build_filename(working_dir, path, NULL);
        if (!absolute)
        {
            g_warning("Failed to build absolute path");
            goto legacy_cleanup;
        }

        uri = g_filename_to_uri(absolute, NULL, &error);
        if (!uri)
        {
            g_warning("Failed to convert path to URI: %s (Error: %s)",
                      absolute, error ? error->message : "unknown error");
            if (error)
            {
                g_error_free(error);
            }
        }

    legacy_cleanup:
        if (working_dir)
        {
            mpv_free(working_dir);
        }
        if (absolute)
        {
            g_free(absolute);
        }
    }
#endif

    if (!uri)
    {
        g_warning("Failed to convert path to URI: %s", path);
    }

    return uri;
}

static void add_metadata_uri(mpv_handle *mpv, GVariantDict *dict)
{
    char *path;
    char *uri;

    path = mpv_get_property_string(mpv, "path");
    if (!path)
    {
        return;
    }

    uri = g_uri_parse_scheme(path);
    if (uri)
    {
        g_variant_dict_insert(dict, "xesam:url", "s", path);
        g_free(uri);
    }
    else
    {
        gchar *converted = path_to_uri(mpv, path);
        g_variant_dict_insert(dict, "xesam:url", "s", converted);
        g_free(converted);
    }

    mpv_free(path);
}

static void add_metadata_item_string(mpv_handle *mpv, GVariantDict *dict,
                                     const char *property, const char *tag)
{
    if (!mpv || !dict || !property || !tag)
    {
        g_warning("Invalid arguments to add_metadata_item_string");
        return;
    }

    char *temp = mpv_get_property_string(mpv, property);
    if (!temp)
    {
        g_debug("Property %s not available", property);
        return;
    }

    char *utf8 = string_to_utf8(temp);
    if (!utf8)
    {
        mpv_free(temp);
        g_warning("Failed to convert %s to UTF-8", property);
        return;
    }

    g_variant_dict_insert(dict, tag, "s", utf8);
    g_free(utf8);
    mpv_free(temp);
}

static void add_metadata_item_int(mpv_handle *mpv, GVariantDict *dict,
                                  const char *property, const char *tag)
{
    int64_t value;
    int res = mpv_get_property(mpv, property, MPV_FORMAT_INT64, &value);
    if (res >= 0)
    {
        g_variant_dict_insert(dict, tag, "x", value);
    }
}

static void add_metadata_item_string_list(mpv_handle *mpv, GVariantDict *dict,
                                          const char *property, const char *tag)
{
    char *temp = mpv_get_property_string(mpv, property);

    if (temp)
    {
        GVariantBuilder builder;
        char **list = g_strsplit(temp, ", ", 0);
        char **iter = list;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));

        for (; *iter; iter++)
        {
            char *item = *iter;
            char *utf8 = string_to_utf8(item);
            g_variant_builder_add(&builder, "s", utf8);
            g_free(utf8);
        }

        g_variant_dict_insert(dict, tag, "as", &builder);

        g_strfreev(list);
        mpv_free(temp);
    }
}


static void add_metadata_art(mpv_handle *mpv, GVariantDict *dict, UserData *ud)
{
    char *path = mpv_get_property_string(mpv, "path");

    if (!path)
    {
        return;
    }

    // Check cache using UserData instead of globals
    if (!ud->cached_path || strcmp(path, ud->cached_path))
    {
        // Clear old cache
        mpv_free(ud->cached_path);
        g_free(ud->cached_art_url);

        // Set new cache
        ud->cached_path = path;

        if (g_str_has_prefix(path, "http"))
        {
            ud->cached_art_url = try_get_youtube_thumbnail(path);
        }
        else
        {
            ud->cached_art_url = try_get_embedded_art(path);
            if (!ud->cached_art_url)
            {
                ud->cached_art_url = try_get_local_art(mpv, path);
            }
        }
    }
    else
    {
        mpv_free(path);
    }

    if (ud->cached_art_url)
    {
        g_variant_dict_insert(dict, "mpris:artUrl", "s", ud->cached_art_url);
    }
}

static void add_metadata_content_created(mpv_handle *mpv, GVariantDict *dict)
{
    char *date_str = mpv_get_property_string(mpv, "metadata/by-key/Date");

    if (!date_str)
    {
        return;
    }

    GDate *date = g_date_new();
    if (strlen(date_str) == 4)
    {
        gint64 year = g_ascii_strtoll(date_str, NULL, 10);
        if (year != 0)
        {
            g_date_set_dmy(date, 1, 1, year);
        }
    }
    else
    {
        g_date_set_parse(date, date_str);
    }

    if (g_date_valid(date))
    {
        gchar iso8601[20];
        g_date_strftime(iso8601, 20, "%Y-%m-%dT00:00:00Z", date);
        g_variant_dict_insert(dict, "xesam:contentCreated", "s", iso8601);
    }

    g_date_free(date);
    mpv_free(date_str);
}

static GVariant *create_metadata(UserData *ud)
{
    g_mutex_lock(&metadata_mutex);

    GVariantDict dict;
    int64_t track;
    double duration;
    char *temp_str;
    int res;

    g_variant_dict_init(&dict, NULL);

    // mpris:trackid
    mpv_get_property(ud->mpv, "playlist-pos", MPV_FORMAT_INT64, &track);
    // playlist-pos < 0 if there is no playlist or current track
    if (track < 0)
    {
        temp_str = g_strdup("/noplaylist");
    }
    else
    {
        temp_str = g_strdup_printf("/%" PRId64, track);
    }
    g_variant_dict_insert(&dict, "mpris:trackid", "o", temp_str);
    g_free(temp_str);

    // mpris:length
    res = mpv_get_property(ud->mpv, "duration", MPV_FORMAT_DOUBLE, &duration);
    if (res == MPV_ERROR_SUCCESS)
    {
        g_variant_dict_insert(&dict, "mpris:length", "x", (int64_t)(duration * 1000000.0));
    }

    // initial value. Replaced with metadata value if available
    add_metadata_item_string(ud->mpv, &dict, "media-title", "xesam:title");

    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/Title", "xesam:title");
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/Album", "xesam:album");
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/Genre", "xesam:genre");

    /* Musicbrainz metadata mappings
       (https://picard-docs.musicbrainz.org/en/appendices/tag_mapping.html) */

    // IDv3 metadata format
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/MusicBrainz Artist Id", "mb:artistId");
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/MusicBrainz Track Id", "mb:trackId");
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/MusicBrainz Album Artist Id", "mb:albumArtistId");
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/MusicBrainz Album Id", "mb:albumId");
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/MusicBrainz Release Track Id", "mb:releaseTrackId");
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/MusicBrainz Work Id", "mb:workId");

    // Vorbis & APEv2 metadata format
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/MUSICBRAINZ_ARTISTID", "mb:artistId");
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/MUSICBRAINZ_TRACKID", "mb:trackId");
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/MUSICBRAINZ_ALBUMARTISTID", "mb:albumArtistId");
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/MUSICBRAINZ_ALBUMID", "mb:albumId");
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/MUSICBRAINZ_RELEASETRACKID", "mb:releaseTrackId");
    add_metadata_item_string(ud->mpv, &dict, "metadata/by-key/MUSICBRAINZ_WORKID", "mb:workId");

    add_metadata_item_string_list(ud->mpv, &dict, "metadata/by-key/uploader", "xesam:artist");
    add_metadata_item_string_list(ud->mpv, &dict, "metadata/by-key/Artist", "xesam:artist");
    add_metadata_item_string_list(ud->mpv, &dict, "metadata/by-key/Album_Artist", "xesam:albumArtist");
    add_metadata_item_string_list(ud->mpv, &dict, "metadata/by-key/Composer", "xesam:composer");

    add_metadata_item_int(ud->mpv, &dict, "metadata/by-key/Track", "xesam:trackNumber");
    add_metadata_item_int(ud->mpv, &dict, "metadata/by-key/Disc", "xesam:discNumber");

    add_metadata_uri(ud->mpv, &dict);
    add_metadata_art(ud->mpv, &dict, ud);
    add_metadata_content_created(ud->mpv, &dict);

    GVariant *result = g_variant_dict_end(&dict);
    g_mutex_unlock(&metadata_mutex);
    return result;
}
