# Doghouse AI 流程总览

> 本文档总结 `doghouse_ai` 子模块的当前主要流程、数据结构与关键算法，为后续迭代做准备。
>
> conda 环境：当前 Linux 工作区使用 `llm`（含 PyTorch + PythonOCC）；旧 Windows/PowerShell 示例中的 `uv01` 仅作历史参考。所有涉及 STEP 几何读取或着色 STEP 导出的脚本都需在含 PythonOCC 的环境运行。

---

## 1. 目标

从一个 CAD B-Rep（STEP）模型中，自动识别出：

1. **Doghouse（卡扣安装柱）实例**：每个 doghouse 的面集合（实例级分割）。
2. 每个 doghouse 上的 **安装面（mount face）**：卡扣/紧固件贴合的主体平面。
3. 每个 doghouse 上的 **安装孔（mounting hole）**：通孔孔壁面组（含"一小开口"的部分圆柱）。

最终产物用于下游装配（`recommend_and_assemble.py` / hybrid 装配）。

---

## 2. 整体流程

```
STEP 文件
  │
  ├─► step_geometry.build_geometry_from_step
  │
  └─► 自研 CAD C++ 内核报告 JSON
      例如: pillar_report.json["doghouse"]
      要求: doghouse_inference_geometry.v1 字段兼容
  │
  ▼
几何 JSON  (doghouse_inference_geometry.v1)
  │        每面: 类型/面积/质心/bbox/法向/半径/u_range/v_range/采样点 + 面邻接边
  │
  ▼  build_point_dataset.build_dataset
点云数据集 .npz  (points / features / face_idx / adjacency / [标签])
  │
  ├─► Point-MAE 冻结编码器提取逐面 embedding (可选)
  │
  ▼  train_graph.py → FaceGraphGNN（生产）；结构角色头可选训练
  │
  ▼  infer_graph / infer_from_step（默认 graph）
面级 doghouse 概率 + AI 实例分割
  │
  ├─► 可选 structure checkpoint 仅补充 mount_prob / hole_wall_prob
  │
  ▼  pipeline_defaults.apply_graph_postprocess
min-instance-faces 过滤 + 实例整体相似性 gallery 过滤（生产默认开启）
  │
  ▼  doghouse_assembly_features.extract_assembly_features
每实例: VF2 可靠解析孔优先，否则 AI 结构路径输出安装面 + 安装孔
  │
  ▼  doghouse_assembly_features.export_assembly_colored_step
着色 STEP (绿=安装面, 蓝=孔壁, 浅红=doghouse) 供目视核对
```

**一键端到端脚本**：`infer_from_step.py`（STEP → 几何 → 点云 → 模型 → 后处理 →〔可选〕装配特征 + 着色 STEP）。生产默认只需 `--step <file.step>`；权重与 gallery 由 `pipeline_defaults.py` 解析。

---

## 3. 阶段详解

### 3.1 几何导出 — `step_geometry.py`
- `build_geometry_from_step(step_path, sample_points_per_face=64)` → `doghouse_inference_geometry.v1`。
- 用 PythonOCC 遍历所有面（`TopExp_Explorer(TopAbs_FACE)`），面索引 = 遍历顺序（0-based）。
- 每面字段：`face_type`（plane/cylinder/cone/sphere/torus/bspline/bezier/other）、`area`、`centroid`、`bbox`、`normal`（已按 `TopAbs_REVERSED` 翻转）、`radius`、`semi_angle_deg`、`has_radius`、`u_range`、`v_range`、`sample_points`（UV 网格采样）。
- `adjacency_edges`：`{a, b, edge_type}`，基于共享边的两面（`TopologyExplorer.faces_from_edge`）。
- **重要**：此处的面索引与 `recommend_and_assemble.load_step` / `doghouse_assembly_features` 中的 `TopologyExplorer` 面列表**一致**，全流程可用同一 `face_idx` 对齐。

### 3.1b 自研 CAD C++ 内核 JSON 输入（2026-07-09）
- 自研 CAD 内核输出的报告 JSON 可以作为几何输入，只要其中包含兼容的 `doghouse_inference_geometry.v1` 段。当前 `pillar_report.json["doghouse"]` 已包含：
  - `faces[*].face_idx / face_type / area / centroid / bbox / normal / radius / semi_angle_deg / has_radius / u_range / v_range / sample_points`
  - `adjacency_edges[*].a / b / edge_type`
  - `num_faces / index_base / sample_points_per_face`
- **推理用途**：可以直接跳过 PythonOCC 的 `step_geometry.build_geometry_from_step`，从 `report["doghouse"]` 构建 `.npz` / 面图，再进入 **FaceGraphGNN** 推理（生产默认）。
- **训练用途**：几何 JSON 只提供输入特征；仍需合并人工标注 JSON，生成 `doghouse / mount / hole_wall / hard_negative` 等监督标签。
- **索引约束**：AI 管线使用 0-based `face_idx`。CAD 内核自己的稳定 `id` 可保留作溯源，但训练、推理、评估和着色 STEP 必须使用同一套 `face_idx` 顺序。
- **风险点**：如果 C++ 内核和 PythonOCC 的面遍历顺序不同，不能直接拿旧标注或 STEP 着色结果对齐；需要建立 `face_idx` 映射或让标注也基于 C++ JSON 的面序生成。

### 3.2 数据集构建 — `build_point_dataset.py` + `labels.py`
- `build_dataset(geometry_json, label_json, model_idx=0)` → `.npz`。
- 每面把采样点展开成点级样本，附 12 维 **逐面特征**（见 §5）。
- 标签（训练用，推理时传空标签 `EMPTY_LABELS`）：
  - 优先读 `face_labels`（逐面 role + instance_id）。
  - 无 `face_labels` 时回退读 `doghouse_instances` 内的 `mount_faces` / `hole_wall_faces` / … 字段。
  - 角色映射见 `labels.ROLE_TO_ID`（background/doghouse/mount/hole_wall/hole_bottom/transition/root_boundary + 各类 hard negative）。
