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
  Windows &amp; Linux / Proton · Version 2.0.0 (experimental) ·
  <a href="LICENSE">HelixSR Freeware License</a>
</p>

---

# HelixSR

**NVIDIA's DLSS Model E upscaler and experimental DLSS frame generation, focused on older GPUs.**

HelixSR runs NVIDIA's DLSS Model E network as standard Direct3D 12 compute shaders, **without NVIDIA Tensor Cores, an NVIDIA driver, CUDA, or ROCm**. It behaves like DLSS: every quality mode from native (DLAA) to Ultra Performance, dynamic resolution. Since 2.0 the same DLL also includes experimental support for **NVIDIA's DLSS frame generation** in games that have it.

> [!IMPORTANT]
> **HelixSR is focused on older graphics cards**, especially AMD RDNA 1 / RDNA 2 and the other older GPUs listed in [GPUs](#gpus). The goal is to help gamers get more from the hardware they already own. **RDNA 3 and newer GPUs may work, but they are not the focus of the project.**

> [!WARNING]
> **Frame generation is still a work in progress.** It may not work at all in some games or on some GPUs. Even when it runs, performance and image quality may be poor, and visual artifacts or instability may occur.

HelixSR is an independent, unofficial project and is **not affiliated with or endorsed by NVIDIA or AMD**.

**Version 2.0.0 experimental**, available on Linux (Proton) and Windows. See [GPUs](#gpus) for compatibility and current testing status. Developed and tested on the AMD BC-250 (gfx1013, Linux, Mesa RADV); other GPUs and Windows are not tested by us.

## New in 2.0.0

- **Experimental DLSS frame generation is back, in one DLL.** HelixSR's FidelityFX DLL (`HelixSR-fsrfg\amd_fidelityfx_dx12.dll`) does upscaling and DLSS frame generation. When a game turns on DLSS frame generation, HelixSR runs NVIDIA's own frame generation through the game's own Streamline. This remains a work in progress and may not work or work well on your setup.
- **Light mode by default:** NVIDIA's frame-generation performance mode (310.9.16). **Full** mode (310.3) is one click away in HelixSR Scaler's menu, with QuickFlow for extra speed.
- **Faster frame generation:** light mode about 20–30% less GPU time per generated frame than at the start of 2.0's development (1080p 2.2 ms, 1440p 3.6 ms, 4K 4.2 ms on the BC-250), full mode about 6–11% less, with the same frames.
- **HelixSR Scaler 1.1.0 handles frame generation exactly like OptiScaler:** HelixSR is a frame-generation provider next to OptiScaler's own (`FGInput = nvngxfg`, `FGNvngxReplacement = HelixSR`). AMD FSR and Intel XeSS frame generation stay available in the menu.
- **Ultra Performance:** the network works at twice the render size and a Lanczos pass scales to the screen (about a third less GPU time); `NetworkResolution = full` gives the sharpest image.
- **Faster upscaling at DLAA / Ultra Quality** (about 15–18% less GPU time, same image) and faster 32-bit paths for GPUs without fast 16-bit math.
- **The setup installs into your games again,** like 1.8.1: it lists your games, Enter installs HelixSR Scaler (or the FSR drop-in for games with anti-cheat), and it asks once about frame generation. The uninstaller is back, and building is faster (it uses every CPU thread).
- **HelixSR Desktop 1.1.0:** simpler, updates itself, can go back to the version a game had before.

## Install: HelixSR Desktop (recommended)

Download **HelixSR Desktop** from the [latest release](https://github.com/lonewolf0622/HelixSR/releases/latest) (Windows: the `.exe` installer; Linux / Steam Deck: the `.AppImage` or `.deb`) and run it. It sets HelixSR up on your PC once (a few minutes) and then installs it into your games with one button each:

- **HelixSR Scaler** for most games: HelixSR through the game's own DLSS, FSR or XeSS option, with an in-game menu (Insert). Not for online games or games with anti-cheat.
- **FSR drop-in** for games with FSR 3.1 and anti-cheat: only the game's FSR file is replaced (see below).

It also updates and removes HelixSR per game, shows each game's log for bug reports, and puts back everything it replaced.

## Install: setup scripts (without the app)

1. Download `HelixSR-2.0.0.zip` and extract it into a folder you keep (for example `C:\HelixSR` or your home folder).
2. Close your games and run the setup in that folder:
   - **Windows:** double-click `helixsr-setup.bat`
   - **Linux / Steam Deck:** `./helixsr-setup.sh` (in a terminal, or double-click and choose *Run in terminal*)
3. The first time, it asks before each download from NVIDIA (the DLSS DLL for the network, NVIDIA's DLSS frame generation DLLs and NVIDIA's NGX core) and builds everything on your PC. This takes a few minutes.
4. It lists your games (Steam, Epic, GOG, Xbox, Heroic, Lutris) and asks for each one: *install? [s/f/n]*. Press Enter. **s** = HelixSR Scaler: pick **DLSS** in the game. **f** = FSR drop-in (games with FSR 3.1): pick **AMD FSR** in the game. Games with anti-cheat get the FSR drop-in, never HelixSR Scaler by default.
5. It asks once: *Turn on HelixSR frame generation?* This experimental feature requires a DirectX 12 game with DLSS frame generation. Turn DLSS frame generation on in the game to try it; it may not work or work well on your setup.
6. Start the game. **Insert** opens HelixSR Scaler's menu (upscaler, HelixSR's settings, frame generation and its quality).

**A game that is not listed:** run `./helixsr-setup.sh /path/to/game`, or drop the game's folder onto `helixsr-setup.bat`.

**Update:** run the setup of the new version. For games that have HelixSR it asks *installed. Update? [Enter/s/f/r/n]*: Enter updates, **s** / **f** switches the way, **r** removes it. The app and the setup install the same way, so each can update or remove what the other installed.

**By hand:** HelixSR Scaler: copy the files of the `helixsr-scaler` folder next to the game's `.exe`, rename `OptiScaler.dll` to `dxgi.dll`, and copy the `HelixSR-fsrfg` folder the setup built next to it (`helixsr-scaler\README.txt`). FSR drop-in: the `fsr3-drop-in` folder holds HelixSR's DLL under both FSR 3.1 file names (its `README.txt` has the details).

## Settings

Most settings are in HelixSR Scaler's menu (Insert). Restart the game after changing them. HelixSR's own file is `helixsr.ini` next to its DLL in the game's `HelixSR-fsrfg` folder:

```
[Upscaling]
Int8 = auto
NetworkResolution = fast

[Sharpening]
Sharpener = off
Sharpness = 0.5

[DlssFrameGeneration]
Preset = c
Memory = auto

[Compatibility]
WaveSize = auto
```

- `Int8`: `auto` (8-bit math on GPUs with fast int8 dot products), `on` or `off`. With HelixSR Scaler, set it in its menu.
- `NetworkResolution`: Ultra Performance `fast` (default, with the Lanczos pass) or `full` (sharpest).
- `Sharpener`: `off` (default, as DLSS), `nvidia` or `rcas`; `Sharpness` 0.0 – 1.0.
- `Preset`: frame generation `c` (light mode, default) or `full`.
- `Memory`: frame generation's memory `auto`, `video` or `system`.
- `WaveSize`: `auto`, `64` or `32`. Leave it on `auto` unless the picture is broken.
- Rarely needed: `[Upscaling] FastOutput = off` (the earlier output kernels), `[DlssFrameGeneration] ShaderModel = 6.6` (frame generation's earlier kernels).

## Uninstall

Run `helixsr-uninstall.bat` (Windows) or `./helixsr-uninstall.sh` (Linux), or remove HelixSR in HelixSR Desktop. Every file is put back as it was; at the end the uninstaller can also delete what the setup built and its downloads.

## Troubleshooting

- **Red border around the image, low resolution, shaking image:** HelixSR's network is not running. `helixsr.log` (in the game's `HelixSR-fsrfg` folder) says why. Run the setup again.
- **No frame generation:** it needs a DirectX 12 game with DLSS frame generation; turn it on in the game. HelixSR Scaler's menu shows the frame-generation state. Frame generation is still a work in progress and may fail even when these requirements are met.
- **Image flickers with frame generation:** try `[DlssFrameGeneration] ShaderModel = 6.6` in `helixsr.ini`, or Full mode in the menu, and open an issue with `helixsr.log`.
- **The game no longer starts or crashes:** remove HelixSR (app or uninstaller), or verify the game's files in Steam.
- **Something else looks wrong:** open an issue with `helixsr.log`.

## GPUs

- **AMD RDNA 1 and the BC-250:** upscaling and experimental frame generation. The BC-250 is our test GPU.
- **AMD RDNA 2** (RX 6000, Steam Deck, RDNA 2-based Ryzen graphics): upscaling with 8-bit math (int8) where it helps, experimental frame generation.
- **AMD RDNA 3 and newer** (including RX 7000 / 9000 and newer Ryzen graphics): may work, but are **not the focus of the project**. Compatibility, performance, and frame-generation quality are not guaranteed.
- **Radeon Vega / Radeon VII, RX 400 / 500 (experimental):** 64-lane shaders, chosen automatically; experimental frame generation with the extra builds the setup makes. Not tested on real hardware: please open an issue with `helixsr.log`.
- **NVIDIA GTX 10 / 16 series and Intel Arc (should work, not fully tested):** GTX 10 uses 32-bit paths. Please open an issue with `helixsr.log` and how it went.
- DirectX 11 and Vulkan games get upscaling through HelixSR Scaler.

### Frame generation: which cards

**Frame generation is experimental and still a work in progress on all GPUs.** It may not work at all or may have poor image quality, performance, or stability. The entries below describe current testing and expected compatibility, not guaranteed results.

Frame generation needs a DirectX 12 game with DLSS frame generation and a GPU with shader model 6.6 and 64-bit integer shader support.

| GPU | Frame generation |
|---|---|
| AMD RDNA 1 / 2 (RX 5000 / 6000), BC-250, Steam Deck, RDNA 2-based Ryzen graphics | **Experimental; tested on the BC-250 only** |
| AMD RDNA 3 and newer (including RX 7000 / 9000 and newer Ryzen graphics) | **May work; not the project's focus**. Not tested by us. |
| AMD Radeon Vega / Radeon VII, RX 400 / 500 | Experimental: uses extra 64-lane builds the setup makes; not tested on real hardware |
| Intel Arc | Experimental; may work, not tested |
| NVIDIA GTX 10 / 16, RTX 20 / 30 | **Not yet:** NVIDIA's frame-generation code checks the real NVIDIA driver and refuses these cards; a fix is being worked on. Upscaling works. |

If frame generation does not work on your card, please open an issue with `helixsr.log`.

## How it works

**This is NVIDIA's network running on different hardware, not a newly trained model.** HelixSR answers the game exactly as DLSS would: DLSS's camera jitter, DLSS's render sizes per quality mode, the game's masks and exposure as DLSS uses them. For frame generation, HelixSR loads NVIDIA's own frame-generation code and runs NVIDIA's GPU kernels, translated to Direct3D 12, on your GPU. Packed FP16 arithmetic makes the output close to, but not bit-identical with, NVIDIA's.

## Performance

GPU time on the **BC-250 at 2000 MHz** (HelixSR 2.0.0):

| Output | Upscaling (Quality) | Frame generation, light mode | Frame generation, full |
|---|---:|---:|---:|
| 1920x1080 | 1.06 ms | 2.2 ms | 5.3 ms |
| 2560x1440 | 1.64 ms | 3.6 ms | 8.8 ms |
| 3840x2160 | 2.07 ms (Ultra Performance) | 4.2 ms | 8.3 ms |

## Limitations

- **Frame generation is a work in progress:** it may not work at all, or may have poor performance, visual artifacts, or instability. It requires DirectX 12 games with DLSS frame generation. Light mode can show moving HUD elements twice in generated frames.
- **Older GPUs are the project's focus.** RDNA 3 and newer may work, but are not the project's development focus; see [GPUs](#gpus).
- HelixSR Scaler is not for online games or games with anti-cheat (use the FSR drop-in there).

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

HelixSR is free to use under the **HelixSR Freeware License** (`LICENSE`); its source code is not published. The license covers the HelixSR DLL, its documentation and the setup scripts (`helixsr-setup.*`, `helixsr-uninstall.*` and the `setup` folder): you may use them free of charge and redistribute them only unmodified, complete as an official release and free of charge; you may read and run the setup scripts but not modify them (setup scripts published up to 1.8.1 keep their Apache 2.0 license). HelixSR contains a small amount of AMD FidelityFX material under the MIT license; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [SOURCE.md](SOURCE.md).

DLSS and associated NVIDIA technologies are trademarks and intellectual property of NVIDIA Corporation. **The HelixSR download contains none of NVIDIA's network weights or translated kernels:** the setup downloads NVIDIA's DLSS DLL from NVIDIA's GitHub (under NVIDIA's license, after asking you) or uses a copy you already have, and builds the upscaling network on your PC. The resulting network remains NVIDIA's property and is not covered by HelixSR's license; do not redistribute the DLL the setup builds.
