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
</p>

<p align="center">
  <a href="#install-helixsr-desktop-recommended">Installation</a> ·
  <a href="#gpus">GPU compatibility</a> ·
  <a href="#performance">Performance</a> ·
  <a href="#troubleshooting">Troubleshooting</a> ·
  <a href="#community">Community</a>
</p>

<p align="center">
  Windows &amp; Linux / Proton · Experimental release ·
  <a href="LICENSE">HelixSR Freeware License</a>
</p>

---

# HelixSR

**NVIDIA's DLSS Model E neural upscaler and DLSS 4 Frame Generation for AMD GPUs.**

HelixSR runs NVIDIA's DLSS Model E network as standard Direct3D 12 compute shaders, **without NVIDIA Tensor Cores, an NVIDIA driver, CUDA, or ROCm**. It behaves like DLSS: every quality mode from native (DLAA) to Ultra Performance, dynamic resolution.

The goal is to help gamers get better image quality from the hardware they already own, particularly older and lower-end GPUs.

HelixSR is an independent, unofficial project and is **not affiliated with or endorsed by NVIDIA or AMD**.

**Version 1.9.0 experimental.** For AMD RDNA 1 and newer GPUs (Radeon Vega: experimental, see below), on Linux (Proton) and Windows. Developed and tested on the AMD BC-250 (gfx1013, Linux, Mesa RADV); other GPUs and Windows are not tested by us.

**If 1.9.0 does not work for you, use [HelixSR 1.8.1](https://github.com/lonewolf0622/HelixSR/releases/tag/v1.8.1)** (and please open an issue with `helixsr.log`).

## New in 1.9.0

- **HelixSR Desktop:** one app for Windows and Linux / Steam Deck that sets HelixSR up and installs it into your games
  with one button (Steam, Epic, GOG, Xbox / Game Pass, Heroic, Lutris, Bottles, itch, or any folder).
- **DLSS 4 Frame Generation with HelixSR:** NVIDIA's DLSS 4 Frame Generation network now runs on GPUs without it,
  through HelixSR Scaler. Select DLSS Frame Generation in the game. Experimental.
- **Faster upscaling:** up to about 8% less GPU time per frame on GPUs that use the normal network (1080p Quality on
  the BC-250: 1.14 → 1.06 ms), including RX 7000 / 9000 and Vega; and an int8 network for GPUs with fast int8 math
  (RX 6000 and newer, Intel Arc, NVIDIA), picked automatically.
- **HelixSR Scaler:** HelixSR through the game's own DLSS, FSR or XeSS option, with an in-game menu (Insert).
- **Radeon Vega** support (experimental).
- **Coming soon: multi-frame generation** (3x and 4x, up to three generated frames per real frame), already running in
  our tests.

## Install: HelixSR Desktop (recommended)

Download **HelixSR Desktop** from this release (Windows: the `.exe` installer; Linux / Steam Deck: the `.AppImage` or `.deb`) and run it. It sets HelixSR up on your PC once (a few minutes) and then installs it into your games with one button each, the right way for every game:

- **HelixSR Scaler** for most games: HelixSR through the game's own DLSS, FSR or XeSS option, plus frame generation and an in-game menu (Insert). Not for online games or games with anti-cheat.
- **FSR drop-in** for games with FSR 3.1 and anti-cheat: only the game's FSR file is replaced (see below).

It also updates and removes HelixSR per game, shows each game's log for bug reports, and puts back everything it replaced. The rest of this README is for the setup scripts in this zip, the way without the app.

## FSR 3.1 drop-in

This download installs HelixSR into **games with FSR 3.1** (also when they have DLSS): HelixSR replaces the game's FSR 3.1 DLL (`amd_fidelityfx_upscaler_dx12.dll`, or `amd_fidelityfx_dx12.dll` in games without it). The game's own file is kept as `*.original.dll`; FSR frame generation keeps working through it. In the game, select **AMD FSR**.

1. Download `HelixSR-1.9.0.zip` and extract it into a folder you keep (for example `C:\HelixSR` or your home folder).
2. Run the setup in that folder:
   - **Windows:** double-click `helixsr-setup.bat`
   - **Linux / Steam Deck:** `./helixsr-setup.sh` (in a terminal, or double-click and choose *Run in terminal*)
3. The first time, it asks to download NVIDIA's DLSS DLL and NVIDIA's DLSS Frame Generation DLL (press Enter) and builds HelixSR's network and frame generation from them (a few minutes).
4. Close your games. For each game with FSR 3.1 it asks *install? [f/n]*: press Enter. Games without FSR 3.1 are listed as not supported.
5. Start the game and select **AMD FSR** in its graphics settings.

No launch options are needed.

**A game that is not listed** (not from Steam, or not detected): at the end the setup asks for more games: drag the game's folder into the window (or paste its path). You can also drop the folder onto `helixsr-setup.bat` (Windows) or pass it (`./helixsr-setup.sh /path/to/game`).

**By hand:** after the setup has run, the `fsr3-drop-in` folder holds HelixSR's DLL with the network inside, under both FSR 3.1 file names. Rename the game's `amd_fidelityfx_upscaler_dx12.dll` (or `amd_fidelityfx_dx12.dll`) to `*.original.dll` and copy the file of the same name from `fsr3-drop-in` next to it. To undo, delete HelixSR's file and rename the original back. `manual-install\README.txt` has the details, including how to point your own OptiScaler at these DLLs.

**Update:** extract the new version into a new folder and run its setup; for games that have HelixSR it asks *installed (FSR). Update? [f/r/n]*: Enter updates, **r** removes. If a game update replaced HelixSR, the setup keeps the game's new file as the backup.

**Installs of earlier versions** (the OptiScaler way or the DLSS loader) are recognized: in games with FSR 3.1 the setup offers to switch them to the drop-in, elsewhere to remove them. Either way everything they replaced (in the game's `HelixSR\backup` folder) is put back.

## Games without FSR 3.1, DLSS 4 Frame Generation

Games with only DLSS, XeSS or FSR 2 are not supported by the drop-in. For those, and for HelixSR's frame generation (NVIDIA's DLSS Frame Generation network, run as Direct3D 12 compute shaders), use **HelixSR Desktop**: it installs **HelixSR Scaler**, an OptiScaler-based front end that uses this HelixSR download and the frame generation its setup builds.
Multi-frame generation (3x and 4x) is coming soon.

## Settings

The setup puts a `helixsr.ini` next to HelixSR's DLL in each game (next to the game's FSR DLL) with the settings at their defaults. Change a value, save, and restart the game:

