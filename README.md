# HelixSR

**An FSR/DLSS hybrid upscaler for Direct3D 12, optimized for AMD GPUs.**

HelixSR combines **NVIDIA's DLSS Model E neural reconstruction** with an **FSR 3.1-compatible interface and input handling**. It is a drop-in replacement for a game's AMD FidelityFX upscaler DLL, with the network running through a custom DirectX 12 compute path.

Games talk to HelixSR as FSR 3.1. HelixSR reconstructs the image using Model E, running as standard compute shaders **without NVIDIA Tensor Cores, an NVIDIA driver, CUDA, or ROCm**. On Linux, it runs through Proton and vkd3d-proton.

The goal is to help gamers get better image quality from the hardware they already own, particularly older and lower-end GPUs.

HelixSR is an independent, unofficial project and is **not affiliated with or endorsed by NVIDIA or AMD**.

**Version 1.4.0.** For AMD RDNA 1 and newer GPUs (Radeon Vega and Radeon VII are not supported yet). Developed and tested on the AMD BC-250 (gfx1013, Linux, Mesa RADV) in an FSR 3.1 game. RDNA 3 and newer use wave64 network shaders automatically; community testing on Windows (ROG Ally X) helped with that. Other GPUs, drivers and games are untested by us.

## License and source

HelixSR is free to use under the **HelixSR Freeware License** (`LICENSE`). Its source code is not published. The setup scripts (`helixsr-setup.*` and the `setup` folder) are licensed under Apache-2.0 (`LICENSE-APACHE-2.0`). **The HelixSR download contains no NVIDIA weights or NVIDIA-derived kernels:** the setup builds them on your PC from NVIDIA's own DLSS DLL (see [Setup](#setup-once-per-pc)), and those files stay NVIDIA's property. AMD FidelityFX components retain their MIT license. The setup downloads Microsoft's DirectX Shader Compiler, Python and numpy from their official sources; they are not included in the HelixSR download. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and [SOURCE.md](SOURCE.md).

## How the hybrid works

| Part | Role in HelixSR |
|---|---|
| **FSR-compatible integration** | The FSR 3.1-facing API and handling of game-provided colour, depth, motion vectors, jitter and exposure. Supports reactive-mask generation and optional FidelityFX RCAS sharpening. |
| **DLSS reconstruction** | NVIDIA's Model E neural network and trained weights provide the reconstruction. They are not in the download: the setup builds them on your PC from NVIDIA's DLSS DLL. |
| **HelixSR execution** | The custom D3D12 compute implementation, AMD-focused arithmetic and memory-access optimizations, network-selection choices and game-compatibility fixes. |

**This is a hybrid pipeline, not a newly trained FSR/DLSS neural network.** Model E remains the reconstruction network; HelixSR does not combine FSR 4's model with DLSS weights. The compute implementation and surrounding input handling have been adapted for the target hardware.

Since v1.1.0, packed FP16 arithmetic, accumulation and rounding choices improve performance, but the output is **not bit-identical to NVIDIA's DLSS arithmetic**. Image quality was unchanged in our tests; that is not a guarantee of identical results in every game.

## Changes

