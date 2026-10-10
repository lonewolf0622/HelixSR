# HelixSR

**NVIDIA's DLSS Model E neural upscaler for AMD GPUs.**

HelixSR runs NVIDIA's DLSS Model E network as standard Direct3D 12 compute shaders, **without NVIDIA Tensor Cores, an NVIDIA driver, CUDA, or ROCm**. Games reach it through [OptiScaler](https://github.com/OptiScaler/OptiScaler), which the setup installs for you: pick DLSS in the game (or FSR / XeSS if the game has no DLSS) and HelixSR does the upscaling.

The goal is to help gamers get better image quality from the hardware they already own, particularly older and lower-end GPUs.

HelixSR is an independent, unofficial project and is **not affiliated with or endorsed by NVIDIA, AMD or the OptiScaler project**.

**Version 1.6.0.** For AMD RDNA 1 and newer GPUs. Developed and tested on the AMD BC-250 (gfx1013) on Linux (Mesa RADV, Proton). **Windows: new in this version and not tested by us yet (beta)**, please report how it works in [issue #20](https://github.com/lonewolf0622/HelixSR/issues/20).

## Install

1. Download `HelixSR-1.6.0.zip` and extract it into a folder you keep (for example your home folder or `Documents`).
2. Run the setup in that folder:
   - **Linux / Steam Deck:** `./helixsr-setup.sh` (in a terminal, or double-click and choose *Run in terminal*)
   - **Windows:** double-click `helixsr-setup.bat`
3. The first time, it asks to download NVIDIA's DLSS DLL and builds HelixSR's network from it (about a minute on a modern CPU).
4. It then lists your Steam games that have DLSS, FSR or XeSS and asks for each one: *install HelixSR? [Y/n]*. Close your games first.
5. Start the game and select **DLSS** in its graphics settings (or **FSR** / **XeSS** if it has no DLSS).

That is all: no launch options, no settings files, nothing to rename.

**Remove or update:** run the setup again. For games that have HelixSR it asks *Update it? [Y/n, r = remove]*. Removing puts back every file exactly as it was.

**A game that is not listed** (not from Steam, or not detected): the setup also makes a `manual-install` folder. Copy its two files, `dxgi.dll` and `OptiScaler.ini`, next to the game's `.exe` (keep a copy of the game's own `dxgi.dll` first if it has one). The HelixSR folder has to stay where it is, because `OptiScaler.ini` points to it.

**Updating HelixSR:** extract the new version into a new folder, run its setup and answer yes to update your games.

## What the setup does in a game

In the folder of the game's `.exe` it puts:
- **OptiScaler** as `dxgi.dll` (or under the name of the OptiScaler the game already has), with an `OptiScaler.ini` set to use DLSS through HelixSR;
- a **`HelixSR`** folder with HelixSR's `nvngx.dll` and a `backup` folder with every file it replaced.

The OptiScaler is HelixSR's build of OptiScaler 0.9.4 with one added setting, `[DLSS] ForceEnabled`: it lets OptiScaler use DLSS on a GPU that is not NVIDIA's. That build, its license (GPL-3.0) and the patch are in the `optiscaler` folder; see [SOURCE.md](SOURCE.md).

## Troubleshooting

