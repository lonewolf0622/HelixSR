# HelixSR

**NVIDIA's DLSS Model E neural upscaler for AMD GPUs.**

HelixSR runs NVIDIA's DLSS Model E network as standard Direct3D 12 compute shaders, **without NVIDIA Tensor Cores, an NVIDIA driver, CUDA, or ROCm**. It behaves like DLSS: no sharpening, every quality mode from native (DLAA) to Ultra Performance, dynamic resolution.

The goal is to help gamers get better image quality from the hardware they already own, particularly older and lower-end GPUs.

HelixSR is an independent, unofficial project and is **not affiliated with or endorsed by NVIDIA, AMD or the OptiScaler project**.

**Version 1.8.1.** For AMD RDNA 1 and newer GPUs, on Linux (Proton) and Windows. Developed and tested on the AMD BC-250 (gfx1013, Linux, Mesa RADV); other GPUs and Windows are not tested by us.

**If 1.8.1 does not work for you, use [HelixSR 1.4.3](https://github.com/lonewolf0622/HelixSR/releases/tag/v1.4.3)** (and please open an issue with `helixsr.log`).

## Install

1. Download `HelixSR-1.8.1.zip` and extract it into a folder you keep (for example `C:\HelixSR` or your home folder).
2. Run the setup in that folder:
   - **Windows:** double-click `helixsr-setup.bat`
   - **Linux / Steam Deck:** `./helixsr-setup.sh` (in a terminal, or double-click and choose *Run in terminal*)
3. The first time, it asks to download NVIDIA's DLSS DLL (press Enter) and builds HelixSR's network from it (about a minute).
4. It asks for each game: *install? [f/o/n]*. Just press Enter. (**f** = FSR: pick **AMD FSR** in the game. **o** = OptiScaler: pick **DLSS** in the game. **n** = skip.) Close your games first. Games with DLSS also show **d** = DLSS without OptiScaler: **experimental**, never the default, read [the warning below](#experimental-dlss-without-optiscaler-d) before using it.
5. Start the game and pick the upscaler the setup told you at the end (**AMD FSR** or **DLSS**).

No launch options, no settings files.

**Update:** run the setup again. For games that have HelixSR it asks *installed (FSR). Update? [f/o/r/n]*: Enter updates it, **f** / **o** switches the way, **r** removes it.

**Uninstall:** run `helixsr-uninstall.bat` (Windows) or `./helixsr-uninstall.sh` (Linux). It removes HelixSR from the games you pick and puts back every file as it was; at the end it can also delete the built HelixSR DLL and the setup's downloads. A game you added by hand: drop its folder onto the uninstaller (Windows) or pass it (`./helixsr-uninstall.sh /path/to/game`).

## How HelixSR gets into a game

Two ways, plus one experimental way; the setup shows only the ones that work for a game (Enter = the first):

| The game has | What the setup does | Select in the game |
|---|---|---|
| **DLSS** (or XeSS / FSR 2): **o** | Installs the official [OptiScaler](https://github.com/OptiScaler/OptiScaler) 0.9.4 next to the game's `.exe` and points its FSR 3.1 backend at HelixSR (a `HelixSR` folder next to it). The game's DLSS inputs reach HelixSR. | **DLSS** |
| **FSR 3.1**: **f** (the default when the game has FSR 3.1) | Replaces the game's FSR 3.1 DLL with HelixSR (drop-in), as in 1.4.3; the game's file is kept as `*.original.dll`. No OptiScaler. | **AMD FSR** |
| **DLSS**: **d** — ⚠️ **EXPERIMENTAL** | Puts the HelixSR DLSS loader (`dxgi.dll`) and HelixSR (as `nvngx.dll`) next to the game's `.exe`: the game's own DLSS option runs HelixSR, no OptiScaler. **Read the section below first.** | **DLSS** |

**Switching a game to another way:** run the setup again and type **f**, **o** or **d** for that game. **Your own OptiScaler:** point its FSR 3.1 backend at the DLLs in `fsr3-drop-in` (`Dx12Upscaler = fsr31`, `FfxDx12Path` / `FfxDx12SRPath`); see `manual-install\README.txt`.

Everything the setup replaces is kept: in the game's `HelixSR\backup` folder (OptiScaler way) or as `*.original.dll`.

**A game that is not listed** (not from Steam, or not detected): at the end the setup asks for more games: drag the game's folder into the window (or paste its path) and answer the same question for it. You can also drop the folder onto `helixsr-setup.bat` (Windows) or pass it (`./helixsr-setup.sh /path/to/game`). By hand: see `manual-install\README.txt`, which the setup creates.

**FSR 3.1 drop-in:** after the setup has run, the `fsr3-drop-in` folder holds a ready HelixSR DLL with the network inside. For a game that has FSR 3.1, rename the game's `amd_fidelityfx_upscaler_dx12.dll` (or `amd_fidelityfx_dx12.dll`) to `*.original.dll` and copy the file of the same name from `fsr3-drop-in` next to it. Keep the `*.original.dll` (FSR frame generation runs through it). Select **AMD FSR** in the game. To undo, delete HelixSR's file and rename the original back.

**Updating HelixSR:** extract the new version into a new folder, run its setup and answer yes to update your games.

## Experimental: DLSS without OptiScaler (d)

> [!WARNING]
> **This option is EXPERIMENTAL.** It is new, it has been tested in very few games, and only on Linux (Proton). **It is not tested on Windows at all.** It may not work in your game, it may crash the game, and it may stop working after a game update. If you want something that just works, use **f** or **o**.

> [!CAUTION]
> **Do not use it in online games or games with anti-cheat.** The loader changes what the game sees (see below). Anti-cheat software can treat that as tampering, and that can get your account **banned**. Use it only in single-player games.

What it does (option **d** in the setup, for games that have DLSS):

- It puts two files next to the game's `.exe`: `dxgi.dll` (the HelixSR DLSS loader, small) and `nvngx.dll` (HelixSR). A `dxgi.dll` the game already had is moved to the game's `HelixSR\backup` folder and put back when you switch or remove.
- The game is told that the GPU is an NVIDIA GPU, so it offers DLSS. Because of that, **other NVIDIA-only options can show up in the game's menu (for example NVIDIA Reflex or DLSS Frame Generation): they do not work with HelixSR**, leave them off.
- The game's DLSS only loads a DLSS core that carries NVIDIA's signature. The loader answers that check with the game's own NVIDIA-signed DLSS file; the core that actually runs is HelixSR.
- Only DLSS super resolution. No frame generation.
- It cannot be used together with OptiScaler, ReShade or any other mod that is also a `dxgi.dll` next to the game's `.exe`.

**By hand** (same warnings apply), after the setup has run once: next to the game's `.exe`, copy `dlss-loader\dxgi.dll`, and copy `amd_fidelityfx_dx12.dll` (from the HelixSR folder, the network is inside it) and rename that copy to `nvngx.dll`. Keep a copy of the game's own `dxgi.dll` first if it has one. Start the game and select **DLSS**. To undo: delete the two files and put the game's `dxgi.dll` back. The same steps are in `manual-install\README.txt`.

If something goes wrong: run the setup again and pick **f** or **o** for the game (or **r** to remove HelixSR). Everything is put back as it was. To report a problem, open an issue and attach `helixsr_loader.log` and `helixsr.log` (both next to the game's `.exe`) and say which way you installed.

## Switching between HelixSR and AMD FSR / FSR 4

In games installed the OptiScaler way (**o**):

1. In the game, press **Insert** to open OptiScaler's menu.
2. **Upscaling** tab → **Upscalers** → **FFX Upscaler** list.
3. Pick **FSR HelixSR (3.1.5)** for HelixSR (the default), or AMD's **FSR 3.1.5** / **FSR 4.x**. It switches right away.

FSR 4 is in that list only on GPUs AMD's DLL supports it on, or with your own FSR 4 DLL (below).

Games installed the FSR way (**f**) have no menu: HelixSR always runs there. To use the game's own FSR again, run the setup and remove HelixSR (`r`).

**Your own FSR DLL (for example FSR 4):**
- **One game:** put the DLL into the `my-fsr` folder inside the game's `HelixSR` folder (next to the game's `.exe`) and start the game again. No setup needed. Delete it to go back.
- **All games:** put the DLL into the `my-fsr` folder of this HelixSR download, run the setup again and press Enter for your games. Take it out and run the setup again to go back.

Either way it is listed in the **FFX Upscaler** list by its version (for example **FSR 4.1.1**), next to HelixSR.

## Sharpening

Off by default, as DLSS 4 (DLSS does not sharpen). To turn it on, put a `helixsr.ini` next to HelixSR's DLL (in the game's `HelixSR` folder, or next to the game's FSR DLL) with:

```
[Sharpening]
Sharpener = nvidia
Sharpness = 0.5
```

`Sharpener`: `off`, `nvidia` (NVIDIA's sharpening filter, as the driver's Image Sharpening) or `rcas` (AMD FSR's). `Sharpness`: 0.0 – 1.0 (0.5 = NVIDIA's default). `UseGameSlider = true` follows the game's sharpness slider when it has one.

## Troubleshooting

- **Red border around the image, low resolution, shaking image:** HelixSR's network is not running. `helixsr.log` (in the game's `HelixSR` folder, or next to the game's FSR DLL) says why. Run the setup again and let it download NVIDIA's DLL.
- **DLSS is not offered in a game:** select FSR or XeSS instead if the game has them (OptiScaler hands them to HelixSR too), or open an issue with `OptiScaler.log` (next to the game's `.exe`).
- **The game crashes or DLSS is missing with the experimental way (d):** switch the game back to **f** or **o** in the setup. Reports are welcome (`helixsr_loader.log`, `helixsr.log`), but it is experimental: expect problems.
- **Something else looks wrong:** open an issue with `helixsr.log` and, for the OptiScaler way, `OptiScaler.log`.

## Setup details

- The setup downloads NVIDIA's DLSS 310.7.0 DLL from NVIDIA's GitHub ([github.com/NVIDIA/DLSS](https://github.com/NVIDIA/DLSS), NVIDIA's license applies) after asking, builds the network for your GPU (both 32- and 64-lane shaders, so the DLL can use either; `--no-wave64` skips the 64-lane ones) into HelixSR's DLL (one file: no separate weights or kernel files are left over), and deletes NVIDIA's DLL again. Already have DLSS 310.7.0 (or 310.2) from a game? `--dlss <path to nvngx_dlss.dll>`.
- **Windows:** Python, numpy and the shader compiler are downloaded once into `%LOCALAPPDATA%\HelixSR`. No admin rights.
- **Linux:** needs Python 3 with numpy (the setup uses your system's, offers to install them, or downloads a portable Python into `~/.local/share/HelixSR` on read-only systems such as SteamOS and Bazzite). The shader compiler runs through Proton.
- Every download is pinned to an exact version and checked by SHA-256.
- **After the setup, HelixSR's DLLs (in this folder, `fsr3-drop-in` and your games) contain NVIDIA's network: they are for your own PC, do not share or upload them.**

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

- Direct3D 12 games only. Only super resolution, no frame generation of its own. FSR frame generation (the game's, or OptiScaler's) keeps working: the setup puts AMD's frame generation DLL next to HelixSR.
- AMD RDNA 1 and newer. Radeon Vega GPUs (Vega 56/64, Radeon VII, Vega graphics in Ryzen APUs) are not supported: they have no wave32.
- Steam games are found automatically; other games through `manual-install`.
- The DLSS way without OptiScaler (**d**) is experimental: few games tested, Linux (Proton) only, not for games with anti-cheat.

## License and legal

HelixSR is free to use under the **HelixSR Freeware License** (`LICENSE`); its source code is not published. This includes HelixSR's experimental DLSS loader (the `dlss-loader` folder). The setup scripts (`helixsr-setup.*`, `helixsr-uninstall.*` and the `setup` folder) are licensed under the Apache License 2.0 (`LICENSE-APACHE-2.0`). The `optiscaler` folder holds the official, unmodified OptiScaler 0.9.4 (GNU GPL 3.0) and fakenvapi (MIT); the `amd-fsr` folder holds AMD's FidelityFX upscaler and frame generation DLLs 4.1.1 under AMD's license (`amd-fsr/FidelityFX_v2_LICENSE.md`); see [SOURCE.md](SOURCE.md). HelixSR contains a small amount of AMD FidelityFX material under the MIT license; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

DLSS and associated NVIDIA technologies are trademarks and intellectual property of NVIDIA Corporation. **The HelixSR download contains none of NVIDIA's weights or code:** the setup downloads NVIDIA's DLSS DLL from NVIDIA's own GitHub (under NVIDIA's license, after asking you) or uses a copy you already have, and builds the network from it on your PC. That network remains NVIDIA's property and is not covered by any license in this download; do not redistribute the DLLs the setup builds.
