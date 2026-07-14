Magpie experimental x64 build

This directory is updated in place. The ZIP name remains stable across builds.

Experimental effects:
- DLSS\DLSS_ZeroMV, DLSS_ZeroMV_Jitter, DLSS_OpticalFlow
- FSR2\FSR2_ZeroMV, FSR2_OpticalFlow
- FSR3\FSR3_ZeroMV, FSR3_OpticalFlow (upscaler only; no frame generation)
- XeSS\XeSS_ZeroMV, XeSS_OpticalFlow
- RTXVideo denoise and VSR quality levels

XeSS uses the cross-vendor XeSS 3.0.1 D3D12 DP4a path through D3D11/D3D12
shared resources. XeSS_OpticalFlow estimates motion on a 50% grid and supplies
low-resolution motion vectors plus flat depth. XeSS_ZeroMV explicitly supplies
zero motion vectors plus flat depth. It does not use the Intel Arc-only native
D3D11 path.

These colour-frame-only integrations do not have real engine depth, motion
vectors, reactive masks, or projection jitter. They are experimental and may
produce ghosting or unstable detail. See logs\magpie.log when reporting errors.

Source fork: https://github.com/SAOG0721/Magpie
Upstream: https://github.com/Blinue/Magpie
