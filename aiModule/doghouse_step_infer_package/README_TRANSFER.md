# Doghouse STEP 推理迁移包

本目录用于复制到另一台 PC，继续开发“导入 STEP 模型并进行 doghouse / 安装面 / 安装孔推理”的流程。

## 目录内容

- `doghouse_ai/`：端到端 STEP 推理、FaceGraphGNN、装配特征提取、着色 STEP 导出、文档与测试。
- `doghouse_ai/checkpoints/`：
  - `doghouse_graph_pmae_7_plus_B126302301001.pt`：生产 doghouse 实例分割 checkpoint，需要 1152 维 PMAE face embedding。
  - `doghouse_graph_v1.pt`：纯 12 维几何面特征 baseline，不需要 PMAE。
  - `doghouse_instance_similarity_gallery_7_plus_B126302301001.npz`：实例整体相似性过滤 gallery。
- `doghouse_ai/structure_s200/checkpoints/doghouse_graph_s200_structure.pt`：结构角色 checkpoint，输出 `mount_prob` / `hole_wall_prob`，需要 1152 维 PMAE face embedding。
- `models/`、`utils/`、`extensions/chamfer_dist/`、`knn_cuda.py`、`extract_pmae_face_features.py`：Point-MAE face embedding 提取所需的最小代码。
- `requirements.txt`：Point-MAE 原始 Python 依赖参考。

## 重要缺失文件

当前工作区没有找到 Point-MAE 预训练权重：

```text
doghouse_ai/ckpt-last.pth
```

如果要在另一台 PC 上对任意新 STEP 运行最新 PMAE 生产 pipeline，需要把该权重补到：

```text
doghouse_step_infer_package/doghouse_ai/ckpt-last.pth
```

没有该文件时，可以先用 `doghouse_graph_v1.pt` 跑通 STEP 导入和纯面图推理，但不能复现当前最新的 PMAE + structure 双 checkpoint 效果。

## 环境要求

建议 Linux / Conda 环境。最低依赖：

- Python 3.10
- PyTorch
- NumPy
- PythonOCC / OCP 相关包，用于读取 STEP 和导出着色 STEP
- scikit-learn 不是主路径必须项，但部分评估/辅助脚本可能会用到
- Point-MAE 路径需要：
  - `timm==0.4.5`
  - `easydict`
  - `termcolor`
  - CUDA 相关扩展：`knn_cuda`、`pointnet2_ops` 或兼容实现、`extensions/chamfer_dist`

当前仓库内提供了 `knn_cuda.py` 这个兼容包装和 `extensions/chamfer_dist` 源码，但不同机器上可能仍需重新编译 CUDA 扩展。

## 推荐运行方式：最新双 checkpoint pipeline

适用于已经补齐 `doghouse_ai/ckpt-last.pth`，并且可以生成 PMAE face embedding 的机器。

```bash
cd doghouse_step_infer_package

python -u doghouse_ai/infer_from_step.py \
  --step "/path/to/model.step" \
  --output-dir outputs/model_run \
  --backbone graph \
  --checkpoint doghouse_ai/checkpoints/doghouse_graph_pmae_7_plus_B126302301001.pt \
  --structure-checkpoint doghouse_ai/structure_s200/checkpoints/doghouse_graph_s200_structure.pt \
  --pmae-face-emb-dir doghouse_ai/pmae_face_emb \
  --pmae-ckpt doghouse_ai/ckpt-last.pth \
  --extract-assembly-features \
  --use-vf2 \
  --assembly-output-step outputs/model_run/model_assembly_colored.step
```

输出文件：

- `{stem}_doghouse_infer.json`：STEP 几何导出。
- `{stem}_doghouse_points.npz`：面图推理输入。
- `{stem}_doghouse_pred_faces.json`：doghouse 实例 + `mount_prob` / `hole_wall_prob`。
- `{stem}_doghouse_assembly_features.json`：安装面 / 安装孔结果。
- `{stem}_assembly_colored.step`：着色 STEP，绿=安装面，蓝=孔壁，浅红=doghouse。

## 无 PMAE 权重时的 fallback

用于先验证 STEP 导入、面图推理、着色导出流程。该模式不会使用最新 structure checkpoint，因此安装面/孔结果可能不如当前 8 模型评估结果。

```bash
cd doghouse_step_infer_package

python -u doghouse_ai/infer_from_step.py \
  --step "/path/to/model.step" \
  --output-dir outputs/model_graph_v1 \
  --backbone graph \
  --checkpoint doghouse_ai/checkpoints/doghouse_graph_v1.pt \
  --no-instance-sim-filter \
  --extract-assembly-features \
  --use-vf2 \
  --no-prefer-ai-holes \
  --assembly-output-step outputs/model_graph_v1/model_assembly_colored.step
```

## 关键文档

- `doghouse_ai/PIPELINE.md`：完整 pipeline、训练/推理、当前算法和命令速查。
- `doghouse_ai/docs/doghouse_assembly_effect_report.md`：8 模型安装面/安装孔效果报告。
- `doghouse_ai/docs/superpowers/specs/2026-07-10-vf2-ai-structure-routing-design.md`：VF2 / AI structure 路由设计。
- `doghouse_ai/docs/superpowers/plans/2026-07-10-vf2-ai-structure-routing-plan.md`：实现历史和回归清单。

## 当前算法摘要

装配特征提取采用：

```text
VF2 可靠解析孔优先，否则 AI 结构路径
```

VF2 要压过 AI，必须满足：

- 孔组内存在 `cylinder/cone`，并能解析 `2 <= R <= 6`。
- 候选安装面或孔壁得到 AI 结构概率支持：
  - `mount_prob >= 0.35`，或
  - `hole_wall_prob >= 0.35`。

否则走 AI 结构路径，通过 `mount_prob`、`hole_wall_prob`、外侧程度和邻接拓扑选择安装面与孔壁，不依赖解析半径。

## 迁移后建议

1. 先用 fallback 命令确认 PythonOCC 可以读取 STEP、输出 JSON 和着色 STEP。
2. 补齐 `ckpt-last.pth` 和 CUDA 扩展后，再跑最新双 checkpoint pipeline。
3. 对新模型先查看 `{stem}_assembly_colored.step`，确认绿色安装面和蓝色孔壁。
4. 若要继续开发孔壁精度，优先处理 AI structure 路径中的 transition-face FP。
