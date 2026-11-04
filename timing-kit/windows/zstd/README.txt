The decompression part of Zstandard 1.5.7 (https://github.com/facebook/zstd, release tarball zstd-1.5.7.tar.gz,
sha256 eb33e51f49a15e023950cd7825ca74a4a2b43db8354825ac24fc1b7ee09e6fa3), unchanged: lib/zstd.h, lib/zstd_errors.h, lib/common
(headers and the decoder's .c files) and lib/decompress (without the x86-64 assembly file; built with ZSTD_DISABLE_ASM).
fsr4time.exe uses it to unpack the timing kit's DLL pack. Zstandard is dual-licensed BSD (LICENSE) or GPLv2 (COPYING).
