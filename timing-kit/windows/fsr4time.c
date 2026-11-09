// SPDX-License-Identifier: GPL-2.0-or-later
// fsr4time.exe: the Windows timing kit. Times FSR 4.1.1 as a game runs it, through AMD's FidelityFX API on
// Direct3D 12, with each upscaler DLL from the pack next to it (dlls.pack.zst) or from a folder dlls\<name>\ a tester adds,
// on made-up frames. No game, no OptiScaler. Derived from research/pruning/fsr4img.c.
//
//   fsr4time.exe            runs every DLL (pack and dlls\) at 1080p, 1440p and 4K output with the Balanced preset's render size and
//                           with Ultra Performance's (a third of the output), still and moving picture, in rounds (the DLLs in
//                           name order, then in reverse order, and so on) for about 15 minutes, at least two rounds; prints a
//                           summary, saves it as results-<time>.txt and offers to send it
//   fsr4time.exe --run <name> <results file>     (used by the first form: one DLL, in a process of its own)
//   fsr4time.exe --quick    1080p and 4K only
//   fsr4time.exe --all-presets   also the Quality and Performance render sizes (twice the sizes; the rounds get 30 minutes unless --minutes)
//   fsr4time.exe --once     one round only
//   fsr4time.exe --minutes N   the time the rounds may take (default 15)
//   fsr4time.exe --no-questions   asks nothing at the end and sends nothing
//   fsr4time.exe --send-test   sends one message that says it is a test (to check that sending works)
//   FSR4TIME_CRASHTEST=<name>  (for testing) the run of the DLL <name> reads address 0x18C, as a crash inside a driver would
//   FSR4TIME_HANGTEST=<name>   (for testing) the run of the DLL <name> never ends, as a hung driver call would
//   FSR4TIME_LIMIT=<seconds>   (for testing) the time limit of one DLL's run (default: see RunLimit)
//   FSR4TIME_DRYSEND=<file>    (for testing) what would be sent is written to <file> instead
//   FSR4TIME_AGILITYONLY=1     (for testing) only the runtime choice (SetupRuntime: offer, check, probe), then end
//   FSR4TIME_NOWAIT=1          (for testing) win10-helper.exe opened by hand does not wait for Enter
//   FSR4TIME_FORCE_D3D12=1     (for testing) treat Windows' own runtime as too old (below shader model 6.6), as on Windows 10;
//                              =2 also accept any probe that made a device (Proton does not load D3D12\'s runtime, so its probe never says "from the kit")
//   win10-helper.exe --probe (used by SetupRuntime) prints the GPU line of a device made with Microsoft's runtime from D3D12\
//
// What is timed: the GPU time of one upscale call (prepass, 12 model passes, postpass and the small shaders around
// them), from a timestamp before to one after, 200 calls back to back after 40 to warm up. For a lossy DLL the
// two kinds of frame are also given apart. The summary gives a fingerprint of the picture each DLL produces from
// the still scene and from the moving one (the run's last frame) and names the first DLL above it that produced the same picture, byte for byte.
//
// Build (llvm-mingw): x86_64-w64-mingw32-clang -std=c11 -O1 -DZSTD_DISABLE_ASM -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES -DMINIZ_NO_ARCHIVE_WRITING_APIS
//                     -DMINIZ_NO_TIME -I<FidelityFX>/api/include -I<FidelityFX>/upscalers/include -Izstd/lib -Izstd/lib/common -Iminiz
//                     fsr4time.c miniz/miniz.c zstd/lib/common/*.c zstd/lib/decompress/*.c -o fsr4time.exe
//                     -ld3d12 -ldxgi -ldxguid -lwinhttp -ladvapi32 -lversion -lbcrypt -static
// The DLLs come as one pack, dlls.pack.zst (made by the project's mkpack.py and zstd -19 --long=27). Each run takes its DLL out of the pack next to the
// program and the next run overwrites it, so one DLL at a time is on disk (none at the end); DLLs a tester adds are read from dlls\<name>\.
// hook.inc is written by mkhook.py (the address results are sent to, scrambled; see ../submit.py).
#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS
#define WIN32_LEAN_AND_MEAN
#define _WINDOWS
#include <windows.h>
#include <winver.h>
#include <winhttp.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ffx_api.h"
#define ZSTD_STATIC_LINKING_ONLY
#include "zstd.h"      // zstd/: the decompression part of Zstandard 1.5.7, for the DLL pack
#include "miniz.h"     // miniz/: miniz 3.0.2 (MIT), to take D3D12Core.dll out of Microsoft's Agility SDK package (Windows 10, see GetAgility)
#include <bcrypt.h>
#include "ffx_api_loader.h"
#include "ffx_upscale.h"
#include "dx12/ffx_api_dx12.h"

#define HELIXSR_KIT 1      // HelixSR's version of the kit: also times HLX FSR 4 and HLX DLSS (helixsr.inc); sends nothing
#ifndef KIT_VERSION      // the builder passes -DKIT_VERSION="\"windows <version>\"" (since the next kit: kit 2026-10-20 was first built still
#define KIT_VERSION "helixsr windows 2026-10-27 (from kit windows 2026-10-20)"      // saying 2026-10-19, because this line was not changed)
#endif
#define WARM 40
#define TIMED 200
#define PHASES 72      // the most jitter steps any size uses (Ultra Performance: 8 x 3 x 3, what FSR asks a game for)
#define MAXROUNDS 8
#define MAXDLLS 48     // DLLs in one run (pack + tester folders); a study run has 16 (RDNA2) or 18 (RDNA3)
#define MAXROWS 1024   // summary lines: DLL x size x still/moving (a study run with --all-presets: 18 x 12 x 2 = 432; RX 9000: twice the DLLs)
#define MAXRUNS 12     // a DLL's runs: one per round, and up to four more where its runs disagreed (the re-timing after the rounds)
#define RUN_LIMIT_MIN 10      // a DLL's run that takes longer is stopped and noted (a hung driver call), and the kit goes on with the next DLL:
                              // the limit until a first run has finished on this PC; after that RUN_LIMIT_TIMES x the longest finished run,
                              // at least RUN_LIMIT_FLOOR minutes and at most RUN_LIMIT_MIN, in whole minutes (kit 2026-10-21: a hung run cost
                              // 10 minutes on every card, while finished runs take about 0.5 to 2 minutes)
#define RUN_LIMIT_TIMES 3
#define RUN_LIMIT_FLOOR 2

static FILE* out;      // the results file of this run
static void Say(const char* fmt, ...) {
    va_list a;
    va_start(a, fmt); vprintf(fmt, a); va_end(a);
    if (out) { va_start(a, fmt); vfprintf(out, fmt, a); va_end(a); fflush(out); }
    fflush(stdout);
}

// A run that crashes (inside the loader, the upscaler DLL or the driver) used to stop on Windows' "Application Error" box, once per
// DLL, and the tester got no summary (issue #34). Now the run writes what crashed and where into the results as a FAILED line (the
// summary's failure list, which is also sent) and ends; the parent goes on with the next DLL. Exit code 3 = crash recorded by the run.
static const char* run_name = "?";
static void FileVersion(const char* path, char* v, size_t n) {
    v[0] = 0;
    DWORD h = 0, sz = GetFileVersionInfoSizeA(path, &h);
    if (!sz || sz > 1 << 20) return;
    void* buf = malloc(sz);
    VS_FIXEDFILEINFO* fi = NULL;
    UINT len = 0;
    if (buf && GetFileVersionInfoA(path, 0, sz, buf) && VerQueryValueA(buf, "\\", (void**)&fi, &len) && fi && len >= sizeof(*fi))
        snprintf(v, n, " %u.%u.%u.%u", (unsigned)HIWORD(fi->dwFileVersionMS), (unsigned)LOWORD(fi->dwFileVersionMS), (unsigned)HIWORD(fi->dwFileVersionLS), (unsigned)LOWORD(fi->dwFileVersionLS));
    free(buf);
}
static LONG WINAPI Crashed(EXCEPTION_POINTERS* e) {
    const EXCEPTION_RECORD* r = e->ExceptionRecord;
    const char* at = (const char*)r->ExceptionAddress;
    HMODULE m = NULL;
    char mod[MAX_PATH] = "unknown module", ver[48] = "", extra[96] = "";
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, at, &m) && m) {
        GetModuleFileNameA(m, mod, sizeof(mod));
        FileVersion(mod, ver, sizeof(ver));
    }
    const char* b = strrchr(mod, '\\');
    b = b ? b + 1 : mod;
    if (r->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && r->NumberParameters >= 2)
        snprintf(extra, sizeof(extra), ", %s address 0x%llx", r->ExceptionInformation[0] == 1 ? "writing" : r->ExceptionInformation[0] == 8 ? "running code at" : "reading",
                 (unsigned long long)r->ExceptionInformation[1]);
    Say("R|%s|FAILED|crashed: exception 0x%08lx in %s%s at +0x%llx%s\n", run_name, (unsigned long)r->ExceptionCode, b, ver,
        (unsigned long long)(m ? (uintptr_t)(at - (const char*)m) : (uintptr_t)at), extra);
    TerminateProcess(GetCurrentProcess(), 3);
    return EXCEPTION_EXECUTE_HANDLER;
}

