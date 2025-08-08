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
#include "mpv-mpris-artwork.h"

gchar* extract_embedded_art(AVFormatContext *context, const char *media_path) {
    AVPacket *packet = NULL;
    gchar *cache_path = NULL;
    gchar *uri = NULL;
    
    for (unsigned int i = 0; i < context->nb_streams; i++) {
        if (context->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            packet = &context->streams[i]->attached_pic;
            break;
        }
    }
    
    if (!packet) {
        return NULL;
    }

    gchar *cache_dir = get_cache_dir();
    if (!cache_dir) {
        return NULL;
    }

    // Use the new function that detects the correct extension
    gchar *cache_filename = generate_cache_filename(media_path, packet->data, packet->size);
    cache_path = g_build_filename(cache_dir, cache_filename, NULL);
    g_free(cache_filename);
    
    if (!g_file_test(cache_path, G_FILE_TEST_EXISTS)) {
        GError *error = NULL;
        if (!g_file_set_contents(cache_path, (const gchar*)packet->data, 
                                packet->size, &error)) {
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

gboolean is_supported_image_file(const char *filename) {
    for (size_t i = 0; i < sizeof(&supported_extensions) / sizeof(supported_extensions[0]); i++) {
        if (g_str_has_suffix(filename, supported_extensions[i])) {
            return TRUE;
        }
    }
    return FALSE;
}

gboolean is_art_file(const char *filename) {
    const int art_files_count = sizeof(&art_files) / sizeof(art_files[0]);
    
    for (int i = 0; i < art_files_count; i++) {
        // Simple string comparison for exact matches
        if (g_strcmp0(filename, art_files[i]) == 0) {
            return TRUE;
        }
        
        // Handle wildcard patterns (basic implementation)
        if (strstr(art_files[i], "{*}") != NULL) {
            // Extract prefix and suffix for pattern matching
            gchar **parts = g_strsplit(art_files[i], "{*}", 2);
            if (parts[0] && parts[1]) {
                if (g_str_has_prefix(filename, parts[0]) && 
                    g_str_has_suffix(filename, parts[1])) {
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

const char* get_image_extension(const uint8_t *data, size_t size) {
    if (!data || size < 4) {
        return ".jpg"; // fallback for invalid input
    }

    // JPEG - FF D8 FF (followed by various markers)
    if (size >= 4 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
        return ".jpg";
    }

    // PNG - 89 50 4E 47 0D 0A 1A 0A
    if (size >= 8 && memcmp(data, "\x89PNG\r\n\x1a\n", 8) == 0) {
        return ".png";
    }

    // GIF87a - 47 49 46 38 37 61
    if (size >= 6 && memcmp(data, "GIF87a", 6) == 0) {
        return ".gif";
    }

    // GIF89a - 47 49 46 38 39 61
    if (size >= 6 && memcmp(data, "GIF89a", 6) == 0) {
        return ".gif";
    }

    // WebP - RIFF header with WEBP signature
    if (size >= 12 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WEBP", 4) == 0) {
        return ".webp";
    }

    // BMP - BM signature
    if (size >= 2 && data[0] == 0x42 && data[1] == 0x4D) {
        return ".bmp";
    }

    // TIFF (Little Endian) - 49 49 2A 00
    if (size >= 4 && memcmp(data, "II\x2A\x00", 4) == 0) {
        return ".tiff";
    }

    // TIFF (Big Endian) - 4D 4D 00 2A
    if (size >= 4 && memcmp(data, "MM\x00\x2A", 4) == 0) {
        return ".tiff";
    }

    // AVIF - ftyp box with AVIF brand
    if (size >= 12 && memcmp(data + 4, "ftypavif", 8) == 0) {
        return ".avif";
    }

    // HEIC/HEIF - ftyp box with various HEIC brands
    if (size >= 12 && memcmp(data + 4, "ftyp", 4) == 0) {
        const char *brand = (const char*)(data + 8);
        if (memcmp(brand, "heic", 4) == 0 || 
            memcmp(brand, "heix", 4) == 0 ||
            memcmp(brand, "hevc", 4) == 0 ||
            memcmp(brand, "hevx", 4) == 0 ||
            memcmp(brand, "heim", 4) == 0 ||
            memcmp(brand, "heis", 4) == 0 ||
            memcmp(brand, "hevm", 4) == 0 ||
            memcmp(brand, "hevs", 4) == 0 ||
            memcmp(brand, "mif1", 4) == 0 ||
            memcmp(brand, "msf1", 4) == 0) {
            return ".heic";
        }
    }

    // ICO - 00 00 01 00 (icon) or 00 00 02 00 (cursor)
    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00 && 
        (data[2] == 0x01 || data[2] == 0x02) && data[3] == 0x00) {
        return data[2] == 0x01 ? ".ico" : ".cur";
    }

    // PSD - 38 42 50 53
    if (size >= 4 && memcmp(data, "8BPS", 4) == 0) {
        return ".psd";
    }

    // GIMP XCF - gimp xcf (with version info)
    if (size >= 9 && memcmp(data, "gimp xcf ", 9) == 0) {
        return ".xcf";
    }

    // OpenEXR - 76 2F 31 01
    if (size >= 4 && memcmp(data, "\x76\x2f\x31\x01", 4) == 0) {
        return ".exr";
    }

    // Radiance HDR - #?RADIANCE or #?RGBE
    if (size >= 10 && (memcmp(data, "#?RADIANCE", 10) == 0 || 
                       memcmp(data, "#?RGBE", 6) == 0)) {
        return ".hdr";
    }

    // JPEG 2000 - 00 00 00 0C 6A 50 20 20
    if (size >= 8 && memcmp(data, "\x00\x00\x00\x0c\x6a\x50\x20\x20", 8) == 0) {
        return ".jp2";
    }

    // JPEG 2000 codestream - FF 4F FF 51
    if (size >= 4 && memcmp(data, "\xff\x4f\xff\x51", 4) == 0) {
        return ".j2k";
    }

    // JPEG XL - FF 0A or 00 00 00 0C 4A 58 4C 20
    if (size >= 2 && memcmp(data, "\xff\x0a", 2) == 0) {
        return ".jxl";
    }
    if (size >= 12 && memcmp(data, "\x00\x00\x00\x0c\x4a\x58\x4c\x20\x0d\x0a\x87\x0a", 12) == 0) {
        return ".jxl";
    }

    // JPEG XR - II BC (little endian) or MM BC (big endian)
    if (size >= 2 && ((data[0] == 0x49 && data[1] == 0x49) || (data[0] == 0x4D && data[1] == 0x4D))) {
        // Check for JPEG XR specific markers
        if (size >= 4 && data[2] == 0xBC && (data[3] == 0x00 || data[3] == 0x01)) {
            return ".jxr";
        }
    }

    // TGA - check footer signature (TRUEVISION-XFILE.)
    if (size >= 26) {
        const char *footer_sig = "TRUEVISION-XFILE.";
        if (memcmp(data + size - 18, footer_sig, 18) == 0) {
            return ".tga";
        }
    }
    // TGA - heuristic check for headerless TGA (very basic)
    if (size >= 18 && data[1] <= 1 && (data[2] == 1 || data[2] == 2 || data[2] == 3 || 
                                        data[2] == 9 || data[2] == 10 || data[2] == 11)) {
        return ".tga";
    }

    // PCX - 0A XX 01
    if (size >= 3 && data[0] == 0x0A && data[2] == 0x01) {
        return ".pcx";
    }

    // SVG - check for XML declaration and svg tag
    if (size >= 5 && (memcmp(data, "<?xml", 5) == 0 || memcmp(data, "<svg", 4) == 0)) {
        // Look for svg tag within first 1000 bytes
        size_t search_len = size > 1000 ? 1000 : size;
        for (size_t i = 0; i < search_len - 3; i++) {
            if (memcmp(data + i, "<svg", 4) == 0) {
                return ".svg";
            }
        }
    }

    // PNM family
    if (size >= 2 && data[0] == 'P') {
        switch (data[1]) {
            case '1': case '4': return ".pbm";  // Portable bitmap
            case '2': case '5': return ".pgm";  // Portable graymap
            case '3': case '6': return ".ppm";  // Portable pixmap
            case '7': return ".pam";            // Portable arbitrary map
        }
    }

    // XBM - #define
    if (size >= 7 && memcmp(data, "#define", 7) == 0) {
        return ".xbm";
    }

    // XPM - /* XPM */
    if (size >= 9 && memcmp(data, "/* XPM */", 9) == 0) {
        return ".xpm";
    }

    // FITS - SIMPLE
    if (size >= 6 && memcmp(data, "SIMPLE", 6) == 0) {
        return ".fits";
    }

    // FLIF - FLIF
    if (size >= 4 && memcmp(data, "FLIF", 4) == 0) {
        return ".flif";
    }

    // QOI - qoif
    if (size >= 4 && memcmp(data, "qoif", 4) == 0) {
        return ".qoi";
    }

    // WBMP - 00 00 (followed by width and height)
    if (size >= 4 && data[0] == 0x00 && data[1] == 0x00) {
        return ".wbmp";
    }

    // Default fallback - JPEG is most common for embedded album art
    return ".jpg";
}

gchar *try_get_local_art(mpv_handle *mpv, char *path) {
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

gchar *try_get_youtube_thumbnail(const char *path)
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

gchar *try_get_embedded_art(char *path)
{
    gchar *uri = NULL;
    AVFormatContext *context = NULL;

    if (!avformat_open_input(&context, path, NULL, NULL))
    {
        uri = extract_embedded_art(context, path);
        avformat_close_input(&context);
    }

    return uri;
}

gchar *get_cache_dir()
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

gchar* generate_cache_filename(const char *path, const uint8_t *image_data, size_t image_size) {
    gchar *hash = g_compute_checksum_for_string(G_CHECKSUM_SHA256, path, -1);
    const char *ext = get_image_extension(image_data, image_size);
    gchar *filename = g_strconcat(hash, ext, NULL);
    g_free(hash);
    return filename;
}

void cleanup_old_cache_files()
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
