# FreeCAD + AutoMate 复现与演示指南

本文记录本项目在 Windows 上复现 deGravity/AutoMate、训练双侧 SB-GCN 多任务模型，并接入 FreeCAD 的完整过程。目标是让另一台电脑能够完成：

1. 构建并调试 FreeCAD；
2. 建立独立的 AutoMate AI 环境；
3. 从 5000 条配合数据生成 B-Rep 图缓存和训练索引；
4. 训练位置推荐与 8 类 Mate Type 模型；
5. 在 FreeCAD 中选择两个零件，一键得到 Top-5 推荐、配合类型，并自动 Placement。

> 本文描述的是当前仓库中的实验版本，不是原始 AutoMate 仓库开箱即用的功能。多任务训练、数据流水线、数值稳定修复、FreeCAD 推理桥接等代码均为本次复现新增或修改的内容。

## 1. 最省时间的公司演示路线

如果目的是在公司再次演示，而不是重新验证训练过程，建议直接复制以下内容：

- 整个修改后的 `FreeCAD` 工作区，或至少提交/复制本次修改后的源码；
- `aiModule/automate/runs/mate_multitask_v2/best.pt`；
- 演示用的两个 STEP 文件；
- `aiModule/automate/pixi.lock`；
- 如果公司电脑不能联网，提前复制 pixi 包缓存或整个 AI `.pixi` 环境。

然后只执行本文的第 3、4、12、13 节。这样无需下载 13 GB STEP 压缩包，也无需重新预处理和训练。

如果要完整证明“从数据到模型”的复现过程，再执行全部章节。

## 2. 已验证的软件与硬件环境

本次成功环境：

- Windows 10/11 x64；
- Visual Studio 2022 Community，安装“使用 C++ 的桌面开发”；
- VS Code；
- VS Code C/C++ Extension；
- pixi 0.48 或更高版本；
- NVIDIA RTX 2060 SUPER（约 8 GB 显存）；
- NVIDIA 驱动可运行 CUDA 11.8 runtime；
- FreeCAD 根环境使用仓库 `pixi.toml`；
- AutoMate AI 环境使用 Python 3.10、PyTorch 2.4、CUDA 11.8、PyG 2.6。

公司电脑先检查：

```powershell
git --version
pixi --version
cmake --version
ninja --version
nvidia-smi
```

Visual Studio 的具体 MSVC 小版本可以不同，但必须让 FreeCAD pixi/CMake 重新配置，不能携带另一台电脑生成的 `CMakeCache.txt`。

## 3. 工作区布局

本文假定 FreeCAD 位于：

```text
E:\FreeCAD
```

AutoMate 位于：

```text
E:\FreeCAD\aiModule\automate
```

换盘符时，大部分命令可以在对应目录执行；但以下文件中的绝对路径需要同步修改：

- `.vscode/launch.json`（如果不使用 `${workspaceFolder}` 则需修改）；
- `aiModule/automate/freecad_mate_prediction.py` 中的 `DEFAULT_PIXI`；
- 在 FreeCAD Python 控制台执行脚本时使用的路径。

注意：必须携带当前修改后的 AutoMate 目录或代码分支。仅重新克隆官方 deGravity/AutoMate，不包含本项目新增的训练和 FreeCAD 接入代码。

## 4. 构建 FreeCAD 和 VS Code 调试

在 FreeCAD 根目录执行：

```powershell
cd E:\FreeCAD
pixi install
pixi run configure
pixi run build-debug
pixi run install-debug
```

首次构建耗时较长。以后修改少量 C++ 文件通常只需：

```powershell
pixi run build-debug
pixi run install-debug
```

VS Code 已配置：

- `.vscode/tasks.json`：依次执行 `build-debug` 和 `install-debug`；
- `.vscode/launch.json`：用 `cppvsdbg` 启动 pixi 环境内的 `FreeCAD.exe`；
- F5 配置名称：`FreeCAD: launch debug`。

为防止 CMake Tools 自动把 Ninja 缓存改成 Visual Studio/MSBuild，`.vscode/settings.json` 必须包含：

```json
{
    "cmake.configureOnOpen": false,
    "cmake.configureOnEdit": false,
    "cmake.automaticReconfigure": false,
    "cmake.useCMakePresets": "always"
}
```

不要点击 CMake Tools 状态栏的手动 Configure。配置统一使用：

