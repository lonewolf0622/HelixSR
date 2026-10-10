# Third-party notices

## AMD FidelityFX — MIT

HelixSR includes a Lanczos weight approximation derived from AMD FidelityFX, under the
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

## NVIDIA DLSS Model E — not included

HelixSR runs NVIDIA's DLSS Model E network (its trained weights and its GPU kernels, translated to DirectX 12 compute
shaders). Since version 1.2.0 none of it is included in the HelixSR download: the setup downloads NVIDIA's DLSS DLL
from NVIDIA's own GitHub (github.com/NVIDIA/DLSS, under NVIDIA's license, after asking the user) or uses a copy the
user already has, and builds the network from it on the user's PC and appends it to HelixSR's DLL. These components remain the property of NVIDIA Corporation and are not covered by the licenses in this
repository; the resulting DLL is for the user's own use and must not be redistributed.

This attribution identifies third-party provenance; it is not a grant of
permission from NVIDIA and does not relicense NVIDIA-derived components.

## Tools downloaded by the setup — not included

The setup downloads, from their official sources and checked by SHA-256, and only on the user's PC:
- Microsoft DirectX Shader Compiler 1.9.2607 (github.com/microsoft/DirectXShaderCompiler), under Microsoft's
  licenses shipped with it;
- Python (python.org on Windows; on Linux the system's, or python-build-standalone) under the PSF license;
- numpy (PyPI) under the BSD license.

## License of HelixSR itself

HelixSR is licensed under the HelixSR Freeware License (`LICENSE`). Its setup scripts (`helixsr-setup.*` and the `setup`
folder) are licensed under the Apache License 2.0 (`LICENSE-APACHE-2.0`). The source code of the HelixSR DLL is not
published; see [SOURCE.md](SOURCE.md).

## OptiScaler — not included

HelixSR is used through OptiScaler (github.com/OptiScaler/OptiScaler, GPL-3.0), which must already be installed in the game. OptiScaler is a separate project under its own license; it is not part of the HelixSR download and HelixSR does not modify it.