- `build_multi_dataset`：多模型合并训练，做 `face_idx` / `adjacency` 全局偏移，保留 `local_face_idx` / `model_idx` / `face_offsets` / `model_names`。

### 3.3 训练 — `train_graph.py`（**当前生产**）

**doghouse 检测与实例分割的训练入口是 `train_graph.py`，不是 PointMLP。**

- **模型**：`FaceGraphGNN`（`graph_model.py`）— 4 层 GraphSAGE 式消息传递，节点头预测 `P(doghouse)`，对称边头预测 `P(同一实例)`；可选结构头预测 `P(mount)` 与 `P(hole_wall)`。
- **数据**：`prepare_graph_data.py` 从 `(STEP, annotation.json)` 生成 `*_graph.npz`（面特征 + 邻接 + 边标签）；可选 `--pmae-dir` 拼接 Point-MAE 逐面 1152 维 embedding。
- **损失**：节点 `BCE(pos_weight)` + 边 `BCE(pos_weight)`；结构训练时额外启用 `--hole-wall-loss-weight` 与 `--mount-loss-weight`，分别训练二分类 `hole_wall_head` / `mount_head`。
- **当前发布权重**：
  - 生产：`checkpoints/doghouse_graph_pmae_7_plus_B126302301001.pt`（混合 PMAE，7 模型 + B12-301）
  - 结构角色：`structure_s200/checkpoints/doghouse_graph_s200_structure.pt`（仅用于补充 `mount_prob` / `hole_wall_prob`，不用于替代生产实例分割）
  - 对照：`checkpoints/doghouse_graph_v1.pt`（纯 12 维面特征，无 PMAE）
- **训练命令示例**：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/train_graph.py \
  --prepare --data-dir doghouse_ai/data/graph_train \
  --step-dir "doghouse_ai/step - 副本2"

conda run --no-capture-output -n llm python -u doghouse_ai/train_graph.py \
  --data-dir doghouse_ai/data/graph_train \
  --pmae-dir doghouse_ai/pmae_face_emb \
  --output doghouse_ai/checkpoints/doghouse_graph_pmae_7_plus_B126302301001.pt
```

结构角色头训练示例：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/train_graph.py \
  --data-dir doghouse_ai/structure_s200/train_npz \
  --pmae-dir doghouse_ai/pmae_face_emb \
  --hole-wall-loss-weight 1.0 \
  --mount-loss-weight 1.0 \
  --output doghouse_ai/structure_s200/checkpoints/doghouse_graph_s200_structure.pt
```

推理与后处理细节见 §3.4b / §3.4c。

### 3.3b 遗留基线 — `train_pointnet_torch.py`（**已弃用，勿用于生产**）

早期轻量基线：`PointMLP`（逐点 MLP，`in_dim = 3(xyz) + 12(feat) = 15`），二分类 doghouse/background。

- 输入归一化：xyz 去中心+尺度归一；area/radius/u/v/bbox 做 log/缩放。
- `BCEWithLogitsLoss` + 正类 `pos_weight`。
- checkpoint 格式：`{model_state, in_dim, num_points}`；历史产物 `pillar_pointmlp.pt` 等**已不在仓库**。
- **弃用原因**：逐点独立、无邻域上下文 → 面 mask 破碎，实例只能靠连通组件 + 几何参数补救，泛化差（见 §3.4b 动机）。
- **代码仍保留**：`infer_from_step.py --backbone pointmlp --checkpoint <旧权重>` 可跑旧链路；默认 `--backbone graph`。

### 3.4 遗留推理 — `infer_pointnet_torch.py` / `face_vote.py`（**已弃用**）
- 逐点前向 → sigmoid 概率 → `>=0.5` 得 0/1。
- `vote_faces`：按 `face_idx` 聚合点预测（多数投票），得逐面 `doghouse` / `role` / `instance_id`。
- **实例恢复**：二分类模型无 instance head，`_component_instance_ids` 用 CAD 面邻接把 doghouse mask 拆成**连通组件**，`min_component_faces` 过滤碎片，按面数降序稳定编号 → `doghouse_instances`。

- 仅在与 `--backbone pointmlp` 配合时使用；生产默认走 `infer_graph.py`。

### 3.4b 面图 GNN — 推理与训练细节（2026-07-09，**推荐 / 与 §3.3 同一模型**）
- **动机**：`PointMLP` 逐点独立、无邻域上下文，边界面判断摇摆 → mask 破碎，只能靠 `min_component_faces` / 闭运算等**几何参数**补救，换模型即失效（泛化差）。
- **方案**：直接在 **CAD 面图**上做 GNN，实例分离交由 AI，不用点云投票。
  - `prepare_graph_data.py`：`(STEP, annotation.json)` → per-model `*_graph.npz`（复用 `build_dataset` 的 `face_features` / `face_instance` / `adjacency`），并派生**边标签** `edge_labels`：边 `(a,b)` 为正 ⇔ `face_instance[a] > 0 且 == face_instance[b]`。
  - `graph_model.py`：`FaceGraphGNN`（纯 PyTorch GraphSAGE 式消息传递，`index_add_` 均值聚合，无需 torch_geometric），4 层、hidden 128、LayerNorm+dropout。
    - **节点头** → `P(doghouse)`（该面是否属于 doghouse）。
    - **对称边头** → `P(同一实例)`，对 `[h_a + h_b, |h_a - h_b|]` 打分（对称，与边方向无关）。
    - **孔壁结构头** → `P(hole_wall)`，用于非解析孔壁召回。
    - **安装面结构头** → `P(mount)`，用于外侧安装平面判断。
  - `train_graph.py`：节点 `BCE(pos_weight)` + 边 `BCE(pos_weight)`；结构角色训练时再加 `hole_wall` / `mount` 二分类 BCE。支持留一法评估和全量训练。纯面图发布权重为 `checkpoints/doghouse_graph_v1.pt`。
  - `infer_graph.py`：前向 → 节点/边概率 → **只保留 AI 判正的边**（`edge_prob >= edge_threshold`）在 doghouse 面上做连通分量 = 实例；结构 checkpoint 还会在 `face_predictions` 中写入 `mount_prob` / `hole_wall_prob`。产出与 `vote_faces` 相同的 `doghouse_face_predictions.v1` result（`backbone="face_graph_gnn"`）。**无 `min_component_faces`、无闭运算比例**。
