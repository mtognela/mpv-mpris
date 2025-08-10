const std = @import("std");

const PkgConfigResult = struct {
    cflags: []const []const u8,
    libs: []const []const u8,

    fn deinit(self: *PkgConfigResult, allocator: std.mem.Allocator) void {
        for (self.cflags) |flag| {
            allocator.free(flag);
        }
        allocator.free(self.cflags);
        
        for (self.libs) |lib| {
            allocator.free(lib);
        }
        allocator.free(self.libs);
    }
};

const CSourceFiles = [_][]const u8{
    "src/c/mpv-mpris-artwork.c",
    "src/c/mpv-mpris-dbus.c",
    "src/c/mpv-mpris-events.c",
    "src/c/mpv-mpris-glob.c",
    "src/c/mpv-mpris-metadata.c",
    "src/c/mpv_mpris_open_cplugin.c",
};

const BaseFlags = [_][]const u8{ "-std=c99", "-Wall", "-Wextra", "-pedantic" };
const DebugFlags = BaseFlags ++ [_][]const u8{ "-O0", "-g", "-DDEBUG" };
const ReleaseFlags = BaseFlags ++ [_][]const u8{"-O2"};

const PkgConfigPackages = [_][]const u8{ "gio-2.0", "gio-unix-2.0", "glib-2.0", "mpv", "libavformat" };

fn runPkgConfig(allocator: std.mem.Allocator, packages: []const []const u8, flag_type: []const u8) ![]u8 {
    var argv = std.ArrayList([]const u8).init(allocator);
    defer argv.deinit();
    
    try argv.append("pkg-config");
    try argv.append(flag_type);
    try argv.appendSlice(packages);

    const result = std.process.Child.run(.{
        .allocator = allocator,
        .argv = argv.items,
    }) catch |err| {
        std.log.err("Failed to run pkg-config {s}: {}\n", .{ flag_type, err });
        return err;
    };
    
    defer allocator.free(result.stderr);
    if (result.term != .Exited or result.term.Exited != 0) {
        std.log.err("pkg-config {s} failed with exit code: {}\n", .{ flag_type, result.term });
        std.log.err("stderr: {s}\n", .{result.stderr});
        allocator.free(result.stdout);
        return error.PkgConfigFailed;
    }

    return result.stdout;
}

fn parsePkgConfigOutput(allocator: std.mem.Allocator, output: []const u8, comptime is_libs: bool) ![]const []const u8 {
    var list = std.ArrayList([]const u8).init(allocator);
    errdefer {
        for (list.items) |item| {
            allocator.free(item);
        }
        list.deinit();
    }

    const trimmed = std.mem.trim(u8, output, " \n\r\t");
    var iter = std.mem.tokenizeAny(u8, trimmed, " \t");
    
    while (iter.next()) |token| {
        if (is_libs) {
            if (std.mem.startsWith(u8, token, "-l")) {
                try list.append(try allocator.dupe(u8, token[2..])); // Remove -l prefix
            }
        } else {
            try list.append(try allocator.dupe(u8, token));
        }
    }

    return list.toOwnedSlice();
}

fn getPkgConfigInfo(allocator: std.mem.Allocator) !PkgConfigResult {
    // Get cflags
    const cflags_output = try runPkgConfig(allocator, &PkgConfigPackages, "--cflags");
    defer allocator.free(cflags_output);
    
    const cflags = try parsePkgConfigOutput(allocator, cflags_output, false);
    errdefer {
        for (cflags) |flag| allocator.free(flag);
        allocator.free(cflags);
    }

    // Get libs
    const libs_output = try runPkgConfig(allocator, &PkgConfigPackages, "--libs");
    defer allocator.free(libs_output);
    
    const libs = try parsePkgConfigOutput(allocator, libs_output, true);
    errdefer {
        for (libs) |lib| allocator.free(lib);
        allocator.free(libs);
    }

    return PkgConfigResult{
        .cflags = cflags,
        .libs = libs,
    };
}

fn buildCFlags(allocator: std.mem.Allocator, base_flags: []const []const u8, pkg_flags: []const []const u8) ![]const []const u8 {
    var flags = std.ArrayList([]const u8).init(allocator);
    errdefer flags.deinit();

    try flags.appendSlice(base_flags);
    try flags.appendSlice(pkg_flags);

    return flags.toOwnedSlice();
}

