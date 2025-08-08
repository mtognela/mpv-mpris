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
#include "mpv-mpris-dbus.h"

const char art_files[][32] = {
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
    "portada.jpg", "portada.png", // Spanish
    "caratula.jpg", "caratula.png", // Spanish
    "capa.jpg", "capa.png", // Portuguese
    "pochette.jpg", "pochette.png", // French
};

const char *STATUS_PLAYING = "Playing";
const char *STATUS_PAUSED = "Paused";
const char *STATUS_STOPPED = "Stopped";
const char *LOOP_NONE = "None";
const char *LOOP_TRACK = "Track";
const char *LOOP_PLAYLIST = "Playlist";
const char *youtube_url_pattern =
    "^https?:\\/\\/(?:youtu.be\\/|(?:www\\.)?youtube\\.com\\/watch\\?v=)(?<id>[a-zA-Z0-9_-]*)\\??.*";

GRegex *youtube_url_regex;

GDBusInterfaceVTable vtable_root = {
    method_call_root, get_property_root, set_property_root, {0}};

GDBusInterfaceVTable vtable_player = {
    method_call_player, get_property_player, set_property_player, {0}};


const char *introspection_xml =
    "<node>\n"
    "  <interface name=\"org.mpris.MediaPlayer2\">\n"
    "    <method name=\"Raise\">\n"
    "    </method>\n"
    "    <method name=\"Quit\">\n"
    "    </method>\n"
    "    <property name=\"CanQuit\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"Fullscreen\" type=\"b\" access=\"readwrite\"/>\n"
    "    <property name=\"CanSetFullscreen\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanRaise\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"HasTrackList\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"Identity\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"DesktopEntry\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"SupportedUriSchemes\" type=\"as\" access=\"read\"/>\n"
    "    <property name=\"SupportedMimeTypes\" type=\"as\" access=\"read\"/>\n"
    "  </interface>\n"
    "  <interface name=\"org.mpris.MediaPlayer2.Player\">\n"
    "    <method name=\"Next\">\n"
    "    </method>\n"
    "    <method name=\"Previous\">\n"
    "    </method>\n"
    "    <method name=\"Pause\">\n"
    "    </method>\n"
    "    <method name=\"PlayPause\">\n"
    "    </method>\n"
    "    <method name=\"Stop\">\n"
    "    </method>\n"
    "    <method name=\"Play\">\n"
    "    </method>\n"
    "    <method name=\"Seek\">\n"
    "      <arg type=\"x\" name=\"Offset\" direction=\"in\"/>\n"
    "    </method>\n"
    "    <method name=\"SetPosition\">\n"
    "      <arg type=\"o\" name=\"TrackId\" direction=\"in\"/>\n"
    "      <arg type=\"x\" name=\"Offset\" direction=\"in\"/>\n"
    "    </method>\n"
    "    <method name=\"OpenUri\">\n"
    "      <arg type=\"s\" name=\"Uri\" direction=\"in\"/>\n"
    "    </method>\n"
    "    <signal name=\"Seeked\">\n"
    "      <arg type=\"x\" name=\"Position\" direction=\"out\"/>\n"
    "    </signal>\n"
    "    <property name=\"PlaybackStatus\" type=\"s\" access=\"read\"/>\n"
    "    <property name=\"LoopStatus\" type=\"s\" access=\"readwrite\"/>\n"
    "    <property name=\"Rate\" type=\"d\" access=\"readwrite\"/>\n"
    "    <property name=\"Shuffle\" type=\"b\" access=\"readwrite\"/>\n"
    "    <property name=\"Metadata\" type=\"a{sv}\" access=\"read\"/>\n"
    "    <property name=\"Volume\" type=\"d\" access=\"readwrite\"/>\n"
    "    <property name=\"Position\" type=\"x\" access=\"read\"/>\n"
    "    <property name=\"MinimumRate\" type=\"d\" access=\"read\"/>\n"
    "    <property name=\"MaximumRate\" type=\"d\" access=\"read\"/>\n"
    "    <property name=\"CanGoNext\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanGoPrevious\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanPlay\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanPause\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanSeek\" type=\"b\" access=\"read\"/>\n"
    "    <property name=\"CanControl\" type=\"b\" access=\"read\"/>\n"
    "  </interface>\n"
    "</node>\n";

const char* supported_extensions[] = {
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
    ".psd", ".psb",           // Adobe Photoshop
    ".xcf",                   // GIMP
    ".exr", ".hdr", ".pic",   // High Dynamic Range
    ".dpx", ".cin",           // Digital cinema
    ".sgi", ".rgb", ".bw",    // SGI formats
    ".sun", ".ras",           // Sun raster
    ".pnm", ".pbm", ".pgm", ".ppm", ".pam", // Netpbm formats
    ".pfm",                   // Portable float map
    ".pcx",                   // PC Paintbrush
    ".tga", ".icb", ".vda", ".vst", // Targa formats
    ".jp2", ".j2k", ".jpf", ".jpx", ".jpm", ".mj2", // JPEG 2000
    ".jxr", ".wdp", ".hdp",   // JPEG XR / HD Photo
    ".jxl",                   // JPEG XL
    
    // Vector formats (if supported)
    ".svg", ".svgz",
    
    // Animation formats
    ".apng", ".mng",
    
    // Less common formats
    ".xbm", ".xpm",           // X11 bitmap/pixmap
    ".wbmp",                  // Wireless bitmap
    ".fits", ".fit", ".fts",  // Flexible Image Transport System
    ".flif",                  // Free Lossless Image Format
    ".qoi",                   // Quite OK Image format
};