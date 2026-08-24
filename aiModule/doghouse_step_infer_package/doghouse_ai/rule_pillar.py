#!/usr/bin/env python3
"""种子面引导的孔组识别 + 过渡面标记

流程:
  1. 用户点选安装面（种子面）
  2. BFS 搜索邻域, 收集所有圆柱/锥面(半角<5°)
  3. 几何签名聚合: 按 (radius, |axis|) 分组, ∑u_range ≥ π 过滤圆角
  4. 标记: 种子面→过渡面→孔面 的拓扑路径
  5. 着色输出 STEP

用法 (先测试标准用例):
  D:/tools/envs/cad_occ_test/python.exe run_pillar.py --step test_cases/1_true.stp --mount-x 7.125 --mount-y 15.143 --mount-z -0.125 --output out.step
"""
import sys, os, argparse, math
from collections import defaultdict, namedtuple, deque

# ============================================================
# AAG 数据结构 (属性邻接图, 用于 MCF-VF2 子图匹配)
# ============================================================

AAGNode = namedtuple('AAGNode', ['face_idx', 'face_type', 'radius', 'depth', 'area', 'has_radius', 'u_range', 'is_true_cone', 'normal'])
AAGEdge = namedtuple('AAGEdge', ['a', 'b', 'convexity', 'dihedral_angle', 'edge_type'])
AAG = namedtuple('AAG', ['nodes', 'adj', 'edges', 'shape', 'faces', 'mount_idx', 'mount_normal'])


def _compute_dihedral_angle(face_a, face_b, edge):
    """计算相邻两面的二面角 (弧度). 用面法向在质心处的夹角判断凸/凹.

    用质心投影方向给角度定号: 正=凸, 负=凹
    """
    from OCC.Core.BRepAdaptor import BRepAdaptor_Surface
    from OCC.Core.GeomLProp import GeomLProp_SLProps
    from OCC.Core.BRep import BRep_Tool
    from OCC.Core.gp import gp_Pnt
    from OCC.Core.BRepGProp import brepgprop
    from OCC.Core.GProp import GProp_GProps
    from OCC.Core.ShapeAnalysis import ShapeAnalysis_Surface

    try:
        sf_a = BRepAdaptor_Surface(face_a, True)
        sf_b = BRepAdaptor_Surface(face_b, True)

        s_a = BRep_Tool.Surface(face_a)
        s_b = BRep_Tool.Surface(face_b)

        # 用质心找 uv 计算法向
        props_a = GProp_GProps(); brepgprop.SurfaceProperties(face_a, props_a)
        c_a = props_a.CentreOfMass()
        props_b = GProp_GProps(); brepgprop.SurfaceProperties(face_b, props_b)
        c_b = props_b.CentreOfMass()

        sas_a = ShapeAnalysis_Surface(s_a); sas_b = ShapeAnalysis_Surface(s_b)
        uv_a = sas_a.ValueOfUV(c_a, 0.01); uv_b = sas_b.ValueOfUV(c_b, 0.01)

        lp_a = GeomLProp_SLProps(s_a, uv_a.X(), uv_a.Y(), 1, 1e-6)
        lp_b = GeomLProp_SLProps(s_b, uv_b.X(), uv_b.Y(), 1, 1e-6)

        if not lp_a.IsNormalDefined() or not lp_b.IsNormalDefined():
            return 0.0

        n_a = lp_a.Normal(); n_b = lp_b.Normal()
        dot = max(-1.0, min(1.0, n_a.X()*n_b.X() + n_a.Y()*n_b.Y() + n_a.Z()*n_b.Z()))
        angle = math.acos(dot)

        # 定号: 从 face_a 看 face_b 的方向
        vec = (c_b.X()-c_a.X(), c_b.Y()-c_a.Y(), c_b.Z()-c_a.Z())
        side = vec[0]*n_a.X() + vec[1]*n_a.Y() + vec[2]*n_a.Z()
        return angle if side > 0 else -angle
    except Exception:
        return 0.0


def _convexity_name(signed_angle):
    """二面角 → 凸性名称."""
    if signed_angle > 0.15:
        return 'convex'
    elif signed_angle < -0.15:
        return 'concave'
    return 'smooth'


def _edge_curve_type(edge):
    """判断边的曲线类型: 'line', 'circle', 'spline', 'other'."""
    from OCC.Core.BRepAdaptor import BRepAdaptor_Curve
    from OCC.Core.GeomAbs import GeomAbs_Line, GeomAbs_Circle, GeomAbs_Ellipse
    try:
        adapt = BRepAdaptor_Curve(edge)
        ct = adapt.GetType()
        if ct == GeomAbs_Line:
            return 'line'
        elif ct == GeomAbs_Circle:
            return 'circle'
        elif ct == GeomAbs_Ellipse:
            return 'ellipse'
        return 'spline'
    except Exception:
        return 'other'


def build_aag(step_path, mount_idx):
    """构建属性邻接图 (AAG) — 含边属性.

    AAG.nodes[i]:  AAGNode(face_idx, face_type, radius, depth, area, has_radius, u_range, is_true_cone, normal)
    AAG.adj:       dict[face_idx → set(neighbor_idx)]
    AAG.edges:     list of AAGEdge(a, b, convexity, dihedral_angle, edge_type)
    """
    from OCC.Core.STEPControl import STEPControl_Reader
    from OCC.Extend.TopologyUtils import TopologyExplorer
    from OCC.Core.TopExp import TopExp_Explorer
    from OCC.Core.TopAbs import TopAbs_EDGE
    from OCC.Core.BRepAdaptor import BRepAdaptor_Surface
    from OCC.Core.GeomAbs import (
        GeomAbs_BSplineSurface,
        GeomAbs_SurfaceOfExtrusion,
        GeomAbs_Cone,
        GeomAbs_Cylinder,
        GeomAbs_Plane,
        GeomAbs_Torus,
    )
    from OCC.Core.BRepGProp import brepgprop
    from OCC.Core.GProp import GProp_GProps
    from OCC.Core.BRep import BRep_Tool
    from OCC.Core.ShapeAnalysis import ShapeAnalysis_Surface
    from OCC.Core.GeomLProp import GeomLProp_SLProps
    from OCC.Core.gp import gp_Pnt

    rd = STEPControl_Reader(); rd.ReadFile(step_path); rd.TransferRoots()
    shape = rd.OneShape()
    topo = TopologyExplorer(shape)
    faces = list(topo.faces())

    # 邻接表 + 边列表
    adj = defaultdict(set)
    fm = {hash(f): i for i, f in enumerate(faces)}
    edge_list = []  # list of AAGEdge
    seen_pairs = set()
    ee = TopExp_Explorer(shape, TopAbs_EDGE)
    while ee.More():
        e = ee.Current()
        af = list(topo.faces_from_edge(e))
        if len(af) == 2:
            i1, i2 = fm.get(hash(af[0])), fm.get(hash(af[1]))
            if i1 is not None and i2 is not None and i1 != i2:
                pair_key = (min(i1, i2), max(i1, i2))
                if pair_key not in seen_pairs:
                    seen_pairs.add(pair_key)
                    adj[i1].add(i2); adj[i2].add(i1)
                    # 计算边属性
                    da = _compute_dihedral_angle(af[0], af[1], e)
                    conv = _convexity_name(da)
                    etype = _edge_curve_type(e)
                    edge_list.append(AAGEdge(i1, i2, conv, da, etype))
        ee.Next()

    # 构建节点属性
    nodes = {}
    for i, face in enumerate(faces):
        sf = BRepAdaptor_Surface(face, True)
        ft = sf.GetType()
        radius = 0.0; depth = 0.0; has_radius = False; u_range = 0.0; is_true_cone = False
        try:
            u_range = abs(float(sf.LastUParameter()) - float(sf.FirstUParameter()))
            depth = abs(float(sf.LastVParameter()) - float(sf.FirstVParameter()))
        except Exception:
            u_range = 0.0
            depth = 0.0
        if ft == GeomAbs_Cylinder:
            radius = sf.Cylinder().Radius()
            has_radius = True
        elif ft == GeomAbs_Cone:
            try:
                cone = sf.Cone()
                if abs(cone.SemiAngle()) < math.radians(5):
                    radius = cone.RefRadius()
                    has_radius = True
                    is_true_cone = True
            except: pass
        props = GProp_GProps(); brepgprop.SurfaceProperties(face, props)
        area = props.Mass()

        type_map = {
            GeomAbs_Plane: 'plane',
            GeomAbs_Cylinder: 'cylinder',
            GeomAbs_Cone: 'cone',
            GeomAbs_Torus: 'torus',
            GeomAbs_BSplineSurface: 'bspline',
            GeomAbs_SurfaceOfExtrusion: 'extrusion',
        }
        face_type = type_map.get(ft, 'other')

        normal = (0.0, 0.0, 0.0)
        if ft == GeomAbs_Plane:
            d = sf.Plane().Axis().Direction()
            normal = (d.X(), d.Y(), d.Z())
        else:
            try:
                c = props.CentreOfMass()
                s = BRep_Tool.Surface(face)
                uv = ShapeAnalysis_Surface(s).ValueOfUV(gp_Pnt(c.X(), c.Y(), c.Z()), 0.01)
                sl = GeomLProp_SLProps(s, uv.X(), uv.Y(), 1, 1e-6)
                if sl.IsNormalDefined():
                    n = sl.Normal()
                    normal = (n.X(), n.Y(), n.Z())
            except: pass

        nodes[i] = AAGNode(i, face_type, radius, depth, area, has_radius, u_range, is_true_cone, normal)

    mount_normal = nodes[mount_idx].normal

    return AAG(nodes, adj, edge_list, shape, faces, mount_idx, mount_normal)


