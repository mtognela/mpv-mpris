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

#include "mpv-mpris-types.h"
#include "mpv-mpris-metadata.h"
#include "mpv-mpris-artwork.h"

gchar *string_to_utf8(gchar *maybe_utf8)
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

gchar *path_to_uri(mpv_handle *mpv, char *path)
{
    #if GLIB_CHECK_VERSION(2, 58, 0)
        // version which uses g_canonicalize_filename which expands .. and .
        // and makes the uris neater
        char *working_dir;
        gchar *canonical;
        gchar *uri;

        working_dir = mpv_get_property_string(mpv, "working-directory");
        canonical = g_canonicalize_filename(path, working_dir);
        uri = g_filename_to_uri(canonical, NULL, NULL);

        mpv_free(working_dir);
        g_free(canonical);

        return uri;
    #else
        // for compatibility with older versions of glib
        gchar *converted;
        if (g_path_is_absolute(path))
        {
            converted = g_filename_to_uri(path, NULL, NULL);
        }
        else
        {
            char *working_dir;
            gchar *absolute;

            working_dir = mpv_get_property_string(mpv, "working-directory");
            absolute = g_build_filename(working_dir, path, NULL);
            converted = g_filename_to_uri(absolute, NULL, NULL);

            mpv_free(working_dir);
            g_free(absolute);
        }

        return converted;
    #endif
}

void add_metadata_item_string(mpv_handle *mpv, GVariantDict *dict,
                                     const char *property, const char *tag)
{
    char *temp = mpv_get_property_string(mpv, property);
    if (temp)
    {
        char *utf8 = string_to_utf8(temp);
        g_variant_dict_insert(dict, tag, "s", utf8);
        g_free(utf8);
        mpv_free(temp);
    }
}

void add_metadata_item_int(mpv_handle *mpv, GVariantDict *dict,
                                  const char *property, const char *tag)
{
    int64_t value;
    int res = mpv_get_property(mpv, property, MPV_FORMAT_INT64, &value);
    if (res >= 0)
    {
        g_variant_dict_insert(dict, tag, "x", value);
    }
}

void add_metadata_item_string_list(mpv_handle *mpv, GVariantDict *dict,
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

void add_metadata_uri(mpv_handle *mpv, GVariantDict *dict)
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

void add_metadata_art(mpv_handle *mpv, GVariantDict *dict, UserData *ud)
{
    char *path = mpv_get_property_string(mpv, "path");

    if (!path) {
        return;
    }

    // Check cache using UserData instead of globals
    if (!ud->cached_path || strcmp(path, ud->cached_path)) {
        // Clear old cache
        mpv_free(ud->cached_path);
        g_free(ud->cached_art_url);
        
        // Set new cache
        ud->cached_path = path;

        if (g_str_has_prefix(path, "http")) {
            ud->cached_art_url = try_get_youtube_thumbnail(path);
        } else {
            ud->cached_art_url = try_get_embedded_art(path);
            if (!ud->cached_art_url) {
                ud->cached_art_url = try_get_local_art_enhanced(mpv, path);
            }
        }
    } else {
        mpv_free(path);
    }

    if (ud->cached_art_url) {
        g_variant_dict_insert(dict, "mpris:artUrl", "s", ud->cached_art_url);
    }
}

void add_metadata_content_created(mpv_handle *mpv, GVariantDict *dict)
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

GVariant *create_metadata(UserData *ud)
{
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

    return g_variant_dict_end(&dict);
}

gchar *try_get_local_art_enhanced(mpv_handle *mpv, const char *path) {
    gchar *dirname = g_path_get_dirname(path);
    gchar *out = NULL;
    gboolean found = FALSE;
    
    // Calculate art_files_count locally instead of using the global variable
    const int local_art_files_count = sizeof(&art_files) / sizeof(art_files[0]);
    
    // First, try the predefined art file names
    for (int i = 0; i < local_art_files_count && !found; i++) {
        // Skip wildcard patterns for now
        if (strstr(art_files[i], "{*}") != NULL) {
            continue;
        }
        
        gchar *filename = g_build_filename(dirname, art_files[i], NULL);
        
        if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
            out = path_to_uri(mpv, filename);
            found = TRUE;
        }
        
        g_free(filename);
    }
    
    // If no predefined art files found, scan directory for any image files
    if (!found) {
        GDir *dir = g_dir_open(dirname, 0, NULL);
        if (dir) {
            const gchar *filename;
            while ((filename = g_dir_read_name(dir)) != NULL && !found) {
                // Use is_art_file function here to make it used
                if (is_art_file(filename)) {
                    gchar *full_path = g_build_filename(dirname, filename, NULL);
                    if (g_file_test(full_path, G_FILE_TEST_IS_REGULAR)) {
                        out = path_to_uri(mpv, full_path);
                        found = TRUE;
                    }
                    g_free(full_path);
                }
            }
            g_dir_close(dir);
        }
    }
    
    g_free(dirname);
    return out;
}
