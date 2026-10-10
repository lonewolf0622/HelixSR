# Source code

## HelixSR

The source code of the HelixSR DLL (`amd_fidelityfx_dx12.dll`) is **not published**. HelixSR is free to use under the
HelixSR Freeware License (`LICENSE`). The setup scripts (`helixsr-setup.sh`, `helixsr-setup.ps1`,
`helixsr-setup.bat` and the `setup` folder) are licensed under the Apache License 2.0 (`LICENSE-APACHE-2.0`) and are
plain text in every release.

## OptiScaler (folder `optiscaler`)

`optiscaler/OptiScaler.dll` and `optiscaler/OptiScaler.ini` are the official, unmodified OptiScaler 0.9.4 release
(GNU GPL 3.0, `optiscaler/LICENSE-GPL-3.0.txt`), from https://github.com/OptiScaler/OptiScaler/releases/tag/v0.9.4
(`Optiscaler_0.9.4-final.20260718._MM.7z`). Source: https://github.com/OptiScaler/OptiScaler, tag `v0.9.4`.
`optiscaler/fakenvapi.dll` and `fakenvapi.ini` are from the same release package (fakenvapi, MIT,
`optiscaler/fakenvapi-LICENSE.txt`, https://github.com/FakeMichau/fakenvapi).

## AMD FSR (folder `amd-fsr`)

`amd-fsr/amd_fidelityfx_upscaler_dx12.dll` is AMD's FidelityFX upscaler DLL 4.1.1 (FSR 3.1.5 and FSR 4), unmodified,
from the same OptiScaler release package, redistributed in binary form under AMD's license
(`amd-fsr/FidelityFX_v2_LICENSE.md`). The setup copies it next to HelixSR as `amd_fidelityfx_upscaler_dx12.amd.dll`,
so OptiScaler can offer AMD's FSR next to HelixSR.

## NVIDIA material

The HelixSR download contains no NVIDIA weights or NVIDIA-derived kernels. The setup builds the network on your PC;
that network remains NVIDIA's property and must not be shared. No license in this download extends to NVIDIA's
components.

## Third-party components

The AMD FidelityFX-derived Lanczos weights retain their MIT license; see `THIRD_PARTY_NOTICES.md`.
