const std = @import("std");
const testing = std.testing;
const expectEqual = testing.expectEqual;
const expect = testing.expect;
const expectEqualStrings = testing.expectEqualStrings;

// C imports with proper GLib integration
const c = @cImport({
    @cInclude("glib.h");
    @cInclude("gio/gio.h");
    // Include your headers after GLib
    @cInclude("mpv-mpris-artwork.h");
    @cInclude("mpv-mpris-dbus.h");
    @cInclude("mpv-mpris-events.h");
    @cInclude("mpv-mpris-metadata.h");
    @cInclude("mpv-mpris-types.h");
});

// GLib type constants - these should match what GLib actually uses
const G_TRUE: c.gboolean = 1;
const G_FALSE: c.gboolean = 0;

// Test data structures and mock data
const TestImageData = struct {
    jpeg: []const u8 = &[_]u8{ 0xFF, 0xD8, 0xFF, 0xE0 },
    png: []const u8 = &[_]u8{ 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A },
    gif87a: []const u8 = &[_]u8{ 0x47, 0x49, 0x46, 0x38, 0x37, 0x61 },
    gif89a: []const u8 = &[_]u8{ 0x47, 0x49, 0x46, 0x38, 0x39, 0x61 },
    webp: []const u8 = &[_]u8{ 0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x45, 0x42, 0x50 },
    bmp: []const u8 = &[_]u8{ 0x42, 0x4D },
    tiff_le: []const u8 = &[_]u8{ 0x49, 0x49, 0x2A, 0x00 },
    tiff_be: []const u8 = &[_]u8{ 0x4D, 0x4D, 0x00, 0x2A },
    ico: []const u8 = &[_]u8{ 0x00, 0x00, 0x01, 0x00 },
    svg: []const u8 = "<svg",
};

// Helper function to convert gboolean to bool for testing
fn gbooleanToBool(gb: c.gboolean) bool {
    return gb != G_FALSE;
}

// ==================== ARTWORK TESTS ====================

test "get_image_extension - JPEG detection" {
    const test_data = TestImageData{};

    const ext = c.get_image_extension(@as([*c]const u8, @ptrCast(test_data.jpeg.ptr)), @as(c.gsize, test_data.jpeg.len));
    try expectEqualStrings(".jpg", std.mem.span(ext));
}

test "get_image_extension - PNG detection" {
    const test_data = TestImageData{};

    const ext = c.get_image_extension(@as([*c]const u8, @ptrCast(test_data.png.ptr)), @as(c.gsize, test_data.png.len));
    try expectEqualStrings(".png", std.mem.span(ext));
}

test "get_image_extension - GIF detection" {
    const test_data = TestImageData{};

    var ext = c.get_image_extension(@as([*c]const u8, @ptrCast(test_data.gif87a.ptr)), @as(c.gsize, test_data.gif87a.len));
    try expectEqualStrings(".gif", std.mem.span(ext));

    ext = c.get_image_extension(@as([*c]const u8, @ptrCast(test_data.gif89a.ptr)), @as(c.gsize, test_data.gif89a.len));
    try expectEqualStrings(".gif", std.mem.span(ext));
}

test "get_image_extension - WebP detection" {
    const test_data = TestImageData{};

    const ext = c.get_image_extension(@as([*c]const u8, @ptrCast(test_data.webp.ptr)), @as(c.gsize, test_data.webp.len));
    try expectEqualStrings(".webp", std.mem.span(ext));
}

test "get_image_extension - BMP detection" {
    const test_data = TestImageData{};

    const ext = c.get_image_extension(@as([*c]const u8, @ptrCast(test_data.bmp.ptr)), @as(c.gsize, test_data.bmp.len));
    try expectEqualStrings(".bmp", std.mem.span(ext));
}

test "get_image_extension - TIFF detection" {
    const test_data = TestImageData{};

    var ext = c.get_image_extension(@as([*c]const u8, @ptrCast(test_data.tiff_le.ptr)), @as(c.gsize, test_data.tiff_le.len));
    try expectEqualStrings(".tiff", std.mem.span(ext));

    ext = c.get_image_extension(@as([*c]const u8, @ptrCast(test_data.tiff_be.ptr)), @as(c.gsize, test_data.tiff_be.len));
    try expectEqualStrings(".tiff", std.mem.span(ext));
}