- **Red border around the image, low resolution, shaking image:** HelixSR's network is not running. Open `helixsr.log` in the game's `HelixSR` folder: it says why. Run the setup again and answer yes to update the game.
- **DLSS is not offered in the game:** select FSR or XeSS instead if the game has them, or open an issue with `OptiScaler.log` (next to the game's `.exe`).
- **Something else looks wrong:** open an issue with `helixsr.log` and `OptiScaler.log` attached.

## Setup details

- The setup downloads NVIDIA's DLSS 310.7.0 DLL from NVIDIA's GitHub ([github.com/NVIDIA/DLSS](https://github.com/NVIDIA/DLSS), NVIDIA's license applies) after asking, builds the network for your GPU (RDNA 3 and newer get extra 64-lane shaders; `--wave64` builds those too, for example before moving to such a GPU), adds it to `nvngx.dll` and deletes NVIDIA's DLL again. Already have DLSS 310.7.0 (or 310.2) from a game? `./helixsr-setup.sh --dlss /path/to/nvngx_dlss.dll`.
- **Linux:** needs Python 3 with numpy (the setup uses your system's, offers to install them with your package manager, or downloads a portable Python into `~/.local/share/HelixSR` on read-only systems such as SteamOS and Bazzite). The shader compiler (Microsoft's DirectX Shader Compiler) runs through Proton, which you already have for your games.
- **Windows:** Python, numpy and the shader compiler are downloaded once into `%LOCALAPPDATA%\HelixSR`. No admin rights.
- Every download is pinned to an exact version and checked by SHA-256.
- **The `nvngx.dll` the setup makes contains NVIDIA's network: it is for your own PC, do not share or upload it.**

## How it works

| Part | Role |
|---|---|
| **OptiScaler** | Takes the game's DLSS, FSR or XeSS calls and hands them to HelixSR through DLSS's interface (NGX). |
| **DLSS reconstruction** | NVIDIA's Model E network and trained weights. Not in the download: the setup builds them on your PC from NVIDIA's DLSS DLL. |
| **HelixSR** | The custom D3D12 compute implementation, AMD-focused arithmetic and memory-access optimizations and game-compatibility fixes. |

**This is NVIDIA's network running on different hardware, not a newly trained model.** HelixSR behaves like DLSS: no sharpening, every quality mode from DLAA to Ultra Performance, dynamic resolution. In Ultra Performance the network outputs twice the render size and a final pass scales that to the screen (less shimmer, about 30% faster). Packed FP16 arithmetic makes the output close to, but not bit-identical with, NVIDIA's DLSS.

## Performance

GPU time per upscaled frame on the **BC-250 at 2000 MHz**:

| Output | Mode | GPU time |
|---|---|---:|
| 1920x1080 | Quality (1280x720) | 1.13 ms |
| 1920x1080 | Ultra Performance (640x360) | 0.75 ms |
| 2560x1440 | Quality (1707x960) | 1.73 ms |
| 2560x1440 | Ultra Performance (853x480) | 1.15 ms |
| 3840x2160 | Performance (1920x1080) | 3.50 ms |
| 3840x2160 | Ultra Performance (1280x720) | 2.16 ms |

Upscaling-pass timings, not total game frame times. RDNA 3 and newer use wave64 network shaders automatically.

## Limitations

- Direct3D 12 games only. Only super resolution (no frame generation, no ray reconstruction).
- AMD RDNA 1 and newer. Radeon Vega GPUs (Vega 56/64, Radeon VII, Vega graphics in Ryzen APUs) are not supported: they have no wave32.
- Steam games are found automatically; other games through the `manual-install` folder.
- Windows is new and untested by us (beta).

## License and legal

HelixSR is free to use under the **HelixSR Freeware License** (`LICENSE`); its source code is not published. The setup scripts (`helixsr-setup.*` and the `setup` folder) are licensed under the Apache License 2.0 (`LICENSE-APACHE-2.0`). OptiScaler (folder `optiscaler`) is licensed under the GNU GPL 3.0; see [SOURCE.md](SOURCE.md) for its source. HelixSR contains a small amount of AMD FidelityFX material under the MIT license; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

DLSS and associated NVIDIA technologies are trademarks and intellectual property of NVIDIA Corporation. **The HelixSR download contains none of NVIDIA's weights or code:** the setup downloads NVIDIA's DLSS DLL from NVIDIA's own GitHub (under NVIDIA's license, after asking you) or uses a copy you already have, and builds the network from it on your PC. That network remains NVIDIA's property and is not covered by any license in this download; do not redistribute the DLL the setup makes.