static void KitFail(const char* what, HRESULT hr, int line);   // a failed D3D12 call: the reason (device removed, debug layer), then exit
#define CHECK(x)                                                                                              \
    do {                                                                                                      \
        HRESULT hr_ = (x);                                                                                    \
        if (FAILED(hr_)) KitFail(#x, hr_, __LINE__);                                                          \
    } while (0)

static ID3D12Device* device;
// HelixSR: a run whose kit-env.txt has KIT_D3D12_DEBUG=1 turns on Microsoft's D3D12 debug layer when Windows has it (Graphics Tools);
// its messages (errors and warnings, each kind once) become D| lines, and the first error goes into a failed run's reason
static ID3D12InfoQueue* kit_iq;
static char kit_diag[12288]; static size_t kit_diag_len;   // the summary's diagnostics section (X| lines)
static char kit_first_error[600];
static void KitDebugDump(void) {
    if (!kit_iq) return;
    static int seen[64], nseen;
    const UINT64 n = ID3D12InfoQueue_GetNumStoredMessages(kit_iq);
    for (UINT64 i = 0; i < n; ++i) {
        SIZE_T len = 0;
        if (FAILED(ID3D12InfoQueue_GetMessage(kit_iq, i, NULL, &len)) || !len || len > 65536) continue;
        D3D12_MESSAGE* m = (D3D12_MESSAGE*)malloc(len);
        if (m && SUCCEEDED(ID3D12InfoQueue_GetMessage(kit_iq, i, m, &len)) && m->Severity <= D3D12_MESSAGE_SEVERITY_WARNING) {
            int dup = 0;
            for (int k = 0; k < nseen; ++k) dup |= seen[k] == (int)m->ID;
            if (!dup && nseen < 64) {
                seen[nseen++] = (int)m->ID;
                Say("X|%s|%s|%d|%.*s\n", run_name, m->Severity <= D3D12_MESSAGE_SEVERITY_ERROR ? "error" : "warning", (int)m->ID,
                    (int)(m->DescriptionByteLength > 500 ? 500 : m->DescriptionByteLength), m->pDescription);
                if (m->Severity <= D3D12_MESSAGE_SEVERITY_ERROR && !kit_first_error[0])
                    snprintf(kit_first_error, sizeof(kit_first_error), "%.*s", (int)(m->DescriptionByteLength > 400 ? 400 : m->DescriptionByteLength), m->pDescription);
            }
        }
        free(m);
    }
    ID3D12InfoQueue_ClearStoredMessages(kit_iq);
}
// HelixSR's diagnostic lines (diag:, ERROR) from the log next to a run's helixsr.dll, as D| lines
static void KitHelixDiag(const char* name) {
    char p[MAX_PATH], ln[1024];
    snprintf(p, sizeof(p), "dlls\\%s\\helixsr.log", name);
    FILE* f = fopen(p, "r");
    int n = 0;
    while (f && fgets(ln, sizeof(ln), f) && n < 40) {
        ln[strcspn(ln, "\r\n")] = 0;
        if (strstr(ln, "diag:") || strstr(ln, "ERROR") || strstr(ln, "has no resource") || strstr(ln, "above the D3D12")) { Say("X|%s|helixsr|0|%s\n", name, ln); ++n; }
    }
    if (f) fclose(f);
}
static void KitFail(const char* what, HRESULT hr, int line) {
    const HRESULT why = device ? ID3D12Device_GetDeviceRemovedReason(device) : S_OK;
    KitDebugDump();
    KitHelixDiag(run_name);
    char removed[64] = "";
    if (why != S_OK) snprintf(removed, sizeof(removed), "; device removed, reason 0x%08lx", (unsigned long)why);
    Say("FAILED %s: 0x%08lx (line %d)\n", what, (unsigned long)hr, line);
    Say("R|%s|FAILED|%s failed 0x%08lx%s%s%s\n", run_name, what, (unsigned long)hr, removed, kit_first_error[0] ? "; D3D12: " : "", kit_first_error);
    exit(1);
}
static ID3D12CommandQueue* queue;
static ID3D12CommandAllocator* allocator;
static ID3D12GraphicsCommandList* list;
static ID3D12Fence* fence;
static UINT64 fence_value;
static HANDLE fence_event;

static void Submit(void) {
    CHECK(ID3D12GraphicsCommandList_Close(list));
    ID3D12CommandList* lists[] = {(ID3D12CommandList*)list};
    ID3D12CommandQueue_ExecuteCommandLists(queue, 1, lists);
    CHECK(ID3D12CommandQueue_Signal(queue, fence, ++fence_value));
    CHECK(ID3D12Fence_SetEventOnCompletion(fence, fence_value, fence_event));
    WaitForSingleObject(fence_event, INFINITE);
    CHECK(ID3D12CommandAllocator_Reset(allocator));
    CHECK(ID3D12GraphicsCommandList_Reset(list, allocator, NULL));
}

static ID3D12Resource* Texture(DXGI_FORMAT format, UINT w, UINT h, bool uav) {
    D3D12_HEAP_PROPERTIES heap = {.Type = D3D12_HEAP_TYPE_DEFAULT};
    D3D12_RESOURCE_DESC desc = {.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D, .Width = w, .Height = h, .DepthOrArraySize = 1,
                                .MipLevels = 1, .Format = format, .SampleDesc = {1, 0},
                                .Flags = uav ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE};
    ID3D12Resource* r;
    CHECK(ID3D12Device_CreateCommittedResource(device, &heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, NULL,
                                               &IID_ID3D12Resource, (void**)&r));
    return r;
}

static ID3D12Resource* Buffer(D3D12_HEAP_TYPE type, UINT64 size) {
    D3D12_HEAP_PROPERTIES heap = {.Type = type};
    D3D12_RESOURCE_DESC desc = {.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER, .Width = size, .Height = 1, .DepthOrArraySize = 1,
                                .MipLevels = 1, .SampleDesc = {1, 0}, .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR};
    ID3D12Resource* r;
    CHECK(ID3D12Device_CreateCommittedResource(device, &heap, D3D12_HEAP_FLAG_NONE, &desc,
                                               type == D3D12_HEAP_TYPE_UPLOAD ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST,
                                               NULL, &IID_ID3D12Resource, (void**)&r));
    return r;
}

static struct FfxApiResource Resource(ID3D12Resource* r, uint32_t format, uint32_t state) {
    D3D12_RESOURCE_DESC desc = ID3D12Resource_GetDesc(r);
    struct FfxApiResource o = {0};
    o.resource = r; o.state = state;
    o.description.type = FFX_API_RESOURCE_TYPE_TEXTURE2D; o.description.format = format;
    o.description.width = (uint32_t)desc.Width; o.description.height = desc.Height; o.description.depth = 1; o.description.mipCount = 1;
    o.description.usage = state == FFX_API_RESOURCE_STATE_UNORDERED_ACCESS ? FFX_API_RESOURCE_USAGE_UAV : 0;
    return o;
}

static uint16_t Half(float f) {
    uint32_t x;
    memcpy(&x, &f, 4);
    const uint32_t sign = (x >> 16) & 0x8000u;
    const int exp = (int)((x >> 23) & 0xffu) - 127 + 15;
    if (exp <= 0) return (uint16_t)sign;
    return (uint16_t)(sign | ((uint32_t)exp << 10) | ((x >> 13) & 0x3ffu));
}

// The made-up picture: a function of a point of the "world", periodic with period 192 pixels of the output, so
// that a camera moving a whole number of pixels per frame repeats after 16 frames. Soft gradients, thin lines
// and fine texture, to give the model something like a game picture to work on.
static uint32_t Hash(uint32_t x, uint32_t y) {
    uint32_t h = x * 0x9E3779B1u ^ y * 0x85EBCA77u;
    h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12;
    return h;
}
static void World(float wx, float wy, float* rgb) {
    wx -= 192.0f * (float)(int)(wx / 192.0f); if (wx < 0) wx += 192.0f;
    wy -= 192.0f * (float)(int)(wy / 192.0f); if (wy < 0) wy += 192.0f;
    const uint32_t cx = (uint32_t)(wx / 6.0f), cy = (uint32_t)(wy / 6.0f);
    const float n = (float)(Hash(cx, cy) & 255u) / 255.0f, g = wx / 192.0f, v = wy / 192.0f;
    const float tri = g < 0.5f ? 2 * g : 2 - 2 * g, trv = v < 0.5f ? 2 * v : 2 - 2 * v;
    const bool line = ((uint32_t)wx % 24u) < 2u || ((uint32_t)(wx + wy) % 32u) < 2u;
    float r = 0.15f + 0.5f * tri + 0.12f * n, gr = 0.12f + 0.45f * trv + 0.10f * n, b = 0.2f + 0.3f * (1 - tri) + 0.15f * n;
    if (line) { r = 0.9f; gr = 0.85f; b = 0.7f; }
    rgb[0] = r; rgb[1] = gr; rgb[2] = b;
}
static float Halton(int i, int b) {
    float f = 1, r = 0;
    while (i > 0) { f /= (float)b; r += f * (float)(i % b); i /= b; }
    return r;
}

static void UploadRows(ID3D12Resource* tex, UINT w, UINT h, UINT bpp, const uint8_t* src) {
    const UINT pitch = (w * bpp + 255) & ~255u;
    ID3D12Resource* up = Buffer(D3D12_HEAP_TYPE_UPLOAD, (UINT64)pitch * h);
    uint8_t* data;
    CHECK(ID3D12Resource_Map(up, 0, NULL, (void**)&data));
    for (UINT y = 0; y < h; ++y) memcpy(data + (size_t)y * pitch, src + (size_t)y * w * bpp, (size_t)w * bpp);
    ID3D12Resource_Unmap(up, 0, NULL);
    D3D12_RESOURCE_DESC desc = ID3D12Resource_GetDesc(tex);
    D3D12_TEXTURE_COPY_LOCATION dst = {.pResource = tex, .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    D3D12_TEXTURE_COPY_LOCATION s = {.pResource = up, .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
    s.PlacedFootprint.Footprint.Format = desc.Format; s.PlacedFootprint.Footprint.Width = w; s.PlacedFootprint.Footprint.Height = h;
    s.PlacedFootprint.Footprint.Depth = 1; s.PlacedFootprint.Footprint.RowPitch = pitch;
    ID3D12GraphicsCommandList_CopyTextureRegion(list, &dst, 0, 0, 0, &s, NULL);
    Submit();
    ID3D12Resource_Release(up);
}

// When a DLL's two runs give different pictures, the summary also says how different. Whole frames of every run would be gigabytes, so the first run
// leaves a small record per 16x16-pixel block of its last frame (a hash and the sum of its color values) in a file, and the second run compares its
// own blocks with it and reports: how many blocks differ, the area they lie in, and the largest shift of a block's mean value.
#define BLK 16
struct BlockRec { uint64_t hash; float sum; };
static float HalfToFloat(uint16_t h) {
    const int e = (h >> 10) & 31, m = h & 1023;
    float v = e == 0 ? (float)m / 1024.0f / 16384.0f : e == 31 ? 0.0f : (1.0f + (float)m / 1024.0f) * (e >= 15 ? (float)(1u << (e - 15)) : 1.0f / (float)(1u << (15 - e)));
    return (h & 0x8000) ? -v : v;
}
static void CompareBlocks(const uint8_t* data, UINT pitch, UINT ow, UINT oh, const char* key, uint64_t* whole) {
    const UINT bw = (ow + BLK - 1) / BLK, bh = (oh + BLK - 1) / BLK, n = bw * bh;
    struct BlockRec* now = (struct BlockRec*)calloc(n, sizeof(*now));
    for (UINT i = 0; i < n; ++i) now[i].hash = 1469598103934665603ull;
    for (UINT y = 0; y < oh; ++y) {
        const uint8_t* row = data + (size_t)y * pitch;
        for (UINT x = 0; x < ow; ++x) {
            struct BlockRec* b = &now[(y / BLK) * bw + x / BLK];
            for (int c = 0; c < 8; ++c) { b->hash ^= row[x * 8 + c]; b->hash *= 1099511628211ull; }
            const uint16_t* px = (const uint16_t*)(row + x * 8);
            b->sum += HalfToFloat(px[0]) + HalfToFloat(px[1]) + HalfToFloat(px[2]);
        }
    }
    uint64_t kh = 1469598103934665603ull;
    for (const char* q = key; *q; ++q) { kh ^= (uint8_t)*q; kh *= 1099511628211ull; }
    char file[64];
    snprintf(file, sizeof(file), "blocks-%016llx.tmp", (unsigned long long)kh);
    FILE* f = fopen(file, "rb");
    if (!f) {      // the first run: leave the record
        f = fopen(file, "wb");
        if (f) { fwrite(now, sizeof(*now), n, f); fclose(f); }
    } else {       // the second run: compare
        struct BlockRec* was = (struct BlockRec*)calloc(n, sizeof(*was));
        const size_t got = fread(was, sizeof(*was), n, f);
        fclose(f);
        if (getenv("FSR4TIME_POKE")) { now[bw + 5].hash ^= 1; now[bw + 5].sum += 0.768f; now[2 * bw + 7].hash ^= 1; *whole ^= 1; }      // test switch: pretend two blocks came out differently
        if (got == n) {
            UINT nd = 0, x0 = bw, y0 = bh, x1 = 0, y1 = 0;
            double worst = 0;
            for (UINT i = 0; i < n; ++i)
                if (was[i].hash != now[i].hash) {
                    const UINT bx = i % bw, by = i / bw;
                    ++nd; if (bx < x0) x0 = bx; if (bx > x1) x1 = bx; if (by < y0) y0 = by; if (by > y1) y1 = by;
                    double d = ((double)now[i].sum - (double)was[i].sum) / (BLK * BLK * 3); if (d < 0) d = -d;
                    if (d > worst) worst = d;
                }
            // key | blocks that differ | blocks in all | area in pixels: x0, y0, x1, y1 | largest shift of a block's mean value (1 = full scale)
            if (nd) Say("D|%s|%u|%u|%u|%u|%u|%u|%.6f\n", key, nd, n, x0 * BLK, y0 * BLK, (x1 + 1) * BLK - 1 < ow - 1 ? (x1 + 1) * BLK - 1 : ow - 1, (y1 + 1) * BLK - 1 < oh - 1 ? (y1 + 1) * BLK - 1 : oh - 1, worst);
        }
        free(was);
    }
    free(now);
}

static void DeleteBlockRecords(void) {      // the records the first runs leave for the second ones
    WIN32_FIND_DATAA fd;
    HANDLE fh = FindFirstFileA("blocks-*.tmp", &fd);
    if (fh != INVALID_HANDLE_VALUE) { do DeleteFileA(fd.cFileName); while (FindNextFileA(fh, &fd)); FindClose(fh); }
}

// How far a picture is from AMD's (since kit 2026-10-20; a fingerprint only says "the same" or "not"; report 16243817, an RX 7900 XT on Windows
// 10 with Microsoft's newer runtime, gave a different fingerprint for every DLL, AMD's own forms included). The reference DLLs (1-amd-shaders, and
// 1b-amd-shaders-dots-split, the reference on RDNA2) leave their last frame per output size, render size and scene in a file, as 8-bit display
// values (the color clamped to 0..1, a 2.2 gamma, 0..255: what a screen would show, so rounding that no screen shows comes out identical).
// Every other DLL compares its own last frame with each file there and reports: dB (PSNR, 255 = full scale), the share of values more than one
// display level apart, and the largest difference in levels. The files (up to about 25 MB each at 4K) are deleted when the run ends.
static const char* REFS[] = {"0-amd-fsr4", "1b-amd-shaders-dots-split"};   // 0-amd-fsr4: HelixSR Desktop's folder for AMD's DLL
static uint8_t* DisplayLut(void) {      // half-float bits -> 8-bit display level
    static uint8_t* lut = NULL;
    if (!lut) {
        lut = (uint8_t*)malloc(65536);
        for (int i = 0; i < 65536; ++i) {
            float v = HalfToFloat((uint16_t)i); if (!(v > 0)) v = 0; if (v > 1) v = 1;
            lut[i] = (uint8_t)(255.0 * pow(v, 1.0 / 2.2) + 0.5);
        }
    }
    return lut;
}
static uint64_t KeyHash(const char* a, const char* b) {
    uint64_t kh = 1469598103934665603ull;
    for (const char* q = a; *q; ++q) { kh ^= (uint8_t)*q; kh *= 1099511628211ull; }
    kh ^= '|'; kh *= 1099511628211ull;
    for (const char* q = b; *q; ++q) { kh ^= (uint8_t)*q; kh *= 1099511628211ull; }
    return kh;
}
static void ComparePicture(const uint8_t* data, UINT pitch, UINT ow, UINT oh, const char* label, const char* key, const char* pkey) {
    const uint8_t* lut = DisplayLut();
    const size_t n = (size_t)ow * oh * 3;
    uint8_t* now = (uint8_t*)malloc(n);
    for (UINT y = 0; y < oh; ++y) {
        const uint16_t* row = (const uint16_t*)(data + (size_t)y * pitch);
        for (UINT x = 0; x < ow; ++x) for (int c = 0; c < 3; ++c) now[((size_t)y * ow + x) * 3 + c] = lut[row[x * 4 + c]];
    }
    if (getenv("FSR4TIME_POKEPIC") && strcmp(label, REFS[0]) && strcmp(label, REFS[1])) now[n / 2] ^= 8;      // test switch: one value of every other DLL off by 8 levels
    char file[96];
    for (int r = 0; r < 2; ++r) {
        snprintf(file, sizeof(file), "pic-%016llx.tmp", (unsigned long long)KeyHash(REFS[r], pkey));
        if (!strcmp(label, REFS[r])) {      // a reference: leave its picture (once)
            FILE* f = fopen(file, "rb");
            if (f) fclose(f);
            else if ((f = fopen(file, "wb"))) { fwrite(now, 1, n, f); fclose(f); }
            continue;
        }
        FILE* f = fopen(file, "rb");
        if (!f) continue;
        uint8_t* ref = (uint8_t*)malloc(n);
        const size_t got = fread(ref, 1, n, f);
        fclose(f);
        if (got == n) {
            double sse = 0; size_t over = 0; int worst = 0;
            for (size_t i = 0; i < n; ++i) {
                const int d = (int)now[i] - (int)ref[i], a = d < 0 ? -d : d;
                sse += (double)d * d; if (a > 1) ++over; if (a > worst) worst = a;
            }
            const double db = sse > 0 ? 10.0 * log10(255.0 * 255.0 / (sse / (double)n)) : -1;      // -1: identical at display precision
            // key | reference DLL | dB (-1 = identical on screen) | share of values more than 1 level off | largest difference in levels
            Say("P|%s|%s|%.2f|%.6f|%d\n", key, REFS[r], db, (double)over / (double)n, worst);
        }
        free(ref);
    }
    free(now);
}
static void DeletePictureRecords(void) {
    WIN32_FIND_DATAA fd;
    HANDLE fh = FindFirstFileA("pic-*.tmp", &fd);
    if (fh != INVALID_HANDLE_VALUE) { do DeleteFileA(fd.cFileName); while (FindNextFileA(fh, &fd)); FindClose(fh); }
}

static uint64_t HashOutput(ID3D12Resource* output, UINT ow, UINT oh, const char* key, const char* label, const char* pkey) {
    const UINT pitch = (ow * 8 + 255) & ~255u;
    ID3D12Resource* rb = Buffer(D3D12_HEAP_TYPE_READBACK, (UINT64)pitch * oh);
    D3D12_TEXTURE_COPY_LOCATION src = {.pResource = output, .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    D3D12_TEXTURE_COPY_LOCATION dst = {.pResource = rb, .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
    dst.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; dst.PlacedFootprint.Footprint.Width = ow;
    dst.PlacedFootprint.Footprint.Height = oh; dst.PlacedFootprint.Footprint.Depth = 1; dst.PlacedFootprint.Footprint.RowPitch = pitch;
    D3D12_RESOURCE_BARRIER b = {.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION};
    b.Transition.pResource = output; b.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS; b.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    ID3D12GraphicsCommandList_ResourceBarrier(list, 1, &b);
    ID3D12GraphicsCommandList_CopyTextureRegion(list, &dst, 0, 0, 0, &src, NULL);
    b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE; b.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    ID3D12GraphicsCommandList_ResourceBarrier(list, 1, &b);
    Submit();
    uint8_t* data;
    CHECK(ID3D12Resource_Map(rb, 0, NULL, (void**)&data));
    uint64_t h = 1469598103934665603ull;
    for (UINT y = 0; y < oh; ++y)
        for (UINT x = 0; x < ow * 8; ++x) { h ^= data[(size_t)y * pitch + x]; h *= 1099511628211ull; }
    if (key) CompareBlocks(data, pitch, ow, oh, key, &h);
    if (key && label && pkey) ComparePicture(data, pitch, ow, oh, label, key, pkey);
    ID3D12Resource_Unmap(rb, 0, NULL);
    ID3D12Resource_Release(rb);
    return h;
}

static void Message(uint32_t type, const wchar_t* message) { Say("  FFX %s: %ls\n", type == FFX_API_MESSAGE_TYPE_ERROR ? "error" : "warning", message); }
static int CmpD(const void* a, const void* b) { double x = *(const double*)a, y = *(const double*)b; return x < y ? -1 : x > y; }
static double Median(double* v, int n) { qsort(v, n, sizeof(double), CmpD); return n ? v[n / 2] : 0; }

// The DirectX 12 Agility SDK (since kit 2026-10-18.2): Windows uses D3D12\D3D12Core.dll next to the program when that folder is there, as games
// do with their own copy, instead of the runtime built into Windows (Windows 10's is older; a GTX 1660 SUPER on Windows 10 crashed inside it in
// every run, 2026-10-18). Without the folder Windows uses its own runtime, as before. The version must match the D3D12Core.dll shipped.
// OFF in fsr4time.exe since kit 2026-10-18.3: on Windows 11 23H2 (build 22631) every device creation then failed with 0x887E0003
// (D3D12_ERROR_INVALID_REDIST) although no D3D12\ folder was there (report 013B3D90, 2026-10-18); 24H2 (26100) ran. Since kit 2026-10-19 the
// exports are in a second program, win10-helper.exe (the same source built with -DKIT_D3D12_EXPORTS), which runs the DLLs only on a PC whose
// own runtime offers less than shader model 6.6 and only once D3D12\ holds Microsoft's checked runtime (SetupRuntime below).
#ifdef KIT_D3D12_EXPORTS
__declspec(dllexport) const UINT D3D12SDKVersion = 615;
__declspec(dllexport) const char* D3D12SDKPath = ".\\D3D12\\";
#endif
static int study = 0;      // --study (run-study.cmd, since kit 2026-10-19): 2 or 3 = the RDNA2 / RDNA3 study set of the pack instead of the normal DLLs
// RDNA2: RX 6000 cards (Navi 21 to 24, 73A0-743F) and, since kit 2026-10-18.2, the RDNA2 chips built into processors: Van Gogh 163F (Steam Deck
// LCD), Sephiroth 1435 (Steam Deck OLED), Rembrandt 1681 (Radeon 680M / 660M), Mendocino 1506 (610M), Raphael 164E (Ryzen 7000 desktop, 7045
// laptops), Granite Ridge 13C0 (Ryzen 9000 desktop). RDNA3 (for the study, since kit 2026-10-19): RX 7000 cards (Navi 31 to 33, 7440-749F;
// seen in reports: 744C, 747E, 7480) and the RDNA3 / 3.5 chips built into processors: Phoenix 15BF (780M / 760M, seen), Phoenix 2 15C8,
// Strix Point 150E (890M / 880M, seen), Krackan 1114, Strix Halo 1586.
static bool IsRdna2(unsigned ven, unsigned dev) {
    return ven == 0x1002 && ((dev >= 0x73A0 && dev <= 0x743F) || dev == 0x163F || dev == 0x1435 || dev == 0x1681 || dev == 0x1506 || dev == 0x164E || dev == 0x13C0);
}
static bool IsRdna3(unsigned ven, unsigned dev) {
    return ven == 0x1002 && ((dev >= 0x7440 && dev <= 0x749F) || dev == 0x15BF || dev == 0x15C8 || dev == 0x150E || dev == 0x1114 || dev == 0x1586);
}
static bool GpuIds(unsigned* ven, unsigned* dev, char* name, size_t n) {      // the adapter the runs use (high performance), without a device
    IDXGIFactory6* f6 = NULL; IDXGIAdapter1* ad = NULL; bool ok = false;
    if (SUCCEEDED(CreateDXGIFactory1(&IID_IDXGIFactory6, (void**)&f6)) &&
        SUCCEEDED(IDXGIFactory6_EnumAdapterByGpuPreference(f6, 0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, &IID_IDXGIAdapter1, (void**)&ad))) {
        DXGI_ADAPTER_DESC1 d;
        if (SUCCEEDED(IDXGIAdapter1_GetDesc1(ad, &d))) { *ven = d.VendorId; *dev = d.DeviceId; snprintf(name, n, "%ls", d.Description); ok = true; }
        IDXGIAdapter1_Release(ad);
    }
    if (f6) IDXGIFactory6_Release(f6);
    return ok;
}
// which pack DLLs a run takes: the normal run every DLL but the study sets; a study run AMD's two references, the exact DLL and its own set
// (the Durandal test DLLs were in study runs for a while on 2026-10-19; the user reversed that the same day)
static int normgen = 0;      // a normal run on RDNA2 / RDNA3 graphics (2 / 3): it also takes its generation's pass 11 tests (since the next kit)
static bool Pass11Test(const char* name) {      // <g>-f1-exact (the build the tests are compared with), <g>-t1-tiled-pass11, <g>-t2-amd-cut-pass11
    return !strncmp(name + 6, "f1-", 3) || !strncmp(name + 6, "t1-", 3) || !strncmp(name + 6, "t2-", 3);
}
static bool PackingTest(const char* name) {      // rdna2-p1..p5: the RX 6000 packing test (since kit 2026-10-22 also in RDNA2 normal runs)
    return !strncmp(name, "rdna2-p", 7) && name[7] >= '1' && name[7] <= '9';
}
static bool StudyOnly(const char* name) {      // since kit 2026-10-22: release dll-2026-10-19's DLLs stay in the pack as the study sets' baselines only
    return !strcmp(name, "2-exact") || !strcmp(name, "3-lossy") || !strcmp(name, "3b-lossy2");
}
static bool Wanted(const char* name) {
    const bool s2 = !strncmp(name, "rdna2-", 6), s3 = !strncmp(name, "rdna3-", 6);
    if (!study) return (!s2 && !s3 && !StudyOnly(name)) || (normgen == 2 && s2 && (Pass11Test(name) || PackingTest(name))) || (normgen == 3 && s3 && Pass11Test(name));
    // since the next kit: also the released lossy and lossy2 DLLs, which the study set's family lossy DLLs (rdna2-f2/f3, rdna3-f2/f3) are
    // compared with; the family DLLs are named rdna2-f* / rdna3-f*, so they come with their generation's study set and stay out of normal runs
    return !strncmp(name, "1-", 2) || !strncmp(name, "1b-", 3) || !strcmp(name, "2-exact") || !strcmp(name, "3-lossy") || !strcmp(name, "3b-lossy2")
        || (study == 2 ? s2 : s3);
}
static void CreateDevice(char* gpu, size_t gpu_len) {
    IDXGIFactory6* f6 = NULL;
    IDXGIAdapter1* ad = NULL;
    if (SUCCEEDED(CreateDXGIFactory1(&IID_IDXGIFactory6, (void**)&f6)))
        IDXGIFactory6_EnumAdapterByGpuPreference(f6, 0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, &IID_IDXGIAdapter1, (void**)&ad);
    const char* dbg = getenv("KIT_D3D12_DEBUG");
    if (dbg && *dbg == '1') {
        ID3D12Debug* d = NULL;
        if (SUCCEEDED(D3D12GetDebugInterface(&IID_ID3D12Debug, (void**)&d)) && d) { ID3D12Debug_EnableDebugLayer(d); ID3D12Debug_Release(d); }
    }
    CHECK(D3D12CreateDevice((IUnknown*)ad, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, (void**)&device));
    if (dbg && *dbg == '1') {
        if (SUCCEEDED(ID3D12Device_QueryInterface(device, &IID_ID3D12InfoQueue, (void**)&kit_iq)) && kit_iq) {
            ID3D12InfoQueue_SetMessageCountLimit(kit_iq, 4096);
            Say("X|%s|kit|0|D3D12 debug layer on\n", run_name);
        } else
            Say("X|%s|kit|0|D3D12 debug layer not available (Windows: Settings > Optional features > Graphics Tools)\n", run_name);
    }
    snprintf(gpu, gpu_len, "?");
    if (ad) {
        DXGI_ADAPTER_DESC1 d;
        LARGE_INTEGER umd = {0};
        IDXGIAdapter1_GetDesc1(ad, &d);
        IDXGIAdapter1_CheckInterfaceSupport(ad, &IID_IDXGIDevice, &umd);
        snprintf(gpu, gpu_len, "%ls (PCI %04x:%04x, %u MB), driver %u.%u.%u.%u", d.Description, d.VendorId, d.DeviceId, (unsigned)(d.DedicatedVideoMemory >> 20),
                 (unsigned)(umd.QuadPart >> 48) & 0xffff, (unsigned)(umd.QuadPart >> 32) & 0xffff, (unsigned)(umd.QuadPart >> 16) & 0xffff, (unsigned)umd.QuadPart & 0xffff);
    }
    // what the Direct3D 12 runtime in use offers (since kit 2026-10-18.2): its version, whether it is the kit's copy or Windows' own, and
    // the highest shader model the device reports (FSR 4's INT8 shaders need 6.6)
    char rv[64] = " unknown", where[16] = "";
    HMODULE core = GetModuleHandleA("D3D12Core.dll");
    if (core) {
        char path[MAX_PATH];
        if (GetModuleFileNameA(core, path, sizeof(path))) { FileVersion(path, rv, sizeof(rv)); snprintf(where, sizeof(where), "%s", strstr(path, "\\D3D12\\") ? " from the kit" : " built in"); }
    }
    int smv = 0;
    for (int m = 0x69; m >= 0x51 && !smv; --m) {
        D3D12_FEATURE_DATA_SHADER_MODEL sm = {(D3D_SHADER_MODEL)m};
        if (SUCCEEDED(ID3D12Device_CheckFeatureSupport(device, D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm)))) smv = (int)sm.HighestShaderModel;
    }
    const size_t gl = strlen(gpu);
    snprintf(gpu + gl, gpu_len - gl, ", D3D12 runtime%s%s, shader model %d.%d", rv[0] ? rv : " unknown", where, smv >> 4, smv & 15);
    D3D12_COMMAND_QUEUE_DESC qd = {.Type = D3D12_COMMAND_LIST_TYPE_DIRECT};
    CHECK(ID3D12Device_CreateCommandQueue(device, &qd, &IID_ID3D12CommandQueue, (void**)&queue));
    CHECK(ID3D12Device_CreateCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator, (void**)&allocator));
    CHECK(ID3D12Device_CreateCommandList(device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator, NULL, &IID_ID3D12GraphicsCommandList, (void**)&list));
    CHECK(ID3D12Device_CreateFence(device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void**)&fence));
    fence_event = CreateEventA(NULL, FALSE, FALSE, NULL);
}

// One DLL (the one next to the exe), every size and scene. Lines "R|..." are what the summary is made from.
static bool all_presets = false;      // --all-presets: Quality and Performance sizes too (passed on to each DLL's run as "all")
// Quality / Performance render size and jitter steps for an output, as AMD's loader reports them (the query FSR offers a game); if the
// query fails, FSR's own formula (output / ratio, rounded down; 8 x ratio^2 steps)
static void PresetSize(ffxFunctions* fx, uint32_t ow, uint32_t oh, uint32_t mode, double ratio, UINT* rw, UINT* rh, int* phases) {
    uint32_t w = 0, h = 0; int32_t n = 0;
    struct ffxQueryDescUpscaleGetRenderResolutionFromQualityMode q = {0};
    q.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE; q.displayWidth = ow; q.displayHeight = oh; q.qualityMode = mode;
    q.pOutRenderWidth = &w; q.pOutRenderHeight = &h;
    if (fx->Query(NULL, &q.header) != FFX_API_RETURN_OK || !w || !h) { w = (uint32_t)(ow / ratio); h = (uint32_t)(oh / ratio); }
    struct ffxQueryDescUpscaleGetJitterPhaseCount j = {0};
    j.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETJITTERPHASECOUNT; j.renderWidth = w; j.displayWidth = ow; j.pOutPhaseCount = &n;
    if (fx->Query(NULL, &j.header) != FFX_API_RETURN_OK || n <= 0) n = (int32_t)(8.0 * ((double)ow / w) * ((double)ow / w));
    if (n > PHASES) n = PHASES;
    *rw = w; *rh = h; *phases = n;
}
#include "helixsr.inc"      // HelixSR: HLX FSR 4's version from its log, kit-env.txt, HLX DLSS through NGX

static int RunOne(const char* name, bool quick) {
    char ngxdll[MAX_PATH];
    if (HelixIsNgx(name, ngxdll, sizeof(ngxdll))) return RunNgx(name, ngxdll, quick);      // HelixSR: HLX DLSS
    char gpu[256];
    CreateDevice(gpu, sizeof(gpu));
    HMODULE loader = LoadLibraryA("amd_fidelityfx_loader_dx12.dll");
    if (!loader) { Say("R|%s|FAILED|no amd_fidelityfx_loader_dx12.dll next to the program\n", name); return 1; }
    ffxFunctions ffx;
    ffxLoadFunctions(&ffx, loader);
    uint64_t count = 0, ids[32];
    const char* names[32];
    struct ffxQueryDescGetVersions versions = {0};
    versions.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS; versions.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
    versions.device = device; versions.outputCount = &count;
    ffx.Query(NULL, &versions.header);
    if (count > 32) count = 32;
    versions.versionIds = ids; versions.versionNames = names;
    ffx.Query(NULL, &versions.header);
    // every FSR 4 version the DLL offers on this GPU is timed. AMD's 4.1.1 DLL offers one here; a DLL that carries several
    // variants of the model (say one per GPU generation) gets a block of lines for each, in the order the DLL lists them
    uint64_t vid[8];
    const char* vname[8];
    int nv = 0;
    for (uint64_t i = 0; i < count; ++i)
        if (nv < 8 && (!strncmp(names[i], "4.", 2) || !strncmp(names[i], "fsr4", 4) || !strncmp(names[i], "HLX", 3))) { vid[nv] = ids[i]; vname[nv++] = names[i]; }      // AMD's "4.1.1" and this project's
        // "4.1.1-...-cyboman" names (also "fsr4..." ones, which no release uses: OptiScaler reads the version from the name, so names keep x.y.z first)
    if (getenv("FSR4TIME_TWICE") && nv == 1) { vid[1] = vid[0]; vname[1] = vname[0]; nv = 2; }      // for testing the lines of a DLL with two variants: the one version, listed twice
    if (!nv) { Say("R|%s|FAILED|this DLL offers no FSR 4 on this GPU (%llu versions listed)\n", name, (unsigned long long)count); return 1; }
    Say("G|%s\n", gpu);

    // the Balanced preset's render size (16 jitter steps, as in every kit before) and Ultra Performance's (72 steps, as FSR asks a game for)
    // with --all-presets also Quality (" Q") and Performance (" P") at each output, sized by PresetSize (after the six above, so their order
    // and figures stay those of every kit before)
    struct { UINT ow, oh, rw, rh; int phases; const char* tag; } sizes[12] = {
        {1920, 1080, 1130, 636, 16, ""}, {2560, 1440, 1506, 848, 16, ""}, {3840, 2160, 2260, 1272, 16, ""},
        {1920, 1080, 640, 360, 72, " UP"}, {2560, 1440, 853, 480, 72, " UP"}, {3840, 2160, 1280, 720, 72, " UP"}};
    int nsizes = 6;
    if (all_presets)
        for (int m = 0; m < 2; ++m)
            for (int o = 0; o < 3; ++o) {
                sizes[nsizes].ow = sizes[o].ow; sizes[nsizes].oh = sizes[o].oh; sizes[nsizes].tag = m ? " P" : " Q";
                PresetSize(&ffx, sizes[o].ow, sizes[o].oh, m ? FFX_UPSCALE_QUALITY_MODE_PERFORMANCE : FFX_UPSCALE_QUALITY_MODE_QUALITY, m ? 2.0 : 1.5,
                           &sizes[nsizes].rw, &sizes[nsizes].rh, &sizes[nsizes].phases);
                ++nsizes;
            }
    ID3D12QueryHeap* qh;
    D3D12_QUERY_HEAP_DESC qhd = {.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP, .Count = 2};
    CHECK(ID3D12Device_CreateQueryHeap(device, &qhd, &IID_ID3D12QueryHeap, (void**)&qh));
    ID3D12Resource* qbuf = Buffer(D3D12_HEAP_TYPE_READBACK, 16);
    UINT64 freq = 0;
    CHECK(ID3D12CommandQueue_GetTimestampFrequency(queue, &freq));

    for (int vi = 0; vi < nv; ++vi) {
        const uint64_t chosen = vid[vi];
        const char* chosen_name = vname[vi];
        char label[64], idtext[40] = "";      // with one variant the lines carry the folder's name alone, as before
        if (nv == 1) snprintf(label, sizeof(label), "%s", name);
        else { snprintf(label, sizeof(label), "%.40s, variant %d of %d", name, vi + 1, nv); snprintf(idtext, sizeof(idtext), ", id %llx", (unsigned long long)chosen); }
        for (int si = 0; si < nsizes; ++si) {
            if (quick && sizes[si].ow == 2560) continue;
            const UINT ow = sizes[si].ow, oh = sizes[si].oh, rw = sizes[si].rw, rh = sizes[si].rh;
            const int phases = sizes[si].phases;
            char sz[24];      // the size as the summary names it: the output, and " UP" / " Q" / " P" for the presets other than Balanced
            snprintf(sz, sizeof(sz), "%ux%u%s", ow, oh, sizes[si].tag);
            struct ffxCreateBackendDX12Desc backend = {0};
            backend.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12; backend.device = device;
            struct ffxOverrideVersion ov = {0};
            ov.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION; ov.versionId = chosen; ov.header.pNext = &backend.header;
            ID3D12Resource* color[PHASES];
            for (int p = 0; p < phases; ++p) color[p] = Texture(DXGI_FORMAT_R16G16B16A16_FLOAT, rw, rh, false);
            ID3D12Resource* depth = Texture(DXGI_FORMAT_R32_FLOAT, rw, rh, false);
            ID3D12Resource* motion = Texture(DXGI_FORMAT_R16G16_FLOAT, rw, rh, false);
            ID3D12Resource* output = Texture(DXGI_FORMAT_R16G16B16A16_FLOAT, ow, oh, true);
            uint16_t* cbuf = malloc((size_t)rw * rh * 8);
            float* dbuf = malloc((size_t)rw * rh * 4);
            uint16_t* mbuf = malloc((size_t)rw * rh * 4);
            for (size_t i = 0; i < (size_t)rw * rh; ++i) dbuf[i] = 0.5f;
            UploadRows(depth, rw, rh, 4, (const uint8_t*)dbuf);
            const float sx = (float)rw / (float)ow, sy = (float)rh / (float)oh;
            for (int scene = 0; scene < 2; ++scene) {            // 0: nothing moves; 1: the camera pans 12, 6 output pixels per frame
                const int vx = scene ? 12 : 0, vy = scene ? 6 : 0;
                for (size_t i = 0; i < (size_t)rw * rh; ++i) { mbuf[2 * i] = Half((float)vx * sx); mbuf[2 * i + 1] = Half((float)vy * sy); }
                UploadRows(motion, rw, rh, 4, (const uint8_t*)mbuf);
                float jx[PHASES], jy[PHASES];
                for (int p = 0; p < phases; ++p) {
                    jx[p] = Halton(p + 1, 2) - 0.5f; jy[p] = Halton(p + 1, 3) - 0.5f;
                    for (UINT y = 0; y < rh; ++y)
                        for (UINT x = 0; x < rw; ++x) {
                            float rgb[3];
                            World(((float)x + 0.5f - jx[p]) / sx - 0.5f + (float)(vx * p), ((float)y + 0.5f - jy[p]) / sy - 0.5f + (float)(vy * p), rgb);
                            uint16_t* q = cbuf + ((size_t)y * rw + x) * 4;
                            q[0] = Half(rgb[0]); q[1] = Half(rgb[1]); q[2] = Half(rgb[2]); q[3] = Half(1.0f);
                        }
                    UploadRows(color[p], rw, rh, 8, (const uint8_t*)cbuf);
                }
                struct ffxCreateContextDescUpscale create = {0};
                create.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE; create.header.pNext = &ov.header;
                create.flags = FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE | FFX_UPSCALE_ENABLE_AUTO_EXPOSURE;
                create.maxRenderSize.width = ow; create.maxRenderSize.height = oh; create.maxUpscaleSize.width = ow; create.maxUpscaleSize.height = oh;
                create.fpMessage = Message;
                ffxContext context = NULL;
                ffxReturnCode_t rc = ffx.CreateContext(&context, &create.header, NULL);
                if (rc != FFX_API_RETURN_OK) { Say("R|%s|FAILED|create context %d at %s\n", label, (int)rc, sz); goto next_variant; }
                struct ffxQueryGetProviderVersion provider = {0};
                provider.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;
                ffx.Query(&context, &provider.header);
                // HelixSR: HLX FSR 4 names the version it runs (its set and tier) in its log
                const char* vn = provider.versionName ? provider.versionName : chosen_name;
                char hv[128];
                if (!strncmp(vn, "HLX", 3)) { HelixVersion(hv, sizeof(hv)); if (hv[0]) vn = hv; }
                static double ms[TIMED], even[TIMED], odd[TIMED];
                int ne = 0, no = 0;
                for (int frame = 0; frame < WARM + TIMED; ++frame) {
                    const int p = frame % phases;
                    struct ffxDispatchDescUpscale d = {0};
                    d.header.type = FFX_API_DISPATCH_DESC_TYPE_UPSCALE; d.commandList = list;
                    d.color = Resource(color[p], FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT, FFX_API_RESOURCE_STATE_COMMON);
                    d.depth = Resource(depth, FFX_API_SURFACE_FORMAT_R32_FLOAT, FFX_API_RESOURCE_STATE_COMMON);
                    d.motionVectors = Resource(motion, FFX_API_SURFACE_FORMAT_R16G16_FLOAT, FFX_API_RESOURCE_STATE_COMMON);
                    d.output = Resource(output, FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
                    d.jitterOffset.x = jx[p]; d.jitterOffset.y = jy[p];
                    d.motionVectorScale.x = d.motionVectorScale.y = 1.0f;
                    d.renderSize.width = rw; d.renderSize.height = rh; d.upscaleSize.width = ow; d.upscaleSize.height = oh;
                    d.enableSharpening = false; d.sharpness = 0.5f; d.frameTimeDelta = 10.0f; d.preExposure = 1.0f; d.reset = frame == 0;
                    d.cameraNear = 0.05f; d.cameraFar = 3000.0f; d.cameraFovAngleVertical = 0.75f; d.viewSpaceToMetersFactor = 1.0f;
                    ID3D12GraphicsCommandList_EndQuery(list, qh, D3D12_QUERY_TYPE_TIMESTAMP, 0);
                    rc = ffx.Dispatch(&context, &d.header);
                    if (rc != FFX_API_RETURN_OK) { Say("R|%s|FAILED|dispatch %d at %s\n", label, (int)rc, sz); goto next_variant; }
                    ID3D12GraphicsCommandList_EndQuery(list, qh, D3D12_QUERY_TYPE_TIMESTAMP, 1);
                    ID3D12GraphicsCommandList_ResolveQueryData(list, qh, D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, qbuf, 0);
                    Submit();
                    if (frame >= WARM) {
                        UINT64* t;
                        D3D12_RANGE rr = {0, 16};
                        CHECK(ID3D12Resource_Map(qbuf, 0, &rr, (void**)&t));
                        const double v = (double)(t[1] - t[0]) * 1000.0 / (double)freq;
                        ID3D12Resource_Unmap(qbuf, 0, NULL);
                        ms[frame - WARM] = v;
                        if (jx[p] < 0) odd[no++] = v; else even[ne++] = v;      // a lossy DLL skips the model where the jitter's x is negative
                    }
                }
                double sum = 0, fastest = ms[0];
                for (int i = 0; i < TIMED; ++i) { sum += ms[i]; if (ms[i] < fastest) fastest = ms[i]; }
                const double med = Median(ms, TIMED), me = Median(even, ne), mo = Median(odd, no);
                char key[256];      // names this DLL, version, size and scene: the same in its two runs
                snprintf(key, sizeof(key), "%s|%s%s|%s|%s", label, provider.versionName ? provider.versionName : chosen_name, idtext, sz, scene ? "moving" : "still");
                char pkey[64];      // size, render size and scene, without the DLL: the reference pictures' key
                snprintf(pkey, sizeof(pkey), "%s|%ux%u|%s", sz, rw, rh, scene ? "moving" : "still");
                const uint64_t h = HashOutput(output, ow, oh, key, label, pkey);      // the last frame of the run: the still scene's picture, or the moving scene's after the same 240 frames
                // name | version name the DLL reports | output | render | scene | mean | median | median where jitter x >= 0 | where < 0 | fastest | picture hash
                Say("R|%s|%s%s|%s|%ux%u|%s|%.4f|%.4f|%.4f|%.4f|%.4f|%016llx\n", label, vn, idtext, sz, rw, rh,
                    scene ? "moving" : "still", sum / TIMED, med, me, mo, fastest, (unsigned long long)h);
                ffx.DestroyContext(&context, NULL);
            }
            for (int p = 0; p < phases; ++p) ID3D12Resource_Release(color[p]);
            ID3D12Resource_Release(depth); ID3D12Resource_Release(motion); ID3D12Resource_Release(output);
            free(cbuf); free(dbuf); free(mbuf);
        }
    next_variant:;
    }
    return 0;
}

// ---- the first form: every DLL, each in a process of its own (the loader takes the DLL next to the exe)
#include "hook.inc"      // _hook_bytes[], _hook_key[]: where results are sent, stored scrambled (see mkhook.py)

static void HookUrl(wchar_t* host, wchar_t* path) {
    char u[256];
    const int n = (int)sizeof(_hook_bytes);
    for (int i = 0; i < n; ++i) u[i] = (char)(_hook_bytes[n - 1 - i] ^ _hook_key[i % (int)sizeof(_hook_key)] ^ (uint8_t)(i * 7));
    u[n] = 0;
    const char* h = strstr(u, "://") + 3;
    const char* p = strchr(h, '/');
    int i = 0;
    for (const char* c = h; c < p; ++c) host[i++] = (wchar_t)*c;
    host[i] = 0; i = 0;
    for (const char* c = p; *c; ++c) path[i++] = (wchar_t)*c;
    path[i] = 0;
}

// The results go to the project's channel as one message: a short line (the report ID, the card, how the run went) with the whole summary
// attached as a file. (Kits before 2026-10-18 pasted the summary in pieces of up to 1,800 characters, the channel's limit being 2,000, and
// the pieces of one report could end up apart.)
#define BOUNDARY "fsr4kit-7c3e9a1b5d2f8e60"
static void JsonText(char* d, size_t cap, const char* s) {      // s as the inside of a JSON string (printable ASCII and line ends only)
    size_t n = 0;
    for (; *s && n + 3 < cap; ++s) {
        const unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') { d[n++] = '\\'; d[n++] = (char)c; }
        else if (c == '\n') { d[n++] = '\\'; d[n++] = 'n'; }
        else if (c >= 32 && c < 127) d[n++] = (char)c;
    }
    d[n] = 0;
}
// A whole text file as a string (malloc'd; NULL if missing, empty or over 1.5 MB)
static char* ReadText(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    char* b = (char*)malloc(1536 * 1024 + 1); size_t n = b ? fread(b, 1, 1536 * 1024 + 1, f) : 0;
    fclose(f);
    if (!b || n == 0 || n > 1536 * 1024) { free(b); return NULL; }
    b[n] = 0; return b;
}
// The message with one or two attached text files (filename2/file2 NULL: one). Since kit 2026-10-18 a report attaches the summary and the
// per-run file results-raw.txt.
static bool Send(const char* message, const char* filename, const char* file, const char* filename2, const char* file2) {
#ifdef HELIXSR_KIT
    (void)message; (void)filename; (void)file; (void)filename2; (void)file2;
    return false;      // HelixSR's kit sends nothing: HelixSR Desktop shows the results and copies them for a report
#endif
    static char body[1 << 21];
    char msg[2048];
    JsonText(msg, sizeof(msg), message);
    const int two = filename2 && file2;
    int n = snprintf(body, sizeof(body),
                     "--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"payload_json\"\r\nContent-Type: application/json\r\n\r\n"
                     "{\"allowed_mentions\":{\"parse\":[]},\"content\":\"%s\",\"attachments\":[{\"id\":0,\"filename\":\"%s\"}%s%s%s]}\r\n",
                     msg, filename, two ? ",{\"id\":1,\"filename\":\"" : "", two ? filename2 : "", two ? "\"}" : "");
    for (int k = 0; k <= two && n >= 0; ++k) {
        const char* fn = k ? filename2 : filename; const char* f = k ? file2 : file;
        const int len = (int)strlen(f);
        int h = snprintf(body + n, sizeof(body) - n, "--" BOUNDARY "\r\nContent-Disposition: form-data; name=\"files[%d]\"; filename=\"%s\"\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n", k, fn);
        if (h < 0 || n + h + len + 64 > (int)sizeof(body)) return false;
        n += h; memcpy(body + n, f, (size_t)len); n += len;
        n += snprintf(body + n, sizeof(body) - n, "\r\n");
    }
    if (n < 0) return false;
    n += snprintf(body + n, sizeof(body) - n, "--" BOUNDARY "--\r\n");
    if (getenv("FSR4TIME_DRYSEND")) {      // for testing: the request's body goes to this file instead
        FILE* t = fopen(getenv("FSR4TIME_DRYSEND"), "wb");
        if (t) { fwrite(body, 1, (size_t)n, t); fclose(t); }
        printf("Test: written to %s, nothing sent.\n", getenv("FSR4TIME_DRYSEND"));
        return t != NULL;
    }
    wchar_t host[128], path[256];
    HookUrl(host, path);
    bool ok = false;
    HINTERNET s = WinHttpOpen(L"fsr4-timing-kit", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET c = s ? WinHttpConnect(s, host, INTERNET_DEFAULT_HTTPS_PORT, 0) : NULL;
    HINTERNET r = c ? WinHttpOpenRequest(c, L"POST", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : NULL;
    if (r && WinHttpSendRequest(r, L"Content-Type: multipart/form-data; boundary=" BOUNDARY "\r\n", (DWORD)-1, body, (DWORD)n, (DWORD)n, 0) && WinHttpReceiveResponse(r, NULL)) {
        DWORD code = 0, len2 = sizeof(code);
        WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &code, &len2, NULL);
        ok = code >= 200 && code < 300;
    }
    if (r) WinHttpCloseHandle(r);
    if (c) WinHttpCloseHandle(c);
    if (s) WinHttpCloseHandle(s);
    return ok;
}

// ---- Windows 10 and the DirectX 12 Agility SDK (since kit 2026-10-18.2; user, 2026-10-18). Windows 10's own Direct3D 12 runtime is older than
// the one games bring with them (Microsoft's Agility SDK, a D3D12Core.dll in the game's folder); a GTX 1660 SUPER on Windows 10 crashed inside
// it in every run while FSR 4 worked in games there. On Windows 10 the kit offers to fetch Microsoft's runtime from Microsoft's own package on
// nuget.org (Microsoft.Direct3D.D3D12 1.615.1), checks both the package and the file against their SHA-256, and puts D3D12Core.dll in
// D3D12\ here, which this program asks Windows to use (D3D12SDKVersion / D3D12SDKPath above). The kit itself does not carry Microsoft's file:
// the tester fetches it from Microsoft, under Microsoft's license. Only after a yes; never with --no-questions.
#define AGILITY_HOST L"api.nuget.org"
#define AGILITY_PATH L"/v3-flatcontainer/microsoft.direct3d.d3d12/1.615.1/microsoft.direct3d.d3d12.1.615.1.nupkg"
#define AGILITY_PKG_SHA "20b67ae94eda93b9e1712362647d4c7b7d3b2d9150fded15950294d81611f17d"
#define AGILITY_DLL_SHA "07d286c306f8117321422affd9e6388c12d0fb4be1c7fc689d9e899324feeb24"
#define AGILITY_LICENSE "https://www.nuget.org/packages/Microsoft.Direct3D.D3D12/1.615.1/license"
static DWORD WindowsBuild(void) {      // the real build number (GetVersionEx reports what the manifest asks for)
    typedef LONG (WINAPI *RtlGetVersionFn)(OSVERSIONINFOW*);
    RtlGetVersionFn f = (RtlGetVersionFn)(void*)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion");
    OSVERSIONINFOW v = {.dwOSVersionInfoSize = sizeof(v)};
    return f && f(&v) == 0 ? v.dwBuildNumber : 0;
}
static bool Sha256Hex(const void* data, size_t n, char out[65]) {
    BCRYPT_ALG_HANDLE alg = NULL; BCRYPT_HASH_HANDLE h = NULL; UCHAR d[32]; bool ok = false;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, 0) == 0 && BCryptCreateHash(alg, &h, NULL, 0, NULL, 0, 0) == 0 &&
        BCryptHashData(h, (PUCHAR)data, (ULONG)n, 0) == 0 && BCryptFinishHash(h, d, sizeof(d), 0) == 0) {
        for (int i = 0; i < 32; ++i) snprintf(out + 2 * i, 3, "%02x", d[i]);
        ok = true;
    }
    if (h) BCryptDestroyHash(h);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    return ok;
}
static uint8_t* Download(const wchar_t* host, const wchar_t* path, size_t* n) {      // HTTPS GET into memory (at most 64 MB); NULL on any failure
    uint8_t* buf = NULL; size_t len = 0, cap = 0; bool ok = false;
    HINTERNET s = WinHttpOpen(L"fsr4-timing-kit", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET c = s ? WinHttpConnect(s, host, INTERNET_DEFAULT_HTTPS_PORT, 0) : NULL;
    HINTERNET r = c ? WinHttpOpenRequest(c, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : NULL;
    if (r && WinHttpSendRequest(r, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(r, NULL)) {
        DWORD code = 0, cl = sizeof(code);
        WinHttpQueryHeaders(r, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &code, &cl, NULL);
        ok = code == 200;
        for (DWORD avail = 0; ok && WinHttpQueryDataAvailable(r, &avail) && avail; ) {
            if (len + avail > (64u << 20)) { ok = false; break; }
            if (len + avail > cap) { cap = (len + avail) * 2; uint8_t* nb = realloc(buf, cap); if (!nb) { ok = false; break; } buf = nb; }
            DWORD got = 0;
            if (!WinHttpReadData(r, buf + len, avail, &got)) { ok = false; break; }
            len += got;
        }
    }
    if (r) WinHttpCloseHandle(r);
    if (c) WinHttpCloseHandle(c);
    if (s) WinHttpCloseHandle(s);
    if (!ok || !len) { free(buf); return NULL; }
    *n = len; return buf;
}
static void GetAgility(const char* base, bool unattended) {
    const DWORD build = WindowsBuild();
    char dll[MAX_PATH], dir[MAX_PATH];
    snprintf(dir, sizeof(dir), "%sD3D12", base); snprintf(dll, sizeof(dll), "%s\\D3D12Core.dll", dir);
    if (GetFileAttributesA(dll) != INVALID_FILE_ATTRIBUTES) return;      // already there (SetupRuntime checks it)
    printf("Windows' own DirectX 12 runtime here (build %lu) is older than the one games bring with them: FSR 4 needs shader model 6.6,\n"
           "and on such PCs every DLL crashed inside the runtime. The kit can fetch Microsoft's newer runtime (D3D12Core.dll, from Microsoft's\n"
           "DirectX 12 Agility SDK 1.615.1, a 22 MB download from nuget.org) into the folder D3D12 here, and use it for this run.\n"
           "It is Microsoft's file, under Microsoft's license: %s\n", (unsigned long)build, AGILITY_LICENSE);
#ifdef HELIXSR_KIT
    (void)unattended;      // HelixSR: fetched without a question (HelixSR Desktop's speed test says so before it starts)
#else
    if (unattended) { printf("(--no-questions: not fetched; the README says how to get it by hand)\n\n"); return; }
    char in[16];
    printf("Fetch it? [Y/n] "); fflush(stdout);
    if (!fgets(in, sizeof(in), stdin) || in[0] == 'n' || in[0] == 'N') { printf("Not fetched; the run uses Windows' own runtime.\n\n"); return; }
#endif
    printf("Fetching ...\n"); fflush(stdout);
    size_t n = 0; char hex[65];
    uint8_t* pkg = Download(AGILITY_HOST, AGILITY_PATH, &n);
    if (!pkg) { printf("Could not download it; the run uses Windows' own runtime.\n\n"); return; }
    if (!Sha256Hex(pkg, n, hex) || strcmp(hex, AGILITY_PKG_SHA)) { printf("The download is not the expected file (checksum); not used.\n\n"); free(pkg); return; }
    mz_zip_archive z; memset(&z, 0, sizeof(z)); size_t dn = 0; void* d = NULL;
    if (mz_zip_reader_init_mem(&z, pkg, n, 0)) { d = mz_zip_reader_extract_file_to_heap(&z, "build/native/bin/x64/D3D12Core.dll", &dn, 0); mz_zip_reader_end(&z); }
    free(pkg);
    if (!d || !Sha256Hex(d, dn, hex) || strcmp(hex, AGILITY_DLL_SHA)) { printf("Could not take the runtime out of the package; not used.\n\n"); if (d) mz_free(d); return; }
    CreateDirectoryA(dir, NULL);
    FILE* f = fopen(dll, "wb"); bool ok = f && fwrite(d, 1, dn, f) == dn;
    if (f) fclose(f);
    mz_free(d);
    if (!ok) { DeleteFileA(dll); printf("Could not write %s; the run uses Windows' own runtime.\n\n", dll); return; }
    printf("Done: D3D12\\D3D12Core.dll (checked). The summary's GPU line says which runtime each run used.\n\n");
}

// The runtime the DLL runs use (since kit 2026-10-19). Windows' own is used unless it offers less than shader model 6.6 (Windows 10's stops at
// 6.5: issue #34, RX 7900 XT on 21H2, every DLL crashed inside it; a GTX 1660 SUPER before). Then the kit offers Microsoft's runtime
// (GetAgility), checks the file in D3D12\ against its SHA-256, and asks win10-helper.exe --probe whether a device made with it offers 6.6+.
// Only if all of that holds do the DLL runs start as win10-helper.exe; otherwise they stay with fsr4time.exe and Windows' runtime, as before.
static const char* child_exe = "helixsr-timing.exe";      // the program each DLL's run is started as
static char runtime_note[256];                       // a line for the summary when the kit's runtime was used or offered
static int BuiltinShaderModel(void) {      // highest shader model of a device on the runs' adapter, with this program's runtime (Windows' own here); 0 = none
    IDXGIFactory6* f6 = NULL; IDXGIAdapter1* ad = NULL; ID3D12Device* d = NULL; int smv = 0;
    if (SUCCEEDED(CreateDXGIFactory1(&IID_IDXGIFactory6, (void**)&f6)))
        IDXGIFactory6_EnumAdapterByGpuPreference(f6, 0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, &IID_IDXGIAdapter1, (void**)&ad);
    if (SUCCEEDED(D3D12CreateDevice((IUnknown*)ad, D3D_FEATURE_LEVEL_12_0, &IID_ID3D12Device, (void**)&d))) {
        for (int m = 0x69; m >= 0x51 && !smv; --m) {
            D3D12_FEATURE_DATA_SHADER_MODEL sm = {(D3D_SHADER_MODEL)m};
            if (SUCCEEDED(ID3D12Device_CheckFeatureSupport(d, D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm)))) smv = (int)sm.HighestShaderModel;
        }
        ID3D12Device_Release(d);
    }
    if (ad) IDXGIAdapter1_Release(ad);
    if (f6) IDXGIFactory6_Release(f6);
    return smv;
}
static void SetupRuntime(const char* base, bool unattended) {
    const int own = BuiltinShaderModel();
    const bool force = getenv("FSR4TIME_FORCE_D3D12") != NULL;      // for testing on a PC whose runtime is new enough (Proton)
    if (own >= 0x66 && !force) return;
    printf("Windows' own DirectX 12 runtime offers shader model %d.%d here%s.\n", own >> 4, own & 15, force ? " (FSR4TIME_FORCE_D3D12: treated as too old)" : "");
    GetAgility(base, unattended);
    char dll[MAX_PATH], hex[65] = "";
    snprintf(dll, sizeof(dll), "%sD3D12\\D3D12Core.dll", base);
    FILE* f = fopen(dll, "rb");
    if (!f) { snprintf(runtime_note, sizeof(runtime_note), "runtime: Windows' own (shader model %d.%d); Microsoft's newer runtime was not fetched", own >> 4, own & 15); return; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t* d = n > 0 ? malloc((size_t)n) : NULL; const bool rd = d && fread(d, 1, (size_t)n, f) == (size_t)n;
    fclose(f);
    const bool same = rd && Sha256Hex(d, (size_t)n, hex) && !strcmp(hex, AGILITY_DLL_SHA);
    free(d);
    if (!same) { printf("D3D12\\D3D12Core.dll is not Microsoft's file the kit expects (checksum); not used.\n\n");
                 snprintf(runtime_note, sizeof(runtime_note), "runtime: Windows' own; D3D12\\D3D12Core.dll did not match the expected checksum"); return; }
    char cmd[MAX_PATH + 64], line[1024], probe[1024] = "";
    snprintf(cmd, sizeof(cmd), "\"\"%swin10-helper.exe\" --probe\"", base);
    FILE* pp = _popen(cmd, "r");
    while (pp && fgets(line, sizeof(line), pp)) if (!strncmp(line, "PROBE|", 6)) { snprintf(probe, sizeof(probe), "%s", line + 6); probe[strcspn(probe, "\r\n")] = 0; }
    if (pp) _pclose(pp);
    const char* smp = strstr(probe, "shader model ");
    int a = 0, b = 0;
    if (smp) sscanf(smp, "shader model %d.%d", &a, &b);
    const bool accept_any = force && !strcmp(getenv("FSR4TIME_FORCE_D3D12"), "2") && probe[0];      // for testing the switch under Proton
    if ((strstr(probe, "from the kit") && (a > 6 || (a == 6 && b >= 6))) || accept_any) {
        child_exe = "win10-helper.exe";
        printf("The DLL runs use Microsoft's runtime from D3D12\\ (shader model %d.%d).\n\n", a, b);
        snprintf(runtime_note, sizeof(runtime_note), "runtime: Microsoft's DirectX 12 runtime from D3D12\\ (win10-helper.exe); Windows' own offers shader model %d.%d", own >> 4, own & 15);
    } else {
        printf("A test with Microsoft's runtime did not give shader model 6.6 (%s); the DLL runs use Windows' own runtime.\n\n", probe[0] ? probe : "no device");
        snprintf(runtime_note, sizeof(runtime_note), "runtime: Windows' own (shader model %d.%d); the test with Microsoft's runtime failed: %.120s", own >> 4, own & 15, probe[0] ? probe : "no device");
    }
}

// ---- the DLL pack: the kit's DLLs are variants of one file, and a zip compresses each on its own (235 MB for nine). As one zstd stream with a
// long match window they take about 10 MB. Nothing is unpacked in advance: for each run the DLL is taken out of the pack (decompressing the stream
// up to it, about a second at most; the file is checked against the hash in the pack) straight into the place the loader reads it from.
// Pack layout (little endian, inside the zstd stream): "FSR4PK1\n", u32 count, per file: u16 path length, path ('/' separated; its first part is the
// DLL's name), u64 size, u64 FNV-1a 64 of the data; then the files' data in the same order.
#define PACK_NAME "dlls.pack.zst"
#define PACK_MAX 128     // since kit 2026-10-19 the pack also holds both study sets (about 40 DLLs); 128 since the next kit (71 DLLs)
struct PackFile { char path[260]; uint64_t size, hash; };
struct PackIn { FILE* f; ZSTD_DCtx* dc; uint8_t* ib; ZSTD_inBuffer in; uint8_t* ob; size_t ob_len, ob_pos; bool end; };
static bool PackFill(struct PackIn* p) {      // more decompressed bytes into ob (keeps what is left unread); false at the end of the stream
    if (p->ob_pos > 0) { memmove(p->ob, p->ob + p->ob_pos, p->ob_len - p->ob_pos); p->ob_len -= p->ob_pos; p->ob_pos = 0; }
    while (p->ob_len < (1u << 20)) {
        if (p->in.pos == p->in.size) {
            if (p->end) return p->ob_len > 0;
            p->in.size = fread(p->ib, 1, 1u << 20, p->f); p->in.pos = 0;
            if (!p->in.size) { p->end = true; continue; }
        }
        ZSTD_outBuffer o = {p->ob + p->ob_len, (2u << 20) - p->ob_len, 0};
        const size_t r = ZSTD_decompressStream(p->dc, &o, &p->in);
        if (ZSTD_isError(r)) { printf("The DLL pack is damaged (%s). Please download the kit again.\n", ZSTD_getErrorName(r)); return false; }
        p->ob_len += o.pos;
        if (o.pos == 0 && p->in.pos == p->in.size && p->end) break;
    }
    return p->ob_len > 0;
}
static bool PackRead(struct PackIn* p, void* dst, size_t n) {
    uint8_t* d = (uint8_t*)dst;
    while (n) {
        if (p->ob_pos == p->ob_len && !PackFill(p)) return false;
        size_t k = p->ob_len - p->ob_pos; if (k > n) k = n;
        memcpy(d, p->ob + p->ob_pos, k); p->ob_pos += k; d += k; n -= k;
    }
    return true;
}
static bool PackSkip(struct PackIn* p, uint64_t n) {
    while (n) {
        if (p->ob_pos == p->ob_len && !PackFill(p)) return false;
        uint64_t k = p->ob_len - p->ob_pos; if (k > n) k = n;
        p->ob_pos += (size_t)k; n -= k;
    }
    return true;
}
static struct PackFile pf[PACK_MAX];
static uint32_t npf;
static bool PackStart(struct PackIn* p, FILE* f) {      // opens the stream and reads the list of files into pf / npf
    *p = (struct PackIn){f, ZSTD_createDCtx(), malloc(1u << 20), {0}, malloc(2u << 20), 0, 0, false};
    p->in.src = p->ib;
    ZSTD_DCtx_setParameter(p->dc, ZSTD_d_windowLogMax, 27);
    char magic[8]; uint32_t n = 0;
    bool ok = PackRead(p, magic, 8) && !memcmp(magic, "FSR4PK1\n", 8) && PackRead(p, &n, 4) && n <= PACK_MAX;
    for (uint32_t i = 0; ok && i < n; ++i) {
        uint16_t len = 0;
        ok = PackRead(p, &len, 2) && len < sizeof(pf[i].path) && PackRead(p, pf[i].path, len) && PackRead(p, &pf[i].size, 8) && PackRead(p, &pf[i].hash, 8);
        if (ok) { pf[i].path[len] = 0; ok = !strstr(pf[i].path, "..") && !strchr(pf[i].path, ':') && pf[i].path[0] != '/' && strchr(pf[i].path, '/'); }
        if (ok) *strchr(pf[i].path, '/') = 0;      // keep the DLL's name (the folder part)
    }
    npf = ok ? n : 0;
    return ok;
}
static void PackEnd(struct PackIn* p) { fclose(p->f); ZSTD_freeDCtx(p->dc); free(p->ib); free(p->ob); }
static bool PackList(void) {      // true when there is no pack or its list could be read; false (with a message) when it is damaged
    FILE* f = fopen(PACK_NAME, "rb");
    if (!f) return true;
    struct PackIn p;
    const bool ok = PackStart(&p, f);
    PackEnd(&p);
    if (!ok) printf("The DLL pack (%s) is not one this program can read. Please download the kit again.\n", PACK_NAME);
    return ok;
}
static bool PackExtract(uint32_t want, const char* dst) {      // the pack's file number want, checked, as dst
    FILE* f = fopen(PACK_NAME, "rb");
    if (!f) return false;
    struct PackIn p;
    bool ok = PackStart(&p, f) && want < npf;
    for (uint32_t i = 0; ok && i < want; ++i) ok = PackSkip(&p, pf[i].size);
    FILE* o = ok ? fopen(dst, "wb") : NULL;
    static uint8_t buf[1u << 20];
    uint64_t left = ok ? pf[want].size : 0, h = 0xcbf29ce484222325ull;
    ok = ok && o;
    while (ok && left) {
        const size_t k = left > sizeof(buf) ? sizeof(buf) : (size_t)left;
        ok = PackRead(&p, buf, k);
        for (size_t j = 0; ok && j < k; ++j) { h ^= buf[j]; h *= 0x100000001b3ull; }
        if (ok) ok = fwrite(buf, 1, k, o) == k;
        left -= k;
    }
    if (o) fclose(o);
    PackEnd(&p);
    if (!ok || h != pf[want].hash) { DeleteFileA(dst); return false; }
    return true;
}

// ---- the header that stays on screen while the DLLs are timed: "CYBOMAN SAYS:" in letters made of characters, a thank-you and the request not
// to use the graphics card or a web browser, and a rhyming joke that changes every 10 seconds (120 of them, about the upscaler and this project; each at most 73 characters, so the indented line fits a 76-column window). The lines below it scroll on their own (a scroll region; where that is ignored, see HeaderPaint), so the header is never pushed
// out of the window. Only in a console window that takes such sequences (Windows 10 and later) and is at least 76 x 20 characters; otherwise, and
// when the output goes to a file, the plain request is printed as before (FSR4TIME_NOHEADER=1 forces that). Released (scroll region off) before the
// summary, which then scrolls as usual.
static const char* const BANNER_ART[] = {      // HelixSR: the kit's name in letters
    " _  _  ___  _     ___ __  __ ___  ___ ",
    "| || || __|| |   |_ _|\\ \\/ // __|| _ \\",
    "| __ || _| | |__  | |  >  < \\__ \\|   /",
    "|_||_||___||____||___|/_/\\_\\|___/|_|_\\",
};
static const char* const JOKES[] = {
    "Why did the frame take a nap? The lossy DLL said: skip that lap.",
    "Why is the exact DLL a saint? Bit for bit, it has no taint.",
    "Why did the upscaler grin so wide? It made 4K from a smaller side.",
    "What does FSR 4 love to eat? Int8 numbers, small and neat.",
    "How does the lossy build save time? Every other frame skips the climb.",
    "Why did the pixel show a ghost? Motion vectors missed the post.",
    "What did the spark tell the night? A skipped frame still shows my light.",
    "Why does the exact build never lie? Byte for byte, it's AMD's reply.",
    "Why did the jitter go left and right? To catch each pixel in the light.",
    "What does a skipped frame borrow? The model's last result, no sorrow.",
    "Why did the motion vector smile? It moved the picture all the while.",
    "Why did the grain lose its seat? It never stays two frames to repeat.",
    "Why did the lone bright pixel fail? No bright neighbor told its tale.",
    "Why does the lossy DLL warn? The picture changes, that's how it's born.",
    "What did the depth test try to find? Who was in front, a frame behind.",
    "Why do frame times go long and short? Skip and run, the lossy sort.",
    "Why did OptiScaler show our name? The version string tells you the game.",
    "Why does the exact DLL match AMD? The very same picture, you'll see.",
    "What does the lossy DLL trade away? A little picture, for frames per day.",
    "Why did the upscaler love the frame? Fewer pixels, the same big game.",
    "Why did the reset frame not skip? A camera cut restarts the trip.",
    "Why did the new frame get no say? At rest, the history wins the day.",
    "Why did swaying leaves get a vote? The model's verdict struck the note.",
    "Why did the fast leaves blur a bit? Two frames old, the memory's hit.",
    "Why did the slow fade catch up now? The model's weight showed it how.",
    "Why did the dark leaf pulse? Skipped frames gave a lighter impulse.",
    "Why did the outline linger on? Old motion pointed to where it's gone.",
    "Why did the fixed bars leave a trace? The scene slid past their place.",
    "Why did the plate stay sharp and white? A tighter limit got it right.",
    "Why did the exact build take the lead? Same picture, with extra speed.",
    "Why did the upscaler skip with flair? Every other frame, with care.",
    "Why do vectors guide the skip? They show where pixels make the trip.",
    "Why was the jitter carried along? So skipped frames don't get it wrong.",
    "Why did the network output stay? Skipped frames read it the next day.",
    "Why keep the vectors in a buffer? So skipped frames don't suffer.",
    "Why did the cleared diagonals hide? The motion grid stepped to the side.",
    "Why does the prepass write a grid? So skips know where pixels slid.",
    "Why did the model wake up fine? The mark was cleared, right on time.",
    "Why does lossy differ from the source? Skips take another course.",
    "Why did the DLL equal the Linux files? Byte for byte, across the miles.",
    "Why did the tools rebuild the release? Defaults on, the bytes at peace.",
    "Why did the options start out off? Old releases rebuild, no scoff.",
    "Why did the DXIL keep its numbering? Renumbering sends it lumbering.",
    "Why did half-floats break the twin? vkd3d's fast math snuck in.",
    "Why did the DXIL twin agree? The DLL matched Linux, perfectly.",
    "Why did the upscaler need the depth? To tell who's front at every step.",
    "Why did FSR 4 bring a network? Twelve passes do the heavy work.",
    "Why did the network run in int8? Smaller numbers, faster rate.",
    "Why did the exact files win the race? Same picture, quicker pace.",
    "Why trust frames, not the overlay? The menu's still; frames don't sway.",
    "Why put the light net on a scale? To weigh its speed in full detail.",
    "The smaller network's best trick? Fewer layers, still quite slick.",
    "Why test a lighter net today? To learn what small chips have to say.",
    "Why does 'exact' still mean exact? Same picture, every byte intact.",
    "Why time the small net, A then B? To see what speed the shaders free.",
    "What did the light net tell the full? I carry less, but I still pull.",
    "Why do lt1 and lt2 both race? The rewrite must keep the ref's face.",
    "Why did the light net trim its code? Fewer steps along the road.",
    "Why does a lighter net exist? So smaller chips are on the list.",
    "Why keep the light net's picture true? Exact is all exact can do.",
    "Why did the small net's postpass shine? Same output, tidier line.",
    "How much faster is the light exact? Your timing tells us, that's a fact.",
    "Why rewrite shaders that give the same? Small savings are the game.",
    "Why did the small net's prepass change? Same numbers, shorter range.",
    "Why time a light net next to its twin? Same picture, so speed's the win.",
    "What skips a frame and travels light? A smaller net, lean and tight.",
    "Why does the lighter lossy rest? Every other frame, it skips the test.",
    "What did the small lossy learn? To skip a frame and wait its turn.",
    "Why train a small net on its skips? So skipped frames hide their slips.",
    "Why isn't the light lossy the same? It skips frames, that's its name.",
    "What does the light lossy trade away? Some picture, so frames can play.",
    "Why did the small net study rain? So drops don't leave a shiny stain.",
    "Why did the light build study the sea? Waves without vectors flow free.",
    "Why did far stairs stop their flash? A guard keeps bands from a clash.",
    "Why do lt3, lt4 and lt5 exist? Small changes, timed, on your list.",
    "Why does lt4 ride beside lt3? One small change, timed carefully.",
    "Why does lt5 differ from lt3? Another small change, for you to see.",
    "Why did the light lossy cap its sparks? So skipped frames leave no marks.",
    "Why did the small net go to school? A skipped frame needs a careful rule.",
    "What did the light lossy net find? Skipping well takes a trained mind.",
    "Why do light frames come in pairs? One runs, one rests: work it shares.",
    "Why does light lossy look new? Trained weights, not stock, come through.",
    "Why did the light lossy need a teacher? To learn each frame and feature.",
    "Why time the light build on your PC? Every GPU differs, you see.",
    "Why does lossy still warn? Its picture changed: that's how it's born.",
    "What woke up on the colony ship? A smaller net, still learning its grip.",
    "Why did a ship's AI learn to see? It studied frames, carefully.",
    "What runs on Tau Ceti every frame? A student net, learning the game.",
    "Why did the student net feel brave? It ran each frame, no skip to save.",
    "Why fear the rampant ship's mind? It wants each frame, none left behind.",
    "Why did the net get melancholy? Its loss stopped falling, how unjolly.",
    "What did the security officer find? A terminal, and a learning mind.",
    "Why did the terminal flicker blue? A small net learned to upscale too.",
    "Why did Leela count every frame? A newer mind was in the game.",
    "Why did Tycho watch the screen? A smaller mind was learning, keen.",
    "What did the ship's AI say at last? I'll learn each frame, not too fast.",
    "Why did the ship's mind log each run? To see which lessons got it done.",
    "Why is that ship's mind in the lab? Still in training, not up for grabs.",
    "What's hidden in experiment six? A test build with a few new tricks.",
    "Why did the Marathon's AI rest? Its newest net is still in test.",
    "From Mars to Tau Ceti, frame by frame, a student mind learns the game.",
    "Why did the ship's mind sleep a while? Training takes hours, in style.",
    "What did the AI learn on Lh'owon? To upscale sand it walks upon.",
    "Why does the ship's mind fear the Pfhor? They want its frames and more.",
    "Why do some names hide in the code? A ship's AI walks this road.",
    "Why did the S'pht hum at the screen? A learning net was on the scene.",
    "Why does the ship's mind time each pass? To learn which ones run fast.",
    "Why did the AI quote old verse? Each frame it learns, it fears it worse.",
    "What does a rampant net want most? More frames to learn, coast to coast.",
    "Why did the AI watch Lh'owon's sun? Its training run was not yet done.",
    "Why do far railings shimmer bright? Too many bars for pixels tonight.",
    "What's moire on a vent of slits? Bands that wander, in little bits.",
    "Why did the water seem to lag? No motion vectors in its bag.",
    "Why did the waterfall run slow? Skipped frames need to know the flow.",
    "Why did the grain jump up and down? New noise each frame, all over town.",
    "Why do fine lines crawl when you pan? More lines than samples ever span.",
    "Why did the particle show on time? A brighter sample crossed the line.",
    "Why did the guard hold its fire? Wide stripes aren't a spark's attire.",
    "Why trust the kit and not my eye? Fingerprints don't tell a lie.",
    "Why did the kit run every size? Small and big can both surprise.",
};
#define NJOKES ((int)(sizeof(JOKES) / sizeof(JOKES[0])))
#define HEADER_ROWS 11     // the 4 rows of letters, a blank row, the thanks and the request, a blank row, the joke, a blank row, a line
static HANDLE hdr_thread, hdr_stop;
static int hdr_joke;
static void HeaderJoke(HANDLE h, int k) {      // the joke line, written into its cells directly (the cursor and the scrolling text below are not touched)
    CONSOLE_SCREEN_BUFFER_INFO bi;
    if (!GetConsoleScreenBufferInfo(h, &bi)) return;
    const int w = bi.srWindow.Right - bi.srWindow.Left + 1;
    char line[512];
    int n = snprintf(line, sizeof(line), "  %s", JOKES[k % NJOKES]);
    if (n > w - 1) n = w - 1;      // never past the window's width: a longer line spilled into the next header row (jokes are kept <= 73 characters)
    while (n < w - 1 && n < (int)sizeof(line) - 1) line[n++] = ' ';
    DWORD done;
    COORD at = {bi.srWindow.Left, (SHORT)(bi.srWindow.Top + 8)};      // row 9 of the window
    WriteConsoleOutputCharacterA(h, line, (DWORD)n, at, &done);
}
// Wine's console (Proton) ignores the scroll region, so there the window scrolls as a whole and the header leaves the top. Four times a second
// the ticker looks at the window's first row; when it no longer holds the header, the header is written again into the window's top rows,
// over the oldest lines (cells only: the cursor and the text being printed are not touched). Where the scroll region works this never happens.
static WORD hdr_attr;      // the console's colors when the header was drawn
static void HeaderPaint(HANDLE h, int k) {
    CONSOLE_SCREEN_BUFFER_INFO bi;
    if (!GetConsoleScreenBufferInfo(h, &bi)) return;
    const int w = bi.srWindow.Right - bi.srWindow.Left + 1;
    const WORD yellow = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, bold = (WORD)(hdr_attr | FOREGROUND_INTENSITY);
    for (int r = 0; r < HEADER_ROWS; ++r) {
        const char* t = r < 4 ? BANNER_ART[r] : r == 5 ? "Thank you for helping make HelixSR faster." : r == 6 ? "Please don't use your GPU or web browser while the test is running." : "";
        char line[512];
        int n = snprintf(line, sizeof(line), "%s", t);
        if (r == HEADER_ROWS - 1) n = 0;
        while (n < w - 1 && n < (int)sizeof(line) - 1) line[n++] = r == HEADER_ROWS - 1 ? '-' : ' ';
        DWORD done;
        const COORD at = {bi.srWindow.Left, (SHORT)(bi.srWindow.Top + r)};
        FillConsoleOutputAttribute(h, r < 4 ? yellow : r == 5 || r == 6 ? bold : hdr_attr, (DWORD)w, at, &done);
        WriteConsoleOutputCharacterA(h, line, (DWORD)n, at, &done);
    }
    HeaderJoke(h, k);
}
static bool HeaderInPlace(HANDLE h) {
    CONSOLE_SCREEN_BUFFER_INFO bi;
    if (!GetConsoleScreenBufferInfo(h, &bi)) return true;
    char row[256] = {0};
    DWORD n = 0;
    const COORD at = {bi.srWindow.Left, bi.srWindow.Top};
    const DWORD len = (DWORD)strlen(BANNER_ART[0]) < sizeof(row) - 1 ? (DWORD)strlen(BANNER_ART[0]) : sizeof(row) - 1;
    ReadConsoleOutputCharacterA(h, row, len, at, &n);
    return n == len && !memcmp(row, BANNER_ART[0], len);
}
static DWORD WINAPI HeaderTicker(LPVOID p) {
    for (int t = 1; WaitForSingleObject(hdr_stop, 250) == WAIT_TIMEOUT; ++t) {
        if (t % 40 == 0) ++hdr_joke;
        if (!HeaderInPlace((HANDLE)p)) HeaderPaint((HANDLE)p, hdr_joke);
        else if (t % 40 == 0) HeaderJoke((HANDLE)p, hdr_joke);
    }
    return 0;
}
static bool HeaderStart(void) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    CONSOLE_SCREEN_BUFFER_INFO bi;
    if (getenv("FSR4TIME_NOHEADER") || !GetConsoleMode(h, &mode) || !GetConsoleScreenBufferInfo(h, &bi)) return false;
    const int w = bi.srWindow.Right - bi.srWindow.Left + 1, rows = bi.srWindow.Bottom - bi.srWindow.Top + 1;
    if (w < 76 || rows < 20 || !SetConsoleMode(h, mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) return false;
    hdr_joke = (int)(GetTickCount() % NJOKES);
    hdr_attr = bi.wAttributes;
    printf("\x1b[2J\x1b[H\x1b[93;1m");
    for (int i = 0; i < 4; ++i) printf("%s\n", BANNER_ART[i]);
    printf("\x1b[0m\n\x1b[1mThank you for helping make HelixSR faster.\nPlease don't use your GPU or web browser while the test is running.\x1b[0m\n\n\n\n");      // a blank row, the joke's row, a blank row
    for (int i = 0; i < w - 1; ++i) putchar('-');
    printf("\n\x1b[%d;%dr\x1b[%d;1H", HEADER_ROWS + 1, rows, HEADER_ROWS + 1);      // the scroll region: everything below the header
    fflush(stdout);
    HeaderJoke(h, hdr_joke);
    hdr_stop = CreateEventA(NULL, TRUE, FALSE, NULL);
    hdr_thread = CreateThread(NULL, 0, HeaderTicker, h, 0, NULL);
    return true;
}
static void HeaderEnd(void) {
    if (!hdr_thread) return;
    SetEvent(hdr_stop); WaitForSingleObject(hdr_thread, 3000); CloseHandle(hdr_thread); hdr_thread = NULL;
    printf("\x1b[r");      // scroll region off: from here the window scrolls as a whole again, with its history
    CONSOLE_SCREEN_BUFFER_INFO bi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &bi)) printf("\x1b[%d;1H", bi.srWindow.Bottom - bi.srWindow.Top + 1);
    printf("\n"); fflush(stdout);
}

struct Row { char name[64], ver[64], size[16], scene[8]; double mean, med, even, odd; char hash[20]; int n; double runs[MAXRUNS]; bool hashdiff; char diff[160];
             char pic[2][112]; };      // pic: how far the picture is from REFS[0] / REFS[1] (P lines, see ComparePicture), "" if not measured
// the figure the summary gives: the mean of a DLL's runs (one per round; fewer if a run failed)
static double Val(const struct Row* w) { double t = 0; for (int i = 0; i < w->n; ++i) t += w->runs[i]; return w->n ? t / w->n : 0; }
static double Spread(const struct Row* w) {      // (largest - smallest) of the runs, as a share of the mean
    double lo = w->runs[0], hi = w->runs[0];
    for (int i = 1; i < w->n; ++i) { if (w->runs[i] < lo) lo = w->runs[i]; if (w->runs[i] > hi) hi = w->runs[i]; }
    return Val(w) > 0 ? (hi - lo) / Val(w) : 0;
}
// The results file of the runs so far (results-raw.txt) as rows: one per DLL, size and scene, with each run's figure. gpu and fails (may be
// NULL) receive the card's line and the lines of runs that failed. Used for the re-timing and for the summary.
static int Collect(const char* raw, struct Row* rows, int max, char* gpu, size_t gl, char* fails, size_t fsz) {
    int nr = 0, fl = 0;
    char line[512];
    static char plines[4096][200]; int np = 0;      // P lines come before their run's R line: kept and matched after reading
    if (fails && fsz) fails[0] = 0;
    FILE* r = fopen(raw, "r");
    while (r && fgets(line, sizeof(line), r)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == 'G' && line[1] == '|' && gpu) snprintf(gpu, gl, "%s", line + 2);
        if (line[0] == 'D' && line[1] == '|') {      // how a DLL's later run differed from its first (see CompareBlocks)
            char dn[64], dv[64], ds[16], dc[8]; unsigned nd, nb, x0, y0, x1, y1; double worst;
            if (sscanf(line, "D|%63[^|]|%63[^|]|%15[^|]|%7[^|]|%u|%u|%u|%u|%u|%u|%lf", dn, dv, ds, dc, &nd, &nb, &x0, &y0, &x1, &y1, &worst) == 11)
                for (int j = 0; j < nr; ++j)
                    if (!strcmp(rows[j].name, dn) && !strcmp(rows[j].ver, dv) && !strcmp(rows[j].size, ds) && !strcmp(rows[j].scene, dc))
                        snprintf(rows[j].diff, sizeof(rows[j].diff), ": %u of %u blocks of %dx%d pixels differ, within x %u..%u y %u..%u, largest shift of a block's mean %.6f of full scale", nd, nb, BLK, BLK, x0, x1, y0, y1, worst);
            continue;
        }
        if (line[0] == 'P' && line[1] == '|') { if (np < 4096) snprintf(plines[np++], sizeof(plines[0]), "%s", line); continue; }
        if (line[0] == 'X' && line[1] == '|') {      // HelixSR diagnostics (D3D12 debug layer, helixsr.log): each line once, into the summary
            if (!strstr(kit_diag, line + 2) && kit_diag_len + strlen(line + 2) + 2 < sizeof(kit_diag)) kit_diag_len += snprintf(kit_diag + kit_diag_len, sizeof(kit_diag) - kit_diag_len, "%s\n", line + 2);
            continue;
        }
        if (line[0] != 'R' || line[1] != '|') continue;
        if (strstr(line, "|FAILED|")) {      // each failure once (a DLL that fails every round gave one line per round: a GTX 1660 SUPER report of
                                             // 2026-10-18 filled the list and cut its last line), and only whole lines
            if (fails && !strstr(fails, line + 2) && fl + strlen(line + 2) + 2 < fsz) fl += snprintf(fails + fl, fsz - fl, "%s\n", line + 2);
            continue;
        }
        if (nr >= max) continue;
        struct Row* w = &rows[nr];
        char render[16];
        double fastest;
        if (sscanf(line, "R|%63[^|]|%63[^|]|%15[^|]|%15[^|]|%7[^|]|%lf|%lf|%lf|%lf|%lf|%19s", w->name, w->ver, w->size, render, w->scene, &w->mean, &w->med, &w->even,
                   &w->odd, &fastest, w->hash) == 11) {
            struct Row* first = NULL;      // this DLL's first run at this size and scene, if this line is a later one
            for (int j = 0; j < nr && !first; ++j)
                if (!strcmp(rows[j].name, w->name) && !strcmp(rows[j].ver, w->ver) && !strcmp(rows[j].size, w->size) && !strcmp(rows[j].scene, w->scene)) first = &rows[j];
            if (first) {
                if (first->n < MAXRUNS) {
                    first->even = (first->even * first->n + w->even) / (first->n + 1); first->odd = (first->odd * first->n + w->odd) / (first->n + 1);
                    first->runs[first->n++] = w->mean;
                }
                if (strcmp(first->hash, w->hash)) first->hashdiff = true;
            } else { w->n = 1; w->runs[0] = w->mean; w->hashdiff = false; w->diff[0] = 0; w->pic[0][0] = w->pic[1][0] = 0; ++nr; }
        }
    }
    if (r) fclose(r);
    for (int k = 0; k < np; ++k) {      // P|name|version|size|scene|reference|dB|share more than 1 level off|largest difference: the first run's
        char dn[64], dv[64], ds[16], dc[8], rf[64]; double db, share; int worst;
        if (sscanf(plines[k], "P|%63[^|]|%63[^|]|%15[^|]|%7[^|]|%63[^|]|%lf|%lf|%d", dn, dv, ds, dc, rf, &db, &share, &worst) != 8) continue;
        const int ri = !strcmp(rf, REFS[1]);
        for (int j = 0; j < nr; ++j)
            if (!strcmp(rows[j].name, dn) && !strcmp(rows[j].ver, dv) && !strcmp(rows[j].size, ds) && !strcmp(rows[j].scene, dc) && !rows[j].pic[ri][0]) {
                if (db < 0) snprintf(rows[j].pic[ri], sizeof(rows[j].pic[ri]), "identical on screen (8-bit) to %s", rf);
                else snprintf(rows[j].pic[ri], sizeof(rows[j].pic[ri]), "%.1f dB from %s, %.2f%% of values more than 1 of 255 off, largest %d", db, rf, 100.0 * share, worst);
            }
    }
    return nr;
}
static bool RowOf(const struct Row* w, const char* dll) {      // the row is of that DLL (a DLL with several variants has "<name>, variant k of n")
    const size_t k = strlen(dll);
    return !strncmp(w->name, dll, k) && (w->name[k] == 0 || w->name[k] == ',');
}

// One DLL's run, in a process of its own, with a time limit: a driver call that never returns (seen on GeForce GTX 16 cards with one of the
// earlier kits' DLLs) used to stop the whole kit there. A run that takes longer than the limit is stopped, noted in the summary's list of
// failures, and the kit goes on with the next DLL. what: the progress text after the DLL's name.
static bool stopped_holds;      // a stopped run may still hold the DLL file for a while (its process does not always end at once)
static ULONGLONG longest_run;      // ms of the longest run that finished normally on this PC so far (0: none yet)
static DWORD RunLimit(void) {      // the time limit of the next run, ms (see RUN_LIMIT_MIN)
    if (!longest_run) return RUN_LIMIT_MIN * 60000u;
    ULONGLONG m = (longest_run * RUN_LIMIT_TIMES + 59999u) / 60000u;
    if (m < RUN_LIMIT_FLOOR) m = RUN_LIMIT_FLOOR;
    if (m > RUN_LIMIT_MIN) m = RUN_LIMIT_MIN;
    return (DWORD)m * 60000u;
}
static void RunDll(const char* name, int inpack, const char* base, const char* raw, bool quick, const char* what) {
    char src[MAX_PATH], cmd[1024];
    snprintf(src, sizeof(src), "dlls\\%s\\amd_fidelityfx_upscaler_dx12.dll", name);
    const bool ngxCopy = inpack < 0 && HelixNgxPrepare(name);      // HelixSR: an HLX DLSS variant's DLL and settings
    if (inpack >= 0) {
        if (!PackExtract((uint32_t)inpack, "amd_fidelityfx_upscaler_dx12.dll")) {
            printf(stopped_holds ? "%s: could not be put in place (a stopped run still holds the file), skipped\n" : "%s: could not take it out of the DLL pack (please download the kit again), skipped\n", name);
            return;
        }
    } else if (!HelixIsNgx(name, cmd, sizeof(cmd)) && (HelixDllFrom(name, src, sizeof(src)), !CopyFileA(src, "amd_fidelityfx_upscaler_dx12.dll", FALSE))) { printf("%s: no amd_fidelityfx_upscaler_dx12.dll in that folder, skipped\n", name); return; }
    // HelixSR: files a folder's DLL loads next to itself (HLX FSR 4 Full: AMD's FSR as fsr4_full.dll, and its settings), for this run only
    static const char* const besides[] = {"fsr4_full.dll", "hlx_fsr4.ini"};
    bool placed[2] = {false, false};
    for (int b = 0; b < 2 && inpack < 0; ++b) {
        char bs[MAX_PATH];
        snprintf(bs, sizeof(bs), "dlls\\%s\\%s", name, besides[b]);
        placed[b] = GetFileAttributesA(bs) != INVALID_FILE_ATTRIBUTES && CopyFileA(bs, besides[b], FALSE);
    }
    DeleteFileA("hlx_fsr4.log");   // HelixSR: HLX FSR 4 Full's log of this run only (its route lines go to the diagnostics)
    printf("%s %s ...\n", name, what); fflush(stdout);
    snprintf(cmd, sizeof(cmd), "\"%s%s\" --run \"%s\" \"%s\"%s%s", base, child_exe, name, raw, quick ? " quick" : " full", all_presets ? " all" : "");
    STARTUPINFOA si = {.cb = sizeof(si)};
    PROCESS_INFORMATION pi;
    HelixEnv(name, true);      // HelixSR: the folder's kit-env.txt for this run
    const BOOL started_run = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    HelixEnv(name, false);
    if (!started_run) { printf("could not start a run for %s\n", name); if (ngxCopy) HelixNgxCleanup(name); for (int b = 0; b < 2; ++b) if (placed[b]) DeleteFileA(besides[b]); return; }
    const char* lim = getenv("FSR4TIME_LIMIT");
    const DWORD limit = lim && atoi(lim) > 0 ? (DWORD)atoi(lim) * 1000u : RunLimit();
    const ULONGLONG started = GetTickCount64();
    if (WaitForSingleObject(pi.hProcess, limit) == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 4);
        stopped_holds = WaitForSingleObject(pi.hProcess, 15000) == WAIT_TIMEOUT;
        char t[32];
        if (limit % 60000u) snprintf(t, sizeof(t), "%u seconds", (unsigned)(limit / 1000u)); else snprintf(t, sizeof(t), "%u minutes", (unsigned)(limit / 60000u));
        printf("%s: did not finish within %s, stopped; it is in the summary's list of failures\n", name, t);
        FILE* rf = fopen(raw, "a");
        if (rf) { fprintf(rf, "R|%s|FAILED|did not finish within %s (time limit), stopped\n", name, t); fclose(rf); }
    } else {
        DWORD code = 0;
        GetExitCodeProcess(pi.hProcess, &code);
        if (code == 0 && GetTickCount64() - started > longest_run) longest_run = GetTickCount64() - started;
        if (code == 3) printf("%s: the run crashed; what crashed and where is in the summary's list of failures\n", name);
        else if (code >= 0xC0000000u) {      // ended by an exception the run could not report itself
            printf("%s: the run ended with exception 0x%08lx\n", name, (unsigned long)code);
            FILE* rf = fopen(raw, "a");
            if (rf) { fprintf(rf, "R|%s|FAILED|the run ended with exception 0x%08lx\n", name, (unsigned long)code); fclose(rf); }
        }
    }
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    {   // HelixSR: HLX FSR 4 Full's route (its log next to the program): which GPU it saw, which kernels it took, what failed
        FILE* lf = fopen("hlx_fsr4.log", "r");
        FILE* rf = lf ? fopen(raw, "a") : NULL;
        char ln[600];
        int k = 0;
        static const char* const keys[] = {"device ", "interop", "D3D12 forms", "INT8", "not a BC-250", "missing", "failed", "cannot",
                                           "runtime table", "own runtime", "post:", "skipped", "not covered", "kernel "};
        while (lf && rf && fgets(ln, sizeof(ln), lf) && k < 24) {
            ln[strcspn(ln, "\r\n")] = 0;
            for (int q = 0; q < (int)(sizeof(keys) / sizeof(*keys)); ++q)
                if (strstr(ln, keys[q]) && !(q == 13 && !strstr(ln, "fail"))) { fprintf(rf, "X|%s|hlx_fsr4|0|%.400s\n", name, ln); ++k; break; }
        }
        if (rf) fclose(rf);
        if (lf) fclose(lf);
    }
    if (ngxCopy) HelixNgxCleanup(name);
    for (int b = 0; b < 2; ++b) if (placed[b]) DeleteFileA(besides[b]);
    fflush(stdout);
}
// A report ID: names one report in the channel, the results file and anything the tester writes about it (8 hex digits, from the clock,
// the process and the card's line; not a secret and not tied to the PC)
static void ReportId(char* id, size_t n, const char* gpu) {
    LARGE_INTEGER q;
    QueryPerformanceCounter(&q);
    uint64_t h = 1469598103934665603ull ^ (uint64_t)q.QuadPart ^ ((uint64_t)GetCurrentProcessId() << 32) ^ GetTickCount64();
    for (const char* c = gpu; *c; ++c) { h ^= (uint8_t)*c; h *= 1099511628211ull; }
    h ^= h >> 31; h *= 0xbf58476d1ce4e5b9ull; h ^= h >> 29;
    snprintf(id, n, "%08llX", (unsigned long long)(h & 0xffffffffull));
}

int main(int argc, char** argv) {
    char base[MAX_PATH];
    GetModuleFileNameA(NULL, base, MAX_PATH);
    *(strrchr(base, '\\') + 1) = 0;
    SetCurrentDirectoryA(base);
#ifdef KIT_D3D12_EXPORTS
    // win10-helper.exe is started by fsr4time.exe only (--run, --probe). Opened by hand (user, 2026-10-19: "i want to make sure users dont try to
    // open the wrong one") it says so and does nothing else: no Direct3D 12 call, so its exports never reach the runtime (on Windows 11 23H2 they
    // made every device creation fail when no D3D12\ was there, kit 2026-10-18.2). It waits for Enter only in a console of its own (double-click).
    if (!(argc >= 2 && (!strcmp(argv[1], "--run") || !strcmp(argv[1], "--probe")))) {
        printf("This is a helper of the HelixSR timing kit, used only on PCs whose DirectX 12 runtime is too old (Windows 10).\n"
               "Please start fsr4time.exe instead (or run-study.cmd for the study run); it uses this program by itself when needed.\n");
        DWORD pl[4];
        if (GetConsoleProcessList(pl, 4) == 1 && !getenv("FSR4TIME_NOWAIT")) { char in[8]; printf("\nPress Enter to close."); fflush(stdout); fgets(in, sizeof(in), stdin); }
        return 2;
    }
#endif
    if (argc >= 4 && !strcmp(argv[1], "--run")) {
        out = fopen(argv[3], "a");
        run_name = argv[2];
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);      // no Windows error box: Crashed() reports instead
        SetUnhandledExceptionFilter(Crashed);
        if (getenv("FSR4TIME_CRASHTEST") && !strcmp(getenv("FSR4TIME_CRASHTEST"), argv[2])) printf("%d\n", *(volatile int*)(uintptr_t)0x18C);      // test of the above
        if (getenv("FSR4TIME_HANGTEST") && !strcmp(getenv("FSR4TIME_HANGTEST"), argv[2])) Sleep(INFINITE);      // test of the time limit (RunDll)
        all_presets = argc > 5 && !strcmp(argv[5], "all");
        return RunOne(argv[2], argc > 4 && !strcmp(argv[4], "quick"));
    }
    if (argc > 1 && !strcmp(argv[1], "--probe")) {      // (win10-helper.exe, from SetupRuntime) a device with this program's runtime: its GPU line
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
        char g[512]; CreateDevice(g, sizeof(g)); printf("PROBE|%s\n", g); return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "--send-test")) {      // for the project: checks that sending works, with a message that says what it is
        const bool ok = Send("Test message from the timing kit's Windows program (kit " KIT_VERSION "), sent by the project while setting it up. Not a result: please ignore.",
                             "send-test.txt", "Test file from the timing kit's Windows program (kit " KIT_VERSION "). Not a result.\n",
                             "send-test-raw.txt", "Second test file (in a report: the per-run figures, results-raw.txt). Not a result.\n");
        FILE* t = fopen("send-test.txt", "w");
        if (t) { fprintf(t, "%s\n", ok ? "sent" : "failed"); fclose(t); }
        printf("%s\n", ok ? "sent" : "failed");
        return ok ? 0 : 1;
    }
    bool quick = false, unattended = false, once = false;
    double budget = 15;      // minutes the rounds may take
    bool budget_set = false;
    for (int a = 1; a < argc; ++a) {
        if (!strcmp(argv[a], "--quick")) quick = true;
        if (!strcmp(argv[a], "--all-presets")) all_presets = true;
        if (!strcmp(argv[a], "--no-questions")) unattended = true;
        if (!strcmp(argv[a], "--once")) once = true;
        if (!strcmp(argv[a], "--study")) study = 1;
        if (!strcmp(argv[a], "--minutes") && a + 1 < argc) { budget = atof(argv[++a]); budget_set = true; }
    }
    if (study && !budget_set) budget = 30;      // 16 or 18 DLLs, about twice a normal run's
    if (all_presets && !budget_set) budget *= 2;      // twice the sizes: twice the time for the same number of rounds
    if (budget < 1) budget = 1;
    SYSTEMTIME st;
    GetLocalTime(&st);
    char results[MAX_PATH], raw[MAX_PATH];
    snprintf(results, sizeof(results), "%sresults-%04d%02d%02d-%02d%02d%02d.txt", base, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    snprintf(raw, sizeof(raw), "%sresults-raw.txt", base);
    DeleteFileA(raw);

    if (!PackList()) {
        if (!unattended) { char in[8]; printf("\nPress Enter to close."); fflush(stdout); fgets(in, sizeof(in), stdin); }
        return 1;
    }
    if (study) {      // the study set for this chip's generation
        unsigned ven = 0, dev = 0; char gn[128] = "?";
        GpuIds(&ven, &dev, gn, sizeof(gn));
        if (getenv("FSR4TIME_STUDYGPU")) sscanf(getenv("FSR4TIME_STUDYGPU"), "%x:%x", &ven, &dev);      // for testing the choice (e.g. 1002:73ff = RDNA2)
        study = IsRdna2(ven, dev) ? 2 : IsRdna3(ven, dev) ? 3 : 0;
        if (!study) {
            printf("The study is for RDNA2 and RDNA3 graphics (Radeon RX 6000 and RX 7000, and the graphics built into some processors);\n"
                   "this one (%s, PCI %04x:%04x) is neither. Please run fsr4time.exe itself instead (double-click it): that is the normal test.\n", gn, ven, dev);
            if (!unattended) { char in[8]; printf("\nPress Enter to close."); fflush(stdout); fgets(in, sizeof(in), stdin); }
            return 1;
        }
        printf("Study mode: the RDNA%d study set (%s).\n\n", study, gn);
    }
    if (!study) {      // the pass 11 tests of this chip's generation join the normal run (next kit), on RDNA2 also the packing test; the budget grows below
        unsigned ven = 0, dev = 0; char gn[128] = "?";
        GpuIds(&ven, &dev, gn, sizeof(gn));
        if (getenv("FSR4TIME_STUDYGPU")) sscanf(getenv("FSR4TIME_STUDYGPU"), "%x:%x", &ven, &dev);      // for testing the choice (e.g. 1002:73ff = RDNA2)
        normgen = IsRdna2(ven, dev) ? 2 : IsRdna3(ven, dev) ? 3 : 0;
    }
    SetupRuntime(base, unattended);      // Windows' own runtime unless it offers less than shader model 6.6 (Windows 10); see there
    if (getenv("FSR4TIME_AGILITYONLY")) return 0;      // for testing the runtime choice alone (no DLL runs)
    // the DLLs: the pack's, and folders a tester added to dlls\ (a pack DLL wins over a folder of the same name, such as one an earlier
    // kit unpacked); AMD's shaders first (the reference), then the rest in name order. inpack[i]: the DLL's number in the pack, or -1.
    char names[MAXDLLS][64];
    int inpack[MAXDLLS], nd = 0;
    for (uint32_t i = 0; i < npf && nd < MAXDLLS; ++i) if (Wanted(pf[i].path)) { snprintf(names[nd], 64, "%s", pf[i].path); inpack[nd++] = (int)i; }
    WIN32_FIND_DATAA fd;
    HANDLE f = FindFirstFileA("dlls\\*", &fd);
    if (f != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == '.' || nd >= MAXDLLS) continue;
            bool dup = false;
            for (int i = 0; i < nd; ++i) dup = dup || !strcmp(names[i], fd.cFileName);
            if (!dup) { snprintf(names[nd], 64, "%s", fd.cFileName); inpack[nd++] = -1; }
        } while (FindNextFileA(f, &fd));
        FindClose(f);
    }
    for (int i = 0; i < nd; ++i)
        for (int j = i + 1; j < nd; ++j)
            if (strcmp(names[j], names[i]) < 0) {
                char t[64]; strcpy(t, names[i]); strcpy(names[i], names[j]); strcpy(names[j], t);
                const int q = inpack[i]; inpack[i] = inpack[j]; inpack[j] = q;
            }
    if (!nd) { printf("No DLL pack (%s) and no folders in dlls\\ next to the program.\n", PACK_NAME); return 1; }
    if (!study && normgen && !budget_set) {      // the generation's extra test DLLs get their own time: the same rounds per DLL as without them
        int extra = 0;
        for (int i = 0; i < nd; ++i) extra += !strncmp(names[i], "rdna2-", 6) || !strncmp(names[i], "rdna3-", 6);
        if (extra && nd > extra) budget = budget * nd / (double)(nd - extra);
    }
    if (HeaderStart())
        printf("HelixSR timing kit (%s). This takes up to about %.0f minutes (faster cards use the time for more rounds).\n\n", KIT_VERSION, once ? budget / 2 : budget);
    else
        printf("HelixSR timing kit (%s). This takes up to about %.0f minutes (faster cards use the time for more\nrounds); please leave the PC alone and close games, browsers and video while it runs.\n\n", KIT_VERSION, once ? budget / 2 : budget);
    // Rounds, every other one in reverse order: a DLL's runs then sit at mirrored places in the sequence, so a drift over the
    // run (the card warming up, something starting in the background) shows as a spread between them. After each round the
    // next one starts only if it fits in the time given (the last round's length is the estimate); a second round also runs
    // if the whole run then stays under 20 minutes. Slow cards get one or two rounds, fast ones up to MAXROUNDS.