```powershell
pixi run configure
```

### 4.1 CMake 生成器冲突恢复

如果看到以下错误：

```text
generator Ninja does not match the generator used previously: Visual Studio 17 2022
MSB1009: 项目文件不存在
LibPack not found
```

说明 `build/debug` 的缓存被错误生成器污染。关闭正在运行的构建，只删除配置缓存（不必删除整个构建目录）：

```powershell
Remove-Item -LiteralPath E:\FreeCAD\build\debug\CMakeCache.txt -Force
Remove-Item -LiteralPath E:\FreeCAD\build\debug\CMakeFiles -Recurse -Force
pixi run configure
pixi run build-debug
pixi run install-debug
```

删除前确认路径确实是 `E:\FreeCAD\build\debug`。

### 4.2 MSVC `<format>` 错误

早期使用 MSVC 14.30 时，`std::format` 在 `/std:c++20` 下可能报：

```text
The contents of <format> are available only in c++latest mode with concepts support
```

更新 Visual Studio 2022/MSVC 后必须重新生成 CMake 缓存，让 CMake 使用新的编译器。不要继续使用旧缓存中的绝对编译器路径。

## 5. 建立独立 AutoMate AI pixi 环境

不要把 PyTorch/PyG 安装进 FreeCAD 自身的 pixi 环境。进入 AutoMate 目录：

```powershell
cd E:\FreeCAD\aiModule\automate
pixi install
pixi run check
```

期望输出包括：

```text
python=3.10.x
torch=2.4.x
torch_geometric=2.6.x
cuda_available=True
cuda_runtime=11.8
gpu=<本机显卡>
```

本项目 `pixi.toml` 锁定的关键依赖：

- Python `>=3.10,<3.11`；
- PyTorch `>=2.4,<2.5`；
- `pytorch-cuda=11.8.*`；
- PyG `>=2.6,<2.7`；
- `pytorch-scatter`；
- OCCT `>=7.8,<7.9`；
- pybind11、Eigen、CMake、Ninja。

## 6. 构建 AutoMate C++ STEP 特征提取扩展

AutoMate 的 Python 流水线依赖 `automate_cpp`。本项目已经修改 `CMakeLists.txt`，使用 OCCT 7.8，并对 `breploader v0.5` 应用兼容补丁。

在 AutoMate 目录执行：

```powershell
cd E:\FreeCAD\aiModule\automate
$aiPrefix = (pixi run python -c "import sys; print(sys.prefix)").Trim()

pixi run cmake -S . -B build-ai -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  "-DCMAKE_PREFIX_PATH=$aiPrefix\Library" `
  "-DPython_EXECUTABLE=$aiPrefix\python.exe" `
  "-DPython3_EXECUTABLE=$aiPrefix\python.exe"

pixi run cmake --build build-ai
pixi run check
```

期望生成：

```text
build-ai\automate_cpp.cp310-win_amd64.pyd
```

`automate/__init__.py` 会在 Python 3.10 环境自动把 `build-ai` 加入 `sys.path`，不需要手工复制 `.pyd`。

首次 CMake 配置需要从 GitHub 获取 `breploader`。公司网络受限时，应提前复制已经生成的 `build-ai`，或准备 GitHub 访问/代理。

## 7. 准备 AutoMate 数据集

数据来自原论文公开的 AutoMate Dataset。完整数据放在：

```text
aiModule\automate\dataset\
├── assemblies.parquet
├── mates.parquet
├── parts.parquet
├── config_encodings.json
├── step.zip
└── step\<part_id>.step
```

本次使用 STEP，不依赖商业 Parasolid kernel。必须将 `step.zip` 解压为 `dataset/step`，不能只保留压缩包。

当前数据规模约为：

- `step.zip`：约 13.2 GB；
- 解压后约 44.7 万个 STEP；
- `mates.parquet`：约 91 MB；
- `parts.parquet`：约 212 MB。

先做基本验证：

```powershell
pixi run validate-data
```

## 8. 审计 5000 条 Mate 数据

原始 Mate Type 分布严重不均衡，本次按论文比例抽取 5000 条：

| 类型 | 数量 |
|---|---:|
| FASTENED | 3110 |
| REVOLUTE | 635 |
| PLANAR | 590 |
| SLIDER | 265 |
| CYLINDRICAL | 255 |
| PARALLEL | 90 |
| BALL | 30 |
| PIN_SLOT | 25 |

