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
  Windows &amp; Linux / Proton · Experimental release ·
  <a href="LICENSE">HelixSR Freeware License</a>
</p>

---

# HelixSR

**NVIDIA's DLSS Model E neural upscaler for compatible GPUs.**

HelixSR runs NVIDIA's DLSS Model E network as standard Direct3D 12 compute shaders, **without NVIDIA Tensor Cores, an NVIDIA driver, CUDA, or ROCm**. It behaves like DLSS: every quality mode from native (DLAA) to Ultra Performance, dynamic resolution.

The goal is to help gamers get better image quality from the hardware they already own, particularly older and lower-end GPUs.

HelixSR is an independent, unofficial project and is **not affiliated with or endorsed by NVIDIA or AMD**.

**Version 1.9.3 experimental.** For AMD RDNA 1 and newer GPUs (Radeon Vega: experimental; NVIDIA Pascal: should work, not fully tested; see below), on Linux (Proton) and Windows. Developed and tested on the AMD BC-250 (gfx1013, Linux, Mesa RADV); other GPUs and Windows are not tested by us.

**If 1.9.3 does not work for you, use [HelixSR 1.8.1](https://github.com/lonewolf0622/HelixSR/releases/tag/v1.8.1)** (and please open an issue with `helixsr.log`).

## New in 1.9.3

**Alternative frame generation is available again. Only HelixSR's experimental frame-generation provider is withdrawn while its startup issues are investigated.**

- FSR, XeSS and other supported Scaler FG providers remain selectable with their usual game/GPU requirements and runtime libraries.
- Updates preserve alternative FG input, output, enabled state and provider settings. An old HelixSR provider choice is redirected to FFX, without selecting the unavailable HelixSR engine.
- Keeps the 1.9.0 INT8 calibration and layer selection, Speed Boost Auto / On / Off, reactive-mask support and newer upscaling fixes from 1.9.2.
- Includes HelixSR Scaler 1.0.6 and the existing HelixSR Desktop 1.0.3 installers.

If you installed 1.9.2, select your preferred alternative again in Scaler's Frame Generation menu after updating; that release disabled those saved settings. HelixSR's own frame-generation engine is absent from the upscaling DLL and cannot be enabled with settings or leftover files.

## Install: HelixSR Desktop (recommended)

Download **HelixSR Desktop** from this release (Windows: the `.exe` installer; Linux / Steam Deck: the `.AppImage` or `.deb`) and run it. It sets HelixSR up on your PC once (a few minutes) and then installs it into your games with one button each, the right way for every game:

- **HelixSR Scaler** for most games: HelixSR through the game's own DLSS, FSR or XeSS option, with an in-game menu (Insert). Not for online games or games with anti-cheat.
- **FSR drop-in** for games with FSR 3.1 and anti-cheat: only the game's FSR file is replaced (see below).

It also updates and removes HelixSR per game, shows each game's log for bug reports, and puts back everything it replaced. The rest of this README is for the setup scripts in this zip, the way without the app.

## FSR 3.1 drop-in

This download installs HelixSR into **games with FSR 3.1** (also when they have DLSS): HelixSR replaces the game's FSR 3.1 DLL (`amd_fidelityfx_upscaler_dx12.dll`, or `amd_fidelityfx_dx12.dll` in games without it). The game's own file is kept as `*.original.dll`; FSR frame generation keeps working through it. In the game, select **AMD FSR**.

1. Download `HelixSR-1.9.3.zip` and extract it into a folder you keep (for example `C:\HelixSR` or your home folder).
2. Run the setup in that folder:
   - **Windows:** double-click `helixsr-setup.bat`
   - **Linux / Steam Deck:** `./helixsr-setup.sh` (in a terminal, or double-click and choose *Run in terminal*)
3. The first time, it asks to download NVIDIA's DLSS DLL and builds the upscaling network locally. Frame-generation components are not downloaded or built.
4. Close your games. The setup offers an FSR drop-in for games with FSR 3.1, or HelixSR Scaler for other supported games. Confirm the method for each game.
5. Start the game and select **AMD FSR** for an FSR drop-in, or **DLSS** for HelixSR Scaler (FSR / XeSS if DLSS is unavailable).

No launch options are needed.

**A game that is not listed** (not from Steam, or not detected): at the end the setup asks for more games: drag the game's folder into the window (or paste its path). You can also drop the folder onto `helixsr-setup.bat` (Windows) or pass it (`./helixsr-setup.sh /path/to/game`).

**By hand:** after the setup has run, the `fsr3-drop-in` folder holds HelixSR's DLL with the network inside, under both FSR 3.1 file names. Rename the game's `amd_fidelityfx_upscaler_dx12.dll` (or `amd_fidelityfx_dx12.dll`) to `*.original.dll` and copy the file of the same name from `fsr3-drop-in` next to it. To undo, delete HelixSR's file and rename the original back. `manual-install\README.txt` has the details, including how to point your own OptiScaler at these DLLs.

**Update:** extract the new version into a new folder and run its setup; for games that have HelixSR it asks *installed (FSR). Update? [f/r/n]*: Enter updates, **r** removes. If a game update replaced HelixSR, the setup keeps the game's new file as the backup.

**Installs of earlier versions** (the OptiScaler way or the DLSS loader) are recognized: in games with FSR 3.1 the setup offers to switch them to the drop-in, elsewhere to remove them. Either way everything they replaced (in the game's `HelixSR\backup` folder) is put back.

## Games without FSR 3.1

For games with DLSS, XeSS or FSR 2, use HelixSR Scaler through HelixSR Desktop or the setup scripts. It provides the upscaler and an Insert menu. HelixSR frame generation is not included. Alternative FG providers remain selectable in Scaler.

## Settings

In HelixSR Scaler, **Speed Boost** selects INT8 **Auto / On / Off**. Restart after changing it. **Reactive-mask support** remains available: untick **Disable Reactive Mask** in Scaler and restart; the game must supply a mask.

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
- **NVIDIA Pascal (GTX 10 series) (should work, not fully tested):** GTX 1050 – 1080 Ti and TITAN X / Xp have no DLSS of their own, and HelixSR should run on them. Not fully tested yet: please open an issue with `helixsr.log` and how it went.

## Setup details

- The setup downloads NVIDIA's DLSS 310.7.0 DLL from NVIDIA's GitHub ([github.com/NVIDIA/DLSS](https://github.com/NVIDIA/DLSS), NVIDIA's license applies) after asking, builds the network for your GPU (both 32- and 64-lane shaders; `--no-wave64` skips the 64-lane ones) into HelixSR's DLL (one file), and deletes NVIDIA's DLL again. Already have DLSS 310.7.0 (or 310.2) from a game? `--dlss <path to nvngx_dlss.dll>`.
- **Windows:** Python, numpy and the shader compiler are downloaded once into `%LOCALAPPDATA%\HelixSR`. No admin rights.
- **Linux:** needs Python 3 with numpy (the setup uses your system's, offers to install them, or downloads a portable Python into `~/.local/share/HelixSR` on read-only systems such as SteamOS and Bazzite). The shader compiler runs through Proton.
- Every download is pinned to an exact version and checked by SHA-256.
- **After the setup, HelixSR's DLLs (in this folder, `fsr3-drop-in` and your games) contain NVIDIA's networks: they are for your own PC, do not share or upload them.**

## How it works

**This is NVIDIA's network running on different hardware, not a newly trained model.** HelixSR answers the game exactly as DLSS would: DLSS's camera jitter (8 × scale² Halton positions), DLSS's render sizes per quality mode, the game's own sharpening request not applied (HelixSR's optional sharpening instead, see [Settings](#settings)), reactive masks ignored. In Ultra Performance the network outputs twice the render size and a final pass scales that to the screen (less shimmer, about 30% faster). Packed FP16 arithmetic makes the output close to, but not bit-identical with, NVIDIA's DLSS.

## Performance

Historical GPU time per upscaled frame for versions 1.8.1 and 1.9.0 on the **BC-250 at 2000 MHz** (upscaling pass only, not the whole game frame):

| Output | Mode | 1.8.1 | 1.9.0 |
|---|---|---:|---:|
| 1920x1080 | Quality (1280x720) | 1.14 ms | **1.06 ms** |
| 1920x804 | Quality | 0.93 ms | **0.86 ms** |
| 3840x2160 | Performance (1920x1080) | 3.52 ms | **3.34 ms** |

## Limitations

- Direct3D 12 games with FSR 3.1 only (others: HelixSR Desktop). The game's FSR frame generation keeps working.
- AMD RDNA 1 and newer. Radeon Vega: experimental, untested on real Vega hardware. NVIDIA Pascal (GTX 10 series): should work, not fully tested.
- Steam games and games from other stores' default folders are found automatically; other games: drag their folder into the setup, or see the generated `manual` folder.

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
