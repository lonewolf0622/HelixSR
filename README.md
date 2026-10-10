# HelixSR

**NVIDIA's DLSS Model E neural upscaler for AMD GPUs.**

HelixSR runs NVIDIA's DLSS Model E network as standard Direct3D 12 compute shaders, **without NVIDIA Tensor Cores, an NVIDIA driver, CUDA, or ROCm**. It behaves like DLSS: no sharpening, every quality mode from native (DLAA) to Ultra Performance, dynamic resolution.

The goal is to help gamers get better image quality from the hardware they already own, particularly older and lower-end GPUs.

HelixSR is an independent, unofficial project and is **not affiliated with or endorsed by NVIDIA, AMD or the OptiScaler project**.

**Version 1.7.0.** For AMD RDNA 1 and newer GPUs, on Linux (Proton) and Windows. Developed and tested on the AMD BC-250 (gfx1013, Linux, Mesa RADV); other GPUs and Windows are not tested by us.

**If 1.7.0 does not work for you, use [HelixSR 1.4.3](https://github.com/lonewolf0622/HelixSR/releases/tag/v1.4.3)** (and please open an issue with `helixsr.log`).

## Install

1. Download `HelixSR-1.7.0.zip` and extract it into a folder you keep (for example `C:\HelixSR` or your home folder).
2. Run the setup in that folder:
   - **Windows:** double-click `helixsr-setup.bat`
   - **Linux / Steam Deck:** `./helixsr-setup.sh` (in a terminal, or double-click and choose *Run in terminal*)
3. The first time, it asks to download NVIDIA's DLSS DLL (press Enter) and builds HelixSR's network from it (about a minute).
4. It then lists your Steam games and asks for each one: *install HelixSR? [Y/n]*. Close your games first.
5. Start the game and select the upscaler the setup told you: **DLSS** for games that have DLSS, **AMD FSR** for games that only have FSR.

No launch options, no settings files.

**Remove or update:** run the setup again. For games that have HelixSR it asks *Update it? [Y/n, r = remove]*. Removing puts back every file as it was.

## How HelixSR gets into a game

The setup picks one of two ways per game:

| The game has | What the setup does | Select in the game |
|---|---|---|
| **DLSS** (or XeSS / FSR 2) | Installs the official [OptiScaler](https://github.com/OptiScaler/OptiScaler) 0.9.4 next to the game's `.exe` and points its FSR 3.1 backend at HelixSR (a `HelixSR` folder next to it). The game's DLSS inputs reach HelixSR. | **DLSS** |
| **only FSR 3.1** | Replaces the game's FSR 3.1 DLL with HelixSR, as in 1.4.3; the game's file is kept as `*.original.dll`. No OptiScaler. | **AMD FSR** |

In OptiScaler's menu (Insert key) HelixSR is listed as **FSR HelixSR (3.1.5)**, together with AMD's own FSR (3.1.5 and 2.3.4, and FSR 4 on GPUs AMD supports it on): switch there to compare. HelixSR is the default. Everything the setup replaces is kept: in the game's `HelixSR\backup` folder (OptiScaler way) or as `*.original.dll`.

**FSR 4 (or another FSR version) next to HelixSR:** put the FSR DLL into the `my-fsr` folder (any file name), run the setup again and press Enter for your games. OptiScaler's menu then offers it next to HelixSR instead of the AMD FSR that comes with HelixSR. Take it out and run the setup again to go back.

**A game that is not listed** (not from Steam, or not detected): see `manual-install\README.txt`, which the setup creates. It has the files and steps for both ways.

**Updating HelixSR:** extract the new version into a new folder, run its setup and answer yes to update your games.

## Troubleshooting

- **Red border around the image, low resolution, shaking image:** HelixSR's network is not running. `helixsr.log` (in the game's `HelixSR` folder, or next to the game's FSR DLL) says why. Run the setup again and let it download NVIDIA's DLL.
- **DLSS is not offered in a game:** select FSR or XeSS instead if the game has them (OptiScaler hands them to HelixSR too), or open an issue with `OptiScaler.log` (next to the game's `.exe`).
- **Something else looks wrong:** open an issue with `helixsr.log` and, for the OptiScaler way, `OptiScaler.log`.

## Setup details

- The setup downloads NVIDIA's DLSS 310.7.0 DLL from NVIDIA's GitHub ([github.com/NVIDIA/DLSS](https://github.com/NVIDIA/DLSS), NVIDIA's license applies) after asking, builds the network for your GPU (RDNA 3 and newer get extra 64-lane shaders; `--wave64` builds those too) into `helixsr_weights.bin` and `helixsr_kernels.pak`, and deletes NVIDIA's DLL again. Already have DLSS 310.7.0 (or 310.2) from a game? `--dlss <path to nvngx_dlss.dll>`.
- **Windows:** Python, numpy and the shader compiler are downloaded once into `%LOCALAPPDATA%\HelixSR`. No admin rights.
- **Linux:** needs Python 3 with numpy (the setup uses your system's, offers to install them, or downloads a portable Python into `~/.local/share/HelixSR` on read-only systems such as SteamOS and Bazzite). The shader compiler runs through Proton.
- Every download is pinned to an exact version and checked by SHA-256.
- **`helixsr_weights.bin` and `helixsr_kernels.pak` contain NVIDIA's network: they are for your own PC, do not share or upload them.**

## How it works

**This is NVIDIA's network running on different hardware, not a newly trained model.** HelixSR answers the game exactly as DLSS would: DLSS's camera jitter (8 × scale² Halton positions), DLSS's render sizes per quality mode, no sharpening (a game's FSR sharpening request is not applied), reactive masks ignored. In Ultra Performance the network outputs twice the render size and a final pass scales that to the screen (less shimmer, about 30% faster). Packed FP16 arithmetic makes the output close to, but not bit-identical with, NVIDIA's DLSS.

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

Upscaling-pass timings, not total game frame times.

## Limitations

- Direct3D 12 games only. Only super resolution (no frame generation of its own; a game's FSR frame generation keeps working).
- AMD RDNA 1 and newer. Radeon Vega GPUs (Vega 56/64, Radeon VII, Vega graphics in Ryzen APUs) are not supported: they have no wave32.
- Steam games are found automatically; other games through `manual-install`.

## License and legal

HelixSR is free to use under the **HelixSR Freeware License** (`LICENSE`); its source code is not published. The setup scripts (`helixsr-setup.*` and the `setup` folder) are licensed under the Apache License 2.0 (`LICENSE-APACHE-2.0`). The `optiscaler` folder holds the official, unmodified OptiScaler 0.9.4 (GNU GPL 3.0) and fakenvapi (MIT); the `amd-fsr` folder holds AMD's FidelityFX upscaler DLL 4.1.1 under AMD's license (`amd-fsr/FidelityFX_v2_LICENSE.md`); see [SOURCE.md](SOURCE.md). HelixSR contains a small amount of AMD FidelityFX material under the MIT license; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

DLSS and associated NVIDIA technologies are trademarks and intellectual property of NVIDIA Corporation. **The HelixSR download contains none of NVIDIA's weights or code:** the setup downloads NVIDIA's DLSS DLL from NVIDIA's own GitHub (under NVIDIA's license, after asking you) or uses a copy you already have, and builds the network from it on your PC. That network remains NVIDIA's property and is not covered by any license in this download; do not redistribute the network files.
