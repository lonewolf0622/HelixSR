# Source code and licenses

The sources of HelixSR's DLL (`HelixSR/helixsr.dll`) and of HLX FSR 4's DLL (`HLX-FSR4/amd_fidelityfx_upscaler_dx12.dll`)
are not published. They and the setup scripts (HLX FSR 4's builder `setup/hlxfsr4` included) are covered by the
HelixSR Freeware License in `LICENSE`. The download contains no NVIDIA or AMD software, network weights or shaders
built from them; the setup gets NVIDIA's and AMD's files from them (or from the PC) and builds the network and HLX
FSR 4's shaders locally.

## HelixSR Scaler (folder `helixsr-scaler`)

HelixSR Scaler is a separate GPL-3.0 program based on OptiScaler. The `helixsr-scaler` folder is HelixSR Scaler 2.1.0,
built from https://github.com/lonewolf0622/HelixSR-Scaler at commit
`312e72f2f845ad5bcade917a39a39ea2c80aba70`
(https://github.com/lonewolf0622/HelixSR-Scaler/tree/312e72f2f845ad5bcade917a39a39ea2c80aba70); its license is
`helixsr-scaler/Licenses/OptiScaler_LICENSE.txt` (GNU GPL 3.0). The HelixSR Freeware License does not apply to it.
HelixSR 3.0 and 3.1 need HelixSR Scaler 2.0.0 or newer; the setup checks the version before it installs it.