执行：

```powershell
cd E:\FreeCAD\aiModule\automate
pixi run audit-mates-5000
```

完成标志：

```text
audit\mates_5000_stratified.jsonl
audit\mates_5000_stratified.summary.json
```

本次实测 5000/5000 读取成功，约 930 秒。FASTENED 的轴召回约 98.67%，轴线召回约 96.38%；原点召回偏低并不等同于模型无法训练，因为位置标签主要依赖轴线与候选 MCF 匹配。

## 9. 生成 B-Rep 图 `.pt` 缓存

执行批量预处理：

```powershell
pixi run preprocess-parts-5000
```

完成后验证：

```powershell
pixi run verify-cache-5000
```

成功标准：

```text
failures = 0
files = 1918
unique_parts = 1918
missing_expected_parts = 0
```

验证报告：

```text
dataset\cache\brep_graph_v2_5000\verification.json
```

本次结果：

- 1918 个唯一零件；
- 缓存约 466 MB；
- 146883 faces；
- 389392 edges；
- 251805 vertices；
- 4000029 个有效 MCF；
- 过滤 4100 个无效 MCF；
- 实测约 974 秒。

预处理具有缓存复用能力。意外中断后可再次运行同一命令，已成功生成的零件不会重复处理。

## 10. 生成并验证 v2 训练索引

执行：

```powershell
pixi run build-training-index-5000
pixi run verify-training-index-5000
```

输出目录：

```text
dataset\training\index_v2_5000\
├── train.jsonl
├── validation.jsonl
├── test.jsonl
├── rejected.jsonl
├── summary.json
└── verification.json
```

本次结果：

- 输入 5000；
- 接受 4698；
- 拒绝 302（候选未召回）；
- train 3582；
- validation 549；
- test 567；
- document leakage = 0；
- part leakage = 0；
- 三个 split 均包含全部 8 类 Mate Type。

必须看到 `verification.json` 中 `status` 为 `ok` 后才能训练。

## 11. 模型结构与本次复现范围

本次模型包含：

1. 两侧共享权重的 SB-GCN B-Rep 编码器；
2. 每个零件的 MCF 编码器；
3. 候选 MCF 配对打分头（location ranking）；
4. 8 类 Mate Type Head；
5. 联合损失：位置 BCE + 加权 Mate Type Cross Entropy。

8 类标签按以下顺序编码：

```text
0 BALL
1 CYLINDRICAL
2 FASTENED
3 PARALLEL
4 PIN_SLOT
5 PLANAR
6 REVOLUTE
7 SLIDER
```

位置任务不是直接回归 FreeCAD Placement，而是在两侧 MCF 的候选笛卡尔积中给真实配对排序。FreeCAD Placement 是后处理：令推荐原点重合、推荐轴反向对齐。

当前 MCF 只有轴和原点，因此绕轴旋转仍有一个自由度。脚本采用最小旋转解，这对轴对称/FASTENED 演示足够，但不能视为完整六自由度位姿回归。

### 11.1 数值稳定修复

完整 5000 条数据首次单 epoch 验证曾在 `NativeLayerNormBackward0` 出现 NaN 梯度。根因是个别合法 B-Rep 产生幅值很大但仍有限的 SB-GCN 激活，使 LayerNorm 反向方差累积溢出。

当前代码已在 v2 `normalize_graph_inputs=True` 路径中：

- 对输入实体特征做安全缩放；
- 在 LayerNorm 前按行使用停止梯度的最大绝对值缩放；
- 保留损失、梯度和梯度范数有限性检查；
- v1 冻结路径保持不变。

不要删除这些稳定性处理。

## 12. 训练前验证

先检查模型和 v1 兼容性：

```powershell
pixi run check-multitask-model
```

期望包含：

```text
mate_classes=8
v1_strict_checkpoint_load=OK
multitask_model_check=OK
```

再运行完整 1 epoch 验证，输出必须放到独立目录：

```powershell
pixi run python scripts/train_mate_model.py `
  --index-dir dataset/training/index_v2_5000 `
  --output-dir runs/mate_multitask_v2_benchmark `
  --epochs 1 `
  --batch-size 4 `
  --negative-count 15 `
  --graph-width 64 `
  --mcf-width 64 `
  --message-passing-steps 2
```

