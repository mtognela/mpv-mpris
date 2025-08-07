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

#include <gio/gio.h>
#include <glib-unix.h>
#include <mpv/client.h>
#include <libavformat/avformat.h>
#include <inttypes.h>
#include <string.h>

#define CACHE_MAX_AGE_DAYS 15
#define SECONDS_PER_DAY 86400

static const char art_files[][32] = {
    // Windows standard
    "Folder.jpg", "Folder.png", "Folder.gif", "Folder.webp",
    "AlbumArtSmall.jpg", "AlbumArt.jpg", "AlbumArt.png",

    // Common album art names
    "Album.jpg", "Album.png", "Album.gif", "Album.webp",
    "cover.jpg", "cover.png", "cover.gif", "cover.webp", "cover.bmp",
    "Cover.jpg", "Cover.png", "Cover.gif", "Cover.webp", "Cover.bmp",
    "COVER.JPG", "COVER.PNG", "COVER.GIF", "COVER.WEBP", "COVER.BMP",

    // Front cover variations
    "front.jpg", "front.png", "front.gif", "front.webp", "front.bmp",
    "Front.jpg", "Front.png", "Front.gif", "Front.webp", "Front.bmp",
    "FRONT.JPG", "FRONT.PNG", "FRONT.GIF", "FRONT.WEBP", "FRONT.BMP",

    // Artwork variations
    "artwork.jpg", "artwork.png", "artwork.gif", "artwork.webp",
    "Artwork.jpg", "Artwork.png", "Artwork.gif", "Artwork.webp",

    // Thumbnail variations
    "thumb.jpg", "thumb.png", "thumb.gif", "thumb.webp",
    "Thumb.jpg", "Thumb.png", "Thumb.gif", "Thumb.webp",
    "thumbnail.jpg", "thumbnail.png", "thumbnail.gif", "thumbnail.webp",

    // Other common names
    "albumart.jpg", "albumart.png", "albumcover.jpg", "albumcover.png",
    "cd.jpg", "cd.png", "disc.jpg", "disc.png",
    "music.jpg", "music.png", "audio.jpg", "audio.png",

    // KDE and other desktop environments
    ".folder.png", ".folder.jpg", ".cover.jpg", ".cover.png",

    // Case variations for case-sensitive filesystems
    "folder.jpg", "folder.png", "folder.gif", "folder.webp",

    // High resolution variants
    "cover-large.jpg", "cover-large.png", "cover-hq.jpg", "cover-hq.png",
    "front-large.jpg", "front-large.png", "front-hq.jpg", "front-hq.png",

    // Specific media player conventions
    "AlbumArt_{*}_Large.jpg", "AlbumArt_{*}_Small.jpg", // Windows Media Player

    // International variations
    "portada.jpg", "portada.png",   // Spanish
    "caratula.jpg", "caratula.png", // Spanish
    "capa.jpg", "capa.png",         // Portuguese
    "pochette.jpg", "pochette.png", // French
};

typedef struct UserData
{
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

    // cache filed
    char *cached_path;     // owned by mpv
    gchar *cached_art_url; // owned by glib
} UserData;

static const char *STATUS_PLAYING = "Playing";
static const char *STATUS_PAUSED = "Paused";
static const char *STATUS_STOPPED = "Stopped";
static const char *LOOP_NONE = "None";
static const char *LOOP_TRACK = "Track";
static const char *LOOP_PLAYLIST = "Playlist";
static const char *youtube_url_pattern =
    "^https?:\\/\\/(?:youtu.be\\/|(?:www\\.)?youtube\\.com\\/watch\\?v=)(?<id>[a-zA-Z0-9_-]*)\\??.*";