```
[Sharpening]
Sharpener = off
Sharpness = 0.1

[Compatibility]
WaveSize = auto
```

- `Sharpener`: `off` (default, as DLSS 4, which does not sharpen), `nvidia` (NVIDIA's sharpening filter, as the driver's Image Sharpening) or `rcas` (AMD FSR's).
- `Sharpness`: 0.0 – 1.0, used when a sharpener is on.
- `WaveSize`: `auto`, `64` or `32`. Leave it on `auto` unless the picture is broken.

HelixSR works the same without the file.

## Uninstall

Run `helixsr-uninstall.bat` (Windows) or `./helixsr-uninstall.sh` (Linux). It removes HelixSR from the games you pick (also installs of earlier versions) and puts back every file as it was; at the end it can also delete the built network and the setup's downloads. A game you added by hand: drop its folder onto the uninstaller (Windows) or pass it (`./helixsr-uninstall.sh /path/to/game`).

## Troubleshooting

- **Red border around the image, low resolution, shaking image:** HelixSR's network is not running. `helixsr.log` (next to the game's FSR DLL) says why. Run the setup again and let it download NVIDIA's DLL.
- **No change in the game:** make sure **AMD FSR** is selected as the upscaler. An OptiScaler you installed yourself next to the game's `.exe` can take over the game's FSR calls: remove it, or point it at HelixSR (see `manual-install\README.txt`).
- **The game no longer starts or crashes:** remove HelixSR with the uninstaller (the game's own file comes back), or verify the game's files in Steam.
- **Something else looks wrong:** open an issue with `helixsr.log`.

## GPUs

- **int8 network (new in 1.9.0):** on GPUs with fast int8 dot products (AMD RDNA 2 and newer, Radeon VII) most of the network runs with 8-bit integer math: expected about 15% faster. Picked automatically; other GPUs keep the normal network. The output is not bit-identical, but the differences are far below anything visible (about 60 dB PSNR on our test frames).
- **Radeon Vega (experimental, new in 1.9.0):** Vega 56/64, Radeon VII and Vega graphics in Ryzen APUs have no wave32, so HelixSR runs every shader with 64 lanes there (automatic). **Not tested on a real Vega GPU**: the same path forced on our test GPU gives the same image. Please open an issue with `helixsr.log` and how it went.

## Setup details

- The setup downloads NVIDIA's DLSS 310.7.0 DLL from NVIDIA's GitHub ([github.com/NVIDIA/DLSS](https://github.com/NVIDIA/DLSS), NVIDIA's license applies) after asking, builds the network for your GPU (both 32- and 64-lane shaders; `--no-wave64` skips the 64-lane ones) into HelixSR's DLL (one file), and deletes NVIDIA's DLL again. Already have DLSS 310.7.0 (or 310.2) from a game? `--dlss <path to nvngx_dlss.dll>`.
- For frame generation (used by HelixSR Scaler) it also downloads, after asking, NVIDIA's DLSS Frame Generation DLL from NVIDIA's Streamline 2.8.0 release ([github.com/NVIDIA-RTX/Streamline](https://github.com/NVIDIA-RTX/Streamline), NVIDIA's license applies; only that DLL is read from the archive), NVIDIA's NGX core (`_nvngx.dll`, from NVIDIA's Linux driver 595.71.05 package on NVIDIA's download server, about 400 MB; only that file is kept) and Intel's XeLL library from the XeSS SDK 3.0.2 ([github.com/intel/xess](https://github.com/intel/xess), Intel's license applies). Have the DLL already? `--dlssg <path to nvngx_dlssg.dll>`. `--no-fg` (Linux) / `-NoFg` (Windows) skips this step; if it fails, the upscaler is installed as before.
- **Windows:** Python, numpy and the shader compiler are downloaded once into `%LOCALAPPDATA%\HelixSR`. No admin rights.
- **Linux:** needs Python 3 with numpy (the setup uses your system's, offers to install them, or downloads a portable Python into `~/.local/share/HelixSR` on read-only systems such as SteamOS and Bazzite). The shader compiler runs through Proton.
- Every download is pinned to an exact version and checked by SHA-256.
- **After the setup, HelixSR's DLLs (in this folder, `fsr3-drop-in` and your games) contain NVIDIA's networks: they are for your own PC, do not share or upload them.**

## How it works

**This is NVIDIA's network running on different hardware, not a newly trained model.** HelixSR answers the game exactly as DLSS would: DLSS's camera jitter (8 × scale² Halton positions), DLSS's render sizes per quality mode, the game's own sharpening request not applied (HelixSR's optional sharpening instead, see [Settings](#settings)), reactive masks ignored. In Ultra Performance the network outputs twice the render size and a final pass scales that to the screen (less shimmer, about 30% faster). Packed FP16 arithmetic makes the output close to, but not bit-identical with, NVIDIA's DLSS.

## Performance

GPU time per upscaled frame on the **BC-250 at 2000 MHz** (upscaling pass only, not the whole game frame):

| Output | Mode | 1.8.1 | 1.9.0 |
|---|---|---:|---:|
| 1920x1080 | Quality (1280x720) | 1.14 ms | **1.06 ms** |
| 1920x804 | Quality | 0.93 ms | **0.86 ms** |
| 3840x2160 | Performance (1920x1080) | 3.52 ms | **3.34 ms** |

Frame generation (DLSS 4 Frame Generation through HelixSR Scaler), GPU time for one generated frame on the same GPU:
about 4.6 ms at 1920x1080 and 7.8 ms at 3840x2160.

## Limitations

- Direct3D 12 games with FSR 3.1 only (others: HelixSR Desktop). The game's FSR frame generation keeps working.
- AMD RDNA 1 and newer. Radeon Vega: experimental, untested on real Vega hardware.
- Steam games and games from other stores' default folders are found automatically; other games: drag their folder into the setup, or see `manual-install`.

## Community

<a href="https://discord.gg/XwxcPD4GR">
  <img src="docs/assets/helixsr-community.svg" alt="Join the HelixSR Discord — get help, share results, and discuss releases" width="100%">
</a>

[Join our Discord](https://discord.gg/XwxcPD4GR) for setup help, testing feedback, and release discussions.

For reproducible bugs, [open a GitHub issue](https://github.com/lonewolf0622/HelixSR/issues) with your GPU, operating system, game, installation method, and `helixsr.log`. Say whether you tested upscaling, frame generation, or both. Check logs for personal information before posting them publicly.

---

## License and legal

HelixSR is free to use under the **HelixSR Freeware License** (`LICENSE`); its source code is not published. The license covers the HelixSR DLL, its documentation and the setup scripts (`helixsr-setup.*`, `helixsr-uninstall.*` and the `setup` folder): you may use them free of charge and redistribute them only unmodified, complete as an official release and free of charge; you may read and run the setup scripts but not modify them (setup scripts published up to 1.8.1 keep their Apache 2.0 license). HelixSR contains a small amount of AMD FidelityFX material under the MIT license; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [SOURCE.md](SOURCE.md).

DLSS and associated NVIDIA technologies are trademarks and intellectual property of NVIDIA Corporation. **The HelixSR download contains none of NVIDIA's weights or code:** the setup downloads NVIDIA's DLSS and DLSS Frame Generation DLLs from NVIDIA's own GitHub and NVIDIA's NGX core from NVIDIA's download server (under NVIDIA's licenses, after asking you) or uses copies you already have, and builds the networks from them on your PC. Those networks remain NVIDIA's property and are not covered by any license in this download; do not redistribute the DLLs the setup builds.
