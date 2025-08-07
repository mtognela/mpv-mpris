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
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

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

#endif // MPV_MPRIS_TYPES_H