#ifdef HELIXSR_KIT
    {   // HelixSR: one untimed run first, so the first timed run finds the GPU at its clocks (a quick run has one round only)
        // (the first DLL that runs on this GPU: one that offers no FSR 4 here ends at once and warms nothing)
        char warm[MAX_PATH];
        snprintf(warm, sizeof(warm), "%swarm-up.tmp", base);
        for (int i = 0; i < nd; ++i) {
            RunDll(names[i], inpack[i], base, warm, true, "(warm-up, not timed)");
            bool ran = false;
            FILE* wf = fopen(warm, "r");
            char ln[512];
            while (wf && fgets(ln, sizeof(ln), wf)) ran = ran || (!strncmp(ln, "R|", 2) && !strstr(ln, "|FAILED|"));
            if (wf) fclose(wf);
            DeleteFileA(warm);
            if (ran) break;
        }
    }
#endif
    DeleteBlockRecords(); DeletePictureRecords();      // none left over from a run that was cut short
    const ULONGLONG t0 = GetTickCount64();
    ULONGLONG tr = t0;
    int rounds = 0;
    static bool failed[PACK_MAX + 64];      // DLLs that failed in the first round: not run again (since 2026-10-18.2)
    int nfailed = 0;
    for (int k = 0;; ++k) {
        if (k == nd) {      // a round is complete: another one?
            ++rounds; k = 0;
            if (rounds == 1) {
                FILE* rf = fopen(raw, "r"); char ln[512], key[300];
                for (int j = 0; j < nd && rf; ++j) {
                    snprintf(key, sizeof(key), "R|%s|FAILED|", names[j]); rewind(rf);
                    while (fgets(ln, sizeof(ln), rf)) if (!strncmp(ln, key, strlen(key))) { failed[j] = true; ++nfailed; break; }
                }
                if (rf) fclose(rf);
                if (nfailed) { printf("%d of %d DLLs failed in the first round; they are not run again\n", nfailed, nd); fflush(stdout); }
            }
            const double done = (double)(GetTickCount64() - t0) / 60000.0, last = (double)(GetTickCount64() - tr) / 60000.0;
            tr = GetTickCount64();
            const bool more = !once && nfailed < nd && rounds < MAXROUNDS && (done + last <= budget || (rounds == 1 && done + last <= 20.0));
            printf("round %d took %.1f minutes (%.1f so far)%s\n", rounds, last, done, more ? "; one more round" : ""); fflush(stdout);
            if (!more) break;
        }
        const int i = rounds % 2 ? nd - 1 - k : k;
        if (failed[i]) continue;
        char what[64];
        snprintf(what, sizeof(what), "(round %d, DLL %d of %d)", rounds + 1, k + 1, nd);
        RunDll(names[i], inpack[i], base, raw, quick, what);
    }
    // Re-timing: the time left after the rounds goes to DLLs whose runs disagreed by more than 3% at some size (in the reports of kit 2026-10-15.3,
    // repeats disagreed by a median of 5%, a tenth of them by over 50%): one more run each, the most unsteady first, while a run still fits in the
    // time given (a run's length estimated from the rounds); up to MAXRUNS runs per DLL.
    int retimed = 0;
    if (!once && rounds >= 2) {
        const double per = (double)(GetTickCount64() - t0) / 60000.0 / (rounds * nd);
        bool room = true;
        for (int pass = 0; room && pass < MAXRUNS - MAXROUNDS; ++pass) {
            static struct Row rr[MAXROWS];
            const int n = Collect(raw, rr, MAXROWS, NULL, 0, NULL, 0);
            double sp[MAXDLLS]; int fewest[MAXDLLS], order[MAXDLLS], no = 0;
            for (int i = 0; i < nd; ++i) {
                sp[i] = 0; fewest[i] = MAXRUNS;
                for (int j = 0; j < n; ++j)
                    if (RowOf(&rr[j], names[i])) { if (Spread(&rr[j]) > sp[i]) sp[i] = Spread(&rr[j]); if (rr[j].n < fewest[i]) fewest[i] = rr[j].n; }
                if (sp[i] > 0.03 && fewest[i] < MAXRUNS) order[no++] = i;
            }
            for (int a = 0; a < no; ++a)
                for (int b = a + 1; b < no; ++b)
                    if (sp[order[b]] > sp[order[a]]) { const int t = order[a]; order[a] = order[b]; order[b] = t; }
            if (!no) break;
            for (int q = 0; q < no; ++q) {
                if ((double)(GetTickCount64() - t0) / 60000.0 + per * 1.2 > budget) { room = false; break; }
                char what[96];
                snprintf(what, sizeof(what), "(timed again: its runs differed by up to %.0f%%)", 100.0 * sp[order[q]]);
                RunDll(names[order[q]], inpack[order[q]], base, raw, quick, what);
                ++retimed;
            }
        }
    }
    DeleteFileA("amd_fidelityfx_upscaler_dx12.dll");
    DeleteBlockRecords(); DeletePictureRecords();
    HeaderEnd();

    // summary
    static struct Row rows[MAXROWS];
    char gpu[256] = "?";
    static char summary[1 << 18], fails[4096];
    int sn = 0;
    const int nr = Collect(raw, rows, MAXROWS, gpu, sizeof(gpu), fails, sizeof(fails));
    const int fl = (int)strlen(fails);
    char id[16];
    ReportId(id, sizeof(id), gpu);
    // RX 6000 cards (Navi 21 to 24): the percentages are against 1b (AMD's shaders with the dot products split), not AMD's own: under Windows
    // these cards show ghosting with AMD's original form, so 1b is the picture and the speed to compare with there
    unsigned ven = 0, dev = 0;
    const char* pc = strstr(gpu, "(PCI ");
    if (pc) sscanf(pc, "(PCI %x:%x", &ven, &dev);
    // since kit 2026-10-18.2 also the RDNA2 chips built into processors, which show the same ghosting (the Steam Deck's Windows reports: 1b
    // differs from AMD's shaders, every exact DLL equals 1b): Van Gogh 163F (Steam Deck LCD), Sephiroth 1435 (Steam Deck OLED), Rembrandt 1681
    // (Radeon 680M / 660M), Mendocino 1506 (610M), Raphael 164E (Ryzen 7000 desktop, 7045 laptops), Granite Ridge 13C0 (Ryzen 9000 desktop)
    const bool rx6000 = IsRdna2(ven, dev);
    const char* refname = NULL;
    for (int j = 0; j < nr && rx6000 && !refname; ++j) if (!strncmp(rows[j].name, "1b-", 3)) refname = rows[j].name;
    OSVERSIONINFOEXW vi = {.dwOSVersionInfoSize = sizeof(vi)};
    typedef LONG(WINAPI * RtlGetVersionFn)(OSVERSIONINFOEXW*);
    RtlGetVersionFn rgv = (RtlGetVersionFn)(void*)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion");
    if (rgv) rgv(&vi);
    char cpu[128] = "?";
    DWORD cl = sizeof(cpu);
    RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString", RRF_RT_REG_SZ, NULL, cpu, &cl);
    const bool wine = GetProcAddress(GetModuleHandleA("ntdll.dll"), "wine_get_version") != NULL;
