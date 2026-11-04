# HelixSR timing kit

The source of the speed test in HelixSR Desktop: `helixsr-timing.exe` and `win10-helper.exe`. It times upscalers on your GPU with
made-up frames (no game) and checks which give the same picture.

## Based on the FSR 4.1.1 timing kit

This is a modified version of the Windows program of the **FSR 4.1.1 timing kit by zgauthier2000**:
https://github.com/zgauthier2000/fsr4-shader-performance-overrides (`timing-kit/windows`, commit in `UPSTREAM.txt`).
`README-original-kit.md` and `windows/README.txt` are that kit's own texts. The method and most of the program are its work.

Changed by lonewolf0622 in October 2026: `windows/fsr4time.c`, `windows/helixsr.inc` (new) and `build.sh`, `run-proton.sh` (new). It times
HLX FSR 4 and HLX DLSS as well, and Windows 10 gets `win10-helper.exe`, the same source with the Agility SDK exports.

## License

GNU General Public License, version 2 or (at your option) any later version: see `LICENSE`. This covers the kit only: it does not cover the
upscalers it times. Parts under their own licenses: `third_party/ffx` (AMD FidelityFX SDK headers, MIT), `windows/miniz` (MIT),
`windows/zstd` (BSD or GPL-2.0).

## Build

    MINGW_BIN=/path/to/llvm-mingw/bin ./build.sh

The programs go into `out/`. With llvm-mingw 20261020 (ucrt) on Linux this gives the programs HelixSR Desktop ships, the same byte for byte
except their build timestamp.
