<p align="center">
  <img src="docs/assets/helixsr-banner.svg" alt="HelixSR — more from the hardware you already own" width="100%">
</p>

<p align="center">
  <a href="https://github.com/lonewolf0622/HelixSR/releases">
    <img src="docs/assets/download-button.svg" alt="Download HelixSR" width="250" height="52">
  </a>
  &nbsp;
  <a href="https://discord.gg/XwxcPD4GR">
    <img src="docs/assets/discord-button.svg" alt="Join the HelixSR Discord" width="220" height="52">
  </a>
  &nbsp;
  <a href="https://www.paypal.com/donate/?business=WL2384PF9Z9W6&amp;no_recurring=0&amp;currency_code=USD">
    <img src="docs/assets/support-button.svg" alt="Support HelixSR via PayPal" width="220" height="52">
  </a>
</p>

<p align="center">
  <a href="#install-helixsr-desktop-recommended">Installation</a> ·
  <a href="#gpus">GPU compatibility</a> ·
  <a href="#performance">Performance</a> ·
  <a href="#troubleshooting">Troubleshooting</a> ·
  <a href="#community">Community</a> ·
  <a href="#support-helixsr">Support</a>
</p>

<p align="center">
  Windows &amp; Linux / Proton · Version 3.1.0 ·
  <a href="LICENSE">HelixSR Freeware License</a>
</p>

---

# HelixSR

**NVIDIA's DLSS Model E upscaler and AMD's FSR 4 network for GPUs that don't have them, focused on older GPUs.**

HelixSR runs NVIDIA's DLSS Model E network as standard Direct3D 12 compute shaders, **without NVIDIA Tensor Cores, an NVIDIA driver, CUDA, or ROCm**. It behaves like DLSS: every quality mode from native (DLAA) to Ultra Performance, dynamic resolution. It comes as two upscalers:

- **HLX DLSS:** the DLSS Model E network, as HelixSR Scaler's DLSS upscaler (DirectX 12, and DirectX 11 and Vulkan games through HelixSR Scaler).
- **HLX FSR 4:** AMD's FSR 4 network on HelixSR's own fast pipeline, as an upscaler of its own in HelixSR Scaler's list, or as a drop-in for games with FSR 3.1.

