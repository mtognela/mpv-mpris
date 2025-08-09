const std = @import("std");

const c = @cImport({
    @cInclude("mpv-mpris-artwork.h");
    @cInclude("mpv-mpris-dbus.h");
    @cInclude("mpv-mpris-events.h");
    @cInclude("mpv-mpris-metadata.h");
    @cInclude("mpv-mpris-types.h");
});

pub fn main() !void {}

test "basic test" {
    const testing = std.testing;
    try testing.expect(true);
}