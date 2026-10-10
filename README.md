# HelixSR

**NVIDIA's DLSS Model E neural reconstruction for AMD GPUs. Used together with [OptiScaler](https://github.com/OptiScaler/OptiScaler).**

HelixSR runs NVIDIA's DLSS Model E network as standard Direct3D 12 compute shaders, **without NVIDIA Tensor Cores, an NVIDIA driver, CUDA, or ROCm**. **HelixSR must be used with OptiScaler:** OptiScaler's DLSS backend loads HelixSR and hands it the game's DLSS calls, so HelixSR works with the game's own jitter, motion vectors and exposure. On Linux it runs through Proton and vkd3d-proton.

The goal is to help gamers get better image quality from the hardware they already own, particularly older and lower-end GPUs.

HelixSR is an independent, unofficial project and is **not affiliated with or endorsed by NVIDIA or AMD**.

**Version 1.5.1.** For AMD RDNA 1 and newer GPUs (Radeon Vega and Radeon VII are not supported yet). Developed and tested on the AMD BC-250 (gfx1013, Linux, Mesa RADV, Proton) in a Direct3D 12 game with DLSS. RDNA 3 and newer use wave64 network shaders automatically. **Linux with Proton only for now: Windows is not supported yet** (see [Limitations](#limitations)). Other GPUs, drivers and games are untested by us.

## What you need

- **Linux with Steam and Proton.**
- **A Direct3D 12 game with DLSS that already runs OptiScaler 0.9.4** ([OptiScaler](https://github.com/OptiScaler/OptiScaler), GPL-3.0). OptiScaler is not part of HelixSR and is not included in the download; install it in the game as its documentation describes.
- An AMD RDNA 1 or newer GPU.
- An internet connection once for the setup (or a local copy of NVIDIA's DLSS 310.7.0 DLL, see below).

## License and source

HelixSR is free to use under the **HelixSR Freeware License** (`LICENSE`). Its source code is not published. The setup scripts (`helixsr-setup.*`, `helixsr-install.*` and the `setup` folder) are licensed under Apache-2.0 (`LICENSE-APACHE-2.0`). **The HelixSR download contains no NVIDIA weights or NVIDIA-derived kernels:** the setup builds them on your PC from NVIDIA's own DLSS DLL (see [Setup](#setup-once-per-pc)), and those files stay NVIDIA's property. The setup downloads Microsoft's DirectX Shader Compiler, Python and numpy from their official sources; they are not included in the HelixSR download. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [SOURCE.md](SOURCE.md).

## How it works

| Part | Role in HelixSR |
|---|---|
| **DLSS interface** | HelixSR is installed as the NGX core (`nvngx.dll`) that OptiScaler's DLSS backend loads. It takes the game's colour, depth, motion vectors, jitter, exposure and reset flag exactly as DLSS defines them. |
| **DLSS reconstruction** | NVIDIA's Model E neural network and trained weights provide the reconstruction. They are not in the download: the setup builds them on your PC from NVIDIA's DLSS DLL. |
| **HelixSR execution** | The custom D3D12 compute implementation, AMD-focused arithmetic and memory-access optimizations, network-selection choices and game-compatibility fixes. |

**This is NVIDIA's network running on different hardware, not a newly trained model.** Packed FP16 arithmetic, accumulation and rounding choices improve performance, but the output is **not bit-identical to NVIDIA's DLSS arithmetic**. Image quality was unchanged in our tests; that is not a guarantee of identical results in every game.

## Setup (once per PC)

HelixSR needs NVIDIA's network (weights and shaders). It is built on your PC from NVIDIA's DLSS DLL, which is NVIDIA's property and is therefore not included in the HelixSR download. The setup adds the network to `nvngx.dll`, so the result is **one file**.

1. Extract the HelixSR release zip into a folder.
2. Run the setup in that folder: `./helixsr-setup.sh`
3. It asks to download NVIDIA's DLSS 310.7.0 DLL from NVIDIA's GitHub ([github.com/NVIDIA/DLSS](https://github.com/NVIDIA/DLSS), NVIDIA's license applies), builds the network (wave32 and wave64 shaders; about 5 minutes), appends it to `nvngx.dll` in the HelixSR folder and deletes NVIDIA's DLL again.

What it needs, and how it gets it:
- Python 3 with numpy. The script uses your system's, offers to install them with your distribution's package manager (pacman, apt, dnf, zypper), or downloads a portable Python into `~/.local/share/HelixSR` (read-only systems such as SteamOS, Bazzite, Silverblue). The shader compiler (Microsoft's DirectX Shader Compiler) is downloaded once from Microsoft's GitHub and runs through Proton, which you already have for your games.
- Every download is pinned to an exact version and checked by SHA-256.
- Already have DLSS 310.7.0 (or 310.2) from a game? Use it instead of the download: `./helixsr-setup.sh --dlss /path/to/nvngx_dlss.dll`.

**The `nvngx.dll` the setup makes contains NVIDIA's network: it is for your own PC, do not share or upload it.**

**Updating HelixSR:** run the setup again after every update, then run the installer again; it updates the files in each game.

## Install (per game)

Run `./helixsr-install.sh` in the HelixSR folder, after the setup, with the game closed.

1. It finds your Steam libraries (also Flatpak Steam) and lists the games that already have OptiScaler. Type the numbers of the games to install into. Commands: `list`, `install 2 5`, `uninstall 2`, `install-all`, `uninstall-all`; `--folder <game folder>` for games outside Steam.
2. For each game it copies `nvngx.dll` into a `HelixSR` folder next to `OptiScaler.ini`, and changes two settings in that `OptiScaler.ini`: `Dx12Upscaler = dlss` and `NvngxPath = <full path of that nvngx.dll>`. The previous values are saved, and your original file is kept as `OptiScaler.ini.helixsr-backup`.
3. **Add the launch option in Steam** (game > Properties > Launch Options):

   ```
   PROTON_FORCE_NVAPI=1 DXVK_NVAPI_GPU_ARCH=AD100 %command%
   ```

   OptiScaler only offers DLSS to a GPU that looks like an NVIDIA one, and this makes Proton present it as one. Keep any launch options you already have in front of `%command%`.
4. Start the game and select **DLSS** as the upscaler.

`u` and a number (or `uninstall 2`) removes HelixSR: it puts the two settings back and deletes its files. Remove the launch option yourself.

**By hand:** copy `nvngx.dll` into a folder of your choice, set `Dx12Upscaler = dlss` and `NvngxPath = <Windows-style full path of that nvngx.dll>` in `OptiScaler.ini` (under Proton `Z:` is the Linux root, e.g. `Z:\home\you\...\HelixSR\nvngx.dll`), and add the launch option above.

## Using it

There is nothing to configure: no settings file. HelixSR behaves like DLSS:

- Every quality mode, native/DLAA through Ultra Performance, with dynamic resolution and quality-mode changes without restarting the game.
- **No sharpening**, as DLSS: the network does not sharpen, and neither does HelixSR. If the game has its own sharpening it runs after HelixSR, as with DLSS.
- **Ultra Performance:** the network outputs twice the render size and a final pass scales that to the screen. In our tests it looks as good or better (less shimmer) and costs about 30% less than running the network at the full output size.
- HDR or LDR colour, inverted or normal depth, render- or display-resolution motion vectors, automatic exposure or the game's own exposure value, and sub-rectangle inputs and outputs are supported.

`helixsr.log` in the `HelixSR` folder shows the network and settings HelixSR chose (including `wave size: ...`) and what the game sends. Setting the environment variable `HELIXSR_PROFILE=1` (launch option `HELIXSR_PROFILE=1 PROTON_FORCE_NVAPI=1 DXVK_NVAPI_GPU_ARCH=AD100 %command%`) logs per-stage GPU times.

## Troubleshooting

- **DLSS is not offered in the game, and OptiScaler.log says `Not running on Nvidia, disabling DLSS`:** the launch option is missing or was overwritten. Check game > Properties > Launch Options.
- **HelixSR is listed but nothing happens, or there is no `helixsr.log`:** check that `OptiScaler.ini` has `Dx12Upscaler = dlss` and that `NvngxPath` is the full path of the installed `nvngx.dll`.
- **`helixsr.log` says the network is missing:** the DLL in the game is the one from the download, before the setup. Run `helixsr-setup.sh`, then `helixsr-install.sh` again.
- **Something looks wrong in a game:** open an issue with `helixsr.log` attached.

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

These are upscaling-pass timings, not total game frame times. RDNA 3 and newer (wave64) were about 5% slower than wave32 on this GPU. In-game times depend on the GPU clock the driver chooses while the game runs.

## Limitations

- **Linux with Proton only for now.** On Windows, OptiScaler only offers DLSS to NVIDIA GPUs, and Windows has no equivalent of the launch option above. The installer says so and does not install there.
- **OptiScaler 0.9.4 is required** and is not included.
- The setup needs an internet connection once (or a local DLSS 310.7.0 / 310.2 DLL). Other DLSS versions are refused.
- Direct3D 12 only. Vulkan games are not supported. Only DLSS super resolution is provided (no frame generation, no ray reconstruction).
- For AMD RDNA 1 and newer. Tuned for and tested on the BC-250 (gfx1013) with Mesa RADV under Proton; other GPUs are untested by us. Radeon Vega GPUs (Vega 56/64, Radeon VII, Vega graphics in Ryzen APUs) are not supported: they have no wave32. HelixSR logs this and does not start the network.
- Images are computed with DLSS's network but are not bit-identical to NVIDIA's DLSS arithmetic: FP16 arithmetic and rounding differ.

## Legal

HelixSR is an independent, unofficial project and is not affiliated with or endorsed by NVIDIA Corporation or Advanced Micro Devices, Inc.

DLSS and associated NVIDIA technologies are trademarks and intellectual property of NVIDIA Corporation. HelixSR runs NVIDIA's DLSS Model E neural network. **The HelixSR download contains none of NVIDIA's weights or code:** the setup downloads NVIDIA's DLSS DLL from NVIDIA's own GitHub (under NVIDIA's license, after asking you) or uses a copy you already have, and builds the network from it on your PC, adding it to HelixSR's DLL. That network remains NVIDIA's property, is not licensed under HelixSR's Apache License 2.0, and is for your own use: do not redistribute the DLL the setup makes.

HelixSR contains a small amount of AMD FidelityFX material under the MIT license; see `THIRD_PARTY_NOTICES.md`. OptiScaler is a separate project under its own license and is not included.

HelixSR is licensed under the HelixSR Freeware License (`LICENSE`); its setup scripts are licensed under the Apache License 2.0 (`LICENSE-APACHE-2.0`). Neither license extends to NVIDIA's components described above. The source code is not published; see [SOURCE.md](SOURCE.md).