- **入口**：`infer_from_step.py --step <file.step>`（生产默认 `--backbone graph`，自动加载混合 PMAE 权重；旧路径 `--backbone pointmlp --checkpoint ...` 仍兼容）。可调 `--node-threshold` / `--edge-threshold` / `--no-instance-sim-filter`。
- **为什么解决碎片化**：① 直接在面上运算，消除「点→面投票」这一碎片源；② 分离由**边头**决定能分开紧贴的两个 doghouse，也能把被切碎的同一实例重新连起来，不依赖面数/比例参数。
- **纯面图当前结论**：
  - `doghouse_graph_v1.pt` 在已标注 6 模型/近训练分布上稳定，pillar、0612 等模型实例数和 face mask 都接近 GT。
  - 对未见新模型会暴露泛化问题：`未命名-6302001001-B126302102001-B12` 纯面图预测 8 个实例，存在 `[9,6,5,4,1]` 等小碎片；加入该 B12 标注训练后可收敛到 2 个实例，但对另一个未训练 B12 变体出现过抑制，说明单模型微调会过拟合，不宜直接作为通用权重。
- **负样本使用与已验证的负结论（2026-07-08）**：标注 JSON 的 `hard_negative_faces`（`negative_rib/boundary/protrusion`）**已作为 `face_doghouse=0` 负监督**喂给节点头（`build_dataset._label_maps`）。两种「更激进利用负样本类型」的尝试经 200 epoch 6 折留一法验证**均未超过基线**：
  - `--hard-neg-weight 3.0`（逐面难负样本损失加权）：精度普遍下降（pillar 0.71→0.36），因 `(loss*w).sum()/w.sum()` 相对压低了占多数的简单背景 → 简单背景假阳增多。
  - `--semantic-loss-weight 0.5`（AAGNet 式辅助多分类语义头，推理仍用二分类节点头）：同样未赢，M5 塌到 0.03/0.25。
  - **根因**：仅 6 个模型、方差极大，额外损失项加剧训练不稳。**提升杠杆是数据量而非损失项**（语义头在 AAGNet 6 万样本下才有效）。
  - **默认关闭**（`--hard-neg-weight 1.0` / `--semantic-loss-weight 0.0`）；两开关保留，数据变多后可复用。发布权重 `doghouse_graph_v1.pt` 为基线配置。
- **Phase 2 — Point-MAE 混合特征（当前推荐 doghouse 预测基线，2026-07-09）**：用 ABC 自监督预训练的 Point-MAE 编码器给面图补几何先验，目标是**少样本泛化更好、少标模型**。当前推荐混合权重为 `checkpoints/doghouse_graph_pmae_7_plus_B126302301001.pt`（原 6 模型 + `B126302301001-B12` 增量训练，`B126302102001-B12` 保留作非典型测试）。
  - **为什么是混合而非纯点云分割**：纯 Point-MAE 分割会重新引入「点→面投票」碎片化、且不做实例分离（倒退）。混合法把 Point-MAE 逐点特征池化到面、拼进面特征，保留面图的节点头+边头（仍输出「各自的 doghouse 实例」喂 §6 规则）。
  - **只用预训练部分**：ABC 预训练只训了 `Encoder + pos_embed + blocks + norm`；分割头（`propagation_0`/`convs`）是随机初始化的。故提取用**无参数 KNN 反距离插值**把组特征传到逐点，绝不碰随机权重。
  - **三步流程**：
    1. 本机 `export_pointcloud_for_pmae.py`：`*_graph.npz` → `{model}_pmae_input.npz`（单位球归一化点云 + `face_idx`）。
    2. GPU/Linux `Point-MAE/extract_pmae_face_features.py`：加载 `ckpt-last.pth`，Group→Encoder→blocks 取 [3,7,11] 层拼 1152 维组特征→无参插值到逐点→按 `face_idx` 均值池化→`{model}_pmae_face_emb.npy [num_faces,1152]`。默认 `--num-group 256`。
    3. `train_graph.py --pmae-dir <emb 目录>`：加载逐面 1152 维嵌入，`FaceGraphGNN` 输入端用小投影（1152→32）拼进 12 维面特征。checkpoint 存 `extra_dim`，推理 `infer_graph` 自动兼容（需同源 `face_pmae`）。
    4. STEP 端到端推理时传 `infer_from_step.py --pmae-face-emb-dir <emb 目录> --pmae-ckpt <ckpt-last.pth>`，脚本会按 STEP stem 自动加载 `{stem}_pmae_face_emb.npy`；若缺失且提供 `--pmae-ckpt`，会自动生成 embedding 后继续 GNN 推理。
  - **冻结编码器**：Point-MAE 只做一次性特征提取并缓存，不微调（本机无 GPU、样本少）。
  - **8 模型 doghouse 评估（`compare_8_mixed_pmae_train301_min2_eval_summary.json`）**：启用 `--min-instance-faces 2` 后，平均 Face IoU **0.9232**，Precision **0.9478**，Recall **0.9625**。`B126302301001-B12` 训练后达到 5/5 实例、Face IoU **0.9937**；非典型测试件 `B126302102001-B12` 从旧混合的 Face IoU **0.4898** 提升到 **0.5862**，但仍是主要失败项（GT=2，预测 6）。
  - **实例级整体相似性过滤（`compare_8_mixed_pmae_train301_min2_simfilter_summary.json`）**：在上述混合 PMAE 结果后接 `doghouse_instance_similarity_gallery_7_plus_B126302301001.npz`（默认阈值 0.2），8 模型 mean Face IoU **0.9233 → 0.9259**，总 extra instances **4 → 1**。非典型 `B126302102001-B12` 从 **6/2** 降到 **3/2**，Face IoU **0.5862 → 0.6073**；其余 7 模型实例数和 IoU 不变。
