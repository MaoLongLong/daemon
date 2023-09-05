const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const strip = b.option(bool, "strip", "Omit debug information");

    const exe = b.addExecutable(.{
        .name = "daemon",
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    exe.strip = strip;
    exe.addCSourceFiles(&.{
        "src/log.c",
        "src/main.c",
    }, &.{
        "-Wall",
        "-W",
        "-pedantic",
        "-std=c99",
        "-DLOG_USE_COLOR",
    });

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