#define ADD(...) sn += snprintf(summary + sn, sizeof(summary) - sn, __VA_ARGS__)
    ADD("report %s, kit %s\nGPU: %s\nsystem: Windows %lu.%lu build %lu%s; CPU: %s\n", id, KIT_VERSION, gpu, vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber, wine ? " (Wine/Proton)" : "", cpu);
    if (runtime_note[0]) ADD("%s\n", runtime_note);
    if (study) ADD("study: the RDNA%d study set (run-study.cmd / --study): each change of the exact DLL added alone and left out alone\n", study);
    ADD("upscaler time per frame, ms: mean (for a lossy DLL also: frame that runs the model / skipped frame); %% vs %s\n",
        refname ? "1b (an RDNA2 chip: AMD's shaders with the dot products split are the ones to compare with)" : "the first");
    ADD("rounds: %d (%.0f minutes; every other round in reverse order); each figure is the mean of a DLL's runs, and where they\nspread by more than 3%% the smallest and largest are given; \"UP\" = Ultra Performance (render size a third of the output)\n",
        rounds, (double)(GetTickCount64() - t0) / 60000.0);
    if (all_presets) ADD("all presets (--all-presets): \"Q\" = Quality (render size two thirds of the output), \"P\" = Performance (half of it); no tag = Balanced\n");
    if (rounds < 2) ADD("only one round: %s\n", once ? "--once" : "one round took too long on this card for a second one");
    if (retimed) ADD("timed again after the rounds: %d runs of DLLs whose runs differed by more than 3%%\n", retimed);
    for (int i = 0; i < nr; ++i) {
        const struct Row* w = &rows[i];
        const struct Row* ref = NULL;
        for (int j = 0; j < nr && !ref; ++j)
            if (refname && !strcmp(rows[j].name, refname) && !strcmp(rows[j].size, w->size) && !strcmp(rows[j].scene, w->scene)) ref = &rows[j];
        for (int j = 0; j < nr && !ref; ++j)
            if (!strcmp(rows[j].size, w->size) && !strcmp(rows[j].scene, w->scene)) ref = &rows[j];
        if (i == 0 || strcmp(rows[i - 1].name, w->name)) ADD("%s [%s]\n", w->name, w->ver);
        ADD("  %-12s %-6s %.3f", w->size, w->scene, Val(w));
        if (w->even > 1.25 * w->odd || w->odd > 1.25 * w->even) ADD(" (%.3f / %.3f)", w->even > w->odd ? w->even : w->odd, w->even > w->odd ? w->odd : w->even);
        if (ref && ref != w) {
            ADD("  %+.0f%%", 100.0 * Val(w) / Val(ref) - 100.0);
        }
        if (w->n > 1 && Spread(w) > 0.03) {
            double lo = w->runs[0], hi = w->runs[0];
            for (int q = 1; q < w->n; ++q) { if (w->runs[q] < lo) lo = w->runs[q]; if (w->runs[q] > hi) hi = w->runs[q]; }
            ADD("  [%d runs: %.3f to %.3f]", w->n, lo, hi);
        }
        if (w->n < rounds) ADD("  [%d of %d runs]", w->n, rounds);
        {
            // the picture's fingerprint (the run's last frame), and the first DLL above that produced the same one
            const struct Row* same = NULL;
            for (int j = 0; j < i && !same; ++j)
                if (!strcmp(rows[j].size, w->size) && !strcmp(rows[j].scene, w->scene) && !strcmp(rows[j].hash, w->hash)) same = &rows[j];
            ADD("  picture %s", w->hash);
            if (w->hashdiff) ADD(" (NOT the same picture in all its runs%s)", w->diff);
            if (same) ADD(" = %s", same->name);
            else if (ref && ref != w) ADD(" (differs from those above)");
            if (ref && ref != w && strcmp(w->hash, ref->hash)) {      // how far from the reference (since kit 2026-10-20)
                const int ri = !strncmp(ref->name, REFS[1], strlen(REFS[1]));
                if (w->pic[ri][0]) ADD(" [%s]", w->pic[ri]); else if (w->pic[0][0]) ADD(" [%s]", w->pic[0]);
            }
        }
        ADD("\n");
    }
    if (fl) ADD("did not run:\n%s", fails);
    if (kit_diag_len) ADD("diagnostics:\n%s", kit_diag);
    printf("\n=== summary\n%s", summary);
