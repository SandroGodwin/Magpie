<p align="center">
  <img src="./src/Magpie/Icons/SVG/Magpie Icon Full Disabled.svg" width="150" height="150" alt="Magpie">
</p>
<h1 align="center">Magpie Experimental</h1>

<div align="center">

[![License](https://img.shields.io/github/license/Blinue/Magpie)](./LICENSE)

</div>

🌍 **English** | [简体中文](./README_ZH.md)

This is an unofficial experimental fork of [Blinue/Magpie](https://github.com/Blinue/Magpie). It focuses on captured-frame experiments with DLSS, DLSS Frame Generation, DLSSNR, XeSS, FSR, and RTX Video. It is not an official Magpie release and is not supported by the upstream project.

## Download and install

1. Download the latest `Magpie-Experimental-x64.zip` from [GitHub Releases](https://github.com/SAOG0721/Magpie/releases).
2. Fully exit any running Magpie instance.
3. Extract the complete ZIP into a new directory. Do not run it from inside the archive or overwrite an older experimental directory.
4. Run `Magpie.exe`.

Settings are normally stored in `%LOCALAPPDATA%\Magpie\config\v4\config.json`. Updating the program does not replace existing scaling modes.

## Default scaling modes

A fresh configuration includes the following modes and selects Lanczos by default:

| Mode | Main use | Hardware |
| --- | --- | --- |
| Lanczos | General spatial scaling | DirectX 11 GPU |
| FSR | General spatial upscaling and sharpening | DirectX 11 GPU |
| DLSS SR | Experimental DLSS super resolution | NVIDIA RTX |
| RTX Video VSR Ultra | Video, visual novels, and compressed images | NVIDIA RTX |
| DLSSFG | Experimental x2/x3/x4 frame generation | NVIDIA RTX |
| XeSSFG | Experimental cross-vendor x2 frame generation | Compatible Intel, NVIDIA, or AMD GPU |
| DLSSNR | Same-resolution SDR AI filter | Supported NVIDIA RTX runtime |

Users upgrading from an older build can import `ScalingModes-v0.5.7-experimental.json` from the Release on the Scaling Modes page. It only appends `DLSSFG`, `XeSSFG`, and `DLSSNR`; it does not remove or replace existing modes.

## Recommended configuration

### DLSS SR

- `Use Motion Vectors` defaults to on.
- `Use Estimated Depth (Experimental)` defaults to off. Enable it only when it visibly improves a specific application.
- Existing `DLSS\DLSS_ZeroMV` configurations migrate to `DLSS\DLSS_SR` while preserving their parameters and scaling type.

Magpie cannot access engine-native motion vectors, depth, exposure, camera matrices, or separated UI. Its inputs are estimated from captured color frames and are not equivalent to a native in-game DLSS integration.

### DLSS Frame Generation

- `Frame Multiplier` offers x2, x3, and x4, subject to GPU, driver, and presentation support.
- `Use Motion Vectors` defaults to on.
- `Use Estimated Depth (Experimental)` defaults to off.
- Do not combine DLSS FG with XeSS FG, NVIDIA Smooth Motion, or another frame generator.

x3/x4 require sufficient display refresh capacity. If the generated target exceeds the monitor refresh rate, not every generated frame can be displayed and final FPS will not simply equal input FPS multiplied by the selected factor.

### DLSSNR

DLSSNR is a same-resolution SDR post-process and does not upscale. It may affect both game content and composed text or UI. HDR is not currently supported.

| Parameter | Range | Default | Description |
| --- | --- | --- | --- |
| NR Preset | 0–3 | 0 | Default, Preset #1, #2, or #3 |
| NR Style | 0–2 | 0 | Default, Natural, or Cinematic |
| NR Intensity | 0–2 | 1 | Overall processing strength |
| Local Tone Strength | 0–2 | 1 | Local tone strength |
| Local Structure Strength | 0–2 | 1 | Local structure strength |
| Skin Structure Strength | -1–2 | -1 | -1 keeps the default behavior |
| Automatic Mask | 0/1 | 0 | Automatic masking, off by default |
| NR UI Correction | 0/1 | 0 | UI correction, off by default |
| Frame Guidance | 0–3 | 0 | Available, Force Zero, Motion Only, or Depth Only |
| Depth Inference Interval | 1–8 | 4 | Minimum real-frame interval for estimated depth |

`Frame Guidance=0 Available` and `Depth Inference Interval=4` are suitable defaults. A lower interval can be tested for faster motion, but it changes estimated-depth update frequency and GPU cost rather than the NR model itself.

### XeSS FG and RTX Video

XeSSFG currently uses the cross-vendor x2 Zero-MV path and does not consume the estimated Motion/Depth used by DLSS. RTX Video VSR Ultra has a relatively high GPU cost, so use it according to target resolution and available GPU headroom.

## DLSSNR DLL choices

The main Release Pack includes a community-modified `nvngx_dlssnr.dll` 310.8.0.0 intended for RTX 40-series and RTX 50-series compatibility. It is not an untouched NVIDIA-signed file, and Windows Authenticode reports a file-hash mismatch.

The same Release provides `DLSSNR-DLL-Options-310.8.0.0.zip`:

- `NVIDIA-Original`: the original NVIDIA-signed runtime.
- `Community-RTX40-RTX50`: the community compatibility runtime used by the main pack.

Fully exit Magpie before replacing `nvngx_dlssnr.dll` in the application directory. Third-party DLLs are not included in this source repository or GitHub-generated source archives.

## Temporary NGX OTA tool

`NGX_OTA_Switch.bat` addresses environments where many `nvngx_update.exe` processes accumulate. It can temporarily disable the system-wide NGX OTA setting, terminate existing updater processes, or remove the override to restore NVIDIA's default behavior.

The tool requires administrator access and affects other programs using NGX. Disabling OTA can prevent NVIDIA NGX components from updating online; restore the default setting when appropriate.

## Troubleshooting

- Logs are stored in `logs\magpie.log` under the application directory.
- Search for `DLSSNR STATUS` to verify DLSSNR creation and evaluation. NVIDIA's on-screen Indicator is not guaranteed to appear in every environment.
- If frame generation shows no image, confirm that no other frame generator is enabled and inspect the log for `DLSSFG` or `XeSSFG` errors.
- If an effect or DLL is missing, extract the complete Release again instead of copying only `Magpie.exe`.
- Include the Magpie version, GPU, driver, effect chain, input/output resolution, and log when reporting a problem.

## Requirements and license

- Windows 10 v1903+ or Windows 11
- DirectX feature level 11
- x64 system

The Magpie-derived source is licensed under [GPLv3](./LICENSE). Third-party SDKs, models, and runtimes remain under their respective licenses; see [Third-party components and redistribution](./docs/THIRD_PARTY_AND_REDISTRIBUTION.md).

- [Experimental Releases](https://github.com/SAOG0721/Magpie/releases)
- [Upstream Magpie](https://github.com/Blinue/Magpie)
- [Upstream FAQ](https://github.com/Blinue/Magpie/wiki/FAQ)
