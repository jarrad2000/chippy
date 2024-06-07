const std = @import("std");

pub fn build(b: *std.Build) void {
    const exe = b.addExecutable(.{
        .name = "chippy",
        .target = b.host,
    });

    exe.addCSourceFile(.{ .file = b.path("chippy.c"), .flags = &.{
        "-std=c99",
        "-Wall",
        "-W",
        "-Wstrict-prototypes",
        "-Wwrite-strings",
        "-Wno-missing-field-initializers",
    } });
    exe.addCSourceFile(.{ .file = b.path("cpu.c"), .flags = &.{
        "-std=c99",
        "-Wall",
        "-W",
        "-Wstrict-prototypes",
        "-Wwrite-strings",
        "-Wno-missing-field-initializers",
    } });
    exe.addCSourceFile(.{ .file = b.path("media.c"), .flags = &.{
        "-std=c99",
        "-Wall",
        "-W",
        "-Wstrict-prototypes",
        "-Wwrite-strings",
        "-Wno-missing-field-initializers",
    } });
    exe.linkSystemLibrary("m");
    exe.linkSystemLibrary("SDL2");
    exe.linkLibC();

    b.installArtifact(exe);
}