fn addSystemLibraries(target: *std.Build.Step.Compile, libs: []const []const u8) void {
    for (libs) |lib_name| {
        target.linkSystemLibrary(lib_name);
    }
    target.linkLibC();
}

fn createSharedLibrary(
    b: *std.Build,
    name: []const u8,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cflags: []const []const u8,
    libs: []const []const u8,
    zig_root_file: ?[]const u8,
) *std.Build.Step.Compile {
    const lib = b.addSharedLibrary(.{
        .name = name,
        .root_source_file = if (zig_root_file) |root_file| b.path(root_file) else null,
        .target = target,
        .optimize = optimize,
    });

    // Add C source files
    lib.addCSourceFiles(.{
        .files = &CSourceFiles,
        .flags = cflags,
    });

    lib.addIncludePath(b.path("include"));
    if (zig_root_file != null) {
        lib.addIncludePath(b.path("src/zig"));
    }
    addSystemLibraries(lib, libs);

    return lib;
}

pub fn build(b: *std.Build) void {
    b.install_prefix = "build/zig-out/lib/";

    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const allocator = b.allocator;

    // Get pkg-config information
    var pkg_info = getPkgConfigInfo(allocator) catch |err| {
        std.log.err("Failed to get pkg-config information: {}\n", .{err});
        std.process.exit(1);
    };
    defer pkg_info.deinit(allocator);

    // Build release flags
    const release_cflags = buildCFlags(allocator, &ReleaseFlags, pkg_info.cflags) catch |err| {
        std.log.err("Failed to build release cflags: {}\n", .{err});
        std.process.exit(1);
    };
    defer allocator.free(release_cflags);

    // Build debug flags  
    const debug_cflags = buildCFlags(allocator, &DebugFlags, pkg_info.cflags) catch |err| {
        std.log.err("Failed to build debug cflags: {}\n", .{err});
        std.process.exit(1);
    };
    defer allocator.free(debug_cflags);

    // Main shared library (C only for now)
    const lib = createSharedLibrary(b, "mpris", target, optimize, release_cflags, pkg_info.libs, null);
    b.installArtifact(lib);

    // Debug shared library (C only for now)
    const debug_lib = createSharedLibrary(b, "mpris-debug", target, .Debug, debug_cflags, pkg_info.libs, null);
    const debug_step = b.step("debug", "Build with debug symbols");
    debug_step.dependOn(&b.addInstallArtifact(debug_lib, .{}).step);

    // FIXED: Remove static library approach that was causing linking issues
    // Instead, create test executable that directly includes C source files

    // Test executable - directly include C sources to avoid linking issues
    const test_exe = b.addExecutable(.{
        .name = "mpv-mpris-test",
        .root_source_file = b.path("test/zig/test-main.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Add C source files directly to the executable
    test_exe.addCSourceFiles(.{
        .files = &CSourceFiles,
        .flags = release_cflags,
    });

    test_exe.addIncludePath(b.path("include"));
    test_exe.addIncludePath(b.path("src/zig"));
    addSystemLibraries(test_exe, pkg_info.libs);
    b.installArtifact(test_exe);

    // Run step for tests
    const run_test_cmd = b.addRunArtifact(test_exe);
    run_test_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the test application");
    run_step.dependOn(&run_test_cmd.step);

    // FIXED: Unit tests - use the same direct approach
    const unit_tests = b.addTest(.{
        .root_source_file = b.path("test/zig/test-main.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Add C source files directly to the test
    unit_tests.addCSourceFiles(.{
        .files = &CSourceFiles,
        .flags = release_cflags,
    });

    unit_tests.addIncludePath(b.path("include"));
    unit_tests.addIncludePath(b.path("src/zig"));
    addSystemLibraries(unit_tests, pkg_info.libs);

    const run_unit_tests = b.addRunArtifact(unit_tests);
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_unit_tests.step);

    // Utility steps
    const print_step = b.step("print-vars", "Print build variables");
    const print_run = b.addSystemCommand(&[_][]const u8{ "echo", "Zig build configuration loaded successfully" });
    print_step.dependOn(&print_run.step);

    // FIXED: Update clean step to clean the new build directory
    const clean_step = b.step("clean", "Clean build artifacts");
    const clean_run = b.addSystemCommand(&[_][]const u8{ "rm", "-rf", "build/zig-out", "build/zig-cache", ".zig-cache" });
    clean_step.dependOn(&clean_run.step);
}
