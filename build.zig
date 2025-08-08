const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Build the main shared library (equivalent to mpris.so)
    const lib = b.addSharedLibrary(.{
        .name = "mpris",
        .target = target,
        .optimize = optimize,
    });

    // Add C source files
    lib.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/mpv-mpris-artwork.c",
            "src/mpv-mpris-dbus.c",
            "src/mpv-mpris-events.c",
            "src/mpv-mpris-glob.c",
            "src/mpv-mpris-metadata.c",
            "src/mpv_mpris_open_cplugin.c",
        },
        .flags = &[_][]const u8{
            "-std=c99",
            "-Wall",
            "-Wextra",
            "-O2",
            "-pedantic",
        },
    });

    // Add include directory
    lib.addIncludePath(b.path("include"));

    // Link system libraries (matching your PKG_CONFIG dependencies)
    lib.linkSystemLibrary("gio-2.0");
    lib.linkSystemLibrary("gio-unix-2.0");
    lib.linkSystemLibrary("glib-2.0");
    lib.linkSystemLibrary("mpv");
    lib.linkSystemLibrary("avformat");
    lib.linkLibC();

    // Install the shared library
    b.installArtifact(lib);

    // Build the test executable
    const test_exe = b.addExecutable(.{
        .name = "mpris.so",
        .root_source_file = b.path("test/zig/main.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Add include paths for the test executable
    test_exe.addIncludePath(b.path("include"));
    
    // Link the shared library we just built
    test_exe.linkLibrary(lib);
    
    // Link system libraries for the test
    test_exe.linkSystemLibrary("gio-2.0");
    test_exe.linkSystemLibrary("gio-unix-2.0");
    test_exe.linkSystemLibrary("glib-2.0");
    test_exe.linkSystemLibrary("mpv");
    test_exe.linkSystemLibrary("avformat");
    test_exe.linkLibC();

    // Install the test executable
    b.installArtifact(test_exe);

    // Create a run step for tests
    const run_test_cmd = b.addRunArtifact(test_exe);
    run_test_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the test application");
    run_step.dependOn(&run_test_cmd.step);

    // Create a debug build step (equivalent to your debug target)
    const debug_lib = b.addSharedLibrary(.{
        .name = "mpris-debug",
        .target = target,
        .optimize = .Debug,
    });

    debug_lib.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/mpv-mpris-artwork.c",
            "src/mpv-mpris-dbus.c",
            "src/mpv-mpris-events.c",
            "src/mpv-mpris-glob.c",
            "src/mpv-mpris-metadata.c",
            "src/mpv_mpris_open_cplugin.c",
        },
        .flags = &[_][]const u8{
            "-std=c99",
            "-Wall",
            "-Wextra",
            "-O0",
            "-g",
            "-DDEBUG",
            "-pedantic",
        },
    });

    debug_lib.addIncludePath(b.path("include"));
    debug_lib.linkSystemLibrary("gio-2.0");
    debug_lib.linkSystemLibrary("gio-unix-2.0");
    debug_lib.linkSystemLibrary("glib-2.0");
    debug_lib.linkSystemLibrary("mpv");
    debug_lib.linkSystemLibrary("avformat");
    debug_lib.linkLibC();

    const debug_step = b.step("debug", "Build with debug symbols");
    debug_step.dependOn(&b.addInstallArtifact(debug_lib, .{}).step);

    // Create unit tests for Zig code
    const unit_tests = b.addTest(.{
        .root_source_file = b.path("test/zig/main.zig"),
        .target = target,
        .optimize = optimize,
    });
    
    unit_tests.addIncludePath(b.path("include"));
    unit_tests.linkSystemLibrary("gio-2.0");
    unit_tests.linkSystemLibrary("gio-unix-2.0");  
    unit_tests.linkSystemLibrary("glib-2.0");
    unit_tests.linkSystemLibrary("mpv");
    unit_tests.linkSystemLibrary("avformat");
    unit_tests.linkLibC();

    const run_unit_tests = b.addRunArtifact(unit_tests);
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_unit_tests.step);

    // Add a step to print build variables (like your print-vars target)
    const print_step = b.step("print-vars", "Print build variables");
    const print_run = b.addSystemCommand(&[_][]const u8{
        "echo", "Zig build configuration loaded successfully"
    });
    print_step.dependOn(&print_run.step);

    // Add a clean step equivalent
    const clean_step = b.step("clean", "Clean build artifacts");
    const clean_run = b.addSystemCommand(&[_][]const u8{
        "rm", "-rf", "zig-out", "zig-cache"
    });
    clean_step.dependOn(&clean_run.step);
}