本机实测约 284 秒，无 NaN、无 OOM。单 epoch 使用 CosineAnnealing 时最终学习率显示 0 是正常的，此目录只用于验证，不作为正式模型。

## 13. 正式训练 30 epoch

前台训练命令：

```powershell
pixi run python scripts/train_mate_model.py `
  --index-dir dataset/training/index_v2_5000 `
  --output-dir runs/mate_multitask_v2 `
  --epochs 30 `
  --batch-size 4 `
  --negative-count 15 `
  --graph-width 64 `
  --mcf-width 64 `
  --message-passing-steps 2
```

如果演示电脑已经携带 `best.pt`，不要重新训练，也不要覆盖 `runs/mate_multitask_v2`。

训练产物：

```text
runs\mate_multitask_v2\
├── best.pt
├── last.pt
└── metrics.jsonl
```

完成判断：

```powershell
(Get-Content .\runs\mate_multitask_v2\metrics.jsonl).Count
Get-Content .\runs\mate_multitask_v2\metrics.jsonl -Tail 1
```

应分别显示 `30` 和最后一条 `"epoch": 29`。

本次训练出现明显过拟合：

- 最佳 checkpoint 是 epoch 0；
- epoch 29 的训练类型准确率约 98.21%；
- epoch 29 的验证类型准确率约 44.81%。

因此演示和测试必须使用 `best.pt`，不要使用 `last.pt`。继续增加 epoch 不能解决该问题。

## 14. 独立测试集评估

执行：

```powershell
pixi run evaluate-mate
```

`evaluate-mate` 必须同时指定：

```text
--checkpoint runs/mate_multitask_v2/best.pt
--index-dir dataset/training/index_v2_5000
```

如果漏掉 `--index-dir`，脚本默认会误用旧的 `index_v1`，样本数只有 144；完整 v2 test 必须显示 567 条。

输出：

```text
runs\mate_multitask_v2\evaluation_test.json
runs\mate_multitask_v2\confusion_matrix_test.png
```

本次完整测试结果：

| 指标 | 结果 |
|---|---:|
| Location Top-1 | 73.37% |
| Location Top-3 | 86.60% |
| Location Top-5 | 89.59% |
| Location MRR | 80.40% |
| Mate Type Accuracy | 45.50% |
| FASTENED Location Top-1 | 78.36% |
| FASTENED Location Top-5 | 95.74% |
| FASTENED Type Precision | 62.69% |
| FASTENED Type Recall | 55.08% |
| FASTENED Type F1 | 58.64% |

这足以作为 FASTENED 实验演示，但不应宣称所有 8 类均达到工程可用精度。

## 15. FreeCAD 演示用零件

推荐按 A、B 顺序导入以下两个 STEP：

```text
零件 A：
dataset\step\77d8b312ac9c0a93b5d520d8_4dd2c93877109655fde0cfed_606169f3ada9bf1edbb45319_default_jjlui.step

零件 B：
dataset\step\77d8b312ac9c0a93b5d520d8_4dd2c93877109655fde0cfed_6c8fc9df10d55e62a2f86971_default_jjieq.step
```

这对零件来自独立 test split。实际 FreeCAD 推理入口测试结果：

- A 有 1020 个 MCF；
- B 有 154 个 MCF；
- 候选对 157080；
- RTX 2060 SUPER 推理约 0.74 秒；
- Top-1 到 Top-5 均预测为 FASTENED；
- Top-1 类型置信度约 63.87%；
- Top-2 到 Top-5 类型置信度约 93.7%。

建议把这两个 STEP 单独复制到演示 U 盘，避免现场依赖完整 dataset。

## 16. 在 FreeCAD 中运行推理和自动 Placement

启动 FreeCAD，可使用根目录 F5，或：

```powershell
cd E:\FreeCAD
pixi run freecad
```

操作步骤：

1. 在同一文档中导入零件 A 和零件 B；
2. 在模型树中先单击 A；
3. 按住 Ctrl 再单击 B；
4. 保证恰好选择两个带 `Shape` 的对象；
5. 打开 `视图 -> 面板 -> Python 控制台`；
6. 执行：

```python
exec(open(r"E:\FreeCAD\aiModule\automate\freecad_mate_prediction.py", encoding="utf-8").read())
```