- **1.4.0:** new quality and speed presets, a one-file DLL and an installer. **Re-run the setup after updating** (the network file format changed; the 1.3.0 files are refused with a message).
  - **QSSM (Quality Super-Sampled Mode):** the network outputs more than the screen and a final pass scales it down: cleaner edges and less shimmer, at a higher GPU cost. Presets from 1.025x to 1.25x the screen (`NetworkResolution = QSSM Min` ... `QSSM Ultra`). At 1.25x it had less error and 4-12% less flicker than NVIDIA's Model E at the screen size in our test scenes.
  - **PRSM (Performance Reduced-Sample Mode):** the network outputs less than the screen and the final pass scales it up: faster, softer. Presets from 0.95x to 0.70x the screen (`PRSM Light` ... `PRSM Extreme`). Example: 4K Performance 3.54 -> 2.74 ms at 0.75x.
  - **Ultra Performance:** `NetworkScale` sets the network size (default 2x the render size; 2.25 and 2.5 give 1620p and 1800p at 4K: more detail for more GPU time).
  - **One-file DLL and installer:** the setup now builds the network into `amd_fidelityfx_dx12.dll` itself, so a game needs one file. `helixsr-install` lists your Steam games with FSR 3.1, installs HelixSR into the ones you pick (keeping the game's DLL as `*.original.dll`) and puts it back on uninstall.
  - **Fixes:** games with dynamic resolution could crash in 1.3.0 (its split k7/k11 kernels are removed); the game's reactive mask is passed to the network correctly when `UseReactiveMask = true` (off by default, as before).
  - **Sharper still images:** the camera jitter sequence is three times longer than NVIDIA's recommended length: 5-13% less error on still and slow scenes in our tests. 1080p Quality is 0.03 ms slower than 1.3.0 (1.13 -> 1.16 ms) after the split kernels were removed.
- **1.3.0:** faster, and new GPU support. **Re-run the setup after updating** (the kernel file format changed).
  - **Ultra Performance about 30% faster** at the same image quality: the network outputs twice the render size and a final pass scales to the screen (`[Upscaling] NetworkResolution`). 4K 3.40 -> 2.16 ms, 1440p 1.63 -> 1.15 ms, 1080p 1.09 -> 0.75 ms. `NetworkResolution = fast` does the same for Performance (4K 3.50 -> 2.71 ms), a little softer.
  - Render sizes just under a 1.5x ratio (games that round up, e.g. 1707x960 for 1440p) use the faster output kernel: 1440p Quality 2.48 -> 1.73 ms. Two small network layers now spread across more of the GPU: 1080p Quality 1.15 -> 1.13 ms.
  - Automatic wave size (`[Compatibility] WaveSize`): RDNA 3, 3.5 and 4 run the network shaders at wave64 (same output as wave32 in our tests), which fixes the blurry/ghosting image reported on RDNA 3 under Windows. RDNA 1/2 keep wave32.
  - Radeon Vega / Radeon VII are not supported: HelixSR now logs this and leaves the network off instead of showing a shaking placeholder.
- **1.2.0:** the download no longer contains any NVIDIA weights or NVIDIA-derived kernels. A one-time setup (`helixsr-setup.sh` on Linux, `helixsr-setup.bat` on Windows) downloads NVIDIA's DLSS 310.7.0 DLL from NVIDIA's GitHub (after asking), builds HelixSR's network files from it on your PC (`helixsr_weights.bin`, `helixsr_kernels.pak`) and deletes the DLL again. The result is the same network as 1.1.0 (identical shaders and weights, same speed). Also: OptiScaler lists HelixSR as "FSR HelixSR (3.1.5)", and a second FidelityFX upscaler DLL (for example AMD's, with FSR 4) can be listed next to it (`[Forwarding] UpscalerDll`).
- **1.1.0:** faster: the network's convolutions use packed FP16 arithmetic and FP16 accumulation (as NVIDIA's own FP16 kernels do), the output stage reads its matrix operands directly from shared memory, and exact-rounding emulation that only mattered for bit-identical output is gone. GPU time per upscaled frame is lower, with the same image quality in our tests; see the measured timings below. Ultra Performance now uses the main network by default (about 30% faster than NVIDIA's Ultra Performance network on GPUs without matrix cores, same look in our tests); `Network = nvidia` in `helixsr.ini` restores NVIDIA's choice. Compatibility: the resource-requirements query gives the same answer as AMD's FSR 3.1.5 (some engines skipped the upscaler otherwise); games whose motion vectors include the camera jitter (FidelityFX's jitter-cancellation flag) no longer break in motion (the jitter was added instead of removed); typeless game textures are read in the format the game declares, as AMD's FSR does; the "generate reactive mask" dispatch is implemented (it used to return without writing the mask).
- **1.0.3:** supports FidelityFX's non-linear colour flags: sRGB- or PQ-encoded (HDR10) colour input is decoded to linear for the network and the output is encoded back the same way (previously treated as linear).
- **1.0.2:** answers the FSR 3.1 resource-requirements query before an upscaler context exists. Games that ask it at startup (with a backend and version override) got "unknown query" and turned FSR off.
- **1.0.1:** fixes for games with dynamic resolution: HelixSR no longer switches networks back and forth when the render size changes (that reset the image history and churned GPU memory, which could crash the compositor), and the auto-exposure buffers are sized for any render size (an overflow could hang the GPU). Output at a fixed resolution is unchanged.

## Features

