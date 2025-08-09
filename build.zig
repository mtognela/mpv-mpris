const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Get pkg-config output
    const allocator = b.allocator;
    
    // Run pkg-config for cflags
    const cflags_result = std.process.Child.run(.{
        .allocator = allocator,
        .argv = &[_][]const u8{
            "pkg-config", "--cflags", 
            "gio-2.0", "gio-unix-2.0", "glib-2.0", "mpv", "libavformat"
        },
    }) catch |err| {
        std.log.err("Failed to run pkg-config --cflags: {}\n", .{err});
        std.process.exit(1);
    };
    defer allocator.free(cflags_result.stdout);
    defer allocator.free(cflags_result.stderr);

    // Run pkg-config for libs
    const ldflags_result = std.process.Child.run(.{
        .allocator = allocator,
        .argv = &[_][]const u8{
            "pkg-config", "--libs",
            "gio-2.0", "gio-unix-2.0", "glib-2.0", "mpv", "libavformat"
        },
    }) catch |err| {
        std.log.err("Failed to run pkg-config --libs: {}\n", .{err});
        std.process.exit(1);
    };
    defer allocator.free(ldflags_result.stdout);
    defer allocator.free(ldflags_result.stderr);

    // Parse cflags
    var cflags_list = std.ArrayList([]const u8).init(allocator);
    defer cflags_list.deinit();
    
    // Add base flags (equivalent to your BASE_CFLAGS)
    cflags_list.appendSlice(&[_][]const u8{
        "-std=c99", "-Wall", "-Wextra", "-O2", "-pedantic"
    }) catch unreachable;

    // Add pkg-config cflags
    var cflags_iter = std.mem.tokenizeAny(u8, std.mem.trim(u8, cflags_result.stdout, " \n\r\t"), " \t");
    while (cflags_iter.next()) |flag| {
        cflags_list.append(b.dupe(flag)) catch unreachable;
    }

    // Parse library names from ldflags
    var libs_list = std.ArrayList([]const u8).init(allocator);
    defer libs_list.deinit();
    
    var ldflags_iter = std.mem.tokenizeAny(u8, std.mem.trim(u8, ldflags_result.stdout, " \n\r\t"), " \t");
    while (ldflags_iter.next()) |flag| {
        if (std.mem.startsWith(u8, flag, "-l")) {
            libs_list.append(flag[2..]) catch unreachable; // Remove -l prefix
        }
    }

    // Build the main shared library (equivalent to mpris.so)
    const lib = b.addSharedLibrary(.{
        .name = "mpris",
        .target = target,
        .optimize = optimize,
    });

    // Add C source files with combined flags
    lib.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/mpv-mpris-artwork.c",
            "src/mpv-mpris-dbus.c",
            "src/mpv-mpris-events.c",
            "src/mpv-mpris-glob.c",
            "src/mpv-mpris-metadata.c",
            "src/mpv_mpris_open_cplugin.c",
        },
        .flags = cflags_list.items,
    });

    // Add include directory
    lib.addIncludePath(b.path("include"));

    // Link system libraries from pkg-config
    for (libs_list.items) |lib_name| {
        lib.linkSystemLibrary(lib_name);
    }
    lib.linkLibC();

    // Install the shared library
    b.installArtifact(lib);

    // Build the test executable
    const test_exe = b.addExecutable(.{
        .name = "mpv-mpris-test",
        .root_source_file = b.path("test/zig/test-main.zig"),
        .target = target,
        .optimize = optimize,   
    });

    // Add include paths for the test executable
    test_exe.addIncludePath(b.path("include"));
    
    // Link the shared library we just built
    test_exe.linkLibrary(lib);
    
    // Link system libraries for the test
    for (libs_list.items) |lib_name| {
        test_exe.linkSystemLibrary(lib_name);
    }
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

    // Debug flags (equivalent to your debug target)
    var debug_cflags = std.ArrayList([]const u8).init(allocator);
    defer debug_cflags.deinit();
    debug_cflags.appendSlice(&[_][]const u8{
        "-std=c99", "-Wall", "-Wextra", "-O0", "-g", "-DDEBUG", "-pedantic"
    }) catch unreachable;
    
    // Add pkg-config flags to debug build too
    var debug_cflags_iter = std.mem.tokenizeAny(u8, std.mem.trim(u8, cflags_result.stdout, " \n\r\t"), " \t");
    while (debug_cflags_iter.next()) |flag| {
        debug_cflags.append(b.dupe(flag)) catch unreachable;
    }

    debug_lib.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/mpv-mpris-artwork.c",
            "src/mpv-mpris-dbus.c",
            "src/mpv-mpris-events.c",
            "src/mpv-mpris-glob.c",
            "src/mpv-mpris-metadata.c",
            "src/mpv_mpris_open_cplugin.c",
        },
        .flags = debug_cflags.items,
    });

    debug_lib.addIncludePath(b.path("include"));
    for (libs_list.items) |lib_name| {
        debug_lib.linkSystemLibrary(lib_name);
    }
    debug_lib.linkLibC();

    const debug_step = b.step("debug", "Build with debug symbols");
    debug_step.dependOn(&b.addInstallArtifact(debug_lib, .{}).step);

    // Create unit tests for Zig code
    const unit_tests = b.addTest(.{
        .root_source_file = b.path("test/zig/test-main.zig"),
        .target = target,
        .optimize = optimize,
    });
    
    unit_tests.addIncludePath(b.path("include"));
    for (libs_list.items) |lib_name| {
        unit_tests.linkSystemLibrary(lib_name);
    }
    unit_tests.linkLibC();

    const run_unit_tests = b.addRunArtifact(unit_tests);
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_unit_tests.step);

    // Print variables step for debugging
    const print_step = b.step("print-vars", "Print build variables");
    const print_run = b.addSystemCommand(&[_][]const u8{
        "echo", "Zig build configuration loaded successfully"
    });
    print_step.dependOn(&print_run.step);

    // Clean step
    const clean_step = b.step("clean", "Clean build artifacts");
    const clean_run = b.addSystemCommand(&[_][]const u8{
        "rm", "-rf", "zig-out", "zig-cache", ".zig-cache"
    });
    clean_step.dependOn(&clean_run.step);
}   