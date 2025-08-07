/*
    MIT License - MPV MPRIS Bridge - Global Variable Definitions
    This file defines all the global variables declared as extern in the headers
*/

#include "mpv-mpris-types.h"

// MPRIS status constants
const char *STATUS_PLAYING = "Playing";
const char *STATUS_PAUSED = "Paused";
const char *STATUS_STOPPED = "Stopped";
const char *LOOP_NONE = "None";
const char *LOOP_TRACK = "Track";
const char *LOOP_PLAYLIST = "Playlist";

// YouTube URL pattern and regex
const char *youtube_url_pattern = 
    "^https?:\\/\\/(?:youtu.be\\/|(?:www\\.)?youtube\\.com\\/watch\\?v=)(?<id>[a-zA-Z0-9_-]*)\\??.*";
GRegex *youtube_url_regex = NULL;

// Global mutex for metadata access
GMutex metadata_mutex;

// Album art file patterns
const char art_files[256][32] = {
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

// Supported image extensions
const char *supported_extensions[256] = {
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

// D-Bus introspection XML
const char *introspection_xml = 
    "<!DOCTYPE node PUBLIC '-//freedesktop//DTD D-BUS Object Introspection 1.0//EN'"
    " 'http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd'>"
    "<node>"
    " <interface name='org.freedesktop.DBus.Introspectable'>"
    "  <method name='Introspect'>"
    "   <arg name='data' direction='out' type='s'/>"
    "  </method>"
    " </interface>"
    " <interface name='org.freedesktop.DBus.Properties'>"
    "  <method name='Get'>"
    "   <arg name='interface' direction='in' type='s'/>"
    "   <arg name='property' direction='in' type='s'/>"
    "   <arg name='value' direction='out' type='v'/>"
    "  </method>"
    "  <method name='GetAll'>"
    "   <arg name='interface' direction='in' type='s'/>"
    "   <arg name='properties' direction='out' type='a{sv}'/>"
    "  </method>"
    "  <method name='Set'>"
    "   <arg name='interface' direction='in' type='s'/>"
    "   <arg name='property' direction='in' type='s'/>"
    "   <arg name='value' direction='in' type='v'/>"
    "  </method>"
    "  <signal name='PropertiesChanged'>"
    "   <arg name='interface' type='s'/>"
    "   <arg name='changed_properties' type='a{sv}'/>"
    "   <arg name='invalidated_properties' type='as'/>"
    "  </signal>"
    " </interface>"
    " <interface name='org.mpris.MediaPlayer2'>"
    "  <method name='Raise'/>"
    "  <method name='Quit'/>"
    "  <property name='CanQuit' type='b' access='read'/>"
    "  <property name='CanRaise' type='b' access='read'/>"
    "  <property name='HasTrackList' type='b' access='read'/>"
    "  <property name='Identity' type='s' access='read'/>"
    "  <property name='DesktopEntry' type='s' access='read'/>"
    "  <property name='SupportedUriSchemes' type='as' access='read'/>"
    "  <property name='SupportedMimeTypes' type='as' access='read'/>"
    "  <property name='Fullscreen' type='b' access='readwrite'/>"
    "  <property name='CanSetFullscreen' type='b' access='read'/>"
    " </interface>"
    " <interface name='org.mpris.MediaPlayer2.Player'>"
    "  <method name='Next'/>"
    "  <method name='Previous'/>"
    "  <method name='Pause'/>"
    "  <method name='PlayPause'/>"
    "  <method name='Stop'/>"
    "  <method name='Play'/>"
    "  <method name='Seek'>"
    "   <arg direction='in' name='Offset' type='x'/>"
    "  </method>"
    "  <method name='SetPosition'>"
    "   <arg direction='in' name='TrackId' type='o'/>"
    "   <arg direction='in' name='Position' type='x'/>"
    "  </method>"
    "  <method name='OpenUri'>"
    "   <arg direction='in' name='Uri' type='s'/>"
    "  </method>"
    "  <signal name='Seeked'>"
    "   <arg name='Position' type='x'/>"
    "  </signal>"
    "  <property name='PlaybackStatus' type='s' access='read'/>"
    "  <property name='LoopStatus' type='s' access='readwrite'/>"
    "  <property name='Rate' type='d' access='readwrite'/>"
    "  <property name='Shuffle' type='b' access='readwrite'/>"
    "  <property name='Metadata' type='a{sv}' access='read'/>"
    "  <property name='Volume' type='d' access='readwrite'/>"
    "  <property name='Position' type='x' access='read'/>"
    "  <property name='MinimumRate' type='d' access='read'/>"
    "  <property name='MaximumRate' type='d' access='read'/>"
    "  <property name='CanGoNext' type='b' access='read'/>"
    "  <property name='CanGoPrevious' type='b' access='read'/>"
    "  <property name='CanPlay' type='b' access='read'/>"
    "  <property name='CanPause' type='b' access='read'/>"
    "  <property name='CanSeek' type='b' access='read'/>"
    "  <property name='CanControl' type='b' access='read'/>"
    " </interface>"
    "</node>";