static const char *supported_extensions[] = {
    // Common formats
    ".jpg", ".jpeg", ".jpe", ".jfif", ".jfi",
    ".png", ".gif", ".webp", ".bmp", ".dib",
    ".tiff", ".tif", ".avif", ".heic", ".heif",
    ".ico", ".cur",

    // RAW camera formats
    ".cr2", ".crw", ".nef", ".nrw", ".arw", ".srf", ".sr2",
    ".orf", ".rw2", ".pef", ".ptx", ".dng", ".raf", ".mrw",
    ".dcr", ".kdc", ".erf", ".3fr", ".mef", ".mos", ".x3f",

    // Professional/specialized formats
    ".psd", ".psb",                                 // Adobe Photoshop
    ".xcf",                                         // GIMP
    ".exr", ".hdr", ".pic",                         // High Dynamic Range
    ".dpx", ".cin",                                 // Digital cinema
    ".sgi", ".rgb", ".bw",                          // SGI formats
    ".sun", ".ras",                                 // Sun raster
    ".pnm", ".pbm", ".pgm", ".ppm", ".pam",         // Netpbm formats
    ".pfm",                                         // Portable float map
    ".pcx",                                         // PC Paintbrush
    ".tga", ".icb", ".vda", ".vst",                 // Targa formats
    ".jp2", ".j2k", ".jpf", ".jpx", ".jpm", ".mj2", // JPEG 2000
    ".jxr", ".wdp", ".hdp",                         // JPEG XR / HD Photo
    ".jxl",                                         // JPEG XL

    // Vector formats (if supported)
    ".svg", ".svgz",

    // Animation formats
    ".apng", ".mng",

    // Less common formats
    ".xbm", ".xpm",          // X11 bitmap/pixmap
    ".wbmp",                 // Wireless bitmap
    ".fits", ".fit", ".fts", // Flexible Image Transport System
    ".flif",                 // Free Lossless Image Format
    ".qoi",                  // Quite OK Image format
};

static GRegex *youtube_url_regex;

static GMutex metadata_mutex;

