const std = @import("std");
pub const c = @cImport({
    @cInclude("mpv-mpris-artwork.h");
    @cInclude("mpv-mpris-dbus.h");
    @cInclude("mpv-mpris-events.h");
    @cInclude("mpv-mpris-metadata.h");
    @cInclude("mpv-mpris-types.h");
});

pub const G_TRUE: c.gboolean = 1;
pub const G_FALSE: c.gboolean = 0;

pub fn gbooleanToBool(gb: c.gboolean) bool {
    return gb != G_FALSE;
}

// Safe string span function that handles null pointers
pub fn safeSpan(ptr: ?[*:0]const u8) []const u8 {
    return if (ptr) |p| std.mem.span(p) else "";
}
