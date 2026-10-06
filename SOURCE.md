# Source availability

## Current publication status

The implementation source and build scripts for the HelixSR release DLL
are not currently included in this repository's `main` branch. The
checked-in material consists of documentation, license notices and
configuration. Those files alone are insufficient to rebuild the DLL.

The repository therefore is not a complete open-source distribution of
the DLL. The presence of `LICENSE` must not be taken to mean that its
implementation source has been published.

This page describes the current publication status. It does not announce
or promise a future source release.

## License scope

The project identifies its original HelixSR code as Apache-2.0 licensed.
That statement is distinct from whether the code is available to download.
The license text is retained in `LICENSE` without modification.

The release is described in the README and third-party notices as also
containing NVIDIA DLSS Model E weights and GPU kernels translated for
DirectX 12 execution. Those NVIDIA-derived components are excluded from
HelixSR's Apache-2.0 license.

The AMD FidelityFX API headers and RCAS sharpening components retain their
MIT license and notices, reproduced in `THIRD_PARTY_NOTICES.md`.

Neither this source-availability statement nor third-party attribution
provides additional permission to use or redistribute NVIDIA materials.

## Building from source

No complete build procedure can be provided using only the files currently
checked into `main`, because the implementation and build scripts are absent.
This documentation should not be treated as evidence of a reproducible
source release.
