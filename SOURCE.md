# Source code

## HelixSR

The source code of the HelixSR DLL (`nvngx.dll`) is **not published**. HelixSR is free to use under the HelixSR
Freeware License (`LICENSE`). The setup scripts (`helixsr-setup.sh`, `helixsr-setup.ps1`, `helixsr-setup.bat` and the
`setup` folder) are licensed under the Apache License 2.0 (`LICENSE-APACHE-2.0`) and are plain text in every release.

## OptiScaler (folder `optiscaler`)

`optiscaler/OptiScaler.dll` is OptiScaler 0.9.4 (GNU GPL 3.0, `optiscaler/LICENSE-GPL-3.0.txt`) with one change: an
opt-in setting `[DLSS] ForceEnabled` that lets OptiScaler's DLSS backend run on a GPU that is not NVIDIA's.

- Upstream source: https://github.com/OptiScaler/OptiScaler, tag `v0.9.4` (commit 7534ad0)
- The change: `optiscaler/0001-dlss-force-enabled-v0.9.4.patch` (4 files, 20 lines)
- Complete source of this build and how it was built: https://github.com/lonewolf0622/OptiScaler-forceenabled
  (branch `v094-forceenabled-ci`, built with MSBuild by its GitHub Actions workflow)

`optiscaler/OptiScaler.ini` is OptiScaler 0.9.4's settings file with `Dx12Upscaler=dlss` and `ForceEnabled=true`.

## NVIDIA material

The HelixSR download contains no NVIDIA weights or NVIDIA-derived kernels. The setup builds the network on your PC
and adds it to the DLL; that network remains NVIDIA's property and must not be shared. No license in this download
extends to NVIDIA's components.

## Third-party components

The AMD FidelityFX-derived Lanczos weights retain their MIT license; see `THIRD_PARTY_NOTICES.md`.