- **当前 doghouse 预测建议**：
  - **默认已整合**：直接运行 `infer_from_step.py --step <file.step>` 即启用混合 PMAE graph GNN、`--min-instance-faces 2`、生产 instance-similarity gallery；无需再手写一长串参数。
  - 完整显式命令（覆盖默认时）：`--backbone graph --checkpoint doghouse_ai/checkpoints/doghouse_graph_pmae_7_plus_B126302301001.pt --pmae-face-emb-dir doghouse_ai/pmae_face_emb --pmae-ckpt doghouse_ai/ckpt-last.pth --min-instance-faces 2 --instance-sim-filter`
  - 纯面图 `doghouse_graph_v1.pt` 保留作对照和低成本 CPU baseline（`--checkpoint doghouse_ai/checkpoints/doghouse_graph_v1.pt --no-instance-sim-filter`）。
  - **泛化优先，不追求边界件 100% 精确**：允许少量 extra 实例；`no_mount` 的候选在装配阶段自然淘汰。不为“封闭孔柱 vs 开口 U 槽”单独加复杂判别层。
- **其他 Phase 2 方向**：按 AAGNet 加边凸性/共轴等边属性、把均值聚合换成 GAT 注意力；继续扩充真实 doghouse 与 false-positive extra component 原型库，降低实例相似性阈值过拟合风险。

### 3.4c 生产默认整合 — `pipeline_defaults.py`（2026-07-09）

`infer_from_step.py` 与 `infer_graph.py` 共用此模块，避免命令行参数散落：

| 常量 / 函数 | 说明 |
|-------------|------|
| `PRODUCTION_GRAPH_CHECKPOINT` | `checkpoints/doghouse_graph_pmae_7_plus_B126302301001.pt` |
| `PRODUCTION_INSTANCE_SIM_GALLERY` | `checkpoints/doghouse_instance_similarity_gallery_7_plus_B126302301001.npz` |
| `DEFAULT_PMAE_CKPT` / `DEFAULT_PMAE_FACE_EMB_DIR` | `ckpt-last.pth` / `pmae_face_emb/` |
| `resolve_checkpoint()` | graph 模式下 `--checkpoint` 可省略 |
| `checkpoint_needs_pmae()` | 仅当 checkpoint `extra_dim > 0` 时自动启用 PMAE |
| `apply_graph_postprocess()` | `min-instance-faces` + instance-similarity 统一后处理 |

**整合后的生产链路**：

```
STEP → 几何 + PMAE（按需）
     → 混合 PMAE FaceGraphGNN
     → min-instance-faces = 2
     → instance-similarity gallery（阈值 0.2）
     → [可选] structure checkpoint 合并 mount_prob / hole_wall_prob
     → [可选] 装配特征（VF2 可靠解析孔优先，否则 AI 结构路径）
```

**固定回归**：`run_e2e_test_0981.py` + `test_e2e_0981.py` 对 `未命名-0981535409815353` 做端到端验证（doghouse 5/5、Face IoU 0.9308、装配 ok=5/5）。

### 3.5 安装面 + 安装孔提取 — `doghouse_assembly_features.py`（**本模块重点，见 §6**）
- `extract_assembly_features(step_path, prediction, use_vf2=..., ...)` → `doghouse_assembly_features.v1`。
- 当前核心策略是 **VF2 可靠解析孔优先，否则 AI 结构路径**：
  1. 先在 doghouse 实例 scope 内跑 `_vf2_mount_and_holes`，利用局部 AAG + `rule_pillar.detect_holes_mcf_vf2` 做拓扑孔匹配。
  2. VF2 只有在找到可靠解析安装孔时才采用。可靠的含义不是“有半径就行”，而是孔组内存在 `cylinder/cone` 解析半径 `R ∈ [2, 6]`，并且 `mount_prob` 或 `hole_wall_prob` 支持该安装面/孔壁。
  3. 如果 VF2 只找到了 freeform/bspline 孔，或解析圆柱其实不是安装孔，则转 `_ai_structure_mount_and_holes`。
  4. AI 结构路径使用 `mount_prob`、`hole_wall_prob`、外侧程度和 2-hop 邻接关系，不依赖解析半径，适合 BSpline / extrusion / freeform 孔壁。
  5. 如果 VF2 和 AI 都未命中，再使用 `_collect_cylinder_candidates` + `_cluster_hole_candidates` 的本地 BREP 圆柱聚类兜底。
- 每实例输出含 `hole_method`：`vf2_topo` / `ai_structure` / `local_brep_cluster` / `vf2`。
- **为什么增加 AI 结构路径**：2001 / 0981 / 009 等模型中，真实孔壁经常是 BSpline、extrusion 或多段曲面，无法稳定解析半径。纯规则路径会漏孔，进而安装面错误；AI 结构头通过上下文学习“外侧安装面”和“通孔孔壁”，能绕开半径解析限制。
- **为什么仍保留 VF2 优先**：pillar、M5、B pillar 等解析孔场景中，VF2 的孔组更干净、FP 少。当前门控保证 pillar 的圆锥/拔模孔仍走 VF2，同时挡住 009 的非安装圆柱 `[738,739]` / `[1024,1025]`。