static gboolean is_supported_image_file(const char *filename)
{
    for (size_t i = 0; i < sizeof(supported_extensions) / sizeof(supported_extensions[0]); i++)
    {
        if (g_str_has_suffix(filename, supported_extensions[i]))
        {
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean is_art_file(const char *filename)
{
    const int art_files_count = sizeof(art_files) / sizeof(art_files[0]);

    for (int i = 0; i < art_files_count; i++)
    {
        // Simple string comparison for exact matches
        if (g_strcmp0(filename, art_files[i]) == 0)
        {
            return TRUE;
        }

        // Handle wildcard patterns (basic implementation)
        if (strstr(art_files[i], "{*}") != NULL)
        {
            // Extract prefix and suffix for pattern matching
            gchar **parts = g_strsplit(art_files[i], "{*}", 2);
            if (parts[0] && parts[1])
            {
                if (g_str_has_prefix(filename, parts[0]) &&
                    g_str_has_suffix(filename, parts[1]))
                {
                    g_strfreev(parts);
                    return TRUE;
                }
            }
            g_strfreev(parts);
        }
    }

    // Also check if it's a supported image file in the same directory
    return is_supported_image_file(filename);
}

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

static const char *get_image_extension(const uint8_t *data, size_t size)
{
    if (!data || size < 4)
    {
        return ".jpg"; // fallback for invalid input
    }

    // JPEG - FF D8 FF (followed by various markers)
    if (size >= 4 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)
    {
        return ".jpg";
    }

    // PNG - 89 50 4E 47 0D 0A 1A 0A
    if (size >= 8 && memcmp(data, "\x89PNG\r\n\x1a\n", 8) == 0)
    {
        return ".png";
    }

    // GIF87a - 47 49 46 38 37 61
    if (size >= 6 && memcmp(data, "GIF87a", 6) == 0)
    {
        return ".gif";
    }

    // GIF89a - 47 49 46 38 39 61
    if (size >= 6 && memcmp(data, "GIF89a", 6) == 0)
    {
        return ".gif";
    }

    // WebP - RIFF header with WEBP signature
    if (size >= 12 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WEBP", 4) == 0)
    {
        return ".webp";
    }

    // BMP - BM signature
    if (size >= 2 && data[0] == 0x42 && data[1] == 0x4D)
    {
        return ".bmp";
    }

    // TIFF (Little Endian) - 49 49 2A 00
    if (size >= 4 && memcmp(data, "II\x2A\x00", 4) == 0)
    {
        return ".tiff";
    }

    // TIFF (Big Endian) - 4D 4D 00 2A
    if (size >= 4 && memcmp(data, "MM\x00\x2A", 4) == 0)
    {
        return ".tiff";
    }

    // AVIF - ftyp box with AVIF brand
    if (size >= 12 && memcmp(data + 4, "ftypavif", 8) == 0)
    {
        return ".avif";
    }

    // HEIC/HEIF - ftyp box with various HEIC brands
    if (size >= 12 && memcmp(data + 4, "ftyp", 4) == 0)
    {
        const char *brand = (const char *)(data + 8);
        if (memcmp(brand, "heic", 4) == 0 ||
            memcmp(brand, "heix", 4) == 0 ||
            memcmp(brand, "hevc", 4) == 0 ||
            memcmp(brand, "hevx", 4) == 0 ||
            memcmp(brand, "heim", 4) == 0 ||
            memcmp(brand, "heis", 4) == 0 ||
            memcmp(brand, "hevm", 4) == 0 ||
            memcmp(brand, "hevs", 4) == 0 ||
            memcmp(brand, "mif1", 4) == 0 ||
            memcmp(brand, "msf1", 4) == 0)
        {
            return ".heic";
        }
    }

    // ICO - 00 00 01 00 (icon) or 00 00 02 00 (cursor)
    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00 &&
        (data[2] == 0x01 || data[2] == 0x02) && data[3] == 0x00)
    {
        return data[2] == 0x01 ? ".ico" : ".cur";
    }

    // PSD - 38 42 50 53
    if (size >= 4 && memcmp(data, "8BPS", 4) == 0)
    {
        return ".psd";
    }

    // GIMP XCF - gimp xcf (with version info)
    if (size >= 9 && memcmp(data, "gimp xcf ", 9) == 0)
    {
        return ".xcf";
    }

    // OpenEXR - 76 2F 31 01
    if (size >= 4 && memcmp(data, "\x76\x2f\x31\x01", 4) == 0)
    {
        return ".exr";
    }

    // Radiance HDR - #?RADIANCE or #?RGBE
    if (size >= 10 && (memcmp(data, "#?RADIANCE", 10) == 0 ||
                       memcmp(data, "#?RGBE", 6) == 0))
    {
        return ".hdr";
    }

    // JPEG 2000 - 00 00 00 0C 6A 50 20 20
    if (size >= 8 && memcmp(data, "\x00\x00\x00\x0c\x6a\x50\x20\x20", 8) == 0)
    {
        return ".jp2";
    }

    // JPEG 2000 codestream - FF 4F FF 51
    if (size >= 4 && memcmp(data, "\xff\x4f\xff\x51", 4) == 0)
    {
        return ".j2k";
    }

    // JPEG XL - FF 0A or 00 00 00 0C 4A 58 4C 20
    if (size >= 2 && memcmp(data, "\xff\x0a", 2) == 0)
    {
        return ".jxl";
    }
    if (size >= 12 && memcmp(data, "\x00\x00\x00\x0c\x4a\x58\x4c\x20\x0d\x0a\x87\x0a", 12) == 0)
    {
        return ".jxl";
    }

    // JPEG XR - II BC (little endian) or MM BC (big endian)
    if (size >= 2 && ((data[0] == 0x49 && data[1] == 0x49) || (data[0] == 0x4D && data[1] == 0x4D)))
    {
        // Check for JPEG XR specific markers
        if (size >= 4 && data[2] == 0xBC && (data[3] == 0x00 || data[3] == 0x01))
        {
            return ".jxr";
        }
    }

    // TGA - check footer signature (TRUEVISION-XFILE.)
    if (size >= 26)
    {
        const char *footer_sig = "TRUEVISION-XFILE.";
        if (memcmp(data + size - 18, footer_sig, 18) == 0)
        {
            return ".tga";
        }
    }
    // TGA - heuristic check for headerless TGA (very basic)
    if (size >= 18 && data[1] <= 1 && (data[2] == 1 || data[2] == 2 || data[2] == 3 || data[2] == 9 || data[2] == 10 || data[2] == 11))
    {
        return ".tga";
    }

    // PCX - 0A XX 01
    if (size >= 3 && data[0] == 0x0A && data[2] == 0x01)
    {
        return ".pcx";
    }

    // SVG - check for XML declaration and svg tag
    if (size >= 5 && (memcmp(data, "<?xml", 5) == 0 || memcmp(data, "<svg", 4) == 0))
    {
        // Look for svg tag within first 1000 bytes
        size_t search_len = size > 1000 ? 1000 : size;
        for (size_t i = 0; i < search_len - 3; i++)
        {
            if (memcmp(data + i, "<svg", 4) == 0)
            {
                return ".svg";
            }
        }
    }

    // PNM family
    if (size >= 2 && data[0] == 'P')
    {
        switch (data[1])
        {
        case '1':
        case '4':
            return ".pbm"; // Portable bitmap
        case '2':
        case '5':
            return ".pgm"; // Portable graymap
        case '3':
        case '6':
            return ".ppm"; // Portable pixmap
        case '7':
            return ".pam"; // Portable arbitrary map
        }
    }

    // XBM - #define
    if (size >= 7 && memcmp(data, "#define", 7) == 0)
    {
        return ".xbm";
    }

    // XPM - /* XPM */
    if (size >= 9 && memcmp(data, "/* XPM */", 9) == 0)
    {
        return ".xpm";
    }

    // FITS - SIMPLE
    if (size >= 6 && memcmp(data, "SIMPLE", 6) == 0)
    {
        return ".fits";
    }

    // FLIF - FLIF
    if (size >= 4 && memcmp(data, "FLIF", 4) == 0)
    {
        return ".flif";
    }

    // QOI - qoif
    if (size >= 4 && memcmp(data, "qoif", 4) == 0)
    {
        return ".qoi";
    }

    // WBMP - 00 00 (followed by width and height)
    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00)
    {
        return ".wbmp";
    }

    // Default fallback - JPEG is most common for embedded album art
    return ".jpg";
}

static gchar *try_get_local_art(mpv_handle *mpv, char *path)
{
    gchar *dirname = g_path_get_dirname(path);
    gchar *out = NULL;
    gboolean found = FALSE;

    // Calculate art_files_count locally instead of using the global variable
    const int local_art_files_count = sizeof(art_files) / sizeof(art_files[0]);

    // First, try the predefined art file names
    for (int i = 0; i < local_art_files_count && !found; i++)
    {
        // Skip wildcard patterns for now
        if (strstr(art_files[i], "{*}") != NULL)
        {
            continue;
        }

        gchar *filename = g_build_filename(dirname, art_files[i], NULL);

        if (g_file_test(filename, G_FILE_TEST_EXISTS))
        {
            out = path_to_uri(mpv, filename);
            found = TRUE;
        }

        g_free(filename);
    }

    // If no predefined art files found, scan directory for any image files
    if (!found)
    {
        GDir *dir = g_dir_open(dirname, 0, NULL);
        if (dir)
        {
            const gchar *filename;
            while ((filename = g_dir_read_name(dir)) != NULL && !found)
            {
                // Use is_art_file function here to make it used
                if (is_art_file(filename))
                {
                    gchar *full_path = g_build_filename(dirname, filename, NULL);
                    if (g_file_test(full_path, G_FILE_TEST_IS_REGULAR))
                    {
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

static gchar *try_get_youtube_thumbnail(const char *path)
{
    gchar *out = NULL;
    if (!youtube_url_regex)
    {
        youtube_url_regex = g_regex_new(youtube_url_pattern, 0, 0, NULL);
    }

    GMatchInfo *match_info;
    gboolean matched = g_regex_match(youtube_url_regex, path, 0, &match_info);

    if (matched)
    {
        gchar *video_id = g_match_info_fetch_named(match_info, "id");
        out = g_strconcat("https://i1.ytimg.com/vi/",
                          video_id, "/hqdefault.jpg", NULL);
        g_free(video_id);
    }

    g_match_info_free(match_info);
    return out;
}

static gchar *get_cache_dir()
{
    gchar *cache_dir = g_build_filename(g_get_user_cache_dir(), "mpv-mpris", "coverart", NULL);

    if (g_mkdir_with_parents(cache_dir, 0755) < 0)
    {
        g_warning("Failed to create cache directory: %s", g_strerror(errno));
        g_free(cache_dir);
        return NULL;
    }

    return cache_dir;
}

static gchar *generate_cache_filename(const char *path, const uint8_t *image_data, size_t image_size)
{
    gchar *hash = g_compute_checksum_for_string(G_CHECKSUM_SHA256, path, -1);
    const char *ext = get_image_extension(image_data, image_size);
    gchar *filename = g_strconcat(hash, ext, NULL);
    g_free(hash);
    return filename;
}

static gchar *extract_embedded_art(AVFormatContext *context, const char *media_path)
{
    AVPacket *packet = NULL;
    gchar *cache_path = NULL;
    gchar *uri = NULL;

    for (unsigned int i = 0; i < context->nb_streams; i++)
    {
        if (context->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC)
        {
            packet = &context->streams[i]->attached_pic;
            break;
        }
    }

    if (!packet)
    {
        return NULL;
    }

    gchar *cache_dir = get_cache_dir();
    if (!cache_dir)
    {
        return NULL;
    }

    // Use the new function that detects the correct extension
    gchar *cache_filename = generate_cache_filename(media_path, packet->data, packet->size);
    cache_path = g_build_filename(cache_dir, cache_filename, NULL);
    g_free(cache_filename);

    if (!g_file_test(cache_path, G_FILE_TEST_EXISTS))
    {
        GError *error = NULL;
        if (!g_file_set_contents(cache_path, (const gchar *)packet->data,
                                 packet->size, &error))
        {
            g_warning("Failed to write cover art to cache: %s", error->message);
            g_error_free(error);
            g_free(cache_path);
            g_free(cache_dir);
            return NULL;
        }
    }

    uri = g_filename_to_uri(cache_path, NULL, NULL);

    g_free(cache_path);
    g_free(cache_dir);
    return uri;
}

static gchar *try_get_embedded_art(char *path)
{
    if (!path)
    {
        g_warning("Null path provided for embedded art lookup");
        return NULL;
    }

    AVFormatContext *context = NULL;

    int ret = avformat_open_input(&context, path, NULL, NULL);
    if (ret < 0)
    {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, sizeof(errbuf));
        g_warning("Failed to open input '%s': %s", path, errbuf);
        return NULL;
    }

    gchar *uri = NULL;

    if (!avformat_open_input(&context, path, NULL, NULL))
    {
        uri = extract_embedded_art(context, path);
        avformat_close_input(&context);
    }

    return uri;
}

static void cleanup_old_cache_files()
{
    gchar *cache_dir = get_cache_dir();

    if (!cache_dir)
    {
        return;
    }

    DIR *dir = opendir(cache_dir);

    if (!dir)
    {
        g_free(cache_dir);
        return;
    }

    time_t current_time = time(NULL);
    time_t max_age = CACHE_MAX_AGE_DAYS * SECONDS_PER_DAY; // Convert days to seconds

    struct dirent *entry;
    struct stat file_stat;

    while ((entry = readdir(dir)) != NULL)
    {
        // Skip . and .. directories
        if (g_strcmp0(entry->d_name, ".") == 0 || g_strcmp0(entry->d_name, "..") == 0)
        {
            continue;
        }

        if (!is_supported_image_file(entry->d_name))
        {
            continue;
        }

        gchar *file_path = g_build_filename(cache_dir, entry->d_name, NULL);

        if (stat(file_path, &file_stat) == 0)
        {
            // Check if file is older than max age
            if (current_time - file_stat.st_mtime > max_age)
            {
                if (unlink(file_path) == 0)
                {
                    g_debug("Cleaned up old cache file: %s", entry->d_name);
                }
                else
                {
                    g_warning("Failed to remove old cache file: %s", file_path);
                }
            }
        }

        g_free(file_path);
    }

    closedir(dir);
    g_free(cache_dir);
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

static void method_call_root(G_GNUC_UNUSED GDBusConnection *connection,
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

static GVariant *get_property_root(G_GNUC_UNUSED GDBusConnection *connection,
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

static gboolean set_property_root(G_GNUC_UNUSED GDBusConnection *connection,
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

static GDBusInterfaceVTable vtable_root = {
    method_call_root, get_property_root, set_property_root, {0}};

static void method_call_player(G_GNUC_UNUSED GDBusConnection *connection,
                               G_GNUC_UNUSED const char *sender,
                               G_GNUC_UNUSED const char *_object_path,
                               G_GNUC_UNUSED const char *interface_name,
                               const char *method_name,
                               G_GNUC_UNUSED GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data)
{
    UserData *ud = (UserData *)user_data;
    if (!ud || !ud->mpv)
    {
        g_warning("Invalid user data in method call");
        return;
    }

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

static GVariant *get_property_player(G_GNUC_UNUSED GDBusConnection *connection,
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

static gboolean set_property_player(G_GNUC_UNUSED GDBusConnection *connection,
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

static GDBusInterfaceVTable vtable_player = {
    method_call_player, get_property_player, set_property_player, {0}};

static gboolean emit_property_changes(gpointer data)
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

static void emit_seeked_signal(UserData *ud)
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

static GVariant *set_playback_status(UserData *ud)
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

static void set_stopped_status(UserData *ud)
{
    const char *prop_name = "PlaybackStatus";
    GVariant *prop_value = g_variant_new_string(STATUS_STOPPED);

    ud->status = STATUS_STOPPED;

    g_hash_table_insert(ud->changed_properties,
                        (gpointer)prop_name, prop_value);

    emit_property_changes(ud);
}

// Register D-Bus object and interfaces
static void on_bus_acquired(GDBusConnection *connection,
                            G_GNUC_UNUSED const char *name,
                            gpointer user_data)
{
    GError *error = NULL;
    UserData *ud = user_data;

    if (!connection)
    {
        g_printerr("D-Bus connection is NULL\n");
        return;
    }

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

static void on_name_lost(GDBusConnection *connection,
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

static void handle_property_change(const char *name, void *data, UserData *ud)
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

static gboolean event_handler(int fd, G_GNUC_UNUSED GIOCondition condition, gpointer data)
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

static void wakeup_handler(void *fd)
{
    (void)!write(*((int *)fd), "0", 1);
}