test "get_image_extension - ICO detection" {
    const test_data = TestImageData{};

    const ext = c.get_image_extension(@as([*c]const u8, @ptrCast(test_data.ico.ptr)), @as(c.gsize, test_data.ico.len));
    try expectEqualStrings(".ico", std.mem.span(ext));
}

test "get_image_extension - fallback to JPEG" {
    const invalid_data = &[_]u8{ 0x00, 0x00, 0x00 };

    const ext = c.get_image_extension(@as([*c]const u8, @ptrCast(invalid_data.ptr)), @as(c.gsize, invalid_data.len));
    try expectEqualStrings(".jpg", std.mem.span(ext));
}

test "get_image_extension - empty data fallback" {
    const ext = c.get_image_extension(null, 0);
    try expectEqualStrings(".jpg", std.mem.span(ext));
}

test "is_supported_image_file - common formats" {
    try expect(gbooleanToBool(c.is_supported_image_file("test.jpg")));
    try expect(gbooleanToBool(c.is_supported_image_file("test.jpeg")));
    try expect(gbooleanToBool(c.is_supported_image_file("test.png")));
    try expect(gbooleanToBool(c.is_supported_image_file("test.gif")));
    try expect(gbooleanToBool(c.is_supported_image_file("test.webp")));
    try expect(gbooleanToBool(c.is_supported_image_file("test.bmp")));
    try expect(gbooleanToBool(c.is_supported_image_file("test.tiff")));
    try expect(gbooleanToBool(c.is_supported_image_file("test.tif")));
}

test "is_supported_image_file - case sensitivity" {
    try expect(gbooleanToBool(c.is_supported_image_file("test.JPG")));
    try expect(gbooleanToBool(c.is_supported_image_file("test.PNG")));
    try expect(gbooleanToBool(c.is_supported_image_file("test.JPEG")));
}

test "is_supported_image_file - unsupported formats" {
    try expect(!gbooleanToBool(c.is_supported_image_file("test.txt")));
    try expect(!gbooleanToBool(c.is_supported_image_file("test.mp4")));
    try expect(!gbooleanToBool(c.is_supported_image_file("test.doc")));
    try expect(!gbooleanToBool(c.is_supported_image_file("test")));
}

test "is_art_file - exact matches" {
    try expect(gbooleanToBool(c.is_art_file("cover.jpg")));
    try expect(gbooleanToBool(c.is_art_file("Cover.jpg")));
    try expect(gbooleanToBool(c.is_art_file("COVER.JPG")));
    try expect(gbooleanToBool(c.is_art_file("front.png")));
    try expect(gbooleanToBool(c.is_art_file("album.gif")));
    try expect(gbooleanToBool(c.is_art_file("artwork.webp")));
    try expect(gbooleanToBool(c.is_art_file("folder.jpg")));
}

test "is_art_file - non-art files" {
    try expect(!gbooleanToBool(c.is_art_file("music.mp3")));
    try expect(!gbooleanToBool(c.is_art_file("video.mp4")));
    try expect(!gbooleanToBool(c.is_art_file("document.txt")));
}

test "try_get_youtube_thumbnail - valid URLs" {
    // Test standard YouTube URL
    const url1 = "https://www.youtube.com/watch?v=dQw4w9WgXcQ";
    const result1 = c.try_get_youtube_thumbnail(url1);

    if (result1) |r1| {
        defer c.g_free(r1);
        const result_str = std.mem.span(@as([*:0]const u8, @ptrCast(r1)));
        try expect(std.mem.indexOf(u8, result_str, "dQw4w9WgXcQ") != null);
        try expect(std.mem.indexOf(u8, result_str, "i1.ytimg.com") != null);
    }

    // Test youtu.be URL
    const url2 = "https://youtu.be/dQw4w9WgXcQ";
    const result2 = c.try_get_youtube_thumbnail(url2);

    if (result2) |r2| {
        defer c.g_free(r2);
        const result_str = std.mem.span(@as([*:0]const u8, @ptrCast(r2)));
        try expect(std.mem.indexOf(u8, result_str, "dQw4w9WgXcQ") != null);
    }
}

test "try_get_youtube_thumbnail - invalid URLs" {
    const url1 = "https://example.com/video";
    const result1 = c.try_get_youtube_thumbnail(url1);
    try expect(result1 == null);

    const url2 = "not_a_url";
    const result2 = c.try_get_youtube_thumbnail(url2);
    try expect(result2 == null);
}