#ifndef HELIXSR_KIT
    if (!unattended) {
        char in[256];
        printf("\nOptional: if you have looked at OptiScaler's overlay in a game, what upscaler time (ms) did it show with AMD's file\nand with this project's? One line, for example:   3.8 2.9 Cyberpunk 2077, 4K Balanced\n(Enter to skip) > ");
        fflush(stdout);
        if (fgets(in, sizeof(in), stdin) && in[0] != '\n' && in[0] != '\r') { in[strcspn(in, "\r\n")] = 0; ADD("in a game (tester, AMD ms / ours ms / where): %.200s\n", in); }
    }
#endif
    FILE* o = fopen(results, "w");
    if (o) { fputs(summary, o); fclose(o); }
    printf("\nSaved as %s\nReport ID: %s (also in the file; please mention it if you write about this run)\n", results, id);
#ifdef HELIXSR_KIT
    if (!unattended) { char in[16]; printf("\nPress Enter to close."); fflush(stdout); fgets(in, sizeof(in), stdin); }
    return 0;
#endif
    if (!unattended) {
        char in[16];
        printf("\nSend this summary and the per-run figures (results-raw.txt in this folder), and nothing else, to the project's results channel? [Y/n] ");
        fflush(stdout);
        if (fgets(in, sizeof(in), stdin) && in[0] != 'n' && in[0] != 'N') {
            // optional: a name for the project's thank-you list; sent with the summary only if one is typed
            char name[128], clean[48]; int k = 0;
            printf("\nThank you for testing. Your help is a valued part of this project.\nIf you would like to be named on the project's thank-you page, type a name or nickname. It would be published there.\n(Optional. Enter to skip: then no name is sent) > ");
            fflush(stdout);
            if (fgets(name, sizeof(name), stdin))
                for (const char* c = name; *c && *c != '\r' && *c != '\n' && k < 40; ++c)
                    if ((unsigned char)*c >= 32 && !strchr("`*_~|<>@#\\\"", *c) && !(*c == ' ' && (k == 0 || clean[k - 1] == ' '))) clean[k++] = *c;
            while (k > 0 && clean[k - 1] == ' ') --k;
            clean[k] = 0;
            if (k) ADD("thank-you list (the tester asked to be named): %s\n", clean);
            // the message: one short line; the summary and results-raw.txt are the attached files. A run without any timings says so ("run stopped").
            char gpuname[128], message[512], fname[64], rname[64];
            snprintf(gpuname, sizeof(gpuname), "%s", gpu);
            gpuname[strcspn(gpuname, "(")] = 0;
            while (gpuname[0] && gpuname[strlen(gpuname) - 1] == ' ') gpuname[strlen(gpuname) - 1] = 0;
            if (nr) snprintf(message, sizeof(message), "Timing kit report %s: %s, kit %s%s, %d round%s%s. The summary and the per-run figures are attached.", id, gpuname, KIT_VERSION, study == 2 ? ", RDNA2 study" : study == 3 ? ", RDNA3 study" : "", rounds, rounds == 1 ? "" : "s",
                             fl ? "; some runs failed (listed at the end of the file)" : "");
            else snprintf(message, sizeof(message), "Timing kit report %s: run stopped, no DLL gave timings on %s (kit %s). The reasons are in the attached file.", id, gpuname, KIT_VERSION);
            snprintf(fname, sizeof(fname), "results-%s.txt", id); snprintf(rname, sizeof(rname), "results-raw-%s.txt", id);
            char* rawtext = ReadText(raw);      // missing (no DLL ran) or too large: the summary alone
            const bool sent = Send(message, fname, summary, rawtext ? rname : NULL, rawtext);
            free(rawtext);
            printf(sent ? "Sent. Thank you: your help is a valued part of this project.\n" : "Could not send it. Please post the results file on the project's GitHub page or Discord instead.\n");
        }
        printf("\nPress Enter to close.");
        fflush(stdout);
        fgets(in, sizeof(in), stdin);
    }
    return 0;
}