### 3.6 着色导出 / 评估
- `export_assembly_colored_step`：绿=安装面、蓝=孔壁、浅红=doghouse、灰=背景 → STEP（XCAF 着色）。
- `export_colored_step.py`：doghouse 预测面着色。
- `eval_face_predictions.py`：对标注 JSON 做 face IoU / precision / recall（`evaluate_face_predictions()` 可编程调用）。

---

## 4. 关键文件清单

| 文件 | 职责 |
|------|------|
| `infer_from_step.py` | **端到端入口**：STEP → 实例 →〔可选〕装配特征 + 着色 STEP |
| `step_geometry.py` | STEP → 几何 JSON（面属性 + 邻接） |
| `pillar_report.json` | 自研 CAD C++ 内核导出的示例报告；其中 `doghouse` 段可作为几何 JSON 输入 |
| `build_point_dataset.py` | 几何 JSON (+标签) → 点云 `.npz` |
| `labels.py` | role / face_type ↔ id 映射 |
| `train_graph.py` | **当前训练入口**：FaceGraphGNN 节点/边头 + 可选 PMAE 特征 |
| `graph_model.py` / `infer_graph.py` | FaceGraphGNN 定义与推理 |
| `train_pointnet_torch.py` | 遗留 PointMLP 基线训练（已弃用） |
| `infer_pointnet_torch.py` | 遗留点模型推理（已弃用） |
| `face_vote.py` | 遗留点预测 → 面级 + 连通组件实例 |
| `extract_pmae_face_features.py` | 冻结 Point-MAE 编码器提取逐面 1152 维 embedding |
| `merge_pmae_face_embeddings.py` | 将已有 `{model}_pmae_face_emb.npy` 合入已有 `.npz` 的辅助工具 |
| `pipeline_defaults.py` | 生产默认 checkpoint / gallery / 后处理整合 |
| `run_e2e_test_0981.py` / `test_e2e_0981.py` | 0981 模型固定端到端回归测试 |
| `instance_similarity.py` / `build_instance_similarity_gallery.py` | doghouse 实例级整体几何拓扑签名、正负原型库构建和相似性过滤 |
| `doghouse_assembly_features.py` | **安装面 + 安装孔提取 + 着色导出** |
| `eval_face_predictions.py` | 面级实例评估 |
| `checkpoints/*.pt` | 训练好的 graph 权重（生产 + 对照） |
| `data/…` | 各阶段中间产物 |

> 相关：`../rule_pillar.py`（MCF-VF2 孔拓扑模板与子图匹配）、`../doghouse_detect.py`（独立几何 doghouse 引擎）、`../recommend_and_assemble.py`（几何工具 + 下游装配）。

---

## 5. 数据结构 / Schema

**逐面 12 维特征**（`_face_numeric_features`，训练/推理共用顺序）：

| # | 列 | 说明 |
|---|----|------|
| 0 | face_type_id | 面类型（`FACE_TYPE_TO_ID`） |
| 1 | area | 面积（推理时 log1p/12 归一） |
| 2 | radius | 半径（/50） |
| 3 | has_radius | 0/1 |
| 4 | u_range | U 参数跨度（/6.5，孔"小开口"信号） |
| 5 | v_range | V 参数跨度（/100，孔深） |
| 6–8 | bbox span x/y/z | 包围盒尺寸（log1p/8） |
| 9–11 | normal x/y/z | 面法向 |

**主要 schema**：
- `doghouse_inference_geometry.v1`：几何 JSON。
- `doghouse_instance_labels.v1`：人工标注 JSON（训练用）。
- `doghouse_face_predictions.v1`：`face_predictions` + `doghouse_instances`。
- `doghouse_assembly_features.v1`：`instances[*].{mount_face, mount_candidates, hole_groups, status}`。

---

## 6. 安装面 / 安装孔算法（当前规则）

### 6.1 当前路由

当前装配特征提取不再是单一规则路径，而是按可靠性路由：

```text
doghouse 实例 scope
  → VF2 拓扑路径
      → 有可靠解析安装孔 + AI 结构概率支持：采用 vf2_topo
      → 否则：转 AI structure
  → AI structure 路径
      → 用 mount_prob / hole_wall_prob + 邻接拓扑选安装面和孔壁
  → 几何圆柱聚类兜底
```

默认关键阈值：

- 解析孔半径：`min_radius=2.0`，`max_radius=6.0`
- 解析孔整圈程度：`min_u_sum=π`，斜孔 `oblique_min_u_sum=π/2`
- 安装面面积：`mount_min_area=35.0`，`mount_max_area=2000.0`
- AI 结构阈值：`ai_mount_score_threshold=0.35`，`ai_hole_score_threshold=0.35`
- AI 结果最低置信度：`ai_hole_min_confidence=0.35`
- 安装面到孔壁搜索范围：`hole_hops=2`

### 6.2 VF2 可靠解析孔路径

VF2 路径用于解析孔稳定的模型，例如 B pillar、M5 和 pillar。

主要步骤：

1. 在实例 scope 内收集孔候选：`cylinder` / `cone` 且半径在 `2~6`；实验分支仍允许疑似 freeform 孔壁作为候选，但它本身不构成“可靠解析孔”。
2. 构建局部 AAG，对候选安装平面跑 `rule_pillar.detect_holes_mcf_vf2`。
3. 用半径签名和拓扑边约束匹配 `direct_hole`、`transition_hole`、`multi_segment_hole`、`doghouse_through_hole`、`drafted_doghouse_hole`、`freeform_wall_hole` 等模板。
4. 只保留孔轮廓靠近安装面且投影位于安装面 bbox 内的孔组。
5. 同一安装面附近优先选择解析孔壁，再考虑平行端面链路、`u_sum` 和半径。
6. 对候选安装面打分，分数最高者为安装面。

VF2 要压过 AI，必须满足两个门：

- **解析门**：孔组里至少一个面本身是 `cylinder` 或 `cone`，并且能解析出 `2 <= R <= 6`。
- **AI 结构支持门**：候选安装面 `mount_prob >= 0.35`，或孔组任一孔壁面 `hole_wall_prob >= 0.35`。