test "generate_cache_filename" {
    const test_data = TestImageData{};
    const path = "/path/to/media/file.mp3";

    const result = c.generate_cache_filename(path, @as([*c]const u8, @ptrCast(test_data.jpeg.ptr)), @as(c.gsize, test_data.jpeg.len));
    defer c.g_free(result);

    const result_str = std.mem.span(@as([*:0]const u8, @ptrCast(result)));
    try expect(std.mem.endsWith(u8, result_str, ".jpg"));
    try expect(result_str.len > 10); // Should have hash + extension
}

// ==================== METADATA TESTS ====================

test "string_to_utf8 - valid UTF-8" {
    const valid_str = c.g_strdup("Hello, World!");
    defer c.g_free(valid_str);

    const result = c.string_to_utf8(@as([*c]u8, @ptrCast(valid_str)));
    defer c.g_free(result);

    try expectEqualStrings("Hello, World!", std.mem.span(@as([*:0]const u8, @ptrCast(result))));
}

test "string_to_utf8 - invalid UTF-8 handling" {
    // Create a string with invalid UTF-8 bytes
    var invalid_bytes = [_]u8{ 0xC0, 0x80, 0x00 }; // Invalid UTF-8 sequence
    const invalid_str = c.g_strdup(@as([*c]const u8, @ptrCast(&invalid_bytes)));
    defer c.g_free(invalid_str);

    const result = c.string_to_utf8(@as([*c]u8, @ptrCast(invalid_str)));
    defer c.g_free(result);

    // Should return some valid UTF-8 string (implementation dependent)
    try expect(result != null);
    try expect(gbooleanToBool(c.g_utf8_validate(@as([*c]const u8, @ptrCast(result)), -1, null)));
}

// ==================== DBUS INTERFACE TESTS ====================

test "status constants are defined" {
    try expect(c.STATUS_PLAYING != null);
    try expect(c.STATUS_PAUSED != null);
    try expect(c.STATUS_STOPPED != null);

    try expectEqualStrings("Playing", std.mem.span(@as([*:0]const u8, @ptrCast(c.STATUS_PLAYING))));
    try expectEqualStrings("Paused", std.mem.span(@as([*:0]const u8, @ptrCast(c.STATUS_PAUSED))));
    try expectEqualStrings("Stopped", std.mem.span(@as([*:0]const u8, @ptrCast(c.STATUS_STOPPED))));
}

test "loop status constants are defined" {
    try expect(c.LOOP_NONE != null);
    try expect(c.LOOP_TRACK != null);
    try expect(c.LOOP_PLAYLIST != null);

    try expectEqualStrings("None", std.mem.span(@as([*:0]const u8, @ptrCast(c.LOOP_NONE))));
    try expectEqualStrings("Track", std.mem.span(@as([*:0]const u8, @ptrCast(c.LOOP_TRACK))));
    try expectEqualStrings("Playlist", std.mem.span(@as([*:0]const u8, @ptrCast(c.LOOP_PLAYLIST))));
}

test "youtube URL regex pattern is defined" {
    try expect(c.youtube_url_pattern != null);

    const pattern = std.mem.span(@as([*:0]const u8, @ptrCast(c.youtube_url_pattern)));
    try expect(std.mem.indexOf(u8, pattern, "youtube\\.com") != null);
    try expect(std.mem.indexOf(u8, pattern, "youtu.be") != null);
}

test "introspection XML is well-formed" {
    try expect(c.introspection_xml != null);

    const xml = std.mem.span(@as([*:0]const u8, @ptrCast(c.introspection_xml)));
    try expect(std.mem.indexOf(u8, xml, "<node>") != null);
    try expect(std.mem.indexOf(u8, xml, "</node>") != null);
    try expect(std.mem.indexOf(u8, xml, "org.mpris.MediaPlayer2") != null);
    try expect(std.mem.indexOf(u8, xml, "org.mpris.MediaPlayer2.Player") != null);
}

