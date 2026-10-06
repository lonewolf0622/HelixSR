# HelixSR

**An FSR/DLSS hybrid upscaler for Direct3D 12, optimized for AMD GPUs.**

HelixSR combines **NVIDIA's DLSS Model E neural reconstruction** with an **FSR 3.1-compatible interface and input handling**. It is a drop-in replacement for a game's AMD FidelityFX upscaler DLL, with the network running through a custom DirectX 12 compute path.

Games talk to HelixSR as FSR 3.1. HelixSR reconstructs the image using Model E, running as standard compute shaders **without NVIDIA Tensor Cores, an NVIDIA driver, CUDA, or ROCm**. On Linux, it runs through Proton and vkd3d-proton.

The goal is to help gamers get better image quality from the hardware they already own, particularly older and lower-end GPUs.

HelixSR is an independent, unofficial project and is **not affiliated with or endorsed by NVIDIA or AMD**.

**Version 1.2.0.** For AMD RDNA 1 and newer GPUs. Developed and tested on the AMD BC-250 (gfx1013, Linux, Mesa RADV) in an FSR 3.1 game; other GPUs, drivers and games are untested.

## Source availability and license scope

**The implementation source and build scripts for the release DLL are not currently included in this repository's `main` branch.** The checked-in files provide documentation, configuration and license notices; they are not a complete source distribution of the upscaler. See [SOURCE.md](SOURCE.md).