这条门控解决了 COMPOUND009 的误判：`[738,739]` / `[1024,1025]` 虽然有解析半径，但对应安装面/孔壁结构概率低，所以不再被视为可靠安装孔。

VF2 安装面评分：

```text
score =
  semantic_bonus
+ coaxial_bonus
+ axis_direction_bonus
+ parallel_bonus
+ outer_endpoint_bonus
+ bottom_alignment_bonus
+ local_reference_bonus
+ hole_center_bonus
+ 5.0 * hole_group_count
+ min(area, 900) / 120
+ min(small_plane_neighbors, 4) * 1.2
```

权重说明：

- `semantic_bonus = 12`：AI role 已认为该面是 mount。
- `coaxial_bonus`：同轴通孔且孔中心性好时最高 `14`；贴孔壁时 `6`；偏心但同轴时 `2`。
- `axis_direction_bonus = 3 * axis_direction_score`。
- `parallel_bonus = 10`：孔壁连接两端平行面。
- `outer_endpoint_bonus = max(0, 5 - outer_margin) * 2`，仅在平行端面 + 同轴时启用。
- `bottom_alignment_bonus = 6 * bottom_alignment_score`。
- `local_reference_bonus = min(distance, 20)/2 + max(0, 6 - outer_margin)*1.5`。
- `hole_center_bonus = 15 * hole_bbox_centrality`。
- 每个孔组 `+5`。
- 面积项最多 `7.5`。
- 小平面邻接项最多 `4.8`。

### 6.3 AI 结构路径

AI 结构路径用于规则无法可靠解析安装孔的场景，尤其是 BSpline、extrusion、freeform 孔壁，或解析圆柱并非真实安装孔的场景。

输入来自 `FaceGraphGNN` 的结构头：

- `mount_prob`：该面是否为外侧安装面。
- `hole_wall_prob`：该面是否为通孔孔壁。

当前推荐使用双 checkpoint 推理：

- 生产 checkpoint 负责 doghouse 实例分割。
- structure checkpoint 只补充 `mount_prob` / `hole_wall_prob`，避免结构模型改变 doghouse 实例边界。

AI 安装面评分：

```text
score =
  20 * mount_prob
+ max(0, 8 - outer_support_margin)
+ min(area, 900) / 150
```

如果没有任何候选面达到 `mount_prob >= 0.35`，兜底时把 `mount_prob` 权重降为 `10`。

AI 孔壁选择：

1. 收集 `hole_wall_prob >= 0.35` 的面。
2. 只保留距离安装面 `hole_hops=2` 内可达的孔壁面。
3. 在 scope 内做连通组件。
4. 选择平均 `hole_wall_prob` 最高的组件作为安装孔孔壁组。
5. 若组件中包含 cylinder/cone，则补充软半径；否则 `radius=null`。

AI 结果置信度：

```text
confidence = 0.5 * mount_prob + 0.5 * mean_hole_wall_prob
```

要求 `confidence >= 0.35`。

### 6.4 几何兜底

当 VF2 和 AI 结构路径都没有得到安装面时，使用本地 BREP 几何聚类：

1. `_collect_cylinder_candidates` 收集 scope 内 `cylinder/cone`，半径 `2~6`，深度 `v_range >= 0.3`。
2. `_cluster_hole_candidates` 按半径、中心距离、轴向相似度聚类，要求 `u_sum >= π`。
3. `_mount_candidates` 按孔邻接、同轴通孔、外侧程度和面积评分选择安装面。

该路径主要用于兼容无结构 checkpoint 的旧流程，不是当前推荐主路径。

### 6.5 评分示例

pillar 实例 1 走 `vf2_topo`：

```text
mount face = 964
hole walls = [7, 8, 972]

semantic_bonus        = 12
coaxial_bonus         = 14
axis_direction_bonus  = 3 * 1.0 = 3
parallel_bonus        = 10
outer_endpoint_bonus  = 0
bottom_alignment      = 6 * 1.0 = 6
local_reference_bonus = 1.25
hole_center_bonus     = 15 * 0.610039 = 9.150585
hole_group_count      = 5 * 1 = 5
area_bonus            = 405.323918 / 120 = 3.377699
small_plane_bonus     = 0

total = 63.778287
```

COMPOUND009 实例 1 走 `ai_structure`：

```text
mount face = 737
hole walls = [743, 999, 1000, 1001, 1002, 1003, 1004, 1005, 1006]

mount_prob           = 0.996247
outer_support_margin = 0
area                 = 761.241

score = 20 * 0.996247 + 8 + 761.241 / 150 = 32.99988
confidence = 0.5 * 0.996247 + 0.5 * 0.812691 = 0.904469
```

### 6.6 性能优化

- **面几何缓存**：`_FACE_GEOM_CACHE` 缓存 `_surface_area / _surface_center / _face_type / _plane_info / _cylinder_info / _radius_u_v / _face_vertices` 等 PythonOCC 查询，单 STEP 加载时清空。
- **VF2 seed 缩小**：不再对 scope 内所有平面跑 VF2，只对孔候选在 `hole_hops + 1` 范围可达的平面作为 seed。
- **JSON-only 快速评估**：批量对比规则时可只输出 `doghouse_assembly_features.json`，需要人工确认时再用 `export_assembly_colored_step` 生成 STEP。

### 6.7 实例级假阳过滤与整体相似性识别

GNN 的节点阈值仍负责逐面召回；为了避免 B12 这类新模型中出现局部相似面组 false positive，实例后处理分两层：