- Every scale ratio: native / DLAA (1x) through Ultra Performance (3x), with NVIDIA's kernel selection at each ratio. The main network is used at every ratio by default; NVIDIA's Ultra Performance network is available on request. Dynamic resolution and quality-mode changes are supported without restarting the game.
- Game inputs through the FSR 3.1 interface: HDR or LDR colour, inverted or normal depth, render- or display-resolution motion vectors (with or without jitter), automatic exposure or the game's own exposure value.
- No sharpening by default (as DLSS); optional RCAS sharpening (FSR's sharpener) with less sharpening on fast motion.
- Other FidelityFX effects, including frame generation, are passed through to the game's own FidelityFX DLLs. HelixSR's neural reconstruction is the upscaling component, not a new frame-generation model.

## Setup (once per PC)

HelixSR's network is built on your PC from NVIDIA's DLSS DLL, which is NVIDIA's property and is therefore not included in the HelixSR download.

1. Extract the HelixSR release zip into a folder.
2. Run the setup in that folder:
   - **Linux:** `./helixsr-setup.sh`
   - **Windows:** double-click `helixsr-setup.bat`
3. It asks to download NVIDIA's DLSS 310.7.0 DLL from NVIDIA's GitHub ([github.com/NVIDIA/DLSS](https://github.com/NVIDIA/DLSS), NVIDIA's license applies), builds the network (wave32 and wave64 shaders; about 4-5 minutes) into the folder's `amd_fidelityfx_dx12.dll` and deletes NVIDIA's DLL again.

What it needs, and how it gets it:
- **Linux:** Python 3 with numpy. The script uses your system's, offers to install them with your distribution's package manager (pacman, apt, dnf, zypper), or downloads a portable Python into `~/.local/share/HelixSR` (read-only systems such as SteamOS, Bazzite, Silverblue). The shader compiler (Microsoft's DirectX Shader Compiler) is downloaded once from Microsoft's GitHub and runs through Proton, which you already have for your games.
- **Windows:** nothing installed: Python (python.org's embeddable package), numpy and the shader compiler are downloaded once into `%LOCALAPPDATA%\HelixSR`. No admin rights.
- Every download is pinned to an exact version and checked by SHA-256.
- Already have DLSS 310.7.0 (or 310.2) from a game? Use it instead of the download: `./helixsr-setup.sh --dlss /path/to/nvngx_dlss.dll` (Windows: `helixsr-setup.bat -Dlss C:\path\to\nvngx_dlss.dll`).

After the setup, `amd_fidelityfx_dx12.dll` carries the network: it is the one file to install in each game. Without the setup the DLL only does a simple upscale with a red frame, and logs why. **The DLL then contains NVIDIA's network: it is for your own PC, do not share or upload it.** (`--separate-files`, on Windows `-SeparateFiles`, keeps the network in `helixsr_weights.bin` and `helixsr_kernels.pak` next to the DLL instead, as before 1.4.0; HelixSR reads either.)

**Updating HelixSR:** extract the new release, run its setup and install again (the installer updates the games that already have HelixSR).

## Install (per game)

**With the installer:** run `./helixsr-install.sh` (Linux) or double-click `helixsr-install.bat` (Windows) in the HelixSR folder, after the setup. It finds your Steam libraries (also Flatpak Steam), lists the games that ship an FSR 3.1 DLL and installs HelixSR into the ones you type. `u` and a number uninstalls. Commands: `list`, `install 2 5`, `uninstall 2`, `install-all`, `uninstall-all`; `--folder <game folder>` for games outside Steam. Close the games first. In the game, select **AMD FSR** as the upscaler.

**By hand:**
1. Close the game and find its FSR 3.1 upscaler DLL. It is named either `amd_fidelityfx_upscaler_dx12.dll` (often under `Engine/Plugins/.../ThirdParty/Win64` in Unreal Engine games) or `amd_fidelityfx_dx12.dll`.
2. Rename the game's file by inserting `.original` before `.dll`, for example `amd_fidelityfx_upscaler_dx12.dll` -> `amd_fidelityfx_upscaler_dx12.original.dll`. Keep it: it is your backup, and for `amd_fidelityfx_dx12.dll` HelixSR forwards frame generation to `amd_fidelityfx_dx12.original.dll`.
3. Copy HelixSR's `amd_fidelityfx_dx12.dll` (after the setup) into the same folder under the game's original file name.
4. Optional: copy `helixsr.ini` next to it (every setting has a default).
5. Start the game normally (no launch options needed) and select **AMD FSR** as the upscaler.

To uninstall, delete HelixSR's file and rename the `.original` file back.

## Using with OptiScaler (DLSS, XeSS and FSR 2 / 3.0 games)

[OptiScaler](https://github.com/OptiScaler/OptiScaler) can route a game's upscaler calls (DLSS, XeSS, FSR 2/3) to an FSR 3.1 DLL, allowing HelixSR to be used in games that do not ship FSR 3.1 as a separate DLL. See the testing limitations below.

1. Install OptiScaler for the game as its documentation describes.
2. Put HelixSR's `amd_fidelityfx_dx12.dll` in a folder of its own (for example a `HelixSR` folder next to the game's executable), and also copy it there as `amd_fidelityfx_upscaler_dx12.dll`. Use the DLL after the setup (it carries the network). Both paths below need to point at HelixSR; otherwise OptiScaler may pick up the game's own FSR DLL.
3. In `OptiScaler.ini`:

   ```ini
   [Upscalers]
   Dx12Upscaler=fsr31

   [Libraries]
   FfxDx12Path=<folder>\amd_fidelityfx_dx12.dll
   FfxDx12SRPath=<folder>\amd_fidelityfx_upscaler_dx12.dll
   ```

   Windows-style full paths; under Proton, `Z:` is the Linux root (e.g. `Z:\home\you\...`).
4. Optional: `helixsr.ini` in the same folder. HelixSR writes `helixsr.log` there, which shows the network it runs.

OptiScaler's "FFX Upscaler" menu lists HelixSR as **FSR HelixSR (3.1.5)** (OptiScaler adds "FSR"; the 3.1.5 is the FidelityFX version games see). To keep **AMD's FSR 4** selectable next to it, put AMD's `amd_fidelityfx_upscaler_dx12.dll` (with FSR 4) in the HelixSR folder under another name, for example `amd_fidelityfx_upscaler_dx12.amd.dll`, and set `UpscalerDll = amd_fidelityfx_upscaler_dx12.amd.dll` in `helixsr.ini` (`[Forwarding]`). Its upscalers are then listed after HelixSR; the one you pick runs in AMD's DLL.

## Settings (`helixsr.ini`)

| Section | Key | Default | Meaning |
|---|---|---|---|
| `[Sharpening]` | `Mode` | `off` | `off` (as DLSS), `game` = the game's FSR sharpness (or `Sharpness` if it sends none), `override` |
| `[Sharpening]` | `Sharpness` | `0.3` | 0-1, FidelityFX scale |
| `[Sharpening]` | `MotionAdaptive` | `true` | Less sharpening on fast-moving pixels |
| `[ModelE]` | `Network` | `auto` | `auto` (the main network at every ratio), `nvidia` (as DLSS selects it), `main`, `ultraperformance` |
| `[ModelE]` | `MotionVectorFrontEnd` | `false` | Convert render-resolution motion vectors to display resolution before the network; used automatically when the game's vectors include jitter |
| `[ModelE]` | `InvertJitter`, `InvertMotionVectors` | `false` | For games whose jitter or motion vectors come out mirrored |
| `[Log]` | `Enabled` | `true` | Writes `helixsr.log` next to the DLL |
| `[Upscaling]` | `NetworkResolution` | `auto` | Size the network works at. `auto`: as DLSS, except Ultra Performance, where the network outputs `NetworkScale` x the render size and a final pass scales to the screen. `QSSM Ultra / High / Balanced / Light / Eco / Min` (1.25 / 1.20 / 1.15 / 1.10 / 1.05 / 1.025x the screen): supersampling, cleaner and more costly; not at Ultra Performance. `PRSM Light / Quality / Balanced / Performance / Ultra / Extreme` (0.95 / 0.90 / 0.85 / 0.80 / 0.75 / 0.70x the screen): faster, softer; never below 1.5x the render size (smaller would be slower). `full`: always the screen size. QSSM and PRSM change only upscaling modes; native (DLAA) stays at the screen size. Only with render-resolution motion vectors |
| `[Upscaling]` | `ScreenScale` | (preset) | Any QSSM / PRSM scale instead of the preset's, e.g. `1.15` |
| `[Upscaling]` | `NetworkScale` | `2.0` | Ultra Performance in `auto`: network output / render size; `2.25` and `2.5` give 1620p and 1800p at 4K |
| `[ModelE]` | `UseReactiveMask` | `false` | Pass the game's reactive mask (effects, particles) to the network |
| `[Compatibility]` | `WaveSize` | `auto` | Shader wave size. `auto`: wave32 on RDNA 1/2; on RDNA 3, 3.5 and 4 the network shaders run at wave64 (HelixSR's own convolution kernels stay wave32). `32` or `64` forces one. `helixsr.log` shows the choice (`wave size: ...`) |
| `[Forwarding]` | `Dll` | (auto) | DLL for FidelityFX effects other than upscaling (frame generation): `amd_fidelityfx_dx12.original.dll` if present, else `amd_fidelityfx_framegeneration_dx12.dll` |
| `[Forwarding]` | `UpscalerDll` | (empty) | Optional second FidelityFX upscaler DLL (e.g. AMD's with FSR 4), listed after HelixSR; a name without a folder is looked up next to HelixSR |

With sharpening off (the default), the network writes the game's output directly (no extra pass); sharpening costs about 0.4 ms at 4K.

## Performance

GPU time per upscaled frame, measured on one example GPU, the **BC-250 (RDNA 1-class) at 2000 MHz, sharpening off**, v1.4.0. Faster GPUs can afford the larger QSSM presets.

| Output | Mode | auto | QSSM Ultra | PRSM |
|---|---|---:|---:|---:|
| 1920x1080 | Quality (1280x720) | 1.16 ms | 2.57 ms | (screen size) |
| 2560x1440 | Quality (1707x960) | 1.76 ms | 4.38 ms | (screen size) |
| 3840x2160 | Performance (1920x1080) | 3.54 ms | | 2.74 ms (PRSM Ultra) |
| 3840x2160 | Ultra Performance (1280x720) | 2.20 ms; 2.62 ms with `NetworkScale = 2.25`, 3.06 ms with `2.5` | | |

These are upscaling-pass timings, not total game frame times. RDNA 3 and newer (wave64) were about 5% slower than wave32 on this GPU. Ultra Performance uses the main network by default; `Network = nvidia` restores NVIDIA's network selection.

In-game times depend on the GPU clock the driver chooses while the game runs. Setting the environment variable `HELIXSR_PROFILE=1` (e.g. launch option `HELIXSR_PROFILE=1 %command%`) logs per-stage GPU times to `helixsr.log`.

## Limitations

- The setup needs an internet connection once (or a local DLSS 310.7.0 / 310.2 DLL). Other DLSS versions are refused.
- Direct3D 12 only. On its own, HelixSR replaces FSR 3.1 DLLs; other Direct3D 12 games need OptiScaler (above). Vulkan games are not supported.
- For AMD RDNA 1 and newer. Tuned for and tested on the BC-250 (gfx1013) with Mesa RADV under Proton; Windows and other GPUs are untested by us. RDNA 3 and newer run the network at wave64, about 5% slower than wave32 in our tests.
- Radeon Vega GPUs (Vega 56/64, Radeon VII, Vega graphics in Ryzen APUs) are not supported in 1.4.0: they have no wave32. HelixSR logs this and does not start the network. Support may come later.
- Images are computed with DLSS's network but are not bit-identical to NVIDIA's DLSS arithmetic: FP16 arithmetic and rounding differ. If a game shows specks or blocks, please open an issue with `helixsr.log` attached.

## Legal

HelixSR is an independent, unofficial project and is not affiliated with or endorsed by NVIDIA Corporation or Advanced Micro Devices, Inc.

DLSS and associated NVIDIA technologies are trademarks and intellectual property of NVIDIA Corporation. HelixSR runs NVIDIA's DLSS Model E neural network. **The HelixSR download contains none of NVIDIA's weights or code:** the setup downloads NVIDIA's DLSS DLL from NVIDIA's own GitHub (under NVIDIA's license, after asking you) or uses a copy you already have, and builds the network files from it on your PC. Those files remain NVIDIA's property, are not licensed under HelixSR's Apache License 2.0, and are for your own use: do not redistribute them.

FidelityFX and FSR are trademarks of Advanced Micro Devices, Inc. FidelityFX components used by HelixSR are subject to their respective AMD licenses; see `THIRD_PARTY_NOTICES.md`.

HelixSR is licensed under the HelixSR Freeware License (`LICENSE`); its setup scripts are licensed under the Apache License 2.0 (`LICENSE-APACHE-2.0`). Neither license extends to NVIDIA's components described above. The source code is not published; see [SOURCE.md](SOURCE.md).
