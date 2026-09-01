# Magpie Experimental v0.6.0 使用说明

## 本版本更新

- DLSSNR 新增可选的输入分辨率调整功能。开启后可将 DLSSNR 的输入分辨率设置为原始分辨率的 25%–100%，以降低性能开销。
- 此功能默认关闭；滑块默认值为 100%，步进为 1%。关闭时保持原有的 DLSSNR 处理方式，且不显示输入分辨率滑块。
- 降低输入分辨率会降低 DLSSNR 的处理质量。DLSSNR 以调整后的分辨率处理颜色、运动和深度输入，随后将处理差值重建到原始分辨率并叠加回原始颜色。
- 调整了 DLSSNR 参数顺序：输入分辨率开关和滑块位于参数列表顶部；Automatic Mask 与 NR UI Correction 位于 Skin Structure Strength 和 Frame Guidance 之间。

## 应该下载哪个文件

### 主包：`Magpie-Experimental-x64.zip`

所有用户都需要下载。请完整解压后运行，不要直接在压缩包内运行，也不要只替换 `Magpie.exe`。

主包不包含可选 TensorRT 深度估算组件。未安装可选组件时，深度估算使用 DirectML。

### 可选 TensorRT 深度估算组件包

仅建议 RTX 40 系用户在深度估算失败或开销过大时使用。

组件包由两个分卷组成：

- `Magpie-v0.6.0-TensorRT-Depth-Components-x64.7z.001`
- `Magpie-v0.6.0-TensorRT-Depth-Components-x64.7z.002`

请下载全部分卷，将它们放在同一目录，并使用 7-Zip 从 `.7z.001` 开始解压。随后按照包内《安装说明.txt》安装。不要将其中的 CUDA、cuDNN 或 TensorRT 文件与其他版本混用。

### `DLSSNR-DLL-Options-310.8.0.0.zip`

用于在 NVIDIA 原版 DLL 与 RTX 40/50 社区兼容 DLL 之间切换。替换前请完全退出 Magpie，并按照包内说明操作。

## DLSSNR 输入分辨率调整

在 DLSSNR 参数列表顶部开启“Adjust Input Resolution (Reduces DLSSNR Quality)”后，可使用输入分辨率滑块选择 25%–100%。数值越低，DLSSNR 的性能开销通常越小，但画面细节、稳定性和降噪质量也可能随之下降。

建议先从较高比例开始逐步降低，并根据内容、输入分辨率、GPU 性能和可接受的画质损失选择合适数值。设置为 100% 时不会降低 DLSSNR 输入分辨率。

## NVIDIA VSR 错误 -2

如果看到“NVIDIA VSR 初始化失败（错误 -2）”，请将 Magpie 完整解压到符合要求的路径。Magpie 所在路径及所有上级目录不能包含中文或特殊字符，然后重新运行。

## 文件校验

发布附件的 SHA-256 校验值见本 Release 页面。