1. **轻量二分类器（旧过滤器）**：标签层新增 `negative_fragment`，与 `negative_rib / negative_boundary / negative_protrusion` 一样属于 doghouse 二分类的外部 hard negative。`train_instance_filter.py` 从 `*_graph.npz` 中抽取 GT doghouse 连通实例为正样本，`negative_*` 连通块为负样本；特征包括面数、面积、doghouse 概率和 bbox span。推理参数为 `--instance-filter ...`，输出 `rejected_doghouse_instances`。
2. **整体相似性原型库（当前推荐）**：`instance_similarity.py` 把每个候选 doghouse component 编码为实例签名：
   - PMAE mean embedding（有 `face_pmae` 时启用）。
   - 局部面图拓扑：face count、internal edge count、edge density。
   - 尺寸/形状：component bbox 三轴、diag、面积统计、span 统计。
   - 面类型比例：plane/cylinder/cone/sphere/torus/freeform、has_radius ratio。
3. `build_instance_similarity_gallery.py` 构建正负原型库：
   - 正原型：训练 NPZ / annotation JSON 中的 GT doghouse instances。
   - 负原型：`negative_*` hard-negative 连通块，以及 GNN 预测中与 GT IoU `< 0.2` 的 extra components。
   - 输出 `pos_sim / neg_sim / keep_score` 到每个实例；低于阈值的实例写入 `rejected_instance_similarity`，对应 faces 的 `doghouse` 改回 0。

当前推荐 gallery：`checkpoints/doghouse_instance_similarity_gallery_7_plus_B126302301001.npz`，默认阈值 **0.2**。在 8 模型评估中，该层主要作用于非典型 `B126302102001-B12`：预测实例 **6 → 3**，extra **4 → 1**；其它 7 模型无回归。

---

## 7. 命令速查（Linux / `llm`）

Notebook 端到端工作流：

`doghouse_ai/STEP_Doghouse_端到端工作流.ipynb`

该 notebook 按单元组织：参数配置（引用 `pipeline_defaults`）→ 一键 `infer_from_step` → 可选分阶段计时 → 结果汇总与标注评估。替换新模型时修改 Cell 1 的 `STEP_PATH`。

最小命令（仅 doghouse 检测）：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/infer_from_step.py \
  --step "doghouse_ai/step - 副本2/pillar.step"
```

端到端（整合生产默认 + 结构角色 checkpoint + 安装面/孔 + 着色 STEP）：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/infer_from_step.py \
  --step "doghouse_ai/step - 副本2/pillar.step" \
  --output-dir doghouse_ai/step_infer_check \
  --structure-checkpoint doghouse_ai/structure_s200/checkpoints/doghouse_graph_s200_structure.pt \
  --extract-assembly-features --use-vf2 \
  --assembly-output-step doghouse_ai/step_infer_check/pillar_mount_hole_colored.step
```

端到端（含安装面/孔 + 着色 STEP，显式覆盖默认权重时）：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/infer_from_step.py \
  --step "doghouse_ai/step - 副本2/pillar.step" \
  --checkpoint doghouse_ai/checkpoints/doghouse_graph_pmae_7_plus_B126302301001.pt \
  --structure-checkpoint doghouse_ai/structure_s200/checkpoints/doghouse_graph_s200_structure.pt \
  --backbone graph \
  --output-dir doghouse_ai/step_infer_check \
  --pmae-face-emb-dir doghouse_ai/pmae_face_emb \
  --extract-assembly-features --use-vf2 \
  --assembly-output-step doghouse_ai/step_infer_check/pillar_mount_hole_colored.step
```

回退纯面图基线（关闭 instance-similarity）：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/infer_from_step.py \
  --step "doghouse_ai/step - 副本2/pillar.step" \
  --checkpoint doghouse_ai/checkpoints/doghouse_graph_v1.pt \
  --no-instance-sim-filter
```

训练旧实例级假阳过滤器（已弃用，仅保留对照）：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/train_graph.py \
  --prepare \
  --data-dir doghouse_ai/data/graph_train \
  --step-dir "doghouse_ai/step - 副本2"

conda run --no-capture-output -n llm python -u doghouse_ai/train_instance_filter.py \
  --data-dir doghouse_ai/data/graph_train \
  --output doghouse_ai/checkpoints/doghouse_instance_filter.npz
```

构建实例级整体相似性原型库（当前推荐后处理）：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/build_instance_similarity_gallery.py \
  --data-dir doghouse_ai/graph_train_7_plus_B126302301001 \
  --prediction-dir doghouse_ai/compare_8_mixed_pmae_train301_min2 \
  --label-dir "doghouse_ai/step - 副本2" \
  --output doghouse_ai/checkpoints/doghouse_instance_similarity_gallery_7_plus_B126302301001.npz \
  --extra-iou 0.2 \
  --threshold 0.2
```

端到端启用实例级整体相似性过滤（现已默认开启，通常无需单独传 gallery）：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/infer_from_step.py \
  --step "doghouse_ai/step - 副本2/未命名-6302001001-B126302102001-B12.step" \
  --output-dir doghouse_ai/step_infer_b12 \
  --structure-checkpoint doghouse_ai/structure_s200/checkpoints/doghouse_graph_s200_structure.pt \
  --extract-assembly-features --use-vf2
```

0981 固定端到端回归（推理 + doghouse 检测评估 + 装配检查）：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/run_e2e_test_0981.py
conda run --no-capture-output -n llm python -u doghouse_ai/run_e2e_test_0981.py --eval-only
conda run --no-capture-output -n llm python -m unittest doghouse_ai.test_e2e_0981 -v
```

输出目录：`doghouse_ai/e2e_test_0981/`；摘要 `{stem}_e2e_summary.json`。

生成 PMAE 逐面 embedding（需要 Point-MAE 预训练权重和 CUDA 环境）：

```bash
conda run --no-capture-output -n llm python extract_pmae_face_features.py \
  --ckpt experiments/pretrain_abc_warmstart/cfgs/abc_pretrain_warmstart/ckpt-last.pth \
  --input-dir doghouse_ai/pma_input \
  --output-dir doghouse_ai/pmae_face_emb \
  --num-group 256
```

