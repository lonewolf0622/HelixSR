# Third-party notices

## AMD FidelityFX — MIT

HelixSR includes a Lanczos weight approximation and the RCAS sharpening filter derived from AMD FidelityFX, under the
MIT license below.

```
MIT License

Copyright © 2025 Advanced Micro Devices, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## AMD FidelityFX Super Resolution 1 (EASU) — MIT

The optional `UpscaleFilter = easu` final pass (Ultra Performance) uses EASU from
AMD FidelityFX Super Resolution 1 (the headers `ffx_a.h` and `ffx_fsr1.h`), under the MIT license below.

```
Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

## NVIDIA Image Scaling SDK (sharpening filter) — MIT

HelixSR's optional `nvidia` sharpener is NVSharpen from the NVIDIA Image Scaling SDK 1.0.3
(github.com/NVIDIAGameWorks/NVIDIAImageScaling), compiled into HelixSR's DLL; its texture load and store are routed
through HelixSR's functions. This is unrelated to DLSS and contains none of NVIDIA's DLSS network.

```
The MIT License(MIT)

Copyright(c) 2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## NVIDIA DLSS Model E — not included

HelixSR runs NVIDIA's DLSS Model E network (its trained weights and its GPU kernels, translated to DirectX 12 compute
shaders). Since version 1.2.0 none of it is included in the HelixSR download: the setup downloads NVIDIA's DLSS DLL
from NVIDIA's own GitHub (github.com/NVIDIA/DLSS, under NVIDIA's license, after asking the user) or uses a copy the
user already has, and builds the network from it on the user's PC and appends it to HelixSR's DLL. These components remain the property of NVIDIA Corporation and are not covered by the licenses in this
repository; the resulting DLL is for the user's own use and must not be redistributed.

This attribution identifies third-party provenance; it is not a grant of
permission from NVIDIA and does not relicense NVIDIA-derived components.

## AMD FidelityFX Super Resolution 4 network — not included

HLX FSR 4 runs AMD's FSR 4 upscaling network (its trained weights and its GPU shaders, rebuilt for HLX FSR 4's own
pipeline as DirectX 12 compute shaders). None of it is included in the HelixSR download: HLX FSR 4's DLL comes without
its data, and the setup builds it on the user's PC from AMD's FSR 4 driver file `amdxcffx64.dll` 2.4.0.4370 (checked
by SHA-256): the copy of an installed AMD Software 26.10.1 driver, or, after asking the user, read out of AMD's driver
package downloaded from AMD's own servers (only that file is kept, nothing is installed):
https://drivers.amd.com/drivers/whql-amd-software-adrenalin-edition-26.10.1-win11-b.exe
The data is appended to HLX FSR 4's DLLs in the HelixSR folder (`HLX-FSR4` and `helixsr-scaler/OptiScaler`).
AMD's driver file is under AMD's license that comes with AMD Software. These components remain the property of
Advanced Micro Devices, Inc. and are not covered by the licenses in this repository; the resulting DLLs are for the
user's own use and must not be redistributed.

This attribution identifies third-party provenance; it is not a grant of permission from AMD and does not relicense
AMD-derived components.

## HelixSR Scaler 2.0.0 (folder `helixsr-scaler`) — GPL-3.0

The `helixsr-scaler` folder is HelixSR Scaler 2.0.0, a separate program based on OptiScaler, as its own release builds
it (one folder for Windows and Linux: the files are the same), with two additions: HLX FSR 4's DLL as
`OptiScaler/amd_fidelityfx_upscaler_dx12.dll` (AMD's own kept next to it as `amd_fidelityfx_upscaler_dx12.amd.dll`)
and OptiPatcher (below). The setup copies it into the games you pick. HelixSR Scaler is licensed under the GNU General
Public License 3.0 (`helixsr-scaler/Licenses/OptiScaler_LICENSE.txt`); the third-party components it carries (AMD
FidelityFX, Intel XeSS, Microsoft DirectX Agility SDK) are under the licenses in `helixsr-scaler/Licenses`. Its
source code: see [SOURCE.md](SOURCE.md). The HelixSR Freeware License does not apply to HelixSR Scaler, and HelixSR's
own files contain none of its code.

## OptiPatcher — MIT

`helixsr-scaler/OptiScaler/plugins/OptiPatcher.asi` is OptiPatcher (https://github.com/optiscaler/OptiPatcher), built
unmodified from commit `ff789437bcd1d313312f5c7f786467d2652ad91b`; it lets supported games offer DLSS on GPUs
without it. MIT license: `helixsr-scaler/Licenses/OptiPatcher_LICENSE.txt`.

## Tools downloaded by the setup — not included

The setup downloads, from their official sources and checked by SHA-256, and only on the user's PC:
- Microsoft DirectX Shader Compiler 1.9.2607 and 1.9.2609 (github.com/microsoft/DirectXShaderCompiler), under
  Microsoft's licenses shipped with them;
- Python (python.org on Windows; on Linux the system's, or python-build-standalone) under the PSF license;
- numpy (PyPI) under the BSD license.

## Setup helper runtime

The setup's prebuilt helper (`setup/lib/model/launch_synth`, `launch_synth.exe`) is statically linked with the
LLVM C++ runtime (libc++, libc++abi, libunwind; Apache 2.0 with LLVM exceptions) and, on Linux, musl (MIT), built with
Zig (MIT); their licenses are in `setup/lib/model/runtime-licenses`.

## License of HelixSR itself

HelixSR, including HLX FSR 4's DLL (without AMD's data) and its setup scripts (`helixsr-setup.*`, `helixsr-uninstall.*` and the `setup`
folder, HLX FSR 4's builder `setup/hlxfsr4` included), is licensed
under the HelixSR Freeware License (`LICENSE`); setup scripts published up to 1.8.1 keep their Apache 2.0 license. The source code of the HelixSR
DLL is not published; see [SOURCE.md](SOURCE.md).
