# Source code and licenses

The HelixSR DLL source is not published. The DLL and setup scripts are covered by
the HelixSR Freeware License in `LICENSE`. The download contains no NVIDIA network
weights or translated NVIDIA kernels; setup creates the upscaling network locally.
This release does not include HelixSR frame generation or its executor.

HelixSR Scaler is a separate GPL-3.0 program based on OptiScaler. Its DLL is the
unmodified CI build from commit 91df07708b2128a07674a5c832b8ff06c7448688. This package
withdraws only the HelixSR frame-generation provider; alternatives remain available. The GPL source is:
https://github.com/lonewolf0622/HelixSR-Scaler/tree/91df07708b2128a07674a5c832b8ff06c7448688

The HelixSR Freeware License does not apply to HelixSR Scaler. Its licenses are in
`helixsr-scaler/Licenses`. Other bundled third-party notices are in
`THIRD_PARTY_NOTICES.md`.
