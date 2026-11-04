// SPDX-License-Identifier: GPL-2.0-or-later
// helixsr.inc: HelixSR's additions to the timing kit (included by fsr4time.c, before RunOne)
//
//   - HLX FSR 4: the kit times it like AMD's FSR 4 (its version name starts with "HLX"); which of its versions ran is read
//     from its log (HLX_FSR.log next to the DLL) and goes into the results line
//   - a DLL folder dlls\<name>\ may have kit-env.txt (NAME=VALUE lines) for its runs, e.g. BCLITE_TIER=7 (one HLX FSR 4
//     version), set while that folder's run is going
//   - HLX DLSS: a folder dlls\<name>\ with helixsr.dll (HelixSR's DLSS DLL) is timed through the NGX API, with the same
//     sizes, scenes, frames, timing and picture fingerprint as the FidelityFX DLLs

// "<set> tier <n>" of the last "own runtime active: <set> (tier <n>)" line in HLX_FSR.log (the run's folder), "" if none
static void HelixVersion(char* v, size_t n) {
    v[0] = 0;
    FILE* f = fopen("HLX_FSR.log", "rb");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        const char* p = strstr(line, "own runtime active: ");
        if (!p) continue;
        char set[96] = "";
        int tier = -1;
        if (sscanf(p + 20, "%95s", set) == 1) {
            const char* t = strstr(p, "(tier ");
            if (t) tier = atoi(t + 6);
            snprintf(v, n, "HLX FSR 4 %s tier %d", set, tier);
        }
    }
    fclose(f);
}

// The NAME=VALUE lines of dlls\<name>\kit-env.txt set (set) or cleared (!set) in this process; a run started after inherits them
static void HelixEnv(const char* name, bool set) {
    char path[MAX_PATH], line[512];
    snprintf(path, sizeof(path), "dlls\\%s\\kit-env.txt", name);
    FILE* f = fopen(path, "r");
    if (!f) return;
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        char* eq = strchr(line, '=');
        if (!eq || line[0] == '#') continue;
        *eq = 0;
        SetEnvironmentVariableA(line, set ? eq + 1 : NULL);
    }
    fclose(f);
}

// A folder without its own DLL whose kit-env.txt says HELIXSR_DLL_FROM=<folder>: that folder's DLL (one copy for every HLX FSR 4
// version timed); src left as it is otherwise
static void HelixDllFrom(const char* name, char* src, size_t n) {
    if (GetFileAttributesA(src) != INVALID_FILE_ATTRIBUTES) return;
    char path[MAX_PATH], line[512];
    snprintf(path, sizeof(path), "dlls\\%s\\kit-env.txt", name);
    FILE* f = fopen(path, "r");
    if (!f) return;
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strncmp(line, "HELIXSR_DLL_FROM=", 17) && !strpbrk(line + 17, "\\/:")) snprintf(src, n, "dlls\\%s\\amd_fidelityfx_upscaler_dx12.dll", line + 17);
    }
    fclose(f);
}

// HLX DLSS variants: a folder without its own helixsr.dll whose kit-env.txt says HELIXSR_DLL_FROM=<folder> with a helixsr.dll
// gets that DLL and its helixsr.ini for the run (HelixSR reads its settings next to its DLL; HelixNgxCleanup removes them
// after), with the kit-env.txt lines HELIXSR_INI=<section>/<key>=<value> written into that helixsr.ini
// (e.g. HELIXSR_INI=Compatibility/WaveSize=64); true if it copied
static bool HelixNgxPrepare(const char* name) {
    char dst[MAX_PATH], path[MAX_PATH], line[512], from[MAX_PATH] = "";
    snprintf(dst, sizeof(dst), "dlls\\%s\\helixsr.dll", name);
    if (GetFileAttributesA(dst) != INVALID_FILE_ATTRIBUTES) return false;
    snprintf(path, sizeof(path), "dlls\\%s\\kit-env.txt", name);
    FILE* f = fopen(path, "r");
    if (!f) return false;
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strncmp(line, "HELIXSR_DLL_FROM=", 17) && !strpbrk(line + 17, "\\/:")) snprintf(from, sizeof(from), "%s", line + 17);
    }
    char src[MAX_PATH];
    snprintf(src, sizeof(src), "dlls\\%s\\helixsr.dll", from);
    if (!from[0] || GetFileAttributesA(src) == INVALID_FILE_ATTRIBUTES || !CopyFileA(src, dst, FALSE)) { fclose(f); return false; }
    char ini[MAX_PATH], iniFull[MAX_PATH];
    snprintf(src, sizeof(src), "dlls\\%s\\helixsr.ini", from);
    snprintf(ini, sizeof(ini), "dlls\\%s\\helixsr.ini", name);
    CopyFileA(src, ini, FALSE);      // a release without one: the lines below make it
    GetFullPathNameA(ini, MAX_PATH, iniFull, NULL);
    rewind(f);
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (strncmp(line, "HELIXSR_INI=", 12)) continue;
        char* sec = line + 12;
        char* key = strchr(sec, '/');
        char* val = key ? strchr(key, '=') : NULL;
        if (!key || !val) continue;
        *key++ = 0; *val++ = 0;
        WritePrivateProfileStringA(sec, key, val, iniFull);
    }
    fclose(f);
    return true;
}
static void HelixNgxCleanup(const char* name) {
    char p[MAX_PATH];
    snprintf(p, sizeof(p), "dlls\\%s\\helixsr.dll", name); DeleteFileA(p);   // a run that still holds it: left in place
    snprintf(p, sizeof(p), "dlls\\%s\\helixsr.ini", name); DeleteFileA(p);
}

