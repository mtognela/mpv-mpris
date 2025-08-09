const std = @import("std");

pub fn main() !void {}

test "basic test" {
    const testing = std.testing;
    try testing.expect(true);
}