# ============================================================
# MCF-VF2 子图匹配 (用于孔拓扑模式识别)
# ============================================================

# 孔拓扑模板定义
# 每个模板描述一种孔类型的局部图模式:
#   nodes:  dict[id → (face_type_pattern, constraints_dict)]
#   edges:  list of (src_id, dst_id)  — 拓扑连接
#   root:   模板的根节点 (安装面)
#
# face_type_pattern 支持:
#   "plane"          — 必须为平面
#   "cylinder"       — 必须为圆柱面
#   "cone"           — 必须为锥面
#   "cylinder|cone"  — 圆柱或锥面
#   "*"              — 任意类型
#
# constraints_dict 支持:
#   radius_min / radius_max  — 半径范围
#   depth_min                — 最小深度
#   area_min / area_max      — 面积范围
#   concavity                — "convex"/"concave" 凹凸性

# 孔拓扑模板定义 (含边约束, 按论文 MCF-VF2)
#   nodes:  dict[id → (face_type_pattern, node_constraints)]
#   edges:  list of (src_id, dst_id, edge_constraints)
#   root:   搜索起始节点
#   seed_node:  种子面映射节点
#
# edge_constraints 支持:
#   convexity: "convex"|"concave"|"smooth"|"*"  — 凹凸性匹配
#   angle_max: 二面角上限 (弧度) — 区分孔壁邻接 vs 圆角
#   curve_type: "line"|"circle"|"spline"|"*"

HOLE_TEMPLATES = [
    {  # T0: 直连孔 (安装面→孔壁→端面/底面)
        'name': 'direct_hole',
        'seed_node': 'N0',
        'root': 'N1',
        'nodes': {
            'N0': ('plane', {}),
            'N1': ('cylinder|cone', {'radius_min': 2.0, 'depth_min': 0.5}),
            'N2': ('plane|cone', {}),  # 端面/底面 — 区分孔壁 vs 台阶的关键
        },
        'edges': [
            ('N0', 'N1', {'convexity': 'concave'}),
            ('N1', 'N2', {'convexity': '*'}),  # 孔壁→端面
        ],
    },
    {  # T1: 过渡孔 (安装面→过渡面→孔壁→端面)
        'name': 'transition_hole',
        'seed_node': 'N0',
        'root': 'N2',
        'nodes': {
            'N0': ('plane', {}),
            'N1': ('cylinder|cone', {'radius_min': 0.3, 'depth_min': 0.2}),
            'N2': ('cylinder|cone', {'radius_min': 2.0, 'depth_min': 1.0}),
            'N3': ('plane|cone', {}),  # 端面
        },
        'edges': [
            ('N0', 'N1', {'convexity': 'concave'}),
            ('N1', 'N2', {'convexity': '*'}),
            ('N2', 'N3', {'convexity': '*'}),
        ],
    },
    {  # T2: 多段孔 (安装面→段1→段2→段3→端面)
        'name': 'multi_segment_hole',
        'seed_node': 'N0',
        'root': 'N3',
        'nodes': {
            'N0': ('plane', {}),
            'N1': ('cylinder|cone', {'radius_min': 2.0, 'depth_min': 0.3}),
            'N2': ('cylinder|cone', {'radius_min': 2.0, 'depth_min': 0.3}),
            'N3': ('cylinder|cone', {'radius_min': 2.0, 'depth_min': 0.3}),
            'N4': ('plane|cone', {}),  # 最后一段的端面
        },
        'edges': [
            ('N0', 'N1', {'convexity': 'concave'}),
            ('N1', 'N2', {'convexity': 'smooth'}),
            ('N2', 'N3', {'convexity': 'smooth'}),
            ('N3', 'N4', {'convexity': '*'}),
        ],
    },
    {  # T3: doghouse 通孔 (主体大平面安装面 → 内孔壁 → 对侧穿出平面)
        #   适配特征: 安装面是主体上的较大平面; 内孔为通孔 (无盲孔底,
        #   孔壁两端各接一平面); 孔壁常带一小开口 → 部分圆柱 (u_range < 2π),
        #   开口本身无独立面, 由 detect_holes_mcf_vf2 的 u_sum / u_range 门限承载.
        'name': 'doghouse_through_hole',
        'seed_node': 'N0',
        'root': 'N1',
        'nodes': {
            # N0: 主体大平面安装面 (area_min 区分主体平面 vs 小 seating 面)
            'N0': ('plane', {'area_min': 100.0}),
            # N1: 内孔孔壁 (doghouse 安装孔规格 R2~6; 通孔壁较薄, depth_min 放低)
            'N1': ('cylinder|cone', {'radius_min': 2.0, 'radius_max': 6.0, 'depth_min': 0.5}),
            # N2: 通孔对侧穿出平面 (纯平面, 无 radius → 非孔壁, 区分盲孔)
            'N2': ('plane', {}),
        },
        'edges': [
            ('N0', 'N1', {'convexity': 'concave'}),  # 安装面→孔壁: 凹
            ('N1', 'N2', {'convexity': '*'}),        # 孔壁→对侧穿出平面
        ],
    },
    {  # T4: 拔模角安装孔 (安装面→圆角/过渡面→近圆柱 cone 孔壁段)
        #   doghouse 的圆孔常带小于 5° 的拔模角, OCC 会识别为 cone。
        #   这类孔口经 torus/other 小圆角与安装面相连, 没有直接的
        #   plane→cylinder 边, 因此 T0/T1/T3 可能匹配不到。这里显式允许
        #   一个过渡面后接多段同半径 cone/cylinder 孔壁。
        'name': 'drafted_doghouse_hole',
        'seed_node': 'N0',
        'root': 'N2',
        'nodes': {
            'N0': ('plane', {}),
            'N1': ('torus', {}),
            'N2': ('cylinder|cone', {'radius_min': 2.0, 'radius_max': 6.0, 'depth_min': 0.2}),
            'N3': ('cylinder|cone', {'radius_min': 2.0, 'radius_max': 6.0, 'depth_min': 0.2}),
        },
        'edges': [
            ('N0', 'N1', {'convexity': '*'}),
            ('N1', 'N2', {'convexity': '*'}),
            ('N2', 'N3', {'convexity': '*'}),
        ],
    },
    {  # T5: 非解析曲面孔 (安装面→自由曲面孔壁→自由曲面孔壁)
        #   部分 STEP 会把安装孔壁表示为 BSpline / extrusion，而不是解析
        #   cylinder/cone。模板只负责拓扑召回；后处理继续用实际圆弧投影
        #   bbox、连续分组等规则校验。
        'name': 'freeform_wall_hole',
        'seed_node': 'N0',
        'root': 'N1',
        'nodes': {
            'N0': ('plane', {}),
            'N1': ('bspline|extrusion|other', {'u_min': 1.0, 'depth_min': 0.5}),
            'N2': ('bspline|extrusion|other', {'u_min': 1.0, 'depth_min': 0.5}),
        },
        'edges': [
            ('N0', 'N1', {'convexity': '*'}),
            ('N1', 'N2', {'convexity': '*'}),
        ],
    },
    {  # T6: 带过渡面的非解析曲面孔 (安装面→过渡面→自由曲面孔壁)
        'name': 'transition_freeform_wall_hole',
        'seed_node': 'N0',
        'root': 'N2',
        'nodes': {
            'N0': ('plane', {}),
            'N1': ('torus|plane|cone|cylinder|other', {}),
            'N2': ('bspline|extrusion|other', {'u_min': 1.0, 'depth_min': 0.5}),
            'N3': ('bspline|extrusion|other', {'u_min': 1.0, 'depth_min': 0.5}),
        },
        'edges': [
            ('N0', 'N1', {'convexity': '*'}),
            ('N1', 'N2', {'convexity': '*'}),
            ('N2', 'N3', {'convexity': '*'}),
        ],
    },
]