脚本内部会：

1. 把当前选择的 A、B 临时导出为 STEP；
2. 通过子进程调用独立 AI pixi 环境；
3. 使用 `runs/mate_multitask_v2/best.pt`；
4. 对所有 MCF 对分块打分；
5. 返回 Top-5 location 和 Mate Type；
6. 固定 A，将 B 自动移动到 rank 1；
7. 在树中建立 `AutoMate Top-5 predictions`；
8. 红色轴表示 A，蓝色轴表示 B；
9. 默认只显示 rank 1，可在树中切换其他 rank；
10. 为每个结果写入 `Score`、`Probability`、`MateType`、`MateTypeConfidence` 和 8 类概率。

当前脚本中：

```python
APPLY_BEST = True
ALIGN_OPPOSITE = True
```

因此会自动 Placement：A 不动，B 的推荐原点与 A 重合，B 的推荐轴与 A 反向对齐。该操作在 FreeCAD transaction 中，按 `Ctrl+Z` 可以撤销。

如果只想看预测，不想移动零件，将 `APPLY_BEST` 改为 `False`。

### 16.1 推理结果的含义

- `Score`：位置配对头的原始 logit；
- `Probability`：对单个 logit 做 sigmoid 后的展示值，不是经过全候选校准的概率；
- `MateType`：8 类中 softmax 最大的类别；
- `MateTypeConfidence`：类型 softmax 概率；
- `CandidateIndex`：AutoMate 重新读取临时 STEP 后得到的 MCF 编号；
- `Placement`：可视化轴对象自身的 Placement，不是模型直接回归的零件 Placement。

不要仅凭 `Probability` 接近 1 就认为预测绝对可靠，应同时看排名、几何位置和类型置信度。

## 17. 当前尚未完成的 FreeCAD Joint 功能

现在已经实现自动 Placement，但尚未自动创建 FreeCAD Assembly Joint。

理论映射：

| AutoMate | FreeCAD Assembly Joint |
|---|---|
| FASTENED | Fixed |
| REVOLUTE | Revolute |
| CYLINDRICAL | Cylindrical |
| SLIDER | Slider |
| BALL | Ball |
| PARALLEL | Parallel |
| PLANAR | 需要 Fixed 或组合约束近似 |
| PIN_SLOT | 需要组合约束近似 |

模型能够给出 MCF 的轴参考和原点参考，但不能直接把内部拓扑编号当成 FreeCAD 的 `FaceN/EdgeN/VertexN`。FreeCAD 导出 STEP、AutoMate 再导入后，拓扑顺序可能改变。可靠实现必须按几何匹配：

- 平面：中心与法向；
- 圆柱面：轴线、半径和中心；
- 圆边：圆心、轴线和半径；
- 顶点：位置。

匹配得到真实 FreeCAD 子元素后，才能安全调用 `JointObject.Joint` 和 `setJointConnectors`。实验下一阶段建议只实现 `FASTENED -> Fixed Joint`。

## 18. 常见故障排查

### 18.1 `NameError: __file__ is not defined`

FreeCAD Python 控制台使用 `exec` 时可能没有 `__file__`。当前脚本已经使用 fallback：

```python
SCRIPT_PATH = globals().get("__file__", r"E:\FreeCAD\aiModule\automate\freecad_mate_prediction.py")
```

如果移动目录，修改该 fallback。

### 18.2 F5 启动后 `0xc0000135`

这是 Windows DLL 搜索路径错误。`launch.json` 中必须把 FreeCAD pixi 环境以下目录放入 `PATH`：

```text
.pixi\envs\default
.pixi\envs\default\Library\bin
.pixi\envs\default\Library\usr\bin
.pixi\envs\default\Scripts
```

### 18.3 断点显示未加载符号

确认：

- 使用当前源码重新 `build-debug` 和 `install-debug`；
- 启动的是 `.pixi/envs/default/Library/bin/FreeCAD.exe`；
- PDB 来自当前 `build/debug`；
- 断点代码确实进入当前执行路径；
- 不要在第三方无 PDB 的 Qt/fmt DLL 中期待符号。

### 18.4 `automate_cpp` 无法导入

执行：

```powershell
cd E:\FreeCAD\aiModule\automate
pixi run python -c "import automate_cpp; print(automate_cpp)"
```

如果失败，检查：

