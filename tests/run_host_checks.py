"""Render every bank entry and check FX bypass using Clang AddressSanitizer.

These host checks use the real engine, effects, and playback code with
hardware shims. Playback checks simulate the interrupt, gate cutoff and USB
serial calls. They do not exercise real USB, controls, or RP2040 audio timing.
"""

from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    host = project / "tests" / "host"
    cc, cxx = shutil.which("clang"), shutil.which("clang++")
    if not cc or not cxx:
        raise SystemExit("Install Clang and clang++ to run the host checks.")

    flags = [
        "-O1", "-g", "-fsanitize=address", "-fno-omit-frame-pointer",
        "-I" + str(host), "-I" + str(project / "src"),
        "-I" + str(project / "lib" / "ESP8266SAM" / "src"),
    ]
    with tempfile.TemporaryDirectory(prefix="talko-sam-checks-") as directory:
        build = Path(directory)
        objects = []
        for name in ("sam", "render", "reciter", "esp8266sam_debug"):
            obj = build / (name + ".o")
            source = project / "lib" / "ESP8266SAM" / "src" / (name + ".c")
            subprocess.run([cc, "-std=c11", *flags, "-c", str(source), "-o", str(obj)], check=True)
            objects.append(str(obj))
        program = build / "check"
        subprocess.run([
            cxx, "-std=c++17", *flags, str(host / "check.cpp"),
            str(project / "src" / "fx.cpp"), *objects, "-o", str(program),
        ], check=True)
        subprocess.run([str(program)], check=True)
        playback = build / "playback"
        subprocess.run([
            cxx, "-std=c++17", *flags, str(host / "playback.cpp"),
            str(project / "src" / "fx.cpp"), *objects, "-o", str(playback),
        ], check=True)
        subprocess.run([str(playback)], check=True)


if __name__ == "__main__":
    main()
