Magpie experimental x64 build

This directory is updated in place. The ZIP name remains stable across builds.
Built-in update checking is temporarily disabled in 0.5.2-experimental.

Experimental effects:
- DLSS\DLSS_ZeroMV, DLSS_ZeroMV_Jitter, DLSS_OpticalFlow
- FSR2\FSR2_ZeroMV, FSR2_OpticalFlow
- FSR3\FSR3_ZeroMV, FSR3_OpticalFlow (upscaler only; no frame generation)
- XeSS\XeSS_ZeroMV, XeSS_OpticalFlow
- RTXVideo denoise and VSR quality levels
- DLSSFG 2x/3x/4x frame generation (do not combine with Smooth Motion)
- XeSSFG cross-vendor x2 frame generation and Intel Arc x2/x3/x4
  multi-frame generation (do not combine with Smooth Motion)

XeSS uses the cross-vendor XeSS 3.0.1 D3D12 DP4a path through D3D11/D3D12
shared resources. XeSS_OpticalFlow estimates motion on a 50% grid and supplies
low-resolution motion vectors plus flat depth. XeSS_ZeroMV explicitly supplies
zero motion vectors plus flat depth. It does not use the Intel Arc-only native
D3D11 path.

These colour-frame-only integrations do not have real engine depth, motion
vectors, reactive masks, or projection jitter. They are experimental and may
produce ghosting or unstable detail. See logs\magpie.log when reporting errors.

DLSSFG keeps CPU/GPU fence synchronization. Repeated evaluation failures fall
back to real captured frames and disable frame generation for that scaling
session instead of retrying forever.

Smooth Motion compatibility restarts preserve window, minimized, or tray state,
release shortcuts before replacement, and wait for the old process to exit.

Native upscalers and RTX Video are dispatched through NativeEffectBackend and
NativeEffectBackendFactory. Frame-generation presenters remain terminal stages.

This package includes build-manifest.json and THIRD-PARTY-NOTICES.md. The
manifest identifies the source commit, enabled optional backends, and hashes of
the packaged files. Third-party redistribution and GPL compatibility must be
reviewed before publishing a binary package.

Source fork: https://github.com/SAOG0721/Magpie
Upstream: https://github.com/Blinue/Magpie