def mcf_score(node_idx, aag):
    """MCF 节点评分: C(u) = λ₁·Deg(u) + λ₂·Attr(u) + λ₃·EdgeConstr(u)

    评分越高 → 节点越"重要/独特", VF2 搜索时优先扩展.

    Args:
        node_idx: AAG 中的节点索引
        aag: AAG 对象

    Returns:
        float 评分 (0~1)
    """
    λ1, λ2, λ3 = 0.4, 0.3, 0.3
    n = aag.nodes[node_idx]

    # Deg(u): 归一化度数 (max 截断 20)
    deg = len(aag.adj.get(node_idx, set()))
    deg_score = min(deg / 20.0, 1.0)

    # Attr(u): 面属性复杂度 [0, 1]
    #   plane=0.2, cylinder=0.6, cone=0.8, torus=1.0, other=0.5
    type_complexity = {
        'plane': 0.2, 'cylinder': 0.6, 'cone': 0.8,
        'torus': 1.0, 'other': 0.5,
    }
    attr_score = type_complexity.get(n.face_type, 0.5)

    # EdgeConstr(u): 邻接面类型的多样性 [0, 1]
    #   邻接多种不同类型 → 约束强 → 评分高
    if n.has_radius:
        nb_types = set()
        for nb in aag.adj.get(node_idx, set()):
            nb_types.add(aag.nodes[nb].face_type)
        edge_score = min(len(nb_types) / 4.0, 1.0)
    else:
        edge_score = 0.3

    return λ1 * deg_score + λ2 * attr_score + λ3 * edge_score


def _type_matches(node_type, pattern):
    """检查面类型是否匹配模板模式."""
    if pattern == '*':
        return True
    patterns = pattern.split('|')
    return node_type in patterns


def _check_constraints(n, constraints):
    """检查节点属性是否满足几何约束."""
    if 'radius_min' in constraints and n.radius < constraints['radius_min']:
        return False
    if 'radius_max' in constraints and n.radius > constraints['radius_max']:
        return False
    if 'depth_min' in constraints and n.depth < constraints['depth_min']:
        return False
    if 'u_min' in constraints and n.u_range < constraints['u_min']:
        return False
    if 'u_max' in constraints and n.u_range > constraints['u_max']:
        return False
    if 'area_min' in constraints and n.area < constraints['area_min']:
        return False
    if 'area_max' in constraints and n.area > constraints['area_max']:
        return False
    return True


def _find_edge(aag, fi_a, fi_b):
    """在 AAG 边列表中查找 (fi_a, fi_b) 的边属性, 返回 AAGEdge 或 None."""
    for e in aag.edges:
        if (e.a == fi_a and e.b == fi_b) or (e.a == fi_b and e.b == fi_a):
            return e
    return None


def _find_path_through_others(aag, fi_a, fi_b, max_hops=3, path_cache=None):
    """在 AAG 中找 fi_a → fi_b 的多跳路径, 中间允许穿过任意类型面.

    优化: VF2 边检查时, 当两面不直接邻接, 允许通过 BFS 找间接连接.
    (pillar 中 seed→plane→other→hole 的路径属于此类)
    """
    cache_key = None
    if path_cache is not None:
        cache_key = (int(fi_a), int(fi_b), int(max_hops))
        rev_key = (int(fi_b), int(fi_a), int(max_hops))
        if cache_key in path_cache:
            return path_cache[cache_key]
        if rev_key in path_cache:
            path = path_cache[rev_key]
            return list(reversed(path)) if path is not None else None

    if fi_b in aag.adj.get(fi_a, set()):
        if path_cache is not None and cache_key is not None:
            path_cache[cache_key] = [fi_a, fi_b]
        return [fi_a, fi_b]
    visited = {fi_a: None}
    q = deque([(fi_a, 0)])
    while q:
        cur, d = q.popleft()
        if d >= max_hops: continue
        for nb in aag.adj.get(cur, set()):
            if nb == fi_b:
                # 重建路径: fi_a → ... → cur → fi_b
                path = [fi_a]
                node = cur
                back = []
                while node != fi_a:
                    back.append(node)
                    node = visited.get(node)
                path.extend(reversed(back))
                path.append(fi_b)
                if path_cache is not None and cache_key is not None:
                    path_cache[cache_key] = path
                return path
            if nb not in visited:
                visited[nb] = cur
                q.append((nb, d + 1))
    if path_cache is not None and cache_key is not None:
        path_cache[cache_key] = None
    return None


def _check_edge_constraints(aag, fi_a, fi_b, edge_constraints, path_cache=None):
    """检查两面之间的边是否满足模板约束.

    支持:
      - 直接邻接: 检查 AAG 边属性 (凸性/二面角)
      - 多跳邻接 (max 3 跳): 中间面穿越任意类型, 放宽检查
    """
    path = _find_path_through_others(aag, fi_a, fi_b, max_hops=3, path_cache=path_cache)
    if path is None:
        return False

    # 直接邻接: 严格检查边属性
    if len(path) == 2:
        if not edge_constraints:
            return True
        e = _find_edge(aag, fi_a, fi_b)
        if e is None:
            return False
        if 'convexity' in edge_constraints:
            if edge_constraints['convexity'] != '*' and e.convexity != edge_constraints['convexity']:
                return False
        if 'angle_max' in edge_constraints:
            if abs(e.dihedral_angle) > edge_constraints['angle_max']:
                return False
        return True

    # 多跳路径: 放宽检查 (仅验证最后一段边的凸性)
    if len(path) >= 3:
        last_a, last_b = path[-2], path[-1]
        e = _find_edge(aag, last_a, last_b)
        if e is None:
            return False
        if 'convexity' in edge_constraints:
            ec = edge_constraints['convexity']
            if ec != '*' and ec != 'smooth' and e.convexity != ec:
                return False
        return True

    return True