- `build-ai/automate_cpp.cp310-win_amd64.pyd` 是否存在；
- 是否在 AutoMate AI pixi 的 Python 3.10 下运行；
- OCCT DLL 是否位于 AI 环境 `Library/bin`；
- 是否错误地拿 Python 3.10 `.pyd` 给 FreeCAD Python 3.11 直接导入。

FreeCAD 主推理脚本不会在 FreeCAD 进程中导入 PyTorch/AutoMate，而是调用 AI pixi 子进程，因此避免了 Python 3.10/3.11 ABI 冲突。

### 18.5 推理结果位置不合理

检查：

- 选择顺序是否为 A 后 B；
- 是否选中了容器/Assembly 而不是两个真实 Shape；
- 是否重复运行后又手工移动了可视化轴；
- 是否使用 `best.pt` 而非 `last.pt`；
- 两个零件是否属于模型见过的几何分布；
- MCF 只有轴和原点，绕轴角度本来就不唯一。

### 18.6 为什么很多类别预测不准

数据中 FASTENED 占约 62.1%，BALL 和 PIN_SLOT 极少。当前实验优先保证 FASTENED 可演示，没有继续做强数据增强、类别专用 head、hard-example mining 或几何等价标签去重。测试集 Mate Type 总准确率约 45.5% 是当前模型真实水平。

## 19. 演示前检查清单

在离开开发环境前逐项确认：

- [ ] 公司电脑已安装 VS 2022 C++、VS Code、pixi、NVIDIA 驱动；
- [ ] 当前修改后的源码已复制，不是纯官方 AutoMate；
- [ ] `pixi.lock` 已复制；
- [ ] `build-ai/automate_cpp.cp310-win_amd64.pyd` 存在，或能在公司重新构建；
- [ ] `runs/mate_multitask_v2/best.pt` 已复制；
- [ ] 两个演示 STEP 已单独复制；
- [ ] `pixi run check` 显示 CUDA 可用；
- [ ] `pixi run check-multitask-model` 通过；
- [ ] 用命令行 `scripts/infer_mate.py` 跑过两个演示 STEP；
- [ ] FreeCAD 能通过 F5 或 `pixi run freecad` 启动；
- [ ] CMake Tools 自动配置已关闭；
- [ ] FreeCAD Python 控制台脚本路径已改成公司电脑实际路径；
- [ ] 自动 Placement 后已验证 `Ctrl+Z` 能撤销；
- [ ] 准备一份录屏或截图作为现场环境异常时的备份。

## 20. 关键文件索引

```text
aiModule/automate/
├── automate/brep.py                  # B-Rep -> PyG 图
├── automate/sbgcn.py                 # SB-GCN
├── automate/mate_dataset.py          # Dataset/DataLoader/负样本
├── automate/mate_model.py            # 双侧编码、位置头、类型头
├── audit_mates.py                    # 数据审计
├── preprocess_parts.py               # B-Rep 缓存
├── verify_graph_cache.py             # 缓存验证
├── build_training_index.py           # 无泄漏索引
├── verify_training_index.py          # 索引验证
├── scripts/train_mate_model.py        # 正式训练
├── scripts/evaluate_mate_model.py     # test 评估
├── scripts/infer_mate.py              # STEP 对推理
├── scripts/find_demo_pairs.py         # 演示样本筛选
├── freecad_mate_prediction.py         # FreeCAD 入口与 Placement
├── pixi.toml                          # AI 环境和任务
├── pixi.lock                          # 锁定依赖
└── runs/mate_multitask_v2/best.pt     # 正式演示模型
```

## 21. 推荐的现场演示话术边界

可以说明：

- 模型从两个零件的 B-Rep 图中编码几何与拓扑；
- 为两侧候选装配坐标系配对打分；
- 同时预测 8 类 Mate Type；
- FreeCAD 根据 rank 1 MCF 做自动 Placement；
- FASTENED 测试集位置 Top-1 约 78.36%，Top-5 约 95.74%。

应明确说明：

- 当前是研究复现实验，不是生产级装配求解器；
- Mate Type 总体准确率仍有限；
- 自动 Placement 尚未创建持久 Assembly Joint；
- 绕轴旋转未由当前 MCF 表达完全约束；
- 后续需要可靠的 MCF 到 FreeCAD Face/Edge 映射才能自动建立 Joint。

