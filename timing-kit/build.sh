#!/bin/bash
# build.sh : helixsr-timing.exe and win10-helper.exe (the same source with the Agility SDK exports, for Windows 10) into out/
set -e
cd "$(dirname "$0")/windows"
# MINGW_BIN: the bin folder of llvm-mingw (github.com/mstorsjo/llvm-mingw; the published programs: 20261020, ucrt)
B=${MINGW_BIN:?set MINGW_BIN to the llvm-mingw bin folder}
F=${FFX_INCLUDE:-../third_party/ffx}
mkdir -p ../out
for t in helixsr-timing:"" win10-helper:-DKIT_D3D12_EXPORTS; do
    "$B/x86_64-w64-mingw32-clang" -std=c11 -O1 ${t#*:} -DZSTD_DISABLE_ASM -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES -DMINIZ_NO_ARCHIVE_WRITING_APIS \
        -DMINIZ_NO_TIME -I"$F" -Izstd/lib -Izstd/lib/common -Iminiz fsr4time.c miniz/miniz.c zstd/lib/common/*.c \
        zstd/lib/decompress/*.c -o "../out/${t%%:*}.exe" -ld3d12 -ldxgi -ldxguid -lwinhttp -ladvapi32 -lversion -lbcrypt -static
    echo "built out/${t%%:*}.exe"
done