def vf2_search(aag, template, seed_idx, max_depth=4, max_candidates_per_step=80, max_results=40):
    """VF2 子图匹配: 用边约束在 AAG 中搜索与模板匹配的子图.

    算法流程 (按 MCF-VF2 论文):
      1. 计算 AAG 全节点 MCF 评分
      2. 候选预筛选: type + 节点约束 + 法向夹角容差
      3. 每步检查 3 类约束: 属性一致性 / 边约束 / 前瞻容量
      4. 回溯直到完整匹配或全部探索完

    3 类约束 (论文 2.5 节):
      (1) 属性一致性: type 匹配 + 几何约束
      (2) 边约束: 凸性 + 二面角 (边属性已在 AAG 中预计算)
      (3) 前瞻容量校验: |Γ(u')\V_M'| ≤ |Γ(u)\V_M|

    Returns:
        list of dict: [{template_node_id: face_idx, ...}, ...]
    """
    # 1. MCF 评分
    mcf_scores = {i: mcf_score(i, aag) for i in range(len(aag.nodes))}

    # 2. 种子面映射
    seed_node_id = template.get('seed_node', template['root'])
    seed_pattern, seed_constraints = template['nodes'][seed_node_id]
    if not _type_matches(aag.nodes[seed_idx].face_type, seed_pattern):
        return []
    if not _check_constraints(aag.nodes[seed_idx], seed_constraints):
        return []

    # BFS 候选池 (5 跳)
    candidate_pool = {seed_idx: 0}
    q = [seed_idx]
    while q:
        cur = q.pop(0); d = candidate_pool[cur]
        if d >= 5: continue
        for nb in aag.adj.get(cur, set()):
            if nb not in candidate_pool:
                candidate_pool[nb] = d + 1; q.append(nb)

    # 3. VF2 递归搜索
    results = []
    path_cache = {}
    reachable_cache = {}

    def _reachable_within(start, max_hops=3):
        key = (int(start), int(max_hops))
        if key in reachable_cache:
            return reachable_cache[key]
        reach = {int(start): 0}
        q2 = deque([int(start)])
        while q2:
            cur = q2.popleft()
            d = reach[cur]
            if d >= max_hops:
                continue
            for nb in aag.adj.get(cur, set()):
                nb = int(nb)
                if nb not in reach:
                    reach[nb] = d + 1
                    q2.append(nb)
        reachable_cache[key] = reach
        return reach

    def _extend(mapping, depth=0):
        if depth > max_depth:
            return
        if len(results) >= max_results:
            return

        # 找到未匹配的 template 节点
        unmatched = [tid for tid in template['nodes'] if tid not in mapping]
        if not unmatched:
            results.append(dict(mapping))
            return

        # 选 next_tid: 优先有边连接的
        next_tid = None
        connected_parents = []
        for tid in unmatched:
            for pid in mapping:
                # 边模板可以是 (src, dst) 或 (src, dst, constraints) 两种格式
                for te in template['edges']:
                    tsrc, tdst = te[0], te[1]
                    if (pid == tsrc and tid == tdst) or (pid == tdst and tid == tsrc):
                        connected_parents.append((pid, tid, te))
                        if next_tid is None:
                            next_tid = tid
        if next_tid is None:
            next_tid = unmatched[0]

        # 收集候选面
        connected_parent_ids = [(p, t, te) for p, t, te in connected_parents if t == next_tid]
        if depth == 0 and not connected_parent_ids:
            # 第一步: 从 BFS 候选池中找
            candidate_faces = set()
            for fi, d in candidate_pool.items():
                if d == 0: continue
                if fi not in mapping.values():
                    candidate_faces.add(fi)
        else:
            # 后续: 邻域 + 多跳扩展 (通过 intermediate 面)
            candidate_faces = set()
            for mid in mapping.values():
                for nb in aag.adj.get(mid, set()):
                    if nb not in mapping.values():
                        candidate_faces.add(nb)
                reach = _reachable_within(mid, max_hops=3)
                for fi, d in candidate_pool.items():
                    if d <= 1 or d > 3:
                        continue
                    if fi in mapping.values():
                        continue
                    if fi in reach and reach[fi] >= 2:
                        candidate_faces.add(fi)

            if connected_parent_ids:
                parent_faces = set()
                for pid, _, _ in connected_parent_ids:
                    parent_faces.add(mapping[pid])
                filtered = set()
                for cf in candidate_faces:
                    for pf in parent_faces:
                        if cf in _reachable_within(pf, max_hops=3):
                            filtered.add(cf)
                            break
                candidate_faces = filtered

        # 按 MCF 评分排序
        pattern, constraints = template['nodes'][next_tid]
        sorted_candidates = sorted(
            [fi for fi in candidate_faces
             if _type_matches(aag.nodes[fi].face_type, pattern)
             and _check_constraints(aag.nodes[fi], constraints)
             and fi not in mapping.values()],
            key=lambda fi: -mcf_scores[fi],
        )[:max_candidates_per_step]

        # 前瞻容量校验 (论文 3.3)
        remaining_t = set(template['nodes']) - set(mapping)
        n_required = len(remaining_t)
        if len(candidate_faces) < max(1, n_required // 2):
            return

        # 扩展
        for fi in sorted_candidates:
            # 边约束检查 (论文 3 类约束的第 2 类)
            edges_ok = True
            for pid, tid, te in connected_parent_ids:
                pf = mapping[pid]
                # te 可以是 (src, dst) 或 (src, dst, constraints)
                e_constraints = te[2] if len(te) >= 3 else {}
                if not _check_edge_constraints(aag, pf, fi, e_constraints, path_cache=path_cache):
                    edges_ok = False
                    break
            if not edges_ok:
                continue

            mapping[next_tid] = fi
            _extend(mapping, depth + 1)
            del mapping[next_tid]

            if results and depth == 0:
                break

    _extend({seed_node_id: seed_idx})
    return results


def detect_holes_mcf_vf2(aag, seed_idx, templates=None, min_radius=2.0, min_u_sum=math.pi):
    """纯 VF2 边约束子图匹配: 只保留 VF2 验证通过的孔候选.

    流程:
      1. BFS 收集候选 → 签名分组 → u_sum 过滤
      2. 对每组候选, VF2 验证 (边约束拓扑匹配)
      3. VF2 未通过 → 丢弃 (无规则回退)
    """
    if templates is None:
        templates = HOLE_TEMPLATES

    # 1. BFS 收集候选
    bfs_pool = {seed_idx: 0}
    q = [seed_idx]
    while q:
        cur = q.pop(0); d = bfs_pool[cur]
        if d >= 5: continue
        for nb in aag.adj.get(cur, set()):
            if nb not in bfs_pool:
                bfs_pool[nb] = d + 1; q.append(nb)

    freeform_hole_types = {'bspline', 'extrusion', 'other'}

    # 2. 签名聚合
    sig_map = {}
    freeform_candidates = set()
    for fi, dist in bfs_pool.items():
        if fi == seed_idx: continue
        n = aag.nodes[fi]
        if n.has_radius and n.radius >= min_radius:
            sig = (round(n.radius, 1),)
            if sig not in sig_map:
                sig_map[sig] = {'radius': n.radius, 'face_indices': [], 'u_sum': 0.0, 'v_max': 0.0, 'best_idx': fi}
            g = sig_map[sig]
            g['face_indices'].append(fi)
            g['u_sum'] += n.u_range
            g['v_max'] = max(g['v_max'], n.depth)
            if n.u_range > aag.nodes[g['best_idx']].u_range:
                g['best_idx'] = fi
        elif n.face_type in freeform_hole_types and n.u_range >= 1.0 and n.depth >= 0.5:
            freeform_candidates.add(fi)

    # Non-analytic hole walls have no radius; group them by tangent/topological
    # connected components so templates can still validate their local topology.
    seen_freeform = set()
    for start in sorted(freeform_candidates):
        if start in seen_freeform:
            continue
        comp = set([start])
        q2 = [start]
        seen_freeform.add(start)
        while q2:
            cur = q2.pop(0)
            for nb in aag.adj.get(cur, set()):
                if nb in freeform_candidates and nb not in seen_freeform:
                    seen_freeform.add(nb)
                    comp.add(nb)
                    q2.append(nb)
        best = max(comp, key=lambda fi: aag.nodes[fi].u_range)
        sig_map[('freeform', best)] = {
            'radius': 0.0,
            'face_indices': sorted(comp),
            'u_sum': sum(aag.nodes[fi].u_range for fi in comp),
            'v_max': max(aag.nodes[fi].depth for fi in comp),
            'best_idx': best,
            'freeform': True,
        }

    # 3. u_sum 过滤
    valid_groups = [g for g in sig_map.values() if g['u_sum'] >= min_u_sum]
    if not valid_groups:
        return []

    # 4. 纯 VF2 验证 (唯一过滤条件)
    # 额外校验: 端面/底面必须远离种子面 (≥3 跳), 否则是台阶面而非孔底面
    hole_groups = []
    for g in valid_groups:
        vf2_ok = False
        for tmpl in templates:
            matches = vf2_search(aag, tmpl, seed_idx)
            for m in matches:
                matched_hole_faces = set(
                    fi for tid, fi in m.items()
                    if tid != tmpl.get('seed_node', tmpl['root'])
                    and (
                        aag.nodes[fi].has_radius
                        or aag.nodes[fi].face_type in freeform_hole_types
                    )
                )
                if not (matched_hole_faces & set(g['face_indices'])):
                    continue

                # 端面校验: 找到匹配中的 end_face (模板中最后一个 plane|cone 节点)
                end_face_id = None
                for tid in sorted(tmpl['nodes'].keys(), reverse=True):
                    if tid == tmpl.get('seed_node', tmpl['root']): continue
                    fi = m.get(tid)
                    if fi is not None and aag.nodes[fi].face_type in ('plane', 'cone') and not aag.nodes[fi].has_radius:
                        end_face_id = tid
                        break
                # 同径邻居校验: 多段孔壁 (≥2 同径段) vs 孤立台阶柱面
                # 单段孔壁 (无同径邻居): 需要 u_range ≥ π (全圆周)
                has_same_r = True
                for hole_fi in matched_hole_faces:
                    n_hole = aag.nodes[hole_fi]
                    if not n_hole.has_radius:
                        continue
                    hole_r = round(n_hole.radius, 1)
                    same_r_nbrs = 0
                    for nb in aag.adj.get(hole_fi, set()):
                        nn = aag.nodes[nb]
                        if nn.has_radius and abs(round(nn.radius, 1) - hole_r) <= 0.5:
                            same_r_nbrs += 1
                    if same_r_nbrs < 1 and n_hole.u_range < math.pi * 0.95:
                        has_same_r = False
                        break
                if not has_same_r:
                    continue  # 孤立柱面且非全圆周 → 跳过

                vf2_ok = True
                break
            if vf2_ok:
                break
        if vf2_ok:
            hole_groups.append(g)

    # 5. 格式统一
    if not hole_groups:
        return []
    result = []
    for g in hole_groups:
        result.append({
            'radius': g['radius'],
            'face_indices': sorted(g['face_indices']),
            'u_sum': g['u_sum'],
            'v_max': g['v_max'],
            'best_idx': g['best_idx'],
        })
    result.sort(key=lambda x: -x['radius'])
    return result


# ============================================================
# 几何工具函数
# ============================================================

def get_face_info(face):
    """提取单面的几何信息"""
    from OCC.Core.BRepAdaptor import BRepAdaptor_Surface
    from OCC.Core.GeomAbs import GeomAbs_Cylinder, GeomAbs_Cone
    from OCC.Core.BRepGProp import brepgprop
    from OCC.Core.GProp import GProp_GProps

    sf = BRepAdaptor_Surface(face, True)
    ft = sf.GetType()

    props = GProp_GProps()
    brepgprop.SurfaceProperties(face, props)
    area = props.Mass()

    radius = 0.0
    v_depth = 0.0
    has_radius = False

    if ft == GeomAbs_Cylinder:
        cyl = sf.Cylinder()
        radius = cyl.Radius()
        v_depth = abs(sf.LastVParameter() - sf.FirstVParameter())
        has_radius = True
    elif ft == GeomAbs_Cone:
        try:
            cone = sf.Cone()
            if abs(cone.SemiAngle()) < math.radians(5):
                radius = cone.RefRadius()
                v_depth = abs(sf.LastVParameter() - sf.FirstVParameter())
                has_radius = True
                ft = GeomAbs_Cylinder
        except:
            pass

    type_map = {0: "plane", 1: "cylinder", 2: "cone", 3: "torus"}
    face_type = type_map.get(ft, f"other({ft})")
    u_range = abs(sf.LastUParameter() - sf.FirstUParameter())

    return {
        'type': face_type,
        'area': area,
        'radius': radius,
        'v_depth': v_depth,
        'u_range': u_range,
        'has_radius': has_radius,
    }


def get_signature(info, face=None):
    """几何签名: (radius, axis_dir)
    axis_dir 用 from_occ_face 获取
    """
    if not info['has_radius']:
        return None
    if face is not None:
        from OCC.Core.BRepAdaptor import BRepAdaptor_Surface
        from OCC.Core.GeomAbs import GeomAbs_Cylinder, GeomAbs_Cone
        sf = BRepAdaptor_Surface(face, True)
        ft = sf.GetType()
        try:
            if ft in (GeomAbs_Cylinder, GeomAbs_Cone):
                cyl = sf.Cylinder() if ft == GeomAbs_Cylinder else sf.Cone()
                axis = cyl.Axis()
                d = axis.Direction()
                return (round(info['radius'], 2),
                        round(abs(d.X()), 3),
                        round(abs(d.Y()), 3),
                        round(abs(d.Z()), 3))
        except:
            pass
    return (round(info['radius'], 2),)


# ============================================================
# 正圆孔 / 斜圆孔 分类阈值
# ============================================================

THROUGH_AXIS_MIN_DOT = 0.7
OBLIQUE_AXIS_MIN_DOT = 0.3
OBLIQUE_AXIS_MAX_DOT = 0.7
DEFAULT_OBLIQUE_MIN_U_SUM = math.pi / 2
DEFAULT_OBLIQUE_MIN_V_DEPTH = 3.0
DEFAULT_OBLIQUE_MOUNT_ADJ_HOPS = 2


def _normalize_vec3(vec):
    norm = math.sqrt(sum(c * c for c in vec))
    if norm < 1e-12:
        return None
    return tuple(c / norm for c in vec)


def _face_cylinder_axis_dot(face, mount_normal):
    """Return |dot(cyl_axis, mount_normal)| for a cylinder/cone face, else None."""
    from OCC.Core.BRepAdaptor import BRepAdaptor_Surface
    from OCC.Core.GeomAbs import GeomAbs_Cylinder, GeomAbs_Cone

    sf = BRepAdaptor_Surface(face, True)
    ft = sf.GetType()
    if ft not in (GeomAbs_Cylinder, GeomAbs_Cone):
        return None
    try:
        if ft == GeomAbs_Cylinder:
            axis_d = sf.Cylinder().Axis().Direction()
        else:
            axis_d = sf.Cone().Axis().Direction()
    except Exception:
        return None
    ad2 = _normalize_vec3((axis_d.X(), axis_d.Y(), axis_d.Z()))
    sn = _normalize_vec3(mount_normal)
    if ad2 is None or sn is None:
        return None
    return abs(ad2[0] * sn[0] + ad2[1] * sn[1] + ad2[2] * sn[2])


def _faces_within_hops(seed_idx, adj, max_hops):
    """Face indices reachable from seed within max_hops on the face adjacency graph."""
    local = {int(seed_idx): 0}
    queue = [(int(seed_idx), 0)]
    while queue:
        cur, depth = queue.pop(0)
        if depth >= max_hops:
            continue
        for nb in adj.get(cur, ()):
            nb = int(nb)
            if nb not in local:
                local[nb] = depth + 1
                queue.append((nb, depth + 1))
    return local


def _group_axis_profile(group, all_faces, mount_normal):
    """Summarize how cylinder/cone axes in a group relate to the mount normal."""
    from OCC.Core.TopAbs import TopAbs_FORWARD

    aligned = oblique = edge = 0
    total = 0
    has_forward = False
    for fi in group.get("face_indices") or ():
        face = all_faces[int(fi)]
        if face.Orientation() == TopAbs_FORWARD:
            has_forward = True
        dot2 = _face_cylinder_axis_dot(face, mount_normal)
        if dot2 is None:
            continue
        total += 1
        if dot2 >= THROUGH_AXIS_MIN_DOT:
            aligned += 1
        elif dot2 >= OBLIQUE_AXIS_MIN_DOT:
            oblique += 1
        else:
            edge += 1
    return {
        "aligned": aligned,
        "oblique": oblique,
        "edge": edge,
        "total": total,
        "has_forward": has_forward,
    }


def _group_adjacent_to_mount(group, mount_reach):
    """True when any hole-wall face is within mount_reach (BFS from seed mount)."""
    for fi in group.get("face_indices") or ():
        if int(fi) in mount_reach:
            return True
    return False


def classify_hole_group_kind(
    group,
    all_faces,
    mount_normal,
    mount_reach,
    *,
    allow_oblique=False,
):
    """Classify an aggregated cylinder group as through/oblique hole, or reject."""
    profile = _group_axis_profile(group, all_faces, mount_normal)
    if profile["has_forward"] or profile["total"] == 0:
        return None
    total = profile["total"]
    if profile["edge"] > total * 0.5:
        return None
    if profile["aligned"] > total * 0.5:
        return "through"
    if not allow_oblique:
        return None
    if profile["oblique"] <= total * 0.5:
        return None
    if not _group_adjacent_to_mount(group, mount_reach):
        return None
    return "oblique"


def evaluate_hole_group(
    group,
    all_faces,
    mount_normal,
    mount_idx,
    adj,
    *,
    allow_oblique=False,
    min_u_sum=math.pi,
    oblique_min_u_sum=DEFAULT_OBLIQUE_MIN_U_SUM,
    oblique_min_v_depth=DEFAULT_OBLIQUE_MIN_V_DEPTH,
    oblique_mount_adj_hops=DEFAULT_OBLIQUE_MOUNT_ADJ_HOPS,
):
    """Return a tagged hole-group copy when it passes through/oblique gates, else None."""
    mount_reach = _faces_within_hops(mount_idx, adj, oblique_mount_adj_hops)
    kind = classify_hole_group_kind(
        group,
        all_faces,
        mount_normal,
        mount_reach,
        allow_oblique=allow_oblique,
    )
    if kind is None:
        return None
    if kind == "through":
        if group["u_sum"] < min_u_sum:
            return None
    else:
        if group["u_sum"] < oblique_min_u_sum:
            return None
        if group.get("v_max", 0.0) < oblique_min_v_depth:
            return None
    tagged = dict(group)
    tagged["hole_kind"] = kind
    return tagged


# ============================================================
# 主流程
# ============================================================

def process_step(step_path, mount_x=None, mount_y=None, mount_z=None,
                 mount_idx=None, n_hops=5, min_radius=2.0, min_u_sum=math.pi,
                 use_vf2=False, allow_oblique_holes=False,
                 oblique_min_u_sum=DEFAULT_OBLIQUE_MIN_U_SUM,
                 oblique_min_v_depth=DEFAULT_OBLIQUE_MIN_V_DEPTH,
                 oblique_mount_adj_hops=DEFAULT_OBLIQUE_MOUNT_ADJ_HOPS,
                 allow_slot_holes=False,
                 slot_mount_adj_hops=2,
                 slot_min_walls=2,
                 slot_min_width=2.0,
                 slot_min_depth=0.3):
    """处理单个 STEP 文件

    Args:
        use_vf2: True 时用 MCF-VF2 子图匹配替代规则判断
        allow_oblique_holes: 额外识别孔轴与安装面法向成角的斜圆孔（装配仍沿安装面法向）
        allow_slot_holes: 额外识别 BSpline/other 开放槽孔 (rule_slot.py)
    """

    from OCC.Core.STEPControl import STEPControl_Reader
    from OCC.Extend.TopologyUtils import TopologyExplorer
    from OCC.Core.TopExp import TopExp_Explorer
    from OCC.Core.TopAbs import TopAbs_EDGE
    from OCC.Core.gp import gp_Pnt, gp_Dir
    from OCC.Core.BRepExtrema import BRepExtrema_DistShapeShape
    from OCC.Core.BRepAdaptor import BRepAdaptor_Surface
    from OCC.Core.BRepBuilderAPI import BRepBuilderAPI_MakeVertex

    # ---- 读取 STEP ----
    reader = STEPControl_Reader()
    reader.ReadFile(step_path)
    reader.TransferRoots()
    shape = reader.OneShape()
    topo = TopologyExplorer(shape)
    all_faces = list(topo.faces())
    print(f"  总面数: {len(all_faces)}")

    # ---- 找种子面 ----
    if mount_idx is None:
        vertex = BRepBuilderAPI_MakeVertex(gp_Pnt(mount_x, mount_y, mount_z)).Shape()
        mount_idx, md = -1, float('inf')
        for i, face in enumerate(all_faces):
            e = BRepExtrema_DistShapeShape(vertex, face)
            e.Perform()
            if e.IsDone() and e.Value() < md:
                md = e.Value(); mount_idx = i
        print(f"  种子面: face[{mount_idx}] (距坐标 {md:.3f}mm)")
    else:
        print(f"  种子面: face[{mount_idx}] (指定索引)")

    # ---- 邻接表 ----
    adj = defaultdict(set)
    fmap = {hash(f): i for i, f in enumerate(all_faces)}
    ee = TopExp_Explorer(shape, TopAbs_EDGE)
    while ee.More():
        e = ee.Current()
        af = list(topo.faces_from_edge(e))
        if len(af) == 2:
            i1, i2 = fmap.get(hash(af[0])), fmap.get(hash(af[1]))
            if i1 is not None and i2 is not None and i1 != i2:
                adj[i1].add(i2); adj[i2].add(i1)
        ee.Next()

    # ---- BFS 局部邻域 ----
    local = {mount_idx: 0}
    q = [(mount_idx, 0)]
    while q:
        cur, d = q.pop(0)
        if d >= n_hops: continue
        for nb in adj.get(cur, set()):
            if nb not in local:
                local[nb] = d + 1
                q.append((nb, d + 1))
    print(f"  BFS {n_hops} 跳邻域: {len(local)} 个面")

    # ---- 种子面法向 (VF2 和规则路径共用) ----
    from OCC.Core.BRepGProp import brepgprop
    from OCC.Core.GProp import GProp_GProps
    from OCC.Core.gp import gp_Pnt, gp_Dir
    from OCC.Core.GeomLProp import GeomLProp_SLProps
    from OCC.Core.BRep import BRep_Tool
    from OCC.Core.ShapeAnalysis import ShapeAnalysis_Surface
    seed_sf = BRepAdaptor_Surface(all_faces[mount_idx], True)
    try:
        seed_normal = (seed_sf.Plane().Axis().Direction().X(),
                       seed_sf.Plane().Axis().Direction().Y(),
                       seed_sf.Plane().Axis().Direction().Z())
    except:
        props_tmp = GProp_GProps()
        brepgprop.SurfaceProperties(all_faces[mount_idx], props_tmp)
        c_tmp = props_tmp.CentreOfMass()
        geom_surf = BRep_Tool.Surface(all_faces[mount_idx])
        sas = ShapeAnalysis_Surface(geom_surf)
        uv = sas.ValueOfUV(gp_Pnt(c_tmp.X(), c_tmp.Y(), c_tmp.Z()), 0.01)
        sl = GeomLProp_SLProps(geom_surf, uv.X(), uv.Y(), 1, 1e-6)
        n_tmp = sl.Normal()
        seed_normal = (n_tmp.X(), n_tmp.Y(), n_tmp.Z())
    sn_norm = math.sqrt(sum(c*c for c in seed_normal))
    sn = tuple(c/sn_norm for c in seed_normal)

    # ---- MCF-VF2 分支 (子图匹配替代规则) ----
    if use_vf2:
        # 同目录内引用 (rule_pillar.py)
        from rule_pillar import build_aag, detect_holes_mcf_vf2, AAG
        print("  ⚡ 使用 MCF-VF2 子图匹配...")
        # 复用已有的 all_faces 和 adj 构建 AAG 的简化版
        # 直接调用 detect_holes_mcf_vf2
        aag_full = build_aag(step_path, mount_idx)
        hole_groups = detect_holes_mcf_vf2(aag_full, mount_idx)
        hole_face_set = set()
        for g in hole_groups:
            for fi in g['face_indices']:
                hole_face_set.add(fi)
        # 过渡面标记
        transition_faces = set()
        for hfi in hole_face_set:
            visited = {mount_idx: (0, None)}
            queue = [mount_idx]
            found = False
            while queue and not found:
                cur = queue.pop(0)
                for nb in adj.get(cur, set()):
                    if nb not in visited:
                        visited[nb] = (visited[cur][0] + 1, cur)
                        if nb == hfi: found = True; break
                        queue.append(nb)
            if found:
                cur = hfi
                path = []
                while cur is not None:
                    path.append(cur)
                    _, prev = visited.get(cur, (0, None))
                    cur = prev
                path.reverse()
                for p in path:
                    if p != mount_idx and p not in hole_face_set:
                        transition_faces.add(p)
        print(f"  VF2 匹配: {len(hole_groups)} 个孔组")
        for g in hole_groups:
            print(f"    R={g['radius']:.1f}mm | {len(g['face_indices'])} 面段 | best=face[{g['best_idx']+1}]")
        print(f"  过渡面: {len(transition_faces)}")
        return shape, all_faces, mount_idx, hole_groups, transition_faces, sn, adj

    # ---- 识别圆柱/锥面并聚合 (规则路径) ----
    sig_map = {}
    for fi in local:
        face = all_faces[fi]
        info = get_face_info(face)
        if not info['has_radius']:
            continue
        if info['radius'] < min_radius:
            continue
        if info['v_depth'] < 0.5:
            continue
        sig = get_signature(info, face)
        if sig is None:
            continue
        if sig not in sig_map:
            sig_map[sig] = {
                'radius': info['radius'],
                'face_indices': [],
                'u_sum': 0.0,
                'v_max': 0.0,
                'best_idx': fi,
                'best_u': info['u_range'],
            }
        g = sig_map[sig]
        g['face_indices'].append(fi)
        g['u_sum'] += info['u_range']
        g['v_max'] = max(g['v_max'], info['v_depth'])
        if info['u_range'] > g['best_u']:
            g['best_u'] = info['u_range']
            g['best_idx'] = fi

    # 过滤: 正圆孔 ∑u≥π；斜圆孔（可选）∑u≥π/2 且邻接安装面
    valid = []
    rejected = 0
    for g in sig_map.values():
        tagged = evaluate_hole_group(
            g,
            all_faces,
            sn,
            mount_idx,
            adj,
            allow_oblique=allow_oblique_holes,
            min_u_sum=min_u_sum,
            oblique_min_u_sum=oblique_min_u_sum,
            oblique_min_v_depth=oblique_min_v_depth,
            oblique_mount_adj_hops=oblique_mount_adj_hops,
        )
        if tagged is None:
            rejected += 1
        else:
            valid.append(tagged)
    n_through = sum(1 for g in valid if g.get("hole_kind") == "through")
    n_oblique = sum(1 for g in valid if g.get("hole_kind") == "oblique")
    valid.sort(key=lambda g: -g['radius'])
    oblique_note = f", 斜圆孔={n_oblique}" if allow_oblique_holes else ""
    print(
        f"  圆柱/锥面聚合: {len(sig_map)} 组, 有效孔={len(valid)} "
        f"(正圆={n_through}{oblique_note}), 排除={rejected} 组"
    )

    # ---- 开放槽孔 (rule_slot.py, 仿斜圆孔独立分支) ----
    n_slot = 0
    if allow_slot_holes:
        from rule_slot import detect_slot_holes_near_mount

        slot_groups = detect_slot_holes_near_mount(
            all_faces,
            mount_idx,
            sn,
            adj,
            local,
            shape,
            topo,
            mount_adj_hops=slot_mount_adj_hops,
            min_walls=slot_min_walls,
            min_width=slot_min_width,
            min_depth=slot_min_depth,
            verbose=True,
        )
        n_slot = len(slot_groups)
        if slot_groups:
            used_faces = {fi for g in valid for fi in g["face_indices"]}
            for sg in slot_groups:
                if not any(fi in used_faces for fi in sg["face_indices"]):
                    valid.append(sg)
            valid.sort(key=lambda g: -g["radius"])
        print(f"  槽孔识别: +{n_slot} 组 (合并后共 {len(valid)} 组)")

    # ---- 标记过渡面: 种子面→孔面的最短路径上的小面 ----
    hole_face_set = set()
    for g in valid:
        for fi in g['face_indices']:
            hole_face_set.add(fi)

    transition_faces = set()
    for hole_fi in hole_face_set:
        visited = {mount_idx: (0, None)}
        queue = [mount_idx]
        found = False
        while queue and not found:
            cur = queue.pop(0)
            for nb in adj.get(cur, set()):
                if nb not in visited:
                    visited[nb] = (visited[cur][0] + 1, cur)
                    if nb == hole_fi:
                        found = True
                        break
                    queue.append(nb)
        if found:
            cur = hole_fi
            path = []
            while cur is not None:
                path.append(cur)
                _, prev = visited.get(cur, (0, None))
                cur = prev
            path.reverse()
            for p in path:
                if p != mount_idx and p not in hole_face_set:
                    transition_faces.add(p)

    print(f"  过渡面 (圆角/倒角): {len(transition_faces)}")

    # ---- 打印 ----
    for g in valid:
        fillet_nearby = sum(1 for fi in g['face_indices']
                            for nb in adj.get(fi, set()) if nb in transition_faces)
        kind = g.get("hole_kind", "through")
        if kind == "slot":
            kind_tag = "槽"
            extra = f"{g.get('width', 0):.1f}×{g.get('length', 0):.1f}mm"
        elif kind == "oblique":
            kind_tag = "斜"
            extra = f"u={g['u_sum']:.1f}"
        else:
            kind_tag = "正"
            extra = f"u={g['u_sum']:.1f}"
        print(f"    R={g['radius']:.1f}mm | {len(g['face_indices'])} 面段 | "
              f"{extra} | best=face[{g['best_idx']+1}] | "
              f"{kind_tag}孔 | 邻接过渡: {fillet_nearby}")

    return shape, all_faces, mount_idx, valid, transition_faces, sn, adj



# ============================================================
# 全模型同规格搜索
# ============================================================

def search_same_spec_in_model(shape, target_radius, tolerance=0.5, min_u_sum=math.pi, min_depth=0.5, mount_normal=None):
    """在全模型中搜索与目标半径相同的圆柱/锥面组（一对多装配）"""
    from OCC.Extend.TopologyUtils import TopologyExplorer
    from OCC.Core.BRepAdaptor import BRepAdaptor_Surface
    from OCC.Core.GeomAbs import GeomAbs_Cylinder, GeomAbs_Cone
    from OCC.Core.BRepGProp import brepgprop
    from OCC.Core.GProp import GProp_GProps

    topo = TopologyExplorer(shape)
    all_faces = list(topo.faces())
    print(f"搜索全模型同规格孔 (R={target_radius:.2f}mm ± {tolerance}mm)...")

    sig_map = {}
    for i, face in enumerate(all_faces):
        sf = BRepAdaptor_Surface(face, True)
        ft = sf.GetType()
        radius = 0.0; vd = 0.0
        if ft == GeomAbs_Cylinder:
            radius = sf.Cylinder().Radius()
            vd = abs(sf.LastVParameter() - sf.FirstVParameter())
        elif ft == GeomAbs_Cone:
            try:
                cone = sf.Cone()
                if abs(cone.SemiAngle()) < math.radians(5):
                    radius = cone.RefRadius()
                    vd = abs(sf.LastVParameter() - sf.FirstVParameter())
                    ft = GeomAbs_Cylinder
                else: continue
            except: continue
        else: continue
        if abs(radius - target_radius) > tolerance: continue
        if vd < min_depth: continue
        if sf.GetType() == GeomAbs_Cylinder:
            axis_d = sf.Cylinder().Axis().Direction()
        else:
            axis_d = sf.Cone().Axis().Direction()
        # 用带符号的 axis 分组 (不同方向/不同位置的孔是不同组)
        sig = (round(radius, 2), round(axis_d.X(), 3),
               round(axis_d.Y(), 3), round(axis_d.Z(), 3))
        if sig not in sig_map:
            sig_map[sig] = {'radius': radius, 'face_indices': [], 'u_sum': 0,
                            'v_max': 0, 'best_idx': i, 'best_u': 0}
        g = sig_map[sig]
        g['face_indices'].append(i)
        g['u_sum'] += abs(sf.LastUParameter() - sf.FirstUParameter())
        g['v_max'] = max(g['v_max'], vd)
        if sf.LastUParameter() - sf.FirstUParameter() > g['best_u']:
            g['best_u'] = abs(sf.LastUParameter() - sf.FirstUParameter())
            g['best_idx'] = i
    valid = [g for g in sig_map.values() if g['u_sum'] >= min_u_sum]

    # 过滤 fillet 与立柱 (排除同规格但非孔的圆柱/锥面)
    if mount_normal is not None:
        from OCC.Core.TopAbs import TopAbs_FORWARD
        filtered = []
        for g in valid:
            best_fi = g['best_idx']
            sf = BRepAdaptor_Surface(all_faces[best_fi], True)
            ft = sf.GetType()

            # ── 立柱过滤: 真孔面 Orientation=REVERSED, 立柱/凸台=FORWARD ──
            if all_faces[best_fi].Orientation() == TopAbs_FORWARD:
                # 检查整组: 如果所有面都是 FORWARD → 全部是立柱, 跳过
                all_forward = all(all_faces[fi].Orientation() == TopAbs_FORWARD
                                  for fi in g['face_indices']
                                  if fi < len(all_faces))
                if all_forward:
                    continue

            # ── fillet 过滤: 面积比 < 0.3 视为 fillet (圆角面积远小于真孔) ──
            props = GProp_GProps()
            brepgprop.SurfaceProperties(all_faces[best_fi], props)
            actual_area = props.Mass()
            if g['radius'] > 0 and g['v_max'] > 0:
                expect = 2 * math.pi * g['radius'] * g['v_max']
                if actual_area / expect < 0.3:
                    continue
            filtered.append(g)
        valid = filtered

    valid.sort(key=lambda g: -len(g['face_indices']))

    # ── 空间位置聚类: 相同 (R, axis) 但不同位置的孔拆分为多组 ──
    CLUSTER_DIST = 20.0  # mm
    merged = []
    for g in valid:
        nf = len(g['face_indices'])
        if nf <= 3:
            merged.append(g)
            continue
        centroids = []
        for fi in g['face_indices']:
            props2 = GProp_GProps()
            brepgprop.SurfaceProperties(all_faces[fi], props2)
            c = props2.CentreOfMass()
            centroids.append((c.X(), c.Y(), c.Z()))
        assigned = [False] * nf
        clusters = []
        for i in range(nf):
            if assigned[i]: continue
            cluster = [g['face_indices'][i]]
            assigned[i] = True
            cx, cy, cz = centroids[i]
            for j in range(i + 1, nf):
                if assigned[j]: continue
                dx = cx - centroids[j][0]
                dy = cy - centroids[j][1]
                dz = cz - centroids[j][2]
                if math.sqrt(dx*dx + dy*dy + dz*dz) < CLUSTER_DIST:
                    cluster.append(g['face_indices'][j])
                    assigned[j] = True
            if cluster:
                clusters.append(cluster)
        if len(clusters) <= 1:
            merged.append(g)
        else:
            for cluster_indices in clusters:
                if not cluster_indices: continue
                sub_g = {'radius': g['radius'], 'face_indices': cluster_indices,
                         'u_sum': 0, 'v_max': 0, 'best_idx': cluster_indices[0], 'best_u': 0}
                for fi in cluster_indices:
                    sf_sub = BRepAdaptor_Surface(all_faces[fi], True)
                    sub_g['u_sum'] += abs(sf_sub.LastUParameter() - sf_sub.FirstUParameter())
                    vd = abs(sf_sub.LastVParameter() - sf_sub.FirstVParameter())
                    if vd > sub_g['v_max']: sub_g['v_max'] = vd
                    u = abs(sf_sub.LastUParameter() - sf_sub.FirstUParameter())
                    if u > sub_g['best_u']: sub_g['best_u'] = u; sub_g['best_idx'] = fi
                merged.append(sub_g)
    valid = merged

    print(f"  找到 {len(valid)} 组同规格孔")
    for g in valid:
        print(f"    R={g['radius']:.1f}mm | {len(g['face_indices'])} 面段 | best=face[{g['best_idx']+1}]")
    return valid


# ============================================================
# 着色导出
# ============================================================

def colorize(shape, faces, mount_idx, hole_groups, transition_faces, output_path, same_groups=None):
    from OCC.Core.TDocStd import TDocStd_Document
    from OCC.Core.XCAFDoc import XCAFDoc_DocumentTool, XCAFDoc_ShapeTool, XCAFDoc_ColorTool, XCAFDoc_ColorSurf
    from OCC.Core.Quantity import Quantity_Color, Quantity_TOC_RGB
    from OCC.Core.STEPCAFControl import STEPCAFControl_Writer
    from OCC.Core.STEPControl import STEPControl_AsIs
    from OCC.Core.Interface import Interface_Static

    doc = TDocStd_Document("XCAF")
    label = doc.Main()
    st = XCAFDoc_DocumentTool.ShapeTool(label)
    ct = XCAFDoc_DocumentTool.ColorTool(label)
    st.AddShape(shape)

    BLUE  = Quantity_Color(0.3, 0.5, 1.0, Quantity_TOC_RGB)
    GRY   = Quantity_Color(0.6, 0.6, 0.6, Quantity_TOC_RGB)
    WHITE = Quantity_Color(0.9, 0.9, 0.9, Quantity_TOC_RGB)
    COLORS = [
        Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
        Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB),
        Quantity_Color(1.0, 1.0, 0.0, Quantity_TOC_RGB),
        Quantity_Color(1.0, 0.65, 0.0, Quantity_TOC_RGB),
        Quantity_Color(0.7, 0.2, 0.8, Quantity_TOC_RGB),
        Quantity_Color(0.0, 1.0, 1.0, Quantity_TOC_RGB),
    ]

    color_map = {mount_idx: BLUE}
    for gi, g in enumerate(hole_groups):
        col = COLORS[gi % len(COLORS)]
        for fi in g['face_indices']:
            color_map[fi] = col

    # 同规格扩展孔用不同色调
    SAME_SPEC_COLORS = [
        Quantity_Color(1.0, 0.5, 0.0, Quantity_TOC_RGB),   # 橙
        Quantity_Color(0.3, 0.7, 1.0, Quantity_TOC_RGB),   # 天蓝
        Quantity_Color(0.7, 0.8, 0.2, Quantity_TOC_RGB),   # 黄绿
        Quantity_Color(1.0, 0.2, 0.6, Quantity_TOC_RGB),   # 粉
        Quantity_Color(0.2, 0.8, 0.8, Quantity_TOC_RGB),   # 青绿
        Quantity_Color(0.8, 0.4, 0.2, Quantity_TOC_RGB),   # 棕
    ]
    if same_groups:
        for gi, g in enumerate(same_groups):
            col = SAME_SPEC_COLORS[gi % len(SAME_SPEC_COLORS)]
            for fi in g['face_indices']:
                if fi not in color_map:
                    color_map[fi] = col

    for i, face in enumerate(faces):
        if i in color_map:
            ct.SetColor(face, color_map[i], XCAFDoc_ColorSurf)
        elif i in transition_faces:
            ct.SetColor(face, WHITE, XCAFDoc_ColorSurf)
        else:
            ct.SetColor(face, GRY, XCAFDoc_ColorSurf)

    cw = STEPCAFControl_Writer()
    cw.Transfer(doc, STEPControl_AsIs)
    Interface_Static.SetCVal("write.step.unit", "MM")
    Interface_Static.SetCVal("write.step.schema", "AP214CD")
    cw.Write(output_path)
    print(f"\n输出: {output_path}")