test "supported extensions array is populated" {
    // Test that common extensions are present
    const extensions = @as([*]const [*c]const u8, @ptrCast(c.supported_extensions));
    var found_jpg = false;
    var found_png = false;
    var found_gif = false;

    var i: usize = 0;
    while (i < 100) : (i += 1) { // Reasonable limit to avoid infinite loop
        const ext_ptr = extensions[i];
        if (ext_ptr == null) break;
        
        const ext = std.mem.span(@as([*:0]const u8, @ptrCast(ext_ptr)));
        if (ext.len == 0) break;

        if (std.mem.eql(u8, ext, ".jpg")) found_jpg = true;
        if (std.mem.eql(u8, ext, ".png")) found_png = true;
        if (std.mem.eql(u8, ext, ".gif")) found_gif = true;
    }

    try expect(found_jpg);
    try expect(found_png);
    try expect(found_gif);
}

// ==================== INTEGRATION TESTS ====================

test "cache directory operations" {
    // Test cache directory creation
    const cache_dir = c.get_cache_dir();
    if (cache_dir) |dir| {
        defer c.g_free(dir);

        const dir_str = std.mem.span(@as([*:0]const u8, @ptrCast(dir)));
        try expect(dir_str.len > 0);
        try expect(std.mem.indexOf(u8, dir_str, "mpv-mpris") != null);

        // Test that directory exists after creation
        try expect(gbooleanToBool(c.g_file_test(@as([*c]const u8, @ptrCast(dir)), c.G_FILE_TEST_IS_DIR)));
    }
}

test "cleanup old cache files function exists" {
    // Just verify the function can be called without crashing
    c.cleanup_old_cache_files();
}

// ==================== ERROR HANDLING TESTS ====================

test "null pointer handling in image detection" {
    const ext = c.get_image_extension(null, 0);
    try expectEqualStrings(".jpg", std.mem.span(@as([*:0]const u8, @ptrCast(ext))));
}

test "empty filename handling" {
    try expect(!gbooleanToBool(c.is_supported_image_file("")));
    try expect(!gbooleanToBool(c.is_art_file("")));
}

test "very long filename handling" {
    var long_filename: [1000]u8 = undefined;
    @memset(&long_filename, 'a');
    // Add null terminator and extension
    long_filename[995] = '.';
    long_filename[996] = 'j';
    long_filename[997] = 'p';
    long_filename[998] = 'g';
    long_filename[999] = 0;

    const result = c.is_supported_image_file(@as([*c]const u8, @ptrCast(&long_filename)));
    try expect(gbooleanToBool(result));
}

// ==================== BOUNDARY CONDITION TESTS ====================

test "image detection with minimal data" {
    const minimal_jpeg = &[_]u8{ 0xFF, 0xD8 };
    const ext = c.get_image_extension(@as([*c]const u8, @ptrCast(minimal_jpeg.ptr)), @as(c.gsize, minimal_jpeg.len));
    // Should fallback to .jpg for insufficient data
    try expectEqualStrings(".jpg", std.mem.span(@as([*:0]const u8, @ptrCast(ext))));
}

test "image detection with maximum supported data" {
    var large_data: [10000]u8 = undefined;
    // Fill with JPEG signature
    large_data[0] = 0xFF;
    large_data[1] = 0xD8;
    large_data[2] = 0xFF;
    large_data[3] = 0xE0;

    const ext = c.get_image_extension(@as([*c]const u8, @ptrCast(large_data[0..].ptr)), @as(c.gsize, large_data.len));
    try expectEqualStrings(".jpg", std.mem.span(@as([*:0]const u8, @ptrCast(ext))));
}

// ==================== PERFORMANCE TESTS ====================

test "image detection performance" {
    const test_data = TestImageData{};
    const iterations = 10000;

    var timer = try std.time.Timer.start();

    var i: usize = 0;
    while (i < iterations) : (i += 1) {
        _ = c.get_image_extension(@as([*c]const u8, @ptrCast(test_data.jpeg.ptr)), @as(c.gsize, test_data.jpeg.len));
    }

    const elapsed = timer.read();

    // Should complete 10k iterations in reasonable time (< 1 second)
    try expect(elapsed < std.time.ns_per_s);
}

test "filename checking performance" {
    const iterations = 10000;

    var timer = try std.time.Timer.start();

    var i: usize = 0;
    while (i < iterations) : (i += 1) {
        _ = c.is_supported_image_file("test.jpg");
        _ = c.is_art_file("cover.jpg");
    }

    const elapsed = timer.read();

    // Should complete 10k iterations in reasonable time (< 1 second)
    try expect(elapsed < std.time.ns_per_s);
}

// ==================== MAIN TEST RUNNER ====================

pub fn main() !void {}