把已有 embedding 合入已有 `.npz`：

```bash
conda run --no-capture-output -n llm python doghouse_ai/merge_pmae_face_embeddings.py \
  --npz doghouse_ai/step_infer_check/pillar_doghouse_points.npz \
  --emb doghouse_ai/pmae_face_emb/pillar_pmae_face_emb.npy \
  --output doghouse_ai/step_infer_check/pillar_doghouse_points_with_pmae.npz
```

仅重跑安装面/孔提取（复用已经合并结构概率的预测 JSON，约 30s）：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/doghouse_assembly_features.py \
  --step "doghouse_ai/step - 副本2/pillar.step" \
  --prediction-json doghouse_ai/step_infer_pmae/pillar_doghouse_pred_faces.json \
  --output doghouse_ai/step_infer_check/pillar_assembly_fixed.json \
  --use-vf2
```

当前默认路由输出着色 STEP（整合管线输出示例）：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/infer_from_step.py \
  --step "doghouse_ai/step - 副本2/未命名-0981535409815353.step" \
  --output-dir doghouse_ai/e2e_test_0981 \
  --structure-checkpoint doghouse_ai/structure_s200/checkpoints/doghouse_graph_s200_structure.pt \
  --extract-assembly-features --use-vf2 \
  --assembly-output-step doghouse_ai/e2e_test_0981/未命名-0981535409815353_assembly_colored.step
```

只走生产 doghouse checkpoint、不合并结构概率时，VF2 可靠门控会保持旧 checkpoint 兼容行为；如果要关闭 AI 结构兜底，可用 `--no-prefer-ai-holes`：

```bash
conda run --no-capture-output -n llm python -u doghouse_ai/infer_from_step.py \
  --step "doghouse_ai/step - 副本2/pillar.step" \
  --output-dir doghouse_ai/step_infer_vf2_only \
  --use-vf2 \
  --no-prefer-ai-holes
```

训练 / 评估：见 `train_graph.py`（§3.3）、`sweep_graph_thresholds.py`、`compare_8_models_full_table_summary.json`。

---

## 8. 当前状态

### 8.1 最新 8 模型装配评估（2026-07-10）

来源：`structure_s200/eval_all8_dual_gate/eval_all8_dual_gate_checked_summary.json`。

输出 STEP：`structure_s200/eval_all8_dual_gate/*/*_assembly_colored.step`。

整体结论：

- 安装面：`34/34`，FP=`0`，FN=`0`。
- 安装孔/孔壁：解析孔场景精度稳定；AI 结构路径召回较好，但孔壁 FP 偏多。

| 模型 | 路径 | 安装面 | 安装孔 TP/GT | 孔 FP | 孔 FN |
|------|------|--------|--------------|-------|-------|
| B pillar trim lower-0612 | 全部 `vf2_topo` | 5/5 | 5/5 | 0 | 0 |
| M5-5402231 | 全部 `vf2_topo` | 4/4 | 7/7 | 0 | 0 |
| pillar | 全部 `vf2_topo` | 5/5 | 15/15 | 0 | 0 |
| 未命名-0981535409815353 | `ai_structure` 为主，1 个 `vf2_topo` | 5/5 | 15/18 | 2 | 3 |
| 未命名-1031001001-B121031101001-B001 | 全部 `ai_structure` | 4/4 | 9/9 | 17 | 0 |
| 未命名-1031010003-B11COMPOUND009 | 全部 `ai_structure` | 4/4 | 12/12 | 17 | 0 |
| 未命名-6302001001-B126302102001-B12 | 全部 `ai_structure` | 2/2 | 3/7 | 3 | 4 |
| 未命名-6302001001-B126302301001-B12 | 全部 `ai_structure` | 5/5 | 5/5 | 7 | 0 |

### 8.2 关键修复状态

- **pillar**：带拔模圆锥孔仍可作为可靠解析孔，5 个实例全部走 VF2，安装面/孔壁 `5/5`、`15/15`。
- **M5-5402231**：采用生产 checkpoint 做 doghouse 实例分割，避免 structure 模型把 4 个 GT 合并成 2 个；当前安装面/孔壁 `4/4`、`7/7`。
- **COMPOUND009**：VF2 的解析半径被 AI 结构概率门控，错误圆柱 `[738,739]` / `[1024,1025]` 不再抢安装面；当前安装面 `4/4`，孔壁 GT 召回 `12/12`。
- **0981**：安装面 `5/5`，孔壁还有 `2 FP / 3 FN`。
- **B12-102001**：安装面 `2/2`，孔壁召回仍不足，是当前最弱样本。

### 8.3 效果报告

详细实例级输出、评分示例和剩余问题见：

`docs/doghouse_assembly_effect_report.md`

---

## 9. 后续迭代方向

1. **AI 孔壁收缩**：AI 结构路径当前最大问题是 FP 偏多，应增加组件收缩策略，只保留高 `hole_wall_prob` 主体孔壁，弱化过渡角/连接面。
2. **B12-102001 FN 诊断**：单独检查 FN `[9, 10, 11, 12]` 的 `hole_wall_prob`、邻接距离和训练标签，判断是模型概率问题还是 `hole_hops` 限制。
3. **结构标签细化**：考虑区分 `hole_wall_core` 与 `transition_wall`，让后处理能输出精确孔壁，同时保留过渡面作为连接上下文。
4. **8 模型自动回归**：把 `structure_s200/eval_all8_dual_gate` 的批量推理和 mount/hole 评估固化成脚本，防止后续规则调整回归。
5. **C++ 内核 JSON 适配**：新增从 `report["doghouse"]` 直接构建 `.npz` / 面图的入口；同步定义 face_idx 与标注、STEP 着色之间的映射策略。
6. **标注扩充**：继续补充带 `mount_faces` / `hole_wall_faces` / transition / hard negative 的样本，支撑结构头和阈值的数据化调参。