static bool HelixIsNgx(const char* name, char* dll, size_t n) {
    snprintf(dll, n, "dlls\\%s\\helixsr.dll", name);
    return GetFileAttributesA(dll) != INVALID_FILE_ATTRIBUTES;
}

// NGX's parameter object (nvsdk_ngx_params.h): 8 Set overloads (unsigned long long, float, double, unsigned int, int,
// ID3D11Resource*, ID3D12Resource*, void*), the 8 Get ones in the same order, then Reset. Its table has the MSVC layout
// (overloads in reverse order), as games and helixsr.dll use; checked with a value read back before anything is timed.
struct NgxParams { void** vt; void* self; bool msvc; };
static int NgxSetSlot(const struct NgxParams* p, int i) { return p->msvc ? 7 - i : i; }
static int NgxGetSlot(const struct NgxParams* p, int i) { return p->msvc ? 15 - i : 8 + i; }
static void NgxSetUI(struct NgxParams* p, const char* k, unsigned v) { ((void (*)(void*, const char*, unsigned))p->vt[NgxSetSlot(p, 3)])(p->self, k, v); }
static void NgxSetI(struct NgxParams* p, const char* k, int v) { ((void (*)(void*, const char*, int))p->vt[NgxSetSlot(p, 4)])(p->self, k, v); }
static void NgxSetF(struct NgxParams* p, const char* k, float v) { ((void (*)(void*, const char*, float))p->vt[NgxSetSlot(p, 1)])(p->self, k, v); }
static void NgxSetRes(struct NgxParams* p, const char* k, ID3D12Resource* v) {
    ((void (*)(void*, const char*, ID3D12Resource*))p->vt[NgxSetSlot(p, 6)])(p->self, k, v);
}
static int NgxGetUI(struct NgxParams* p, const char* k, unsigned* v) { return ((int (*)(void*, const char*, unsigned*))p->vt[NgxGetSlot(p, 3)])(p->self, k, v); }
static bool NgxAttach(struct NgxParams* p, void* obj) {
    p->self = obj;
    p->vt = *(void***)obj;
    for (int m = 1; m >= 0; --m) {      // the MSVC layout first; the other one if the value does not come back
        p->msvc = m;
        unsigned v = 0;
        NgxSetUI(p, "HelixSR.Kit.Check", 0x5eed1234u);
        if (NgxGetUI(p, "HelixSR.Kit.Check", &v) == 1 && v == 0x5eed1234u) return true;
    }
    return false;
}

static void HelixBarrier(ID3D12Resource* r, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
    D3D12_RESOURCE_BARRIER b = {.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION};
    b.Transition.pResource = r; b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    b.Transition.StateBefore = from; b.Transition.StateAfter = to;
    ID3D12GraphicsCommandList_ResourceBarrier(list, 1, &b);
}