# ============================================================
# CLI
# ============================================================

def main():
    parser = argparse.ArgumentParser(description="种子面引导的孔组识别")
    parser.add_argument("--step", required=True)
    parser.add_argument("--mount-x", type=float, default=None)
    parser.add_argument("--mount-y", type=float, default=None)
    parser.add_argument("--mount-z", type=float, default=None)
    parser.add_argument("--mount-idx", type=int, default=None,
                        help="直接指定安装面索引 (替代坐标)")
    parser.add_argument("--output", default="hole_vis.step")
    parser.add_argument("--n-hops", type=int, default=5)
    parser.add_argument("--min-radius", type=float, default=2.0,
                        help="最小半径 (mm, 默认 2.0 过滤圆角/加强筋)")
    parser.add_argument("--find-all-same", action="store_true",
                        help="全模型搜索同规格孔（一对多装配）")
    parser.add_argument("--same-tolerance", type=float, default=0.5,
                        help="同规格半径容差 (mm)")
    parser.add_argument("--use-vf2", action="store_true",
                        help="使用 MCF-VF2 子图匹配 (替代规则过滤)")
    parser.add_argument("--allow-oblique-holes", action="store_true",
                        help="额外识别孔轴与安装面法向成角的斜圆孔")
    parser.add_argument("--oblique-min-u-sum", type=float, default=DEFAULT_OBLIQUE_MIN_U_SUM,
                        help="斜圆孔最小 ∑u_range (rad, 默认 π/2)")
    parser.add_argument("--oblique-min-v-depth", type=float, default=DEFAULT_OBLIQUE_MIN_V_DEPTH,
                        help="斜圆孔最小轴向深度 mm")
    parser.add_argument("--oblique-mount-adj-hops", type=int, default=DEFAULT_OBLIQUE_MOUNT_ADJ_HOPS,
                        help="斜圆孔孔壁到种子安装面的最大 BFS 跳数")
    parser.add_argument("--allow-slot-holes", action="store_true",
                        help="额外识别 BSpline/other 开放槽孔 (rule_slot.py)")
    parser.add_argument("--slot-mount-adj-hops", type=int, default=2,
                        help="槽孔壁到种子安装面的最大 BFS 跳数")
    args = parser.parse_args()

    if not os.path.exists(args.step):
        print(f"文件不存在: {args.step}")
        sys.exit(1)

    if args.mount_idx is None and (args.mount_x is None or args.mount_y is None or args.mount_z is None):
        print("❌ 需要指定 --mount-x/y/z 或 --mount-idx")
        sys.exit(1)

    shape, faces, mount_idx, hole_groups, transition_faces, mount_normal, _adj = process_step(
        args.step, mount_x=args.mount_x, mount_y=args.mount_y, mount_z=args.mount_z,
        mount_idx=args.mount_idx,
        n_hops=args.n_hops, min_radius=args.min_radius,
        use_vf2=args.use_vf2,
        allow_oblique_holes=args.allow_oblique_holes,
        oblique_min_u_sum=args.oblique_min_u_sum,
        oblique_min_v_depth=args.oblique_min_v_depth,
        oblique_mount_adj_hops=args.oblique_mount_adj_hops,
        allow_slot_holes=args.allow_slot_holes,
        slot_mount_adj_hops=args.slot_mount_adj_hops,
    )

    same_groups = []
    target_group = None
    if args.find_all_same and hole_groups:
        target_group = hole_groups[0]
        target_r = round(target_group["radius"], 2)
        same_groups = search_same_spec_in_model(
            shape, target_r, tolerance=args.same_tolerance, mount_normal=mount_normal)
        # 用同一组着色（同规格同色）
        if same_groups:
            hole_groups = same_groups

    parts = [f"结果: {len(hole_groups)} 个孔组"]
    if same_groups:
        parts.append(f"同规格扩展: {len(same_groups)} 组")
    else:
        parts.append(f"{len(transition_faces)} 个过渡面")
    print()
    print(", ".join(parts))

    colorize(shape, faces, mount_idx, hole_groups, transition_faces, args.output, same_groups)
    if same_groups and target_group:
        tr = target_group["radius"]
        print()
        print(f"全模型同规格孔 (R={tr:.1f}mm):")
        for g in same_groups:
            marker = " <-- 安装面邻域" if g is target_group else ""
            r = g["radius"]
            idx = g["best_idx"] + 1
            faces_str = ", ".join(f"Face{fi+1}" for fi in sorted(g["face_indices"]))
            faces_str = ", ".join(f"Face{fi+1}" for fi in sorted(g['face_indices']))
            print(f"  R={r:.1f}mm | {len(g['face_indices'])} 面段 | best=Face{idx} | faces: {faces_str}{marker}")


    print()
    print("颜色说明:")
    print("  🔵 蓝色 = 种子面（安装面）")
    for gi, g in enumerate(hole_groups):
        icons = ["🔴", "🟢", "🟡", "🟠", "🟣", "🔵"]
        r = g["radius"]
        nf = len(g["face_indices"])
        print(f"  {icons[gi%6]}  = 孔组 R={r:.1f}mm ({nf} 面)")
    print("  ⚪ 白色 = 过渡面（圆角/倒角）")
    print("  ⬜ 灰色 = 其他")

if __name__ == "__main__":
    main()