> [!IMPORTANT]
> **HelixSR is focused on older graphics cards**, especially AMD RDNA 1 / RDNA 2 and the other older GPUs listed in [GPUs](#gpus). The goal is to help gamers get more from the hardware they already own. **RDNA 3 and newer GPUs may work, but they are not the focus of the project.**

> [!NOTE]
> **HelixSR 3.x has no frame generation of its own.** AMD FSR and Intel XeSS frame generation can be picked in HelixSR Scaler's menu. HelixSR 2.0.0's experimental DLSS frame generation is still on the releases page.

HelixSR is an independent, unofficial project and is **not affiliated with or endorsed by NVIDIA or AMD**.

**Version 3.1.0**, available on Linux (Proton) and Windows. See [GPUs](#gpus) for compatibility and current testing status. Developed and tested on the AMD BC-250 (gfx1013, Linux, Mesa RADV); other GPUs and Windows are not tested by us.

## New in 3.1.0

- **HLX FSR 4 is an upscaler of its own** in HelixSR Scaler's list, next to AMD's FSR. On GPUs that have AMD's FSR 4, AMD's FSR 4 is listed again with its FSR 4 preset and model choices.
- **HLX FSR 4 is faster on AMD GPUs** (a new prepass and post pass, the same picture bit for bit) and picks its INT8 path first on GTX 16.
- **HLX DLSS in Vulkan games** through HelixSR Scaler, as in DirectX 11 games.
- **GTX 10 (Pascal) on Windows:** HLX DLSS's faster conversions are back.
- **The setup lists only games** (editors and tools from Windows' installed apps are no longer offered), and **Enter skips a game**: type **s** or **f** for the games you want.
- **Games with anti-cheat** are treated like any other game.
- **HelixSR Desktop 1.3.0 has a speed test:** it times HLX FSR 4 and HLX DLSS on your GPU and checks the picture. Send the results in an issue: it helps make HelixSR faster on your GPU.

## Install: HelixSR Desktop (recommended)

Download **HelixSR Desktop** from the [latest release](https://github.com/lonewolf0622/HelixSR/releases/latest) (Windows: the `.exe` installer; Linux / Steam Deck: the `.AppImage` or `.deb`) and run it. It sets HelixSR up on your PC once (a few minutes) and then installs it into your games with one button each:

- **HelixSR Scaler** for most games: HLX DLSS through the game's own DLSS, FSR or XeSS option, with HLX FSR 4 in its list and an in-game menu (Insert).
- **FSR drop-in** for games with FSR 3.1 (under *Advanced*): only the game's FSR file is replaced with HLX FSR 4 (see below).

It also updates and removes HelixSR per game, shows each game's log for bug reports, puts back everything it replaced, and has a **speed test** (top bar).

## Install: setup scripts (without the app)

1. Download `HelixSR-3.1.0.zip` and extract it into a folder you keep (for example `C:\HelixSR` or your home folder).
2. Close your games and run the setup in that folder:
   - **Windows:** double-click `helixsr-setup.bat`
   - **Linux / Steam Deck:** `./helixsr-setup.sh` (in a terminal, or double-click and choose *Run in terminal*)
3. The first time, it asks before each download and builds everything on your PC: the upscaling network from NVIDIA's DLSS DLL (downloaded from NVIDIA), and HLX FSR 4's shaders from AMD's FSR 4 driver file (an installed AMD driver's copy on Windows, else read out of AMD's driver package, about 420 MB, nothing is installed). This takes a few minutes.
4. It lists your games (Steam, Epic, GOG, Xbox, Heroic, Lutris) and asks for each one: *install? [s/f/n]*. Type **s** = HelixSR Scaler (pick **DLSS** in the game) or **f** = FSR drop-in (games with FSR 3.1: pick **AMD FSR** in the game) for the games you want; Enter skips a game.
5. Start the game. **Insert** opens HelixSR Scaler's menu (upscaler: HLX DLSS, HLX FSR 4, AMD FSR, XeSS; HelixSR's settings).

**A game that is not listed:** run `./helixsr-setup.sh /path/to/game`, or drop the game's folder onto `helixsr-setup.bat` (Enter installs a game you name).

**Update:** run the setup of the new version. For games that have HelixSR it asks *installed. Update? [Enter/s/f/r/n]*: Enter updates, **s** / **f** switches the way, **r** removes it. The app and the setup install the same way, so each can update or remove what the other installed.

**By hand:** the FSR drop-in: the `fsr3-drop-in` folder the setup makes holds HLX FSR 4 under both FSR 3.1 file names (its `README.txt` has the details). HelixSR Scaler is best installed by the app or the setup.

## Settings

HelixSR's settings are under `[HelixSR]` in the game's `OptiScaler.ini` (HelixSR Scaler writes them to `helixsr.ini` next to HelixSR's DLL, in the game's `HelixSR` folder, when the game starts). Restart the game after changing them. The upscaler and sharpening are picked in HelixSR Scaler's menu (Insert).

- `Int8`: `auto` (8-bit math on GPUs with fast int8 dot products), `on` or `off`.
- `WaveSize`: `auto`, `64` or `32`. Leave it on `auto` unless the picture is broken.
- With the FSR drop-in, HelixSR's `helixsr.ini` sits next to the game's FSR DLL.

## Uninstall

Run `helixsr-uninstall.bat` (Windows) or `./helixsr-uninstall.sh` (Linux), or remove HelixSR in HelixSR Desktop. Every file is put back as it was; at the end the uninstaller can also delete what the setup built and its downloads.

## Troubleshooting

- **Red border around the image, low resolution, shaking image:** HelixSR's network is not running. `helixsr.log` (in the game's `HelixSR` folder) says why. Run the setup again.
- **HLX FSR 4:** with HelixSR Scaler it writes to `OptiScaler.log` next to the game's `.exe`; as the FSR drop-in, to `HLX_FSR.log` next to its DLL.
- **The game no longer starts or crashes:** remove HelixSR (app or uninstaller), or verify the game's files in Steam.
- **Something else looks wrong:** open an issue with `helixsr.log`.

## GPUs

- **AMD RDNA 1 and the BC-250:** HLX DLSS and HLX FSR 4. The BC-250 is our test GPU.
- **AMD RDNA 2** (RX 6000, Steam Deck, RDNA 2-based Ryzen graphics): HLX DLSS with 8-bit math (int8) where it helps, HLX FSR 4.
- **AMD RDNA 3 and newer** (including RX 7000 / 9000 and newer Ryzen graphics): may work, but are **not the focus of the project**. Compatibility, performance, and frame-generation quality are not guaranteed.
- **Radeon Vega / Radeon VII, RX 400 / 500 (experimental):** 64-lane shaders, chosen automatically. Not tested on real hardware: please open an issue with `helixsr.log`.
- **NVIDIA GTX 10 / 16 series and Intel Arc (should work, not fully tested):** GTX 10 uses 32-bit paths; GTX 16 uses HLX FSR 4's INT8 path. Please open an issue with `helixsr.log` and how it went, or the speed test's results.
- DirectX 11 and Vulkan games get upscaling through HelixSR Scaler.

## How it works

**This is NVIDIA's network running on different hardware, not a newly trained model.** HelixSR answers the game exactly as DLSS would: DLSS's camera jitter, DLSS's render sizes per quality mode, the game's masks and exposure as DLSS uses them. Packed FP16 arithmetic makes the output close to, but not bit-identical with, NVIDIA's. HLX FSR 4 likewise runs AMD's FSR 4 network, built on your PC from AMD's driver file.

## Performance

GPU time per frame on the **BC-250** (HelixSR 3.1.0, Quality mode, the speed test's method):

| Output | HLX DLSS | HLX FSR 4 |
|---|---:|---:|
| 1920x1080 | 1.04 ms | 1.10 ms |
| 2560x1440 | 1.60 ms | 1.90 ms |
| 3840x2160 | 3.40 ms | 4.21 ms |

## Limitations

- **No HelixSR frame generation in 3.x** (AMD FSR and XeSS frame generation are in HelixSR Scaler's menu).
- **Older GPUs are the project's focus.** RDNA 3 and newer may work, but are not the project's development focus; see [GPUs](#gpus).

## Community

<a href="https://discord.gg/XwxcPD4GR">
  <img src="docs/assets/helixsr-community.svg" alt="Join the HelixSR Discord — get help, share results, and discuss releases" width="100%">
</a>

[Join our Discord](https://discord.gg/XwxcPD4GR) for setup help, testing feedback, and release discussions.

For reproducible bugs, [open a GitHub issue](https://github.com/lonewolf0622/HelixSR/issues) with your GPU, operating system, game, installation method, and `helixsr.log`. Include the HelixSR version you tested. Check logs for personal information before posting them publicly.

## Support HelixSR

If HelixSR has helped you get more from your GPU, consider supporting its continued development, testing, and compatibility improvements. Contributions are optional and appreciated.

[Support HelixSR via PayPal](https://www.paypal.com/donate/?business=WL2384PF9Z9W6&no_recurring=0&currency_code=USD)

---

## License and legal

HelixSR is free to use under the **HelixSR Freeware License** (`LICENSE`); its source code is not published. The license covers the HelixSR DLL, HLX FSR 4's DLL, their documentation and the setup scripts (`helixsr-setup.*`, `helixsr-uninstall.*` and the `setup` folder): you may use them free of charge and redistribute them only unmodified, complete as an official release and free of charge; you may read and run the setup scripts but not modify them (setup scripts published up to 1.8.1 keep their Apache 2.0 license). HelixSR contains a small amount of AMD FidelityFX material under the MIT license; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [SOURCE.md](SOURCE.md).

DLSS and associated NVIDIA technologies are trademarks and intellectual property of NVIDIA Corporation. **The HelixSR download contains none of NVIDIA's network weights or translated kernels:** the setup downloads NVIDIA's DLSS DLL from NVIDIA's GitHub (under NVIDIA's license, after asking you) or uses a copy you already have, and builds the upscaling network on your PC. The resulting network remains NVIDIA's property and is not covered by HelixSR's license; do not redistribute the DLL the setup builds. The same goes for HLX FSR 4's shaders, which the setup builds from AMD's FSR 4 driver file.