// HLX DLSS through NGX: the FidelityFX run's sizes (Balanced and Ultra Performance; with --all-presets also Quality and
// Performance), scenes, frames and lines, with NGX's DLSS flags for the same input (HDR, low-resolution motion vectors,
// auto exposure); the inputs in NON_PIXEL_SHADER_RESOURCE and the output in UNORDERED_ACCESS while it runs, as games pass them
static int RunNgx(const char* name, const char* dllpath, bool quick) {
    char gpu[256];
    CreateDevice(gpu, sizeof(gpu));
    char full[MAX_PATH];
    GetFullPathNameA(dllpath, MAX_PATH, full, NULL);
    HMODULE dll = LoadLibraryExA(full, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!dll) { Say("R|%s|FAILED|helixsr.dll could not be loaded (error %lu)\n", name, GetLastError()); return 1; }
    typedef int (*InitFn)(unsigned long long, const wchar_t*, ID3D12Device*, unsigned, const void*);
    typedef int (*ParamsFn)(void**);
    typedef int (*CreateFn)(ID3D12GraphicsCommandList*, unsigned, void*, void**);
    typedef int (*EvalFn)(ID3D12GraphicsCommandList*, const void*, void*, void*);
    typedef int (*ReleaseFn)(void*);
    InitFn init = (InitFn)(void*)GetProcAddress(dll, "NVSDK_NGX_D3D12_Init_Ext");
    ParamsFn allocate = (ParamsFn)(void*)GetProcAddress(dll, "NVSDK_NGX_D3D12_AllocateParameters");
    ReleaseFn destroyParams = (ReleaseFn)(void*)GetProcAddress(dll, "NVSDK_NGX_D3D12_DestroyParameters");
    CreateFn create = (CreateFn)(void*)GetProcAddress(dll, "NVSDK_NGX_D3D12_CreateFeature");
    EvalFn evaluate = (EvalFn)(void*)GetProcAddress(dll, "NVSDK_NGX_D3D12_EvaluateFeature");
    ReleaseFn release = (ReleaseFn)(void*)GetProcAddress(dll, "NVSDK_NGX_D3D12_ReleaseFeature");
    if (!init || !allocate || !create || !evaluate || !release) { Say("R|%s|FAILED|helixsr.dll is not an NGX DLL\n", name); return 1; }
    wchar_t temp[MAX_PATH];
    GetTempPathW(MAX_PATH, temp);
    if (init(0x48454C49ull, temp, device, 0x15, NULL) != 1) { Say("R|%s|FAILED|NGX init\n", name); return 1; }
    Say("G|%s\n", gpu);

    struct { UINT ow, oh, rw, rh; int phases; const char* tag; } sizes[12] = {
        {1920, 1080, 1130, 636, 16, ""}, {2560, 1440, 1506, 848, 16, ""}, {3840, 2160, 2260, 1272, 16, ""},
        {1920, 1080, 640, 360, 72, " UP"}, {2560, 1440, 853, 480, 72, " UP"}, {3840, 2160, 1280, 720, 72, " UP"}};
    int nsizes = 6;
    if (all_presets)
        for (int m = 0; m < 2; ++m)
            for (int o = 0; o < 3; ++o) {
                const double ratio = m ? 2.0 : 1.5;
                sizes[nsizes].ow = sizes[o].ow; sizes[nsizes].oh = sizes[o].oh;
                sizes[nsizes].rw = (UINT)((double)sizes[o].ow / ratio + 0.5); sizes[nsizes].rh = (UINT)((double)sizes[o].oh / ratio + 0.5);
                sizes[nsizes].phases = m ? 32 : 18; sizes[nsizes].tag = m ? " P" : " Q";
                ++nsizes;
            }
    ID3D12QueryHeap* qh;
    D3D12_QUERY_HEAP_DESC qhd = {.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP, .Count = 2};
    CHECK(ID3D12Device_CreateQueryHeap(device, &qhd, &IID_ID3D12QueryHeap, (void**)&qh));
    ID3D12Resource* qbuf = Buffer(D3D12_HEAP_TYPE_READBACK, 16);
    UINT64 freq = 0;
    CHECK(ID3D12CommandQueue_GetTimestampFrequency(queue, &freq));
    const D3D12_RESOURCE_STATES ST_IN = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, ST_OUT = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    for (int si = 0; si < nsizes; ++si) {
        if (quick && sizes[si].ow == 2560) continue;
        const UINT ow = sizes[si].ow, oh = sizes[si].oh, rw = sizes[si].rw, rh = sizes[si].rh;
        const int phases = sizes[si].phases;
        char sz[24];
        snprintf(sz, sizeof(sz), "%ux%u%s", ow, oh, sizes[si].tag);
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
        for (int scene = 0; scene < 2; ++scene) {      // 0: nothing moves; 1: the camera pans 12, 6 output pixels per frame
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
            for (int p = 0; p < phases; ++p) HelixBarrier(color[p], D3D12_RESOURCE_STATE_COMMON, ST_IN);
            HelixBarrier(depth, D3D12_RESOURCE_STATE_COMMON, ST_IN);
            HelixBarrier(motion, D3D12_RESOURCE_STATE_COMMON, ST_IN);
            HelixBarrier(output, D3D12_RESOURCE_STATE_COMMON, ST_OUT);
            void* obj = NULL;
            struct NgxParams prm;
            if (allocate(&obj) != 1 || !obj || !NgxAttach(&prm, obj)) { Say("R|%s|FAILED|NGX parameters at %s\n", name, sz); return 1; }
            NgxSetUI(&prm, "Width", rw); NgxSetUI(&prm, "Height", rh); NgxSetUI(&prm, "OutWidth", ow); NgxSetUI(&prm, "OutHeight", oh);
            NgxSetI(&prm, "PerfQualityValue", 1);      // Balanced (the render size is set above)
            NgxSetI(&prm, "DLSS.Feature.Create.Flags", 0x1 | 0x2 | 0x40);      // HDR, low-resolution motion vectors, auto exposure
            NgxSetUI(&prm, "CreationNodeMask", 1); NgxSetUI(&prm, "VisibilityNodeMask", 1);
            void* feature = NULL;
            const int rc = create(list, 1 /* SuperSampling */, obj, &feature);
            Submit();
            if (rc != 1 || !feature) { Say("R|%s|FAILED|create feature 0x%08x at %s\n", name, (unsigned)rc, sz); return 1; }
            NgxSetRes(&prm, "Depth", depth); NgxSetRes(&prm, "MotionVectors", motion); NgxSetRes(&prm, "Output", output);
            NgxSetF(&prm, "MV.Scale.X", 1.0f); NgxSetF(&prm, "MV.Scale.Y", 1.0f); NgxSetF(&prm, "Sharpness", 0.0f);
            NgxSetF(&prm, "DLSS.Pre.Exposure", 1.0f); NgxSetF(&prm, "DLSS.Exposure.Scale", 1.0f); NgxSetF(&prm, "FrameTimeDeltaInMsec", 10.0f);
            NgxSetUI(&prm, "DLSS.Render.Subrect.Dimensions.Width", rw); NgxSetUI(&prm, "DLSS.Render.Subrect.Dimensions.Height", rh);
            static double ms[TIMED];
            for (int frame = 0; frame < WARM + TIMED; ++frame) {
                const int p = frame % phases;
                NgxSetRes(&prm, "Color", color[p]);
                NgxSetF(&prm, "Jitter.Offset.X", jx[p]); NgxSetF(&prm, "Jitter.Offset.Y", jy[p]);
                NgxSetI(&prm, "Reset", frame == 0);
                ID3D12GraphicsCommandList_EndQuery(list, qh, D3D12_QUERY_TYPE_TIMESTAMP, 0);
                const int er = evaluate(list, feature, obj, NULL);
                if (er != 1) { Say("R|%s|FAILED|evaluate 0x%08x at %s\n", name, (unsigned)er, sz); return 1; }
                ID3D12GraphicsCommandList_EndQuery(list, qh, D3D12_QUERY_TYPE_TIMESTAMP, 1);
                ID3D12GraphicsCommandList_ResolveQueryData(list, qh, D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, qbuf, 0);
                Submit();
                if (frame >= WARM) {
                    UINT64* t;
                    D3D12_RANGE rr = {0, 16};
                    CHECK(ID3D12Resource_Map(qbuf, 0, &rr, (void**)&t));
                    ms[frame - WARM] = (double)(t[1] - t[0]) * 1000.0 / (double)freq;
                    ID3D12Resource_Unmap(qbuf, 0, NULL);
                }
            }
            double sum = 0, fastest = ms[0];
            for (int i = 0; i < TIMED; ++i) { sum += ms[i]; if (ms[i] < fastest) fastest = ms[i]; }
            const double med = Median(ms, TIMED);
            char key[256], pkey[64];
            snprintf(key, sizeof(key), "%s|HLX DLSS|%s|%s", name, sz, scene ? "moving" : "still");
            snprintf(pkey, sizeof(pkey), "%s|%ux%u|%s", sz, rw, rh, scene ? "moving" : "still");
            const uint64_t h = HashOutput(output, ow, oh, key, name, pkey);
            Say("R|%s|HLX DLSS|%s|%ux%u|%s|%.4f|%.4f|%.4f|%.4f|%.4f|%016llx\n", name, sz, rw, rh, scene ? "moving" : "still", sum / TIMED, med, med,
                med, fastest, (unsigned long long)h);
            release(feature);
            if (destroyParams) destroyParams(obj);
            Submit();
            for (int p = 0; p < phases; ++p) HelixBarrier(color[p], ST_IN, D3D12_RESOURCE_STATE_COMMON);
            HelixBarrier(depth, ST_IN, D3D12_RESOURCE_STATE_COMMON);
            HelixBarrier(motion, ST_IN, D3D12_RESOURCE_STATE_COMMON);
            HelixBarrier(output, ST_OUT, D3D12_RESOURCE_STATE_COMMON);
            Submit();
        }
        for (int p = 0; p < phases; ++p) ID3D12Resource_Release(color[p]);
        ID3D12Resource_Release(depth); ID3D12Resource_Release(motion); ID3D12Resource_Release(output);
        free(cbuf); free(dbuf); free(mbuf);
    }
    return 0;
}
