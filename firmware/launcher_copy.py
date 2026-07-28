"""Copy launcher app firmware.bin to firmware/dist/ after build."""

from pathlib import Path

Import("env")


def copy_launcher_bin(source, target, env):
    build_dir = Path(env.subst("$BUILD_DIR"))
    project_dir = Path(env.subst("$PROJECT_DIR"))
    dist_dir = project_dir / "dist"
    dist_dir.mkdir(exist_ok=True)

    src = build_dir / "firmware.bin"
    output_name = env.GetProjectOption("custom_launcher_output_bin", "family-hub-waveshare7b-launcher.bin")
    dst = dist_dir / output_name
    if not src.exists():
        print(f"[launcher] skip copy: missing {src}")
        return

    dst.write_bytes(src.read_bytes())
    size = dst.stat().st_size
    limit = 0x180000
    pct = (size / limit) * 100 if limit else 0
    print(f"[launcher] {dst} ({size} bytes, {pct:.1f}% of 0x180000 test slot)")
    if size > limit:
        print("[launcher] ERROR: firmware.bin exceeds Launcher test partition size")
        env.Exit(1)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_launcher_bin)