The Apache-2.0 notice applies to HelixSR's original code. It does not mean that this code has already been published. **Since 1.2.0 the HelixSR download contains no NVIDIA weights or NVIDIA-derived kernels:** the setup builds them on your PC from NVIDIA's own DLSS DLL (see [Setup](#setup-once-per-pc)). AMD FidelityFX components retain their MIT license. The setup downloads Microsoft's DirectX Shader Compiler, Python and numpy from their official sources; they are not included in the HelixSR download. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## How the hybrid works

| Part | Role in HelixSR |
|---|---|
| **FSR-compatible integration** | The FSR 3.1-facing API and handling of game-provided colour, depth, motion vectors, jitter and exposure. Supports reactive-mask generation and optional FidelityFX RCAS sharpening. |
| **DLSS reconstruction** | NVIDIA's Model E neural network and trained weights provide the reconstruction. They are not in the download: the setup builds them on your PC from NVIDIA's DLSS DLL. |
| **HelixSR execution** | The custom D3D12 compute implementation, AMD-focused arithmetic and memory-access optimizations, network-selection choices and game-compatibility fixes. |

**This is a hybrid pipeline, not a newly trained FSR/DLSS neural network.** Model E remains the reconstruction network; HelixSR does not combine FSR 4's model with DLSS weights. The compute implementation and surrounding input handling have been adapted for the target hardware.

Since v1.1.0, packed FP16 arithmetic, accumulation and rounding choices improve performance, but the output is **not bit-identical to NVIDIA's DLSS arithmetic**. Image quality was unchanged in our tests; that is not a guarantee of identical results in every game.

## Changes

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

HelixSR needs two network files, `helixsr_weights.bin` and `helixsr_kernels.pak`, next to its DLL. They are built on your PC from NVIDIA's DLSS DLL, which is NVIDIA's property and is therefore not included in the HelixSR download.

1. Extract the HelixSR release zip into a folder.
2. Run the setup in that folder:
   - **Linux:** `./helixsr-setup.sh`
   - **Windows:** double-click `helixsr-setup.bat`
3. It asks to download NVIDIA's DLSS 310.7.0 DLL from NVIDIA's GitHub ([github.com/NVIDIA/DLSS](https://github.com/NVIDIA/DLSS), NVIDIA's license applies), builds the two network files next to `amd_fidelityfx_dx12.dll` (2-3 minutes) and deletes the DLL again.

What it needs, and how it gets it:
- **Linux:** Python 3 with numpy. The script uses your system's, offers to install them with your distribution's package manager (pacman, apt, dnf, zypper), or downloads a portable Python into `~/.local/share/HelixSR` (read-only systems such as SteamOS, Bazzite, Silverblue). The shader compiler (Microsoft's DirectX Shader Compiler) is downloaded once from Microsoft's GitHub and runs through Proton, which you already have for your games.
- **Windows:** nothing installed: Python (python.org's embeddable package), numpy and the shader compiler are downloaded once into `%LOCALAPPDATA%\HelixSR`. No admin rights.
- Every download is pinned to an exact version and checked by SHA-256.
- Already have DLSS 310.7.0 (or 310.2) from a game? Use it instead of the download: `./helixsr-setup.sh --dlss /path/to/nvngx_dlss.dll` (Windows: `helixsr-setup.bat -Dlss C:\path\to\nvngx_dlss.dll`).

Wherever you install HelixSR (below), copy `helixsr_weights.bin` and `helixsr_kernels.pak` next to its DLL. Without them HelixSR logs that they are missing and only does a simple upscale. **These files contain NVIDIA's network: they are for your own PC, do not share or upload them.**

## Install (per game)

1. Close the game and find its FSR 3.1 upscaler DLL. It is named either `amd_fidelityfx_upscaler_dx12.dll` (often under `Engine/Plugins/.../ThirdParty/Win64` in Unreal Engine games) or `amd_fidelityfx_dx12.dll`.
2. Rename the game's file by inserting `.original` before `.dll`, for example `amd_fidelityfx_upscaler_dx12.dll` -> `amd_fidelityfx_upscaler_dx12.original.dll`. Keep it: it is your backup, and for `amd_fidelityfx_dx12.dll` HelixSR forwards frame generation to `amd_fidelityfx_dx12.original.dll`.
3. Copy HelixSR's `amd_fidelityfx_dx12.dll` into the same folder under the game's original file name, together with `helixsr_weights.bin` and `helixsr_kernels.pak` from the setup.
4. Optional: copy `helixsr.ini` next to it (every setting has a default).
5. Start the game normally (no launch options needed) and select **AMD FSR** as the upscaler.

To uninstall, delete HelixSR's file and rename the `.original` file back.

## Using with OptiScaler (DLSS, XeSS and FSR 2 / 3.0 games)

[OptiScaler](https://github.com/OptiScaler/OptiScaler) can route a game's upscaler calls (DLSS, XeSS, FSR 2/3) to an FSR 3.1 DLL, allowing HelixSR to be used in games that do not ship FSR 3.1 as a separate DLL. See the testing limitations below.

1. Install OptiScaler for the game as its documentation describes.
2. Put HelixSR's `amd_fidelityfx_dx12.dll` in a folder of its own (for example a `HelixSR` folder next to the game's executable), and also copy it there as `amd_fidelityfx_upscaler_dx12.dll`. Put `helixsr_weights.bin` and `helixsr_kernels.pak` from the setup in the same folder. Both paths below need to point at HelixSR; otherwise OptiScaler may pick up the game's own FSR DLL.
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
| `[Forwarding]` | `Dll` | (auto) | DLL for FidelityFX effects other than upscaling (frame generation): `amd_fidelityfx_dx12.original.dll` if present, else `amd_fidelityfx_framegeneration_dx12.dll` |
| `[Forwarding]` | `UpscalerDll` | (empty) | Optional second FidelityFX upscaler DLL (e.g. AMD's with FSR 4), listed after HelixSR; a name without a folder is looked up next to HelixSR |

With sharpening off (the default), the network writes the game's output directly (no extra pass); sharpening costs about 0.4 ms at 4K.

## Performance

GPU time per upscaled frame on the **BC-250 at 2000 MHz, sharpening off**, using v1.1.0 (v1.2.0 runs the identical network at the same speed):

| Output | Mode | GPU time |
|---|---|---:|
| 1920x1080 | Quality (1280x720) | 1.15 ms |
| 1920x1080 | Native | 1.72 ms |
| 3840x2160 | Performance (1920x1080) | 3.50 ms |
| 3840x2160 | Ultra Performance (1280x720) | 3.41 ms |
| 3840x2160 | Native | 6.06 ms |

These are upscaling-pass timings, not total game frame times. Ultra Performance uses the main network by default in v1.1.0; `Network = nvidia` restores NVIDIA's network selection.

In-game times depend on the GPU clock the driver chooses while the game runs. Setting the environment variable `HELIXSR_PROFILE=1` (e.g. launch option `HELIXSR_PROFILE=1 %command%`) logs per-stage GPU times to `helixsr.log`.

## Limitations

- The setup needs an internet connection once (or a local DLSS 310.7.0 / 310.2 DLL). Other DLSS versions are refused.
- Direct3D 12 only. On its own, HelixSR replaces FSR 3.1 DLLs; other Direct3D 12 games need OptiScaler (above). Vulkan games are not supported.
- For AMD RDNA 1 and newer. Tuned for and tested on the BC-250 (gfx1013) with Mesa RADV under Proton; Windows and other GPUs are untested.
- Images are computed with DLSS's network but are not bit-identical to NVIDIA's DLSS arithmetic: FP16 arithmetic and rounding differ. If a game shows specks or blocks, please open an issue with `helixsr.log` attached.

## Legal

HelixSR is an independent, unofficial project and is not affiliated with or endorsed by NVIDIA Corporation or Advanced Micro Devices, Inc.

DLSS and associated NVIDIA technologies are trademarks and intellectual property of NVIDIA Corporation. HelixSR runs NVIDIA's DLSS Model E neural network. **The HelixSR download contains none of NVIDIA's weights or code:** the setup downloads NVIDIA's DLSS DLL from NVIDIA's own GitHub (under NVIDIA's license, after asking you) or uses a copy you already have, and builds the network files from it on your PC. Those files remain NVIDIA's property, are not licensed under HelixSR's Apache License 2.0, and are for your own use: do not redistribute them.

FidelityFX and FSR are trademarks of Advanced Micro Devices, Inc. FidelityFX components used by HelixSR are subject to their respective AMD licenses; see `THIRD_PARTY_NOTICES.md`.

HelixSR's original source code is licensed under the Apache License 2.0. See `LICENSE` for details. This does not extend Apache-2.0 to NVIDIA's components described above. Source availability is separate from license scope; see [SOURCE.md](SOURCE.md).
