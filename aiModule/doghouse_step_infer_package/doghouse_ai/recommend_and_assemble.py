"""种子面→找孔→推荐第1名卡扣→装到所有同规格孔."""
import os, sys, json, math

# Hybrid mode loads PyTorch/UV-Net before OCC assembly. On Windows/conda this can
# load both LLVM OpenMP and Intel OpenMP; allow the process to continue instead
# of aborting during the later geometric placement stage.
os.environ.setdefault("KMP_DUPLICATE_LIB_OK", "TRUE")
os.environ.setdefault("OMP_NUM_THREADS", "1")

from pathlib import Path
import numpy as np
import rule_pillar as rp
from rule_pillar import search_same_spec_in_model, process_step

HERE = Path(__file__).parent
sys.path.insert(0, str(HERE))

def _default_uvnet_checkpoint() -> str:
    try:
        from uvnet_adapter.defaults import DEFAULT_CHECKPOINT

        return str(DEFAULT_CHECKPOINT)
    except ImportError:
        return ""


from OCC.Core.STEPControl import STEPControl_Reader, STEPControl_Writer, STEPControl_AsIs
from OCC.Core.IFSelect import IFSelect_RetDone
from OCC.Core.TopExp import TopExp_Explorer
from OCC.Core.TopAbs import TopAbs_FACE, TopAbs_EDGE
from OCC.Core.BRepAdaptor import BRepAdaptor_Surface
from OCC.Core.GeomAbs import GeomAbs_Cylinder, GeomAbs_Plane, GeomAbs_Cone, GeomAbs_Torus
from OCC.Core.BRepGProp import brepgprop
from OCC.Core.GProp import GProp_GProps
from OCC.Core.gp import gp_Pnt, gp_Vec, gp_Dir, gp_Trsf, gp_Ax1
from OCC.Core.BRepBuilderAPI import BRepBuilderAPI_Transform
from OCC.Core.BRep import BRep_Builder
from OCC.Core.TopoDS import TopoDS_Compound


def load_step(path):
    rd = STEPControl_Reader()
    rd.ReadFile(path); rd.TransferRoots()
    shape = rd.OneShape()
    faces = []
    exp = TopExp_Explorer(shape, TopAbs_FACE)
    while exp.More(): faces.append(exp.Current()); exp.Next()
    return shape, faces


def build_face_adjacency(shape, faces):
    """Build undirected adjacency between face indices via shared edges."""
    from collections import defaultdict
    from OCC.Extend.TopologyUtils import TopologyExplorer

    topo = TopologyExplorer(shape)
    fmap = {hash(f): i for i, f in enumerate(faces)}
    adj = defaultdict(set)
    exp = TopExp_Explorer(shape, TopAbs_EDGE)
    while exp.More():
        edge = exp.Current()
        edge_faces = list(topo.faces_from_edge(edge))
        if len(edge_faces) != 2:
            exp.Next()
            continue
        i1 = fmap.get(hash(edge_faces[0]))
        i2 = fmap.get(hash(edge_faces[1]))
        if i1 is None or i2 is None or i1 == i2:
            exp.Next()
            continue
        adj[i1].add(i2)
        adj[i2].add(i1)
        exp.Next()
    return adj


def face_neighborhood(face_indices, adj, max_hops=3):
    """Faces reachable within max_hops from any seed face index."""
    seeds = {int(i) for i in face_indices if i is not None}
    visited = dict.fromkeys(seeds, 0)
    queue = list(seeds)
    while queue:
        cur = queue.pop(0)
        depth = visited[cur]
        if depth >= max_hops:
            continue
        for nb in adj.get(cur, ()):
            if nb not in visited:
                visited[nb] = depth + 1
                queue.append(nb)
    return set(visited.keys())


def cyl_info(face):
    """获取柱/锥面的 (轴线中点, 轴方向, 半径).

    注意: 返回的 center 是轴线上中点, 不是面上点 (用于平移对齐).
    """
    sf = BRepAdaptor_Surface(face, True)
    ft = sf.GetType()
    if ft == GeomAbs_Cylinder:
        c = sf.Cylinder()
        r = c.Radius()
        ax = c.Axis().Direction(); loc = c.Axis().Location()
        v_mid = (sf.FirstVParameter() + sf.LastVParameter()) / 2
        # 轴线中点 = location + axis * v_mid
        cx = loc.X() + ax.X() * v_mid
        cy = loc.Y() + ax.Y() * v_mid
        cz = loc.Z() + ax.Z() * v_mid
        return (cx, cy, cz), (ax.X(), ax.Y(), ax.Z()), r
    if ft == GeomAbs_Cone:
        co = sf.Cone()
        r = co.RefRadius()
        ax = co.Axis().Direction(); loc = co.Axis().Location()
        v_mid = (sf.FirstVParameter() + sf.LastVParameter()) / 2
        # 锥面: 轴线中点 = location + axis * v_mid (v=0 在 apex)
        cx = loc.X() + ax.X() * v_mid
        cy = loc.Y() + ax.Y() * v_mid
        cz = loc.Z() + ax.Z() * v_mid
        return (cx, cy, cz), (ax.X(), ax.Y(), ax.Z()), r
    return None, None, None


def plane_info(face):
    """获取面的中心点 + 法向.

    部分 STEP 会把视觉上的平面导入为 BSpline/其他曲面, 所以最后会
    fallback 到面质心处的切平面法向, 供 CONTACT 贴合约束使用。
    """
    sf = BRepAdaptor_Surface(face, True)
    ft = sf.GetType()
    if ft == GeomAbs_Plane:
        ax = sf.Plane().Axis().Direction()
        p = GProp_GProps(); brepgprop.SurfaceProperties(face, p)
        c = p.CentreOfMass()
        ret_center = (c.X(), c.Y(), c.Z())
    elif ft == GeomAbs_Torus:
        ax = sf.Torus().Axis().Direction()
        loc = sf.Torus().Axis().Location()
        ret_center = (loc.X(), loc.Y(), loc.Z())
    elif ft == GeomAbs_Cone:
        cone = sf.Cone()
        if abs(cone.SemiAngle()) < 1.3:
            return None, None
        ax = cone.Axis().Direction()
        loc = cone.Axis().Location()
        ret_center = (loc.X(), loc.Y(), loc.Z())
    else:
        try:
            from OCC.Core.BRep import BRep_Tool
            from OCC.Core.GeomLProp import GeomLProp_SLProps
            from OCC.Core.ShapeAnalysis import ShapeAnalysis_Surface
            p = GProp_GProps(); brepgprop.SurfaceProperties(face, p)
            c = p.CentreOfMass()
            geom_surf = BRep_Tool.Surface(face)
            sas = ShapeAnalysis_Surface(geom_surf)
            uv = sas.ValueOfUV(gp_Pnt(c.X(), c.Y(), c.Z()), 0.01)
            props = GeomLProp_SLProps(geom_surf, uv.X(), uv.Y(), 1, 1e-6)
            if not props.IsNormalDefined():
                return None, None
            ax = props.Normal()
            ret_center = (c.X(), c.Y(), c.Z())
        except Exception:
            return None, None
    if face.Orientation() == 1: ax.Reverse()
    return ret_center, (ax.X(), ax.Y(), ax.Z())


def _planar_face_anchor(face):
    """Return a point guaranteed on a planar face and its outward normal."""
    sf = BRepAdaptor_Surface(face, True)
    if sf.GetType() == GeomAbs_Plane:
        pln = sf.Plane()
        loc = pln.Location()
        ax = pln.Axis().Direction()
        if face.Orientation() == 1:
            ax.Reverse()
        point = np.array([loc.X(), loc.Y(), loc.Z()], dtype=float)
        normal = np.array([ax.X(), ax.Y(), ax.Z()], dtype=float)
        normal /= np.linalg.norm(normal) + 1e-8
        return point, normal
    center, normal = plane_info(face)
    if center is None or normal is None:
        return None, None
    n = np.array(normal, dtype=float)
    n /= np.linalg.norm(n) + 1e-8
    return np.array(center, dtype=float), n


def _refine_transform_mount_contact_gap(trsf, clip_shape, contact_face_idx, mount_face, mount_normal):
    """Nudge clip along mount normal until CONTACT and mount faces meet."""
    try:
        from OCC.Core.BRepBuilderAPI import BRepBuilderAPI_Transform
        from OCC.Core.BRepExtrema import BRepExtrema_DistShapeShape

        clip_faces = []
        exp = TopExp_Explorer(clip_shape, TopAbs_FACE)
        while exp.More():
            clip_faces.append(exp.Current())
            exp.Next()
        if not (0 <= contact_face_idx < len(clip_faces)):
            return trsf, None

        moved = BRepBuilderAPI_Transform(clip_faces[contact_face_idx], trsf, True).Shape()
        dist = BRepExtrema_DistShapeShape(moved, mount_face)
        dist.Perform()
        if not dist.IsDone() or dist.NbSolution() < 1:
            return trsf, None
        gap_before = float(dist.Value())
        if gap_before < 1e-7:
            return trsf, gap_before

        p_clip = dist.PointOnShape1(1)
        p_mount = dist.PointOnShape2(1)
        delta = np.array(
            [p_mount.X() - p_clip.X(), p_mount.Y() - p_clip.Y(), p_mount.Z() - p_clip.Z()],
            dtype=float,
        )
        mn = np.array(mount_normal, dtype=float)
        mn /= np.linalg.norm(mn) + 1e-8
        corr_along_n = float(np.dot(delta, mn))
        if abs(corr_along_n) < 1e-7:
            return trsf, gap_before

        corr = corr_along_n * mn
        t = trsf.TranslationPart()
        trsf.SetTranslationPart(
            gp_Vec(
                float(t.X() + corr[0]),
                float(t.Y() + corr[1]),
                float(t.Z() + corr[2]),
            )
        )
        return trsf, gap_before
    except Exception:
        return trsf, None


def find_holes(faces, seed_idx=None, radius=4.8, tol=0.5):
    """检测指定半径的孔组（同规格）。"""
    from collections import defaultdict
    sig_map = {}
    for i, f in enumerate(faces):
        sf = BRepAdaptor_Surface(f, True)
        ft = sf.GetType()
        if ft not in (GeomAbs_Cylinder, GeomAbs_Cone): continue
        try:
            if ft == GeomAbs_Cylinder:
                r = sf.Cylinder().Radius(); ax_d = sf.Cylinder().Axis().Direction()
            else:
                co = sf.Cone()
                if abs(co.SemiAngle()) >= 0.087: continue
                r = co.RefRadius(); ax_d = co.Axis().Direction()
        except: continue
        if abs(r - radius) > tol: continue
        vd = abs(sf.LastVParameter() - sf.FirstVParameter())
        if vd < 0.5: continue
        sig = (round(r, 2), round(ax_d.X(), 3), round(ax_d.Y(), 3), round(ax_d.Z(), 3))
        g = sig_map.get(sig)
        if g is None:
            sig_map[sig] = {'r': r, 'faces': [], 'usum': 0.0, 'vmax': 0.0,
                            'ax': (ax_d.X(), ax_d.Y(), ax_d.Z())}
            g = sig_map[sig]
        g['faces'].append(i)
        g['usum'] += abs(sf.LastUParameter() - sf.FirstUParameter())
        if vd > g['vmax']: g['vmax'] = vd

    valid = [g for g in sig_map.values() if g['usum'] >= math.pi]
    if seed_idx is not None:
        # 找种子面所在的孔组
        seed_g = None
        for g in valid:
            if seed_idx in g['faces']:
                seed_g = g; break
        if seed_g is None:
            print("  ✗ 种子面未在孔组中"); return [], None
        target_r = seed_g['r']
    else:
        target_r = radius

    # 空间聚类
    CLUSTER_DIST = 20.0
    groups = []
    for g in valid:
        if abs(g['r'] - target_r) > tol: continue
        centers = []
        for fi in g['faces']:
            c, _, _ = cyl_info(faces[fi])
            if c: centers.append(np.array(c))
        if not centers: continue
        assigned = [False] * len(centers)
        for ii in range(len(centers)):
            if assigned[ii]: continue
            cl = [g['faces'][ii]]; assigned[ii] = True
            for jj in range(ii+1, len(centers)):
                if not assigned[jj] and np.linalg.norm(centers[ii]-centers[jj]) < CLUSTER_DIST:
                    cl.append(g['faces'][jj]); assigned[jj] = True
            if cl:
                c0, a0, r0 = cyl_info(faces[cl[0]])
                if c0:
                    groups.append({'center': c0, 'axis': a0, 'radius': r0,
                                    'face_idx': cl[0]})
    return groups, target_r


def find_mount_face(
    faces,
    hc,
    ha,
    hr,
    preferred_normal=None,
    uvnet_prediction=None,
    min_mount_prob=0.35,
    require_uvnet_mount=False,
    hole_face_indices=None,
    face_adj=None,
    mount_hops=3,
    seed_mount=None,
):
    """为单个孔选择局部安装面。

    优先在孔壁邻域内的平面中，选择与 seed 安装面同侧、最外侧的 seating 面
    （避免误选孔口内侧台阶面，如 pillar 的 Face820 vs Face961）。
    """
    hc_a = np.array(hc, dtype=float)
    ha_a = np.array(ha, dtype=float) / (np.linalg.norm(ha) + 1e-8)
    pref_n = None
    seed_center = None
    if preferred_normal is not None:
        pref_n = np.array(preferred_normal, dtype=float)
        pref_n /= np.linalg.norm(pref_n) + 1e-8
    if seed_mount is not None:
        seed_center = np.array(seed_mount[1], dtype=float)
        if pref_n is None:
            pref_n = np.array(seed_mount[2], dtype=float)
            pref_n /= np.linalg.norm(pref_n) + 1e-8
    mount_id = None
    if uvnet_prediction is not None:
        mount_id = uvnet_prediction.class_id("MOUNT_FACE")

    local_faces = None
    if hole_face_indices and face_adj is not None:
        local_faces = face_neighborhood(hole_face_indices, face_adj, max_hops=mount_hops)
        if seed_mount is not None:
            local_faces = set(local_faces)
            local_faces.add(int(seed_mount[0]))

    def _collect_candidates(search_local_faces):
        out = []
        for i, f in enumerate(faces):
            if search_local_faces is not None and i not in search_local_faces:
                continue
            sf = BRepAdaptor_Surface(f, True)
            if sf.GetType() != GeomAbs_Plane:
                continue
            ax = sf.Plane().Axis().Direction()
            if f.Orientation() == 1:
                ax.Reverse()
            n = np.array([ax.X(), ax.Y(), ax.Z()], dtype=float)
            n /= np.linalg.norm(n)
            if pref_n is not None:
                if abs(np.dot(n, pref_n)) < 0.85:
                    continue
                if np.dot(n, pref_n) < 0:
                    n = -n
            elif abs(np.dot(n, ha_a)) < 0.7:
                continue
            p = GProp_GProps()
            brepgprop.SurfaceProperties(f, p)
            c = np.array(
                [p.CentreOfMass().X(), p.CentreOfMass().Y(), p.CentreOfMass().Z()],
                dtype=float,
            )
            proj = project_point_to_plane(hc_a, c, n)
            axial = np.dot(proj - hc_a, ha_a)
            axis_point = hc_a + axial * ha_a
            if pref_n is not None and abs(float(np.dot(ha_a, pref_n))) > 0.85:
                # 孔轴 ∥ 安装面法向时，用孔轴到面心的垂直距离（XZ 偏移），避免远处共面被误选
                offset = c - hc_a
                axis_dist = float(np.linalg.norm(offset - np.dot(offset, ha_a) * ha_a))
            else:
                axis_dist = float(np.linalg.norm(proj - axis_point))
            plane_gap = abs(float(np.dot(hc_a - c, n)))
            if plane_gap > max(hr * 2.5, 12.0):
                continue
            outward = float(np.dot(proj - hc_a, pref_n)) if pref_n is not None else 0.0
            seed_plane_offset = (
                abs(float(np.dot(c - seed_center, pref_n)))
                if seed_center is not None
                else 0.0
            )
            mount_prob = 0.0
            mount_pred = False
            if mount_id is not None and 0 <= i < len(uvnet_prediction.probs):
                mount_prob = float(uvnet_prediction.probs[i, mount_id])
                mount_pred = int(uvnet_prediction.pred_ids[i]) == mount_id
            out.append(
                {
                    "idx": i,
                    "center": c,
                    "normal": n,
                    "axis_dist": axis_dist,
                    "plane_gap": plane_gap,
                    "outward": outward,
                    "seed_plane_offset": seed_plane_offset,
                    "mount_prob": mount_prob,
                    "mount_pred": mount_pred,
                    "is_seed_mount": seed_mount is not None and i == int(seed_mount[0]),
                    "adjacent_hole": any(
                        i in face_adj.get(int(hf), ()) for hf in (hole_face_indices or ())
                    ) if face_adj is not None else False,
                }
            )
        return out

    candidates = _collect_candidates(local_faces)

    if (
        hole_face_indices
        and face_adj is not None
        and require_uvnet_mount
        and pref_n is None
    ):
        adjacent_pool = [c for c in candidates if c["adjacent_hole"]]
        if adjacent_pool:
            candidates = adjacent_pool

    if pref_n is not None:
        positive = [c for c in candidates if c["outward"] >= 0.05]
        if not positive:
            candidates = _collect_candidates(None)
            positive = [c for c in candidates if c["outward"] >= 0.05]
        if positive:
            candidates = positive

    if not candidates and local_faces is not None:
        return find_mount_face(
            faces,
            hc,
            ha,
            hr,
            preferred_normal=preferred_normal,
            uvnet_prediction=uvnet_prediction,
            min_mount_prob=min_mount_prob,
            require_uvnet_mount=require_uvnet_mount,
            hole_face_indices=None,
            face_adj=face_adj,
            mount_hops=mount_hops,
            seed_mount=seed_mount,
        )

    if not candidates:
        return None

    if mount_id is not None and pref_n is None:
        mount_pool = [
            cand
            for cand in candidates
            if cand["mount_pred"] or cand["mount_prob"] >= min_mount_prob
        ]
        if mount_pool:
            candidates = mount_pool

    if pref_n is not None:
        axis_tol = max(hr * 0.75, 4.0)
        near_axis = [c for c in candidates if c["axis_dist"] <= axis_tol]
        if not near_axis:
            near_axis = [c for c in candidates if c["axis_dist"] <= max(hr * 2.5, 12.0)]
        if not near_axis:
            return None
        pool = near_axis
        best = max(
            pool,
            key=lambda c: (
                c["is_seed_mount"],
                c["outward"],
                c["mount_prob"],
                -c["seed_plane_offset"],
                -c["axis_dist"],
                -c["plane_gap"],
            ),
        )
    else:
        best = min(
            candidates,
            key=lambda c: (c["plane_gap"], c["axis_dist"], -c["mount_prob"], abs(c["outward"])),
        )
    return (best["idx"], best["center"], best["normal"])


def _seed_near_face_indices(seed_hole_groups: list[dict]) -> set[int]:
    """Collect face indices from holes detected near the seed mount face."""
    out: set[int] = set()
    for group in seed_hole_groups:
        for key in ("face_indices",):
            for idx in group.get(key) or []:
                out.add(int(idx))
        if group.get("face_idx") is not None:
            out.add(int(group["face_idx"]))
        if group.get("best_idx") is not None:
            out.add(int(group["best_idx"]))
    return out


def _hole_group_face_key(group: dict) -> frozenset[int]:
    faces = list(group.get("face_indices") or [])
    if group.get("face_idx") is not None:
        faces.append(int(group["face_idx"]))
    if group.get("best_idx") is not None:
        faces.append(int(group["best_idx"]))
    return frozenset(int(f) for f in faces if f is not None)


def merge_seed_holes_into_groups(
    seed_hole_groups: list[dict],
    hole_groups: list[dict],
    target_r: float,
    radius_tol: float,
) -> list[dict]:
    """doghouse 定域扩孔后补回种子邻域孔, 避免漏掉用户点选处的孔."""
    existing = [_hole_group_face_key(g) for g in hole_groups]
    merged = list(hole_groups)
    for seed in seed_hole_groups:
        if abs(float(seed.get("radius", 0)) - target_r) > radius_tol:
            continue
        key = _hole_group_face_key(seed)
        if not key or any(key & ek for ek in existing):
            continue
        merged.append(dict(seed))
        existing.append(key)
    return merged


def _hole_group_has_mount_face(
    faces,
    hole_group: dict,
    preferred_normal,
    uvnet_prediction,
    min_mount_prob: float,
    face_adj=None,
) -> bool:
    hole_faces = hole_group.get("face_indices") or [hole_group.get("face_idx")]
    mount = find_mount_face(
        faces,
        hole_group["center"],
        hole_group["axis"],
        hole_group["radius"],
        preferred_normal=preferred_normal,
        uvnet_prediction=uvnet_prediction,
        min_mount_prob=min_mount_prob,
        require_uvnet_mount=False,
        hole_face_indices=hole_faces,
        face_adj=face_adj,
    )
    if mount is None:
        return False
    if uvnet_prediction is None:
        return True
    from uvnet_infer import mount_face_uvnet_summary

    mount_summary = mount_face_uvnet_summary(mount[0], uvnet_prediction)
    return bool(mount_summary["mount_pred"]) or float(mount_summary["mount_prob"]) >= min_mount_prob


def _pick_mount_for_hole(
    hole_group: dict,
    faces,
    seed_mount,
    seed_near_faces: set[int],
    preferred_normal,
    uvnet_prediction,
    mount_face_mode: str,
    min_mount_prob: float,
    seed_idx: int,
    face_adj=None,
):
    """按模式为单个孔选择安装面。"""
    hole_faces = {
        int(fi)
        for fi in (hole_group.get("face_indices") or [hole_group.get("face_idx")])
        if fi is not None
    }
    near_seed = bool(hole_faces & seed_near_faces)
    require_uvnet = (
        uvnet_prediction is not None
        and mount_face_mode in ("auto", "hybrid")
        and seed_mount is None
    )
    hole_face_list = list(hole_faces)

    if mount_face_mode == "seed":
        if seed_mount is None:
            return None, "none"
        return seed_mount, "seed"

    if near_seed and seed_mount is not None:
        return seed_mount, "seed(near)"

    if (
        mount_face_mode == "hybrid"
        and seed_mount is not None
        and face_adj is not None
        and any(seed_idx in face_adj.get(hf, ()) for hf in hole_faces)
    ):
        return seed_mount, "local(seed-adj)"

    local_m = find_mount_face(
        faces,
        hole_group["center"],
        hole_group["axis"],
        hole_group["radius"],
        preferred_normal=preferred_normal,
        uvnet_prediction=uvnet_prediction,
        min_mount_prob=min_mount_prob,
        require_uvnet_mount=require_uvnet,
        hole_face_indices=hole_face_list,
        face_adj=face_adj,
        seed_mount=seed_mount,
    )
    if local_m is not None:
        source = "local"
        if local_m[0] == seed_idx:
            source = "local(seed)"
        return local_m, source

    if seed_mount is not None and near_seed:
        return seed_mount, "seed(fallback)"
    return None, "none"


def normalize_hole_group(hole_group: dict, faces) -> dict:
    """Ensure hole group has center/axis/radius for mount pairing."""
    normalized = dict(hole_group)
    if (
        normalized.get("center") is not None
        and normalized.get("axis") is not None
        and normalized.get("geom_axis") is not None
    ):
        return normalized
    face_idx = normalized.get("face_idx")
    if face_idx is None:
        face_idx = normalized.get("best_idx")
    if face_idx is None:
        indices = normalized.get("face_indices") or []
        face_idx = indices[0] if indices else None

    indices = normalized.get("face_indices") or ([int(face_idx)] if face_idx is not None else [])
    if normalized.get("center") is None and indices:
        cents = []
        for fi in indices:
            if fi is None or not (0 <= int(fi) < len(faces)):
                continue
            props = GProp_GProps()
            brepgprop.SurfaceProperties(faces[int(fi)], props)
            c = props.CentreOfMass()
            cents.append(np.array([c.X(), c.Y(), c.Z()], dtype=float))
        if cents:
            normalized["center"] = tuple(np.mean(cents, axis=0).tolist())

    if face_idx is None or not (0 <= int(face_idx) < len(faces)):
        return normalized
    center, axis, radius = cyl_info(faces[int(face_idx)])
    if center is not None and normalized.get("center") is None:
        normalized["center"] = center
    if axis is not None:
        normalized.setdefault("geom_axis", axis)
        if normalized.get("axis") is None:
            normalized["axis"] = axis
    if radius is not None and normalized.get("radius") is None:
        normalized["radius"] = radius
    normalized.setdefault("face_idx", int(face_idx))
    return normalized


def enrich_hole_group_with_mount(
    hole_group: dict,
    faces,
    face_adj,
    seed_mount,
    seed_near_faces: set[int],
    preferred_normal,
    uvnet_prediction,
    mount_face_mode: str,
    min_mount_prob: float,
    seed_idx: int,
) -> dict:
    """Attach local mount face to a hole group (孔-安装面对)."""
    enriched = normalize_hole_group(hole_group, faces)
    if enriched.get("center") is None or enriched.get("axis") is None:
        enriched["mount_valid"] = False
        enriched["mount_source"] = "none"
        return enriched
    m, mount_source = _pick_mount_for_hole(
        enriched,
        faces,
        seed_mount,
        seed_near_faces,
        preferred_normal,
        uvnet_prediction,
        mount_face_mode,
        min_mount_prob,
        seed_idx,
        face_adj=face_adj,
    )
    if m is None:
        enriched["mount_valid"] = False
        enriched["mount_source"] = mount_source
        return enriched

    mount_plane_point = m[1]
    mount_normal = m[2]
    mn = np.array(mount_normal, dtype=float)
    mn /= np.linalg.norm(mn) + 1e-8
    mount_normal = tuple(mn.tolist())
    enriched["axis"] = mount_normal
    opening = opening_center_on_mount(enriched, mount_plane_point, mount_normal)
    mount_point = project_point_to_plane(opening, mount_plane_point, mount_normal)
    enriched["mount"] = (m[0], tuple(mount_point), mount_normal)
    enriched["mount_face_idx"] = m[0]
    enriched["mount_source"] = mount_source
    enriched["mount_valid"] = True
    enriched["placement_center"] = tuple(np.array(opening, dtype=float).tolist())
    if uvnet_prediction is not None:
        from uvnet_infer import mount_face_uvnet_summary

        mount_summary = mount_face_uvnet_summary(m[0], uvnet_prediction)
        enriched["mount_prob"] = float(mount_summary["mount_prob"])
        enriched["mount_pred"] = bool(mount_summary["mount_pred"])
    return enriched


def enrich_hole_groups_with_mount(
    hole_groups: list[dict],
    faces,
    face_adj,
    seed_mount,
    seed_near_faces: set[int],
    preferred_normal,
    uvnet_prediction,
    mount_face_mode: str,
    min_mount_prob: float,
    seed_idx: int,
) -> list[dict]:
    """Attach a mount face to every hole group."""
    return [
        enrich_hole_group_with_mount(
            group,
            faces,
            face_adj,
            seed_mount,
            seed_near_faces,
            preferred_normal,
            uvnet_prediction,
            mount_face_mode,
            min_mount_prob,
            seed_idx,
        )
        for group in hole_groups
    ]


def _seed_hole_mount_template(
    seed_hole_groups: list[dict],
    seed_near_faces: set[int],
    seed_mount,
    target_radius: float | None = None,
) -> dict | None:
    """Reference mount normal / radius from seed-neighborhood hole-mount pairs."""
    candidates = []
    for group in seed_hole_groups:
        if not group.get("mount_valid"):
            continue
        hole_faces = {
            int(fi)
            for fi in (group.get("face_indices") or [group.get("face_idx")])
            if fi is not None
        }
        if not (hole_faces & seed_near_faces):
            continue
        candidates.append(group)
    if target_radius is not None and candidates:
        candidates.sort(key=lambda g: abs(float(g["radius"]) - float(target_radius)))
        group = candidates[0]
    elif candidates:
        group = candidates[0]
    else:
        group = None
    if group is not None:
        return {
            "radius": float(group["radius"]),
            "hole_axis": np.array(group["axis"], dtype=float),
            "mount_normal": np.array(group["mount"][2], dtype=float),
            "mount_face_idx": int(group["mount"][0]),
        }
    if seed_mount is not None:
        return {
            "mount_normal": np.array(seed_mount[2], dtype=float),
            "mount_face_idx": int(seed_mount[0]),
        }
    return None


def align_hole_group_frame(hole_group: dict, ref: dict | None) -> dict:
    """Align hole axis and mount normal to seed reference (fix 180° flips)."""
    if ref is None:
        return hole_group
    ha = np.array(hole_group["axis"], dtype=float)
    ha /= np.linalg.norm(ha) + 1e-8
    ref_ha = ref.get("hole_axis")
    if ref_ha is not None:
        ref_ha = np.array(ref_ha, dtype=float)
        ref_ha /= np.linalg.norm(ref_ha) + 1e-8
        if float(np.dot(ha, ref_ha)) < 0:
            ha = -ha
            hole_group["axis"] = tuple(ha.tolist())

    if hole_group.get("mount") and ref.get("mount_normal") is not None:
        mount_idx, mount_point, mount_normal = hole_group["mount"]
        mn = np.array(mount_normal, dtype=float)
        mn /= np.linalg.norm(mn) + 1e-8
        ref_mn = np.array(ref["mount_normal"], dtype=float)
        ref_mn /= np.linalg.norm(ref_mn) + 1e-8
        if float(np.dot(mn, ref_mn)) < 0:
            mn = -mn
        hole_group["mount"] = (mount_idx, mount_point, tuple(mn.tolist()))
        opening = opening_center_on_mount(hole_group, mount_point, tuple(mn.tolist()))
        hole_group["placement_center"] = tuple(np.array(opening, dtype=float).tolist())
    return hole_group


def align_hole_groups_to_seed_frame(hole_groups: list[dict], ref: dict | None) -> list[dict]:
    """Align every hole-mount pair to the seed hole orientation."""
    return [align_hole_group_frame(group, ref) for group in hole_groups]


def project_point_to_plane(point, plane_point, plane_normal):
    """将 point 投影到由 plane_point + plane_normal 定义的平面上."""
    p = np.array(point, dtype=float)
    pp = np.array(plane_point, dtype=float)
    n = np.array(plane_normal, dtype=float)
    n /= np.linalg.norm(n) + 1e-8
    return p - np.dot(p - pp, n) * n


def hole_axis_point_at_mount(hole_center, hole_axis, mount_point, mount_normal):
    """孔轴线与安装面的交点, 用作卡扣 BOLT_CYL 横向对齐参考."""
    hc = np.array(hole_center, dtype=float)
    ha = np.array(hole_axis, dtype=float)
    ha /= np.linalg.norm(ha) + 1e-8
    mp = np.array(mount_point, dtype=float)
    mn = np.array(mount_normal, dtype=float)
    mn /= np.linalg.norm(mn) + 1e-8
    denom = float(np.dot(ha, mn))
    if abs(denom) < 1e-6:
        return project_point_to_plane(hc, mp, mn)
    t = float(np.dot(mp - hc, mn) / denom)
    return hc + t * ha


def placement_axis_for_hole(hole_group: dict):
    """Axis used to locate the hole opening on the mount face."""
    return hole_group.get("geom_axis") or hole_group.get("axis")


def opening_center_on_mount(hole_group: dict, mount_plane_point, mount_normal):
    """Hole opening anchor on the mount face (BOLT_CYL lateral target)."""
    center = hole_group.get("center")
    axis = placement_axis_for_hole(hole_group)
    if center is None or axis is None:
        return center
    return hole_axis_point_at_mount(center, axis, mount_plane_point, mount_normal)


def resolve_auto_prediction_json(step_path: str | Path, explicit: str | Path | None = None) -> Path | None:
    """Resolve the doghouse prediction/annotation JSON next to a STEP file."""
    if explicit:
        path = Path(explicit)
        return path if path.exists() else None
    step_path = Path(step_path)
    stem = step_path.stem
    candidates = [
        step_path.with_name(f"{stem}_doghouse_pred_faces.json"),
        step_path.with_name(f"{stem}_annotation.json"),
        step_path.with_name(f"{stem} annotation.json"),
        step_path.with_suffix(".json"),
    ]
    return next((path for path in candidates if path.exists()), None)


def _doghouse_infer_output_dir(args) -> Path:
    explicit = getattr(args, "doghouse_infer_output_dir", "") or ""
    if explicit:
        return Path(explicit)
    for attr in ("recommend_output", "placement_output", "assembly_features_output"):
        value = getattr(args, attr, "") or ""
        if value:
            return Path(value).parent / "infer"
    return Path("outputs") / "doghouse_infer" / Path(args.step).stem


def _load_or_infer_prediction(args) -> tuple[dict | None, Path | None]:
    explicit_prediction = getattr(args, "prediction_json", "") or ""
    if explicit_prediction:
        pred_path = resolve_auto_prediction_json(args.step, explicit_prediction)
        if pred_path is None:
            raise FileNotFoundError(f"prediction JSON not found: {explicit_prediction}")
        print(f"  doghouse 预测 JSON: {pred_path}")
        with open(pred_path, encoding="utf-8") as f:
            return json.load(f), pred_path

    from argparse import Namespace
    from infer_from_step import infer_step
    from pipeline_defaults import DEFAULT_EDGE_THRESHOLD, DEFAULT_MIN_INSTANCE_FACES, DEFAULT_NODE_THRESHOLD

    output_dir = _doghouse_infer_output_dir(args)
    output_dir.mkdir(parents=True, exist_ok=True)
    print("  未提供 annotation，开始自动推理 doghouse ...")
    print(f"  inference output: {output_dir}")
    infer_args = Namespace(
        step=str(args.step),
        output_dir=output_dir,
        checkpoint=getattr(args, "doghouse_checkpoint", None) or None,
        structure_checkpoint=getattr(args, "doghouse_structure_checkpoint", None) or None,
        backbone="graph",
        sample_points_per_face=64,
        threshold=0.5,
        node_threshold=float(getattr(args, "doghouse_node_threshold", DEFAULT_NODE_THRESHOLD)),
        edge_threshold=float(getattr(args, "doghouse_edge_threshold", DEFAULT_EDGE_THRESHOLD)),
        min_instance_faces=int(getattr(args, "doghouse_min_instance_faces", DEFAULT_MIN_INSTANCE_FACES)),
        instance_sim_filter=True,
        instance_sim_gallery=None,
        instance_sim_threshold=None,
        instance_filter=None,
        instance_filter_threshold=None,
        pmae_face_emb_dir=None,
        pmae_ckpt=None,
        pmae_input_dir=None,
        pmae_num_group=256,
        pmae_group_size=32,
        pmae_cpu=bool(getattr(args, "infer_cpu", True)),
        min_component_faces=8,
        close_ratio=0.35,
        close_iters=1,
        cpu=bool(getattr(args, "infer_cpu", True)),
        extract_assembly_features=False,
        assembly_output_step=None,
        use_vf2=bool(getattr(args, "auto_use_vf2", True)),
        vf2_required=False,
        prefer_ai_holes=True,
        ai_mount_score_threshold=0.35,
        ai_hole_score_threshold=0.35,
        ai_hole_min_confidence=0.35,
        experimental_freeform_endpoint=False,
        hole_wall_threshold=0.35,
        mount_threshold=0.35,
    )
    prediction = infer_step(infer_args)
    return prediction, None


def assembly_features_to_hole_groups(assembly_features: dict, faces=None) -> list[dict]:
    """Convert instance-level doghouse assembly features to clip placement groups."""
    out = []
    for inst in assembly_features.get("instances", []):
        mount = inst.get("mount_face") or {}
        mount_idx = mount.get("face_idx")
        mount_center = mount.get("center")
        mount_normal = mount.get("normal")
        if mount_idx is None or mount_center is None or mount_normal is None:
            continue
        mn = np.array(mount_normal, dtype=float)
        if np.linalg.norm(mn) <= 1e-8:
            continue
        mn /= np.linalg.norm(mn) + 1e-8
        mp = tuple(np.array(mount_center, dtype=float).tolist())

        for hole in inst.get("hole_groups", []):
            group = dict(hole)
            if faces is not None:
                group = normalize_hole_group(group, faces)
            if group.get("center") is None or group.get("radius") is None:
                continue
            original_axis = group.get("axis")
            if original_axis is not None:
                group.setdefault("geom_axis", tuple(np.array(original_axis, dtype=float).tolist()))
            group["axis"] = tuple(mn.tolist())
            group["mount"] = (int(mount_idx), mp, tuple(mn.tolist()))
            group["mount_face_idx"] = int(mount_idx)
            group["mount_valid"] = True
            group["mount_source"] = "assembly_features"
            group["instance_id"] = int(inst.get("instance_id", len(out) + 1))
            if group.get("depth_mm") is None:
                for depth_key in ("v_max", "v_depth", "depth"):
                    if group.get(depth_key) is not None and float(group.get(depth_key) or 0.0) > 0:
                        group["depth_mm"] = float(group[depth_key])
                        break
            group["placement_center"] = tuple(
                np.array(opening_center_on_mount(group, mp, mn), dtype=float).tolist()
            )
            out.append(group)
    return out


def _load_or_extract_assembly_features(args, all_faces=None) -> dict | None:
    if args.assembly_features_json:
        with open(args.assembly_features_json, encoding="utf-8") as f:
            return json.load(f)

    prediction, pred_path = _load_or_infer_prediction(args)
    if prediction is None:
        print("  ✗ doghouse 自动推理失败")
        return None

    from doghouse_assembly_features import extract_assembly_features

    if pred_path is not None:
        print(f"  doghouse 预测/标注: {pred_path}")
    features = extract_assembly_features(
        args.step,
        prediction,
        use_vf2=bool(args.auto_use_vf2),
        prefer_ai_structure_fallback=True,
    )
    if args.assembly_features_output:
        out_path = Path(args.assembly_features_output)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(json.dumps(features, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"  assembly features: {out_path}")
    return features


def _select_target_hole_groups(hole_groups: list[dict], clip_defs_raw: list[tuple], tolerance: float) -> tuple[list[dict], float]:
    clip_radii = [float(r) for _, r, *_ in clip_defs_raw]

    def clip_radius_score(group):
        hole_diameter = 2.0 * float(group["radius"])
        return min(
            abs((hole_diameter - 2.0 * cr) - 2.25)
            for cr in clip_radii
            if hole_diameter > 2.0 * cr
        )

    ranked = sorted(
        hole_groups,
        key=lambda g: (clip_radius_score(g), -float(g.get("u_sum") or 0.0), float(g["radius"])),
    )
    target_r = float(ranked[0]["radius"])
    spec_tol = max(0.25, float(tolerance))
    selected = [g for g in hole_groups if abs(float(g["radius"]) - target_r) <= spec_tol]
    return selected, target_r


def _hole_depth_for_groups(hole_groups: list[dict]) -> float:
    """Representative doghouse hole height/thickness from detected hole groups."""
    depths = []
    for group in hole_groups:
        for key in ("depth_mm", "v_max", "v_depth", "depth"):
            value = group.get(key)
            if value is None:
                continue
            depth = float(value)
            if depth > 0:
                depths.append(depth)
                break
    if not depths:
        return 0.0
    return float(np.median(np.asarray(depths, dtype=float)))


def _json_vec(values, default=(0.0, 0.0, 0.0)) -> list[float]:
    if values is None:
        values = default
    return [round(float(x), 6) for x in values]


def _hole_face_indices(group: dict) -> list[int]:
    values = group.get("face_indices")
    if not values and group.get("face_idx") is not None:
        values = [group.get("face_idx")]
    return [int(idx) for idx in (values or []) if idx is not None]


def _hole_payload(group: dict, idx: int, target_r: float, spec_index: int | None = None) -> dict:
    payload = {
        "index": idx + 1,
        "instance_id": group.get("instance_id"),
        "center": _json_vec(group.get("center", (0.0, 0.0, 0.0))),
        "radius_mm": round(float(group.get("radius", target_r)), 6),
        "depth_mm": round(float(group.get("depth_mm", group.get("v_max", 0.0)) or 0.0), 6),
        "mount_face_idx": group.get("mount_face_idx"),
        "hole_face_indices": _hole_face_indices(group),
    }
    if spec_index is not None:
        payload["spec_index"] = spec_index
    return payload


def _clip_payloads(ranked: list[tuple], spec_index: int | None = None, hole_count: int | None = None) -> list[dict]:
    clips = []
    for rank, (score, _ideal, d_gap, stem, clip_r, diameter_gap, clip_depth, step_path, _cj) in enumerate(ranked, start=1):
        payload = {
            "rank": rank,
            "name": stem,
            "clip_radius_mm": round(float(clip_r), 6),
            "clip_diameter_mm": round(2.0 * float(clip_r), 6),
            "clip_depth_mm": round(float(clip_depth), 6),
            "clip_step": str(step_path),
            "diameter_gap_mm": round(float(diameter_gap), 6),
            "depth_gap_mm": round(float(d_gap), 6),
            "diameter_valid": _diameter_valid(float(diameter_gap)),
            "score": round(float(score), 6),
        }
        if spec_index is not None:
            payload["spec_index"] = spec_index
        if hole_count is not None:
            payload["hole_count"] = hole_count
        clips.append(payload)
    return clips


def _group_hole_specs(hole_groups: list[dict], tolerance: float = 0.5) -> list[list[dict]]:
    spec_tol = max(0.25, float(tolerance))
    groups: list[list[dict]] = []
    for hole in sorted(hole_groups, key=lambda h: float(h["radius"])):
        for group in groups:
            if abs(float(group[0]["radius"]) - float(hole["radius"])) <= spec_tol:
                group.append(hole)
                break
        else:
            groups.append([hole])
    groups.sort(key=lambda group: (-len(group), float(group[0]["radius"])))
    return groups


def _majority_hole_group(hole_groups: list[dict], tolerance: float = 0.5) -> list[dict]:
    groups = _group_hole_specs(hole_groups, tolerance)
    return groups[0] if groups else []


def _diameter_valid(diameter_gap_mm: float) -> bool:
    return 1.5 <= float(diameter_gap_mm) <= 3.0


def build_clip_recommendation_payload(
    hole_groups: list[dict],
    clip_defs_raw: list[tuple],
    *,
    tolerance: float = 0.5,
    source_step: str | Path | None = None,
    all_hole_specs: bool = False,
    all_holes_same_clip: bool = False,
) -> dict:
    """Build JSON-serializable clip recommendation data for UI clients."""
    if not hole_groups:
        return {
            "schema": "doghouse_clip_recommendation.v1",
            "source_step": "" if source_step is None else str(source_step),
            "hole_count": 0,
            "clips": [],
            "selected_clip": None,
        }
    if all_holes_same_clip:
        representative_holes = _majority_hole_group(hole_groups, tolerance)
        target_r = float(representative_holes[0]["radius"])
        hole_depth_mm = _hole_depth_for_groups(representative_holes)
        ranked = rank_clip_defs_for_hole(clip_defs_raw, target_r, hole_depth_mm)
        clips = _clip_payloads(ranked)
        return {
            "schema": "doghouse_clip_recommendation.v1",
            "source_step": "" if source_step is None else str(source_step),
            "mode": "all_holes_same_clip",
            "hole_count": len(hole_groups),
            "representative_hole_count": len(representative_holes),
            "representative_hole_radius_mm": round(float(target_r), 6),
            "representative_hole_diameter_mm": round(2.0 * float(target_r), 6),
            "representative_hole_depth_mm": round(float(hole_depth_mm), 6),
            "hole_radius_mm": round(float(target_r), 6),
            "hole_diameter_mm": round(2.0 * float(target_r), 6),
            "hole_depth_mm": round(float(hole_depth_mm), 6),
            "holes": [
                _hole_payload(group, idx, float(group.get("radius", target_r)))
                for idx, group in enumerate(hole_groups)
            ],
            "clips": clips,
            "selected_clip": clips[0]["name"] if clips else None,
        }
    if all_hole_specs:
        specs = []
        aggregate_clips = []
        seen_clips = set()
        all_holes_payload = []
        for spec_index, spec_holes in enumerate(_group_hole_specs(hole_groups, tolerance), start=1):
            target_r = float(spec_holes[0]["radius"])
            hole_depth_mm = _hole_depth_for_groups(spec_holes)
            ranked = rank_clip_defs_for_hole(clip_defs_raw, target_r, hole_depth_mm)
            clips = _clip_payloads(ranked, spec_index=spec_index, hole_count=len(spec_holes))
            holes_payload = [
                _hole_payload(group, len(all_holes_payload) + idx, target_r, spec_index=spec_index)
                for idx, group in enumerate(spec_holes)
            ]
            all_holes_payload.extend(holes_payload)
            for clip in clips:
                if clip["name"] not in seen_clips:
                    seen_clips.add(clip["name"])
                    aggregate = dict(clip)
                    aggregate["rank"] = len(aggregate_clips) + 1
                    aggregate_clips.append(aggregate)
            specs.append(
                {
                    "spec_index": spec_index,
                    "hole_count": len(spec_holes),
                    "hole_radius_mm": round(float(target_r), 6),
                    "hole_diameter_mm": round(2.0 * float(target_r), 6),
                    "hole_depth_mm": round(float(hole_depth_mm), 6),
                    "holes": holes_payload,
                    "clips": clips,
                    "selected_clip": clips[0]["name"] if clips else None,
                }
            )
        return {
            "schema": "doghouse_clip_recommendation.v1",
            "source_step": "" if source_step is None else str(source_step),
            "mode": "all_hole_specs",
            "hole_count": len(all_holes_payload),
            "holes": all_holes_payload,
            "hole_specs": specs,
            "clips": aggregate_clips,
            "selected_clip": aggregate_clips[0]["name"] if aggregate_clips else None,
            "selected_clips": [
                {"spec_index": spec["spec_index"], "clip": spec["selected_clip"]}
                for spec in specs
                if spec["selected_clip"]
            ],
        }
    selected_holes, target_r = _select_target_hole_groups(hole_groups, clip_defs_raw, tolerance)
    hole_depth_mm = _hole_depth_for_groups(selected_holes)
    ranked = rank_clip_defs_for_hole(clip_defs_raw, target_r, hole_depth_mm)
    clips = _clip_payloads(ranked)
    return {
        "schema": "doghouse_clip_recommendation.v1",
        "source_step": "" if source_step is None else str(source_step),
        "hole_count": len(selected_holes),
        "hole_radius_mm": round(float(target_r), 6),
        "hole_diameter_mm": round(2.0 * float(target_r), 6),
        "hole_depth_mm": round(float(hole_depth_mm), 6),
        "holes": [
            _hole_payload(group, idx, target_r)
            for idx, group in enumerate(selected_holes)
        ],
        "clips": clips,
        "selected_clip": clips[0]["name"] if clips else None,
    }


def _trsf_to_matrix4(trsf) -> list[list[float]]:
    matrix = [
        [float(trsf.Value(1, 1)), float(trsf.Value(1, 2)), float(trsf.Value(1, 3)), float(trsf.Value(1, 4))],
        [float(trsf.Value(2, 1)), float(trsf.Value(2, 2)), float(trsf.Value(2, 3)), float(trsf.Value(2, 4))],
        [float(trsf.Value(3, 1)), float(trsf.Value(3, 2)), float(trsf.Value(3, 3)), float(trsf.Value(3, 4))],
        [0.0, 0.0, 0.0, 1.0],
    ]
    return [[round(v, 9) for v in row] for row in matrix]


def build_clip_placement_payload(
    *,
    all_faces,
    hole_groups: list[dict],
    clip_defs_raw: list[tuple],
    clip_name: str = "",
    clip_contact_faces=None,
    placement_mode: str = "original",
    flip_indices=None,
    invert_direction_indices=None,
    tolerance: float = 0.5,
    all_holes_same_clip: bool = False,
) -> dict:
    """Compute selected clip placements without exporting a final STEP."""
    clip_contact_faces = clip_contact_faces or []
    flip_indices = flip_indices or set()
    invert_direction_indices = invert_direction_indices or set()
    if all_holes_same_clip:
        representative_holes = _majority_hole_group(hole_groups, tolerance)
        target_r = float(representative_holes[0]["radius"])
        hole_depth_mm = _hole_depth_for_groups(representative_holes)
        selected_holes = list(representative_holes) + [
            group for group in hole_groups if group not in representative_holes
        ]
    else:
        selected_holes, target_r = _select_target_hole_groups(hole_groups, clip_defs_raw, tolerance)
        hole_depth_mm = _hole_depth_for_groups(selected_holes)
    ranked = rank_clip_defs_for_hole(clip_defs_raw, target_r, hole_depth_mm)
    if not ranked:
        raise ValueError("no matching clips")
    if clip_name:
        picked = next((r for r in ranked if r[3] == clip_name.strip()), None)
        if picked is None:
            raise ValueError(f"clip not found: {clip_name}")
    else:
        picked = ranked[0]
    _score, _ideal, _d_gap, best_name, clip_r, diameter_gap, clip_depth, clip_step, clip_json = picked
    clip_shape, clip_faces = load_step(clip_step)
    clip_json = remap_clip_bolt_cyl_json(clip_json, clip_faces, best_name)

    seed_ref = {
        "radius": float(selected_holes[0]["radius"]),
        "hole_axis": np.array(selected_holes[0].get("axis"), dtype=float),
        "mount_normal": np.array(selected_holes[0]["mount"][2], dtype=float),
        "mount_face_idx": int(selected_holes[0]["mount_face_idx"]),
    }
    selected_holes = align_hole_groups_to_seed_frame(selected_holes, seed_ref)
    ref_hole_axis = seed_ref["hole_axis"]
    ref_mount_normal = seed_ref["mount_normal"]
    placements = []
    for i, h in enumerate(selected_holes):
        hole_no = i + 1
        flip_side = hole_no in flip_indices
        invert_direction = hole_no in invert_direction_indices
        mount_info = h.get("mount", (None, h["center"], (0, 0, 1)))
        hole_anchor = h.get("placement_center", h["center"])
        if placement_mode == "original":
            trsf = position_clip_by_original_bolt(
                clip_shape, clip_json, hole_anchor, h["axis"], mount_info[1], mount_info[2],
                contact_face_indices=clip_contact_faces,
                pillar_faces=all_faces,
                preferred_mount_face_idx=mount_info[0],
                ref_hole_axis=ref_hole_axis,
                ref_mount_normal=ref_mount_normal,
                invert_direction=invert_direction,
            )
        else:
            trsf = position_clip(
                clip_shape, clip_json, mount_info[1], h["axis"], mount_info[2],
                flip_side=False, invert_direction=invert_direction,
            )
        if trsf is None:
            continue
        if flip_side:
            mp = mount_info[1]
            ha = np.array(h["axis"], dtype=float)
            ha /= np.linalg.norm(ha) + 1e-8
            flip_trsf = gp_Trsf()
            flip_trsf.SetRotation(
                gp_Ax1(gp_Pnt(mp[0], mp[1], mp[2]), gp_Dir(ha[0], ha[1], ha[2])),
                math.pi,
            )
            trsf = flip_trsf.Multiplied(trsf)
        placements.append(
            {
                "hole_index": hole_no,
                "instance_id": h.get("instance_id"),
                "matrix": _trsf_to_matrix4(trsf),
                "flipped": bool(flip_side),
                "invert_direction": bool(invert_direction),
            }
        )
    return {
        "schema": "doghouse_clip_placement.v1",
        "selected_clip": best_name,
        "clip_step": str(clip_step),
        "clip_radius_mm": round(float(clip_r), 6),
        "clip_depth_mm": round(float(clip_depth), 6),
        "diameter_gap_mm": round(float(diameter_gap), 6),
        "placement_count": len(placements),
        "placements": placements,
    }


def _write_json(path: str | Path, payload: dict):
    out_path = Path(path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"  ✓ 写出 JSON: {out_path}")


def _export_positioned_clips(step_shape, positioned, output, step_path):
    b = BRep_Builder(); c = TopoDS_Compound(); b.MakeCompound(c)
    b.Add(c, step_shape)
    for p in positioned:
        b.Add(c, p)
    w = STEPControl_Writer()
    w.Transfer(c, STEPControl_AsIs)
    w.Write(str(output))
    part_name = Path(step_path).stem
    print(f"  ✓ {output} (1 个 {part_name} + {len(positioned)} 个卡扣)")


def assemble_hole_groups_to_step(
    *,
    step_shape,
    all_faces,
    step_path,
    hole_groups,
    clip_defs_raw,
    clip_dir,
    output,
    clip_name="",
    clip_contact_faces=None,
    placement_mode="original",
    flip_indices=None,
    invert_direction_indices=None,
    tolerance=0.5,
) -> int:
    """Recommend one clip and place it on the supplied doghouse hole groups."""
    clip_contact_faces = clip_contact_faces or []
    flip_indices = flip_indices or set()
    invert_direction_indices = invert_direction_indices or set()
    if not hole_groups:
        print("  ✗ 无可用 doghouse 孔-安装面对")
        return 0
    hole_groups, target_r = _select_target_hole_groups(hole_groups, clip_defs_raw, tolerance)
    hole_depth_mm = _hole_depth_for_groups(hole_groups)
    depth_msg = f", 孔高/孔厚={hole_depth_mm:.2f}mm" if hole_depth_mm > 0 else ""
    print(f"  自动选择装配孔规格: R={target_r:.2f}mm{depth_msg}, {len(hole_groups)} 个孔-安装面对")

    print("\n[4] 从卡扣库匹配卡扣 ...")
    ranked = rank_clip_defs_for_hole(clip_defs_raw, target_r, hole_depth_mm)
    for score, _, d_gap, stem, clip_r, diameter_gap, clip_depth, _, _ in ranked:
        depth_part = f"厚度={clip_depth:.2f}mm " if clip_depth > 0 else ""
        hole_depth_part = f"孔高={hole_depth_mm:.2f} " if hole_depth_mm > 0 else ""
        print(
            f"    {stem:20s} R={clip_r:.2f}mm {depth_part}"
            f"(孔R={target_r:.1f} {hole_depth_part}直径差={diameter_gap:.2f}, 厚度差={d_gap:.2f}, score={score:.2f})"
        )
    if not ranked:
        print("  ✗ 无匹配卡扣")
        return 0
    _, _, _, best_name, clip_r, diameter_gap, _, clip_step, clip_json = ranked[0]
    if clip_name:
        picked = next((r for r in ranked if r[3] == clip_name.strip()), None)
        if picked is None:
            print(f"  ✗ 未找到卡扣: {clip_name}")
            return 0
        _, _, _, best_name, clip_r, diameter_gap, _, clip_step, clip_json = picked
        print(f"\n  强制选择: {best_name} (R={clip_r:.2f}mm)")
    else:
        print(f"\n  选择: {best_name} (R={clip_r:.2f}mm, 直径差={diameter_gap:.2f}mm)")

    print(f"\n[5] 加载卡扣文件 ...")
    clip_shape, clip_faces = load_step(clip_step)
    clip_json = remap_clip_bolt_cyl_json(clip_json, clip_faces, best_name)
    print(f"  ✓ {best_name} ({len(clip_faces)} 面)")

    seed_ref = {
        "radius": float(hole_groups[0]["radius"]),
        "hole_axis": np.array(hole_groups[0].get("axis"), dtype=float),
        "mount_normal": np.array(hole_groups[0]["mount"][2], dtype=float),
        "mount_face_idx": int(hole_groups[0]["mount_face_idx"]),
    }
    hole_groups = align_hole_groups_to_seed_frame(hole_groups, seed_ref)
    ref_hole_axis = seed_ref["hole_axis"]
    ref_mount_normal = seed_ref["mount_normal"]

    print(f"\n[6] 装到 {len(hole_groups)} 个孔 ...")
    positioned = []
    for i, h in enumerate(hole_groups):
        hole_no = i + 1
        flip_side = hole_no in flip_indices
        invert_direction = hole_no in invert_direction_indices
        print(
            f"  [{hole_no}/{len(hole_groups)}] instance={h.get('instance_id', '-')} "
            f"@ ({h['center'][0]:.1f},{h['center'][1]:.1f})...",
            end="",
        )
        mount_info = h.get("mount", (None, h["center"], (0, 0, 1)))
        hole_anchor = h.get("placement_center", h["center"])
        if placement_mode == "original":
            trsf = position_clip_by_original_bolt(
                clip_shape, clip_json, hole_anchor, h["axis"], mount_info[1], mount_info[2],
                contact_face_indices=clip_contact_faces,
                pillar_faces=all_faces,
                preferred_mount_face_idx=mount_info[0],
                ref_hole_axis=ref_hole_axis,
                ref_mount_normal=ref_mount_normal,
                invert_direction=invert_direction,
            )
        else:
            trsf = position_clip(
                clip_shape, clip_json, mount_info[1], h["axis"], mount_info[2],
                flip_side=False, invert_direction=invert_direction,
            )
        if trsf is None:
            print(" ⚠ 跳过")
            continue
        pos = BRepBuilderAPI_Transform(clip_shape, trsf, True).Shape()
        if flip_side:
            mp = mount_info[1]
            ha = np.array(h["axis"], dtype=float)
            ha /= np.linalg.norm(ha) + 1e-8
            flip_trsf = gp_Trsf()
            flip_trsf.SetRotation(
                gp_Ax1(gp_Pnt(mp[0], mp[1], mp[2]), gp_Dir(ha[0], ha[1], ha[2])),
                math.pi,
            )
            pos = BRepBuilderAPI_Transform(pos, flip_trsf, True).Shape()
        positioned.append(pos)
        suffix = ""
        if flip_side:
            suffix += " (绕轴翻转180°)"
        if invert_direction:
            suffix += " (安装方向反向)"
        print(" ✓" + suffix)

    print(f"\n[7] 导出 STEP → {output} ...")
    _export_positioned_clips(step_shape, positioned, output, step_path)
    return len(positioned)


def _orthogonal_unit(v):
    """返回一个与 v 垂直的单位向量."""
    v = np.array(v, dtype=float)
    v /= np.linalg.norm(v) + 1e-8
    ref = np.array([1.0, 0.0, 0.0])
    if abs(np.dot(v, ref)) > 0.9:
        ref = np.array([0.0, 1.0, 0.0])
    out = ref - np.dot(ref, v) * v
    out /= np.linalg.norm(out) + 1e-8
    return out


def _frame_from_primary_secondary(primary, secondary):
    """由主方向和辅助方向构造右手正交基."""
    e1 = np.array(primary, dtype=float)
    e1 /= np.linalg.norm(e1) + 1e-8
    e2 = np.array(secondary, dtype=float)
    e2 = e2 - np.dot(e2, e1) * e1
    if np.linalg.norm(e2) < 1e-6:
        e2 = _orthogonal_unit(e1)
    else:
        e2 /= np.linalg.norm(e2) + 1e-8
    e3 = np.cross(e1, e2)
    e3 /= np.linalg.norm(e3) + 1e-8
    return np.column_stack([e1, e2, e3])


def _rotation_about_axis(axis, angle):
    """Rodrigues: 绕 axis 旋转 angle 弧度."""
    a = np.array(axis, dtype=float)
    a /= np.linalg.norm(a) + 1e-8
    x, y, z = a
    c, s = math.cos(angle), math.sin(angle)
    C = 1.0 - c
    return np.array([
        [c + x*x*C, x*y*C - z*s, x*z*C + y*s],
        [y*x*C + z*s, c + y*y*C, y*z*C - x*s],
        [z*x*C - y*s, z*y*C + x*s, c + z*z*C],
    ])


def _signed_angle_in_plane(v_from, v_to, plane_normal):
    """计算 v_from 到 v_to 绕 plane_normal 的有符号角."""
    n = np.array(plane_normal, dtype=float)
    n /= np.linalg.norm(n) + 1e-8
    a = np.array(v_from, dtype=float)
    b = np.array(v_to, dtype=float)
    a = a - np.dot(a, n) * n
    b = b - np.dot(b, n) * n
    if np.linalg.norm(a) < 1e-6 or np.linalg.norm(b) < 1e-6:
        return 0.0
    a /= np.linalg.norm(a) + 1e-8
    b /= np.linalg.norm(b) + 1e-8
    return math.atan2(np.dot(n, np.cross(a, b)), np.dot(a, b))


def load_clip_definition(clip_dir, clip_name):
    """加载卡扣 JSON + STEP. 返回 (shape, faces, json, step_path)"""
    jp = clip_dir / f"{clip_name}.json"
    if not jp.exists():
        # 找匹配的 .json
        for f in os.listdir(str(clip_dir)):
            if f.endswith('.json') and clip_name in f:
                jp = clip_dir / f; break
    if not jp.exists(): return None, None, None, None
    with open(jp, encoding='utf-8') as f: cj = json.load(f)
    # 找 STEP
    step_path = None
    stem = jp.stem
    for ext in ('.step', '.stp', '.STEP', '.STP'):
        p = clip_dir / f"{stem}{ext}"
        if p.exists(): step_path = p; break
    if step_path is None: return None, None, None, None
    shape, faces = load_step(str(step_path))
    cj = remap_clip_bolt_cyl_json(cj, faces, stem)
    return shape, faces, cj, str(step_path)


def _json_bolt_cyl_indices(clip_json):
    return [
        int(face["index"]) - 1
        for face in clip_json.get("faces", [])
        if str(face.get("type", "")).upper() == "BOLT_CYL" and "index" in face
    ]


def _valid_bolt_cyl_indices(faces, indices):
    valid = []
    for idx in indices:
        if 0 <= idx < len(faces):
            c, a, r = cyl_info(faces[idx])
            if c is not None and a is not None and r and r > 1.0:
                valid.append(idx)
    return valid


def _scan_bolt_cyl_indices(faces):
    """Fallback for sparse clip JSON whose face order does not match OCC."""
    candidates = []
    for idx, face in enumerate(faces):
        c, a, r = cyl_info(face)
        if c is not None and a is not None and r and r > 1.0:
            candidates.append((idx, float(r)))
    if not candidates:
        return []

    by_radius = {}
    for idx, radius in candidates:
        key = round(radius, 2)
        by_radius.setdefault(key, []).append(idx)
    _, indices = max(by_radius.items(), key=lambda item: (len(item[1]), item[0]))
    return indices


def _clip_depth_from_geom_json(clip_dir, stem, bolt_indices, clip_radius=None):
    """Read BOLT_CYL height/depth from a clip .geom.json file."""
    geom_path = Path(clip_dir) / f"{stem}.geom.json"
    if not geom_path.exists():
        return 0.0
    try:
        with open(geom_path, encoding="utf-8") as gf:
            geom = json.load(gf)
    except Exception:
        return 0.0

    faces = geom.get("faces", [])
    index_set = {int(idx) for idx in bolt_indices if idx is not None}
    depths = []
    for row in faces:
        face_idx = row.get("face_idx")
        if face_idx is None or int(face_idx) not in index_set:
            continue
        depth = float(row.get("depth", 0.0) or 0.0)
        if depth > 0:
            depths.append(depth)
    if depths:
        return float(sum(depths) / len(depths))

    if clip_radius is None:
        return 0.0
    radius_matches = []
    for row in faces:
        depth = float(row.get("depth", 0.0) or 0.0)
        radius = float(row.get("radius", 0.0) or 0.0)
        if depth > 0 and radius > 0:
            radius_matches.append((abs(radius - float(clip_radius)), depth))
    if not radius_matches:
        return 0.0
    radius_matches.sort(key=lambda item: item[0])
    return float(radius_matches[0][1])


def remap_clip_bolt_cyl_json(clip_json, faces, clip_name=""):
    """Return a clip JSON copy whose BOLT_CYL entries point to OCC face indices."""
    mapped = dict(clip_json)
    mapped["faces"] = [dict(face) for face in clip_json.get("faces", [])]

    json_indices = _json_bolt_cyl_indices(mapped)
    if _valid_bolt_cyl_indices(faces, json_indices):
        return mapped

    fallback_indices = _scan_bolt_cyl_indices(faces)
    if not fallback_indices:
        return mapped

    other_faces = [
        face for face in mapped["faces"]
        if str(face.get("type", "")).upper() != "BOLT_CYL"
    ]
    bolt_faces = [
        {
            "index": idx + 1,
            "label": 1,
            "type": "BOLT_CYL",
            "mapped_from": "occ_geometry",
        }
        for idx in fallback_indices
    ]
    mapped["faces"] = bolt_faces + other_faces
    if clip_name:
        face_labels = ", ".join(f"Face{idx + 1}" for idx in fallback_indices[:8])
        suffix = "..." if len(fallback_indices) > 8 else ""
        print(f"    ↪ {clip_name}: BOLT_CYL 映射到 OCC {face_labels}{suffix}")
    return mapped


def collect_clip_defs(clip_dir, *, prefer_json=True):
    """扫描卡扣库, 提取每个卡扣 BOLT_CYL 半径/厚度 (优先 JSON+geom)."""
    clip_dir = Path(clip_dir)
    if prefer_json:
        try:
            from clip_json_recommend import collect_clip_specs_from_json

            specs = collect_clip_specs_from_json(clip_dir)
            clip_defs = []
            for spec in specs:
                label_path = clip_dir / f"{spec.name}.json"
                if not label_path.exists():
                    continue
                with open(label_path, encoding="utf-8") as jf:
                    cj = json.load(jf)
                step_path = None
                for ext in (".step", ".stp", ".STEP", ".STP"):
                    p = clip_dir / f"{spec.name}{ext}"
                    if p.exists():
                        step_path = str(p)
                        break
                if step_path is None:
                    continue
                clip_defs.append((spec.name, spec.radius_mm, spec.depth_mm, step_path, cj))
            if clip_defs:
                return clip_defs
        except ImportError:
            pass
    return _collect_clip_defs_from_step(clip_dir)


def _collect_clip_defs_from_step(clip_dir):
    """从 STEP 提取 BOLT_CYL 半径（无 geom JSON 时 fallback）。"""
    clip_dir = Path(clip_dir)
    clip_defs = []
    for f in sorted(os.listdir(str(clip_dir))):
        if not f.endswith(".json"):
            continue
        if f.endswith(".geom.json") or f.endswith("_geom.json"):
            continue
        with open(clip_dir / f, encoding="utf-8") as jf:
            cj = json.load(jf)
        step_path = None
        stem = Path(f).stem
        for ext in (".step", ".stp", ".STEP", ".STP"):
            p = clip_dir / f"{stem}{ext}"
            if p.exists():
                step_path = p
                break
        if step_path is None:
            continue
        shape_tmp, faces_tmp = load_step(str(step_path))
        if shape_tmp is None:
            continue
        cj = remap_clip_bolt_cyl_json(cj, faces_tmp, stem)
        bi = _json_bolt_cyl_indices(cj)
        clip_r = None
        for idx in bi:
            if 0 <= idx < len(faces_tmp):
                _, _, cr = cyl_info(faces_tmp[idx])
                if cr and cr > 1.0:
                    clip_r = cr
                    break
        if clip_r is None:
            print(f"    ⚠ {stem}: 无 BOLT_CYL")
            continue
        clip_depth = _clip_depth_from_geom_json(clip_dir, stem, bi, clip_radius=clip_r)
        clip_defs.append((stem, clip_r, clip_depth, str(step_path), cj))
    return clip_defs


def rank_clip_defs_for_hole(clip_defs_raw, hole_radius_mm, hole_depth_mm=0.0):
    """按直径差 1.5~3.0mm + 厚度差排序卡扣。

    直径差 = 圆孔直径 - 卡扣 BOLT_CYL 圆柱直径，要求孔大、卡扣圆柱小。
    """
    DIAMETER_GAP_MIN_MM = 1.5
    DIAMETER_GAP_MAX_MM = 3.0
    DIAMETER_GAP_IDEAL_MM = (DIAMETER_GAP_MIN_MM + DIAMETER_GAP_MAX_MM) / 2.0

    def score_clip_match_scalar(hole_radius, clip_radius, hole_depth=0.0, clip_depth=0.0):
        diameter_gap = 2.0 * (float(hole_radius) - float(clip_radius))
        ideal_dist = abs(diameter_gap - DIAMETER_GAP_IDEAL_MM)
        invalid_penalty = 100.0 if diameter_gap <= 0 else 0.0
        tight_penalty = max(0.0, DIAMETER_GAP_MIN_MM - diameter_gap) * 20.0
        loose_penalty = max(0.0, diameter_gap - DIAMETER_GAP_MAX_MM) * 20.0
        depth_penalty = (
            abs(float(hole_depth) - float(clip_depth)) * 0.1
            if hole_depth > 0 and clip_depth > 0
            else 0.0
        )
        return invalid_penalty + tight_penalty + loose_penalty + ideal_dist + depth_penalty

    ranked = []
    for stem, clip_r, clip_depth, step_path, cj in clip_defs_raw:
        diameter_gap = 2.0 * (float(hole_radius_mm) - float(clip_r))
        diameter_valid = DIAMETER_GAP_MIN_MM <= diameter_gap <= DIAMETER_GAP_MAX_MM
        d_gap = (
            abs(float(hole_depth_mm) - float(clip_depth))
            if hole_depth_mm > 0 and clip_depth > 0
            else 0.0
        )
        score = score_clip_match_scalar(
            hole_radius_mm, clip_r, hole_depth_mm, clip_depth
        )
        ideal_dist = abs(diameter_gap - DIAMETER_GAP_IDEAL_MM)
        valid_rank = 0 if diameter_valid else 1
        depth_rank = d_gap if hole_depth_mm > 0 and clip_depth > 0 else float("inf")
        diameter_violation = (
            0.0
            if diameter_valid
            else min(
                abs(diameter_gap - DIAMETER_GAP_MIN_MM),
                abs(diameter_gap - DIAMETER_GAP_MAX_MM),
            )
        )
        ranked.append(
            (
                valid_rank,
                depth_rank,
                diameter_violation,
                d_gap,
                ideal_dist,
                score,
                stem,
                clip_r,
                diameter_gap,
                clip_depth,
                step_path,
                cj,
            )
        )
    ranked.sort(key=lambda x: (x[0], x[1], x[2], x[3], x[5]))
    return [
        (score, ideal_dist, d_gap, stem, clip_r, diameter_gap, clip_depth, step_path, cj)
        for (
            _valid_rank,
            _depth_rank,
            _diameter_violation,
            d_gap,
            ideal_dist,
            score,
            stem,
            clip_r,
            diameter_gap,
            clip_depth,
            step_path,
            cj,
        ) in ranked
    ]


def search_compatible_holes(
    faces, target_radius, tolerance=0.8, min_u_sum=math.pi, face_indices=None,
):
    """搜索可兼容同一卡扣的孔位。

    与 rule_pillar.search_same_spec_in_model 不同:
      - 不要求孔轴方向一致;
      - 允许半径在 tolerance 内变化;
      - 只保留 REVERSED/非 FORWARD 的内壁面, 避免把凸台/圆角当孔;
      - 通过空间聚类把同一孔的多个圆柱/锥面段合并。

    face_indices: 可选, 仅在这些面索引中搜索 (doghouse 组件域).
    """
    from OCC.Core.TopAbs import TopAbs_FORWARD

    if face_indices is not None:
        indices = sorted({int(i) for i in face_indices if 0 <= int(i) < len(faces)})
    else:
        indices = range(len(faces))

    candidates = []
    for i in indices:
        face = faces[i]
        if face.Orientation() == TopAbs_FORWARD:
            continue
        sf = BRepAdaptor_Surface(face, True)
        ft = sf.GetType()
        if ft not in (GeomAbs_Cylinder, GeomAbs_Cone):
            continue
        try:
            if ft == GeomAbs_Cylinder:
                radius = sf.Cylinder().Radius()
            else:
                cone = sf.Cone()
                if abs(cone.SemiAngle()) >= math.radians(5):
                    continue
                radius = cone.RefRadius()
        except Exception:
            continue
        if abs(radius - target_radius) > tolerance:
            continue
        center, axis, _ = cyl_info(face)
        if center is None or axis is None:
            continue
        u = abs(sf.LastUParameter() - sf.FirstUParameter())
        v = abs(sf.LastVParameter() - sf.FirstVParameter())
        if v < 0.3:
            continue
        candidates.append({
            'face_idx': i,
            'center': np.array(center, dtype=float),
            'axis': np.array(axis, dtype=float),
            'radius': radius,
            'u': u,
            'v': v,
        })

    clusters = []
    assigned = [False] * len(candidates)
    CLUSTER_DIST = 20.0
    for i, cand in enumerate(candidates):
        if assigned[i]:
            continue
        cluster = [cand]
        assigned[i] = True
        for j in range(i + 1, len(candidates)):
            if assigned[j]:
                continue
            if np.linalg.norm(cand['center'] - candidates[j]['center']) < CLUSTER_DIST:
                cluster.append(candidates[j])
                assigned[j] = True
        u_sum = sum(c['u'] for c in cluster)
        # 真实孔通常由两个半圆孔壁段组成, 或者是一个接近完整圆柱面。
        # 单个短弧段更容易是边角圆角; u≥π 的单段内壁仍视为有效孔。
        if len(cluster) < 2 and u_sum < min_u_sum:
            continue
        if u_sum < min_u_sum:
            continue
        best = max(cluster, key=lambda c: c['u'])
        center = np.mean([c['center'] for c in cluster], axis=0)
        axis = best['axis'] / (np.linalg.norm(best['axis']) + 1e-8)
        radius = float(best['radius'])
        clusters.append({
            'center': tuple(center.tolist()),
            'axis': tuple(axis.tolist()),
            'radius': radius,
            'face_idx': best['face_idx'],
            'face_indices': [c['face_idx'] for c in cluster],
            'u_sum': u_sum,
        })

    clusters.sort(key=lambda h: (h['center'][2], h['center'][0], h['center'][1]))
    print(f"  兼容孔搜索: 找到 {len(clusters)} 个孔位 (R={target_radius:.2f}±{tolerance:.2f})")
    for h in clusters:
        print(
            f"    R={h['radius']:.2f}mm | {len(h['face_indices'])} 面段 | "
            f"best=face[{h['face_idx']+1}] | @ "
            f"({h['center'][0]:.1f},{h['center'][1]:.1f},{h['center'][2]:.1f})"
        )
    return clusters


def _rodrigues(f, t):
    """Rodrigues 旋转: 将向量 f 旋转到 t 方向 (参考 automate 实现)."""
    f = np.array(f, dtype=float); f = f / (np.linalg.norm(f) + 1e-8)
    t = np.array(t, dtype=float); t = t / (np.linalg.norm(t) + 1e-8)
    c = np.dot(f, t)
    if c > 1.0 - 1e-8:
        return np.eye(3)
    if c < -1.0 + 1e-8:
        return _rotation_about_axis(_orthogonal_unit(f), math.pi)
    v = np.cross(f, t)
    s = np.linalg.norm(v)
    if s < 1e-8:
        return np.eye(3)
    vx = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]])
    return np.eye(3) + vx + vx @ vx * (1 - c) / (s * s)


def position_clip(clip_shape, clip_json, mount_center, ha, mount_normal, flip_side=False, invert_direction=False):
    """Rodrigues 轴对齐 + 安装面贴合.

    策略:
      1. BOLT_CYL 轴 → Rodrigues 旋转到孔轴向
      2. 找到安装面中心:
         a) 优先: CONTACT 面 (Plane/Torus/Cone)
         b) 若 CONTACT 是 Torus (可能是圆角), 搜索近平面锥面
         c) 最后 fallback BOLT_CYL 中心
      3. 平移使安装面中心 → 安装面中心
    """
    faces = []
    exp = TopExp_Explorer(clip_shape, TopAbs_FACE)
    while exp.More(): faces.append(exp.Current()); exp.Next()

    bi = [f['index']-1 for f in clip_json['faces'] if f['type'] == 'BOLT_CYL']
    ci = [f['index']-1 for f in clip_json['faces'] if f['type'] == 'CONTACT']

    # BOLT_CYL 轴
    bc, ba, br = None, None, None
    for idx in bi:
        if idx >= len(faces): continue
        c, a, r = cyl_info(faces[idx])
        if c is not None and a is not None:
            bc, ba, br = c, a, r; break
    if bc is None:
        print("    ⚠ 无 BOLT_CYL 面, 跳过"); return None

    ba_a = np.array(ba); ba_a /= np.linalg.norm(ba_a)
    bc_a = np.array(bc)

    # 安装面中心
    mount_face_center = None

    for idx in ci:
        if idx >= len(faces): continue
        c, n = plane_info(faces[idx])
        if c is not None:
            # 检查该面是否是 Torus (环面 = 圆角过渡)
            sf = BRepAdaptor_Surface(faces[idx], True)
            if sf.GetType() == GeomAbs_Torus:
                # Torus: 搜索最平坦的锥面 (半角最接近 90°) 作安装面
                best_angle = 0
                for j, f in enumerate(faces):
                    sf2 = BRepAdaptor_Surface(f, True)
                    if sf2.GetType() != GeomAbs_Cone: continue
                    cone = sf2.Cone()
                    sa = abs(cone.SemiAngle())
                    if sa < 1.35 or sa > 1.56: continue  # 77°~89°
                    n2 = np.array([cone.Axis().Direction().X(),
                                   cone.Axis().Direction().Y(),
                                   cone.Axis().Direction().Z()])
                    if abs(np.dot(n2, ba_a)) < 0.9: continue
                    if sa > best_angle:
                        best_angle = sa
                        loc = cone.Axis().Location()
                        mount_face_center = np.array([loc.X(), loc.Y(), loc.Z()])
            else:
                mount_face_center = np.array(c)
            break

    if mount_face_center is None:
        print("    ⚠ 无安装面, fallback BOLT_CYL 中心")
        mount_face_center = np.array(bc)

    # Kabsch 双向量对齐:
    # e1 = BOLT_CYL 轴, e2 = clip 最大侧平面法向 (⊥ e1)
    # f1 = -mount_normal (插入方向), f2 = f1×global_up (一致侧向)
    mn_a = np.array(mount_normal, dtype=float); mn_a /= np.linalg.norm(mn_a)
    target_z = -mn_a  # clip BOLT_CYL 轴指向
    if np.dot(ba_a, target_z) < 0: target_z = -target_z
    if invert_direction:
        target_z = -target_z

    # 找 clip 最大侧平面 (法向 ⟂ ba)
    e2 = None; best_a = 0
    for i, f in enumerate(faces):
        sf = BRepAdaptor_Surface(f, True)
        if sf.GetType() != GeomAbs_Plane: continue
        ax = sf.Plane().Axis().Direction()
        if f.Orientation() == 1: ax.Reverse()
        n = np.array([ax.X(), ax.Y(), ax.Z()])
        n /= np.linalg.norm(n)
        if abs(np.dot(n, ba_a)) > 0.3: continue
        p = GProp_GProps(); brepgprop.SurfaceProperties(f, p)
        if p.Mass() > best_a:
            best_a = p.Mass()
            # 确保 e2 有确定方向 (与 +Z 夹角 < 90°)
            e2 = n if np.dot(n, [0,0,1]) >= 0 else -n
    if e2 is None:
        e2 = np.array([0.0, 1.0, 0.0])  # fallback
        e2 -= np.dot(e2, ba_a) * ba_a
        e2 /= np.linalg.norm(e2)

    # 目标 x 轴 = global_ref 投影到 ⟂target_z 平面 (方向一致)
    # 用 (1,0,0) 作参考, 不管 target_z 方向都能得到一致 X
    global_ref = np.array([1.0, 0.0, 0.0])
    if abs(np.dot(target_z, global_ref)) > 0.9:
        global_ref = np.array([0.0, 1.0, 0.0])
    target_x = global_ref - np.dot(global_ref, target_z) * target_z
    tx_n = np.linalg.norm(target_x)
    if tx_n < 0.01: target_x = np.array([1.0,0.0,0.0]); tx_n = 1.0
    target_x /= tx_n
    if flip_side:
        target_x = -target_x
    # 使 e2→target_x 方向一致: 若 e2 旋转后与 target_x 夹 > 90°, 翻转
    target_y = np.cross(target_z, target_x)

    # 正交基: clip {e1, e2, e1×e2}, target {target_z, target_x, target_y}
    e3 = np.cross(ba_a, e2)
    e3 /= np.linalg.norm(e3)

    # Kabsch: R = sum_i f_i ⊗ e_i
    R = np.outer(target_z, ba_a) + np.outer(target_x, e2) + np.outer(target_y, e3)

    trsf = gp_Trsf()
    trsf.SetValues(R[0,0], R[0,1], R[0,2], 0,
                   R[1,0], R[1,1], R[1,2], 0,
                   R[2,0], R[2,1], R[2,2], 0)

    mount_rot = R @ mount_face_center
    mc_a = np.array(mount_center)
    offset = mc_a - mount_rot
    trsf.SetTranslationPart(gp_Vec(offset[0], offset[1], offset[2]))
    return trsf


def position_clip_by_original_bolt(
    clip_shape, clip_json, hole_center, hole_axis, mount_center, mount_normal,
    contact_face_indices=None, pillar_faces=None, debug_info=None,
    preferred_mount_face_idx=None, uvnet_prediction=None,
    ref_hole_axis=None, ref_mount_normal=None, invert_direction=False,
):
    """复制原始卡扣姿态, 同时满足 BOLT_CYL 轴线对孔轴线和 CONTACT 平面对安装面."""
    faces = []
    exp = TopExp_Explorer(clip_shape, TopAbs_FACE)
    while exp.More(): faces.append(exp.Current()); exp.Next()

    bi = [f['index']-1 for f in clip_json['faces'] if f['type'] == 'BOLT_CYL']
    if contact_face_indices:
        ci = [idx - 1 for idx in contact_face_indices]
    else:
        ci = [f['index']-1 for f in clip_json['faces'] if f['type'] == 'CONTACT']
    bc, ba = None, None
    for idx in bi:
        if idx >= len(faces): continue
        c, a, _ = cyl_info(faces[idx])
        if c is not None and a is not None:
            bc, ba = c, a
            break
    if bc is None:
        print("    ⚠ 无 BOLT_CYL 面, 跳过"); return None

    ba_a = np.array(ba, dtype=float)
    ba_a /= np.linalg.norm(ba_a) + 1e-8
    contact_centers, contact_normals, contact_weights = [], [], []
    primary_contact_idx = None
    for idx in ci:
        if idx < 0 or idx >= len(faces): continue
        anchor, normal = _planar_face_anchor(faces[idx])
        if anchor is not None and normal is not None:
            p = GProp_GProps(); brepgprop.SurfaceProperties(faces[idx], p)
            contact_centers.append(anchor)
            contact_normals.append(normal)
            contact_weights.append(max(p.Mass(), 1e-8))
            if primary_contact_idx is None:
                primary_contact_idx = idx
    if not contact_centers:
        print("    ⚠ 无 CONTACT 面, fallback BOLT_CYL 中心")
        contact_center = np.array(bc, dtype=float)
        contact_normal = ba_a
    else:
        weights = np.array(contact_weights, dtype=float)
        weights /= weights.sum()
        contact_center = sum(w * c for w, c in zip(weights, contact_centers))
        ref_n = contact_normals[0] / (np.linalg.norm(contact_normals[0]) + 1e-8)
        aligned_normals = []
        for n in contact_normals:
            n = n / (np.linalg.norm(n) + 1e-8)
            aligned_normals.append(n if np.dot(n, ref_n) >= 0 else -n)
        contact_normal = sum(w * n for w, n in zip(weights, aligned_normals))

    ha_a = np.array(hole_axis, dtype=float)
    ha_a /= np.linalg.norm(ha_a) + 1e-8
    mn_a = np.array(mount_normal, dtype=float)
    mn_a /= np.linalg.norm(mn_a) + 1e-8
    if ref_mount_normal is not None:
        ref_mn = np.array(ref_mount_normal, dtype=float)
        ref_mn /= np.linalg.norm(ref_mn) + 1e-8
        if float(np.dot(mn_a, ref_mn)) < 0:
            mn_a = -mn_a
    if invert_direction:
        mn_a = -mn_a
    contact_normal /= np.linalg.norm(contact_normal) + 1e-8

    ref_ha = None
    if ref_hole_axis is not None:
        ref_ha = np.array(ref_hole_axis, dtype=float)
        ref_ha /= np.linalg.norm(ref_ha) + 1e-8

    # 孔轴方向以种子孔为准, 避免不同孔位 OCC 轴反向导致卡扣插入方向翻转。
    if ref_ha is not None:
        target_axis = ha_a if float(np.dot(ha_a, ref_ha)) >= 0 else -ha_a
    else:
        target_axis = ha_a if np.dot(ba_a, ha_a) >= np.dot(ba_a, -ha_a) else -ha_a
    R_axis = _rodrigues(ba_a, target_axis)
    contact_after_axis = R_axis @ contact_normal
    desired_contact_normal = -mn_a
    in_plane_angle = _signed_angle_in_plane(contact_after_axis, desired_contact_normal, target_axis)
    R_spin = _rotation_about_axis(target_axis, in_plane_angle)
    R = R_spin @ R_axis

    # 若 CONTACT 仍背向安装面, 绕孔轴翻转 180°。
    contact_oriented = R @ contact_normal
    if float(np.dot(contact_oriented, desired_contact_normal)) < 0:
        R = _rotation_about_axis(target_axis, math.pi) @ R
    if ref_ha is not None and float(np.dot(R @ ba_a, ref_ha)) < 0:
        R = _rotation_about_axis(target_axis, math.pi) @ R
    trsf = gp_Trsf()
    trsf.SetValues(R[0,0], R[0,1], R[0,2], 0,
                   R[1,0], R[1,1], R[1,2], 0,
                   R[2,0], R[2,1], R[2,2], 0)

    bc_rot = R @ np.array(bc, dtype=float)
    contact_rot = R @ contact_center
    hc_a = np.array(hole_center, dtype=float)
    mc_a = np.array(mount_center, dtype=float)

    # 先让 BOLT_CYL 轴线落到孔轴上。
    bolt_delta = hc_a - bc_rot
    lateral_offset = bolt_delta - np.dot(bolt_delta, target_axis) * target_axis

    # Step 4: 参考 automate-freecad 的 anchor-based 思路:
    # 用轴对齐后的 CONTACT 中心作为 anchor, 在 pillar 上找最近的局部安装面,
    # 再沿该面法向贴合。这样每个孔可以使用自己的局部安装面。
    contact_anchor = contact_rot + lateral_offset
    mount_point_used = mc_a
    mount_normal_used = mn_a
    mount_face_used = preferred_mount_face_idx
    if pillar_faces is not None:
        try:
            from OCC.Core.BRepBuilderAPI import BRepBuilderAPI_MakeVertex
            from OCC.Core.BRepExtrema import BRepExtrema_DistShapeShape
            mount_id = None
            if uvnet_prediction is not None:
                mount_id = uvnet_prediction.class_id("MOUNT_FACE")

            if (
                preferred_mount_face_idx is not None
                and 0 <= preferred_mount_face_idx < len(pillar_faces)
            ):
                face = pillar_faces[preferred_mount_face_idx]
                sf = BRepAdaptor_Surface(face, True)
                if sf.GetType() == GeomAbs_Plane:
                    mount_pt, _ = _planar_face_anchor(face)
                    if mount_pt is not None:
                        mount_normal_used = mn_a
                        contact_world = contact_rot + lateral_offset
                        normal_gap = float(np.dot(contact_world - mount_pt, mount_normal_used))
                        offset = lateral_offset - normal_gap * mount_normal_used
                        trsf.SetTranslationPart(gp_Vec(offset[0], offset[1], offset[2]))
                        mount_face_used = preferred_mount_face_idx
                        if primary_contact_idx is not None:
                            trsf, gap_before = _refine_transform_mount_contact_gap(
                                trsf,
                                clip_shape,
                                primary_contact_idx,
                                face,
                                mount_normal_used,
                            )
                            if gap_before is not None and gap_before > 1e-4:
                                print(f" localFace{mount_face_used+1} seat={gap_before:.2f}mm", end="")
                            else:
                                print(f" localFace{mount_face_used+1}", end="")
                        else:
                            print(f" localFace{mount_face_used+1}", end="")
                        if debug_info is not None:
                            debug_info["mount_face_idx"] = mount_face_used
                            debug_info["mount_point"] = tuple(
                                project_point_to_plane(contact_world, mount_pt, mount_normal_used)
                            )
                            debug_info["mount_normal"] = mount_normal_used
                        return trsf

            best_score = 1e99
            contact_n = R @ contact_normal
            contact_n /= np.linalg.norm(contact_n) + 1e-8
            for face_idx, face in enumerate(pillar_faces):
                sf = BRepAdaptor_Surface(face, True)
                if sf.GetType() != GeomAbs_Plane:
                    continue
                info = plane_info(face)
                if info[0] is None:
                    continue
                plane_c = np.array(info[0], dtype=float)
                plane_n = np.array(info[1], dtype=float)
                plane_n /= np.linalg.norm(plane_n) + 1e-8
                # 两个接触面的法向应相对。
                if np.dot(plane_n, contact_n) > 0:
                    plane_n = -plane_n
                facing = -float(np.dot(plane_n, contact_n))
                if facing < 0.85:
                    continue
                projected = project_point_to_plane(contact_anchor, plane_c, plane_n)
                vertex = BRepBuilderAPI_MakeVertex(gp_Pnt(projected[0], projected[1], projected[2])).Shape()
                dist = BRepExtrema_DistShapeShape(vertex, face)
                dist.Perform()
                bounded_dist = dist.Value() if dist.IsDone() else 1e6
                normal_gap_candidate = abs(float(np.dot(contact_anchor - plane_c, plane_n)))
                score = bounded_dist * 100.0 + normal_gap_candidate * 0.01
                if mount_id is not None and 0 <= face_idx < len(uvnet_prediction.probs):
                    if int(uvnet_prediction.pred_ids[face_idx]) == mount_id:
                        score -= 80.0
                    score -= float(uvnet_prediction.probs[face_idx, mount_id]) * 20.0
                if preferred_mount_face_idx is not None and face_idx == preferred_mount_face_idx:
                    score -= 200.0
                if score < best_score:
                    best_score = score
                    mount_face_used = face_idx
                    mount_point_used = projected
                    mount_normal_used = plane_n
            if mount_face_used is not None:
                print(f" localFace{mount_face_used+1}", end="")
        except Exception as e:
            print(f" localMountFail:{e}", end="")

    if debug_info is not None:
        debug_info["mount_face_idx"] = mount_face_used
        debug_info["mount_point"] = mount_point_used
        debug_info["mount_normal"] = mount_normal_used

    normal_gap = np.dot((contact_rot + lateral_offset) - mount_point_used, mount_normal_used)
    offset = lateral_offset - normal_gap * mount_normal_used
    trsf.SetTranslationPart(gp_Vec(offset[0], offset[1], offset[2]))
    if (
        pillar_faces is not None
        and primary_contact_idx is not None
        and mount_face_used is not None
        and 0 <= mount_face_used < len(pillar_faces)
    ):
        trsf, gap_before = _refine_transform_mount_contact_gap(
            trsf,
            clip_shape,
            primary_contact_idx,
            pillar_faces[mount_face_used],
            mount_normal_used,
        )
        if gap_before is not None and gap_before > 1e-4:
            print(f" seat={gap_before:.2f}mm", end="")
    return trsf


def main():
    import argparse
    parser = argparse.ArgumentParser(description="种子面→找孔→推荐→装卡扣到所有同规格孔")
    parser.add_argument("--step", default=str(HERE.parent/"step/pillar.step"))
    parser.add_argument("--seed-idx", type=int, default=960)
    parser.add_argument("--output", default="pillar_assembled.step")
    parser.add_argument("--tolerance", type=float, default=0.5)
    parser.add_argument(
        "--hole-search-mode",
        choices=("compatible", "same-spec"),
        default="compatible",
        help="compatible=按半径兼容搜索孔位, 不要求轴向一致; same-spec=旧的同规格孔搜索",
    )
    parser.add_argument(
        "--compatible-tolerance",
        type=float,
        default=0.8,
        help="compatible 模式下的孔半径兼容公差(mm)",
    )
    parser.add_argument(
        "--hole-detector",
        choices=("rule", "uvnet", "hybrid"),
        default="hybrid",
        help="rule=纯规则; hybrid=规则找孔+UV-Net校验过滤(推荐); uvnet=预留",
    )
    parser.add_argument(
        "--uvnet-model",
        default=_default_uvnet_checkpoint(),
        help="UV-Net 面级分类模型路径, 供 --hole-detector uvnet/hybrid 使用",
    )
    parser.add_argument(
        "--uvnet-json",
        default="",
        help="UV-Net 几何特征 JSON 路径; 默认使用 STEP 同名 .json, 缺失则回退规则流程",
    )
    parser.add_argument(
        "--uvnet-device",
        default="cpu",
        choices=("cpu", "cuda"),
        help="UV-Net 推理设备 (有 GPU 时可用 cuda)",
    )
    parser.add_argument(
        "--no-uvnet-graph-cache",
        dest="uvnet_graph_cache",
        action="store_false",
        default=True,
        help="禁用 UV-Net 图磁盘缓存 (默认开启, 换模型时仍复用同一 STEP 的 UV 图)",
    )
    parser.add_argument(
        "--target-radius",
        type=float,
        default=None,
        help="指定目标孔半径(mm)。若种子邻域检测到多个孔组, 选择最接近该半径的孔组。",
    )
    parser.add_argument(
        "--mount-face-mode",
        choices=("seed", "auto", "hybrid"),
        default="hybrid",
        help="seed=全部使用 seed-idx; auto=每孔局部搜索; hybrid=每孔局部 MOUNT_FACE + seed 回退(推荐)",
    )
    parser.add_argument(
        "--auto-mount-threshold",
        type=float,
        default=2.0,
        help="(已弃用) 保留参数兼容旧脚本, hybrid 现改为每孔局部选安装面",
    )
    parser.add_argument(
        "--placement-mode",
        choices=("original", "contact"),
        default="original",
        help="original=复制原始卡扣并按 BOLT_CYL 对齐圆孔; contact=使用 CONTACT 面贴安装面",
    )
    parser.add_argument(
        "--clip-contact-faces",
        default="",
        help="逗号分隔的 FreeCAD 1-based 卡扣 CONTACT 面号, 例如: 85,86",
    )
    parser.add_argument(
        "--clip-name",
        default="",
        help="强制使用指定卡扣(卡扣库 JSON 文件名不含扩展名), 例如: IX-05402112",
    )
    parser.add_argument(
        "--flip-indices",
        default="",
        help="逗号分隔的 1-based 孔位编号, 用于将指定卡扣绕孔轴翻转 180°, 例如: 3,5",
    )
    parser.add_argument(
        "--invert-direction-indices",
        default="",
        help="逗号分隔的 1-based 孔位编号, 用于将指定卡扣沿安装面法向反向安装, 例如: 3,4",
    )
    parser.add_argument(
        "--uvnet-min-hole-mean-prob",
        type=float,
        default=0.45,
        help="hybrid 模式下保留孔组所需的最小 HOLE_WALL 平均概率",
    )
    parser.add_argument(
        "--uvnet-min-hole-max-prob",
        type=float,
        default=0.55,
        help="hybrid 模式下保留孔组所需的最小 HOLE_WALL 最大概率",
    )
    parser.add_argument(
        "--uvnet-min-mount-prob",
        type=float,
        default=0.35,
        help="hybrid 模式下优先选择 MOUNT_FACE 预测概率不低于该值的安装面",
    )
    parser.add_argument(
        "--uvnet-require-mount-face",
        action="store_true",
        default=True,
        help="hybrid 模式下要求每个孔能找到 UV-Net MOUNT_FACE 安装面, 否则丢弃该孔",
    )
    parser.add_argument(
        "--no-uvnet-require-mount-face",
        dest="uvnet_require_mount_face",
        action="store_false",
        help="关闭 hybrid 模式下对 MOUNT_FACE 安装面的硬性要求",
    )
    parser.add_argument(
        "--allow-oblique-holes",
        action="store_true",
        help="种子邻域额外识别孔轴与安装面法向成角的斜圆孔（装配仍沿安装面法向）",
    )
    parser.add_argument(
        "--oblique-min-u-sum",
        type=float,
        default=math.pi / 2,
        help="斜圆孔最小 ∑u_range (rad, 默认 π/2)",
    )
    parser.add_argument(
        "--oblique-min-v-depth",
        type=float,
        default=3.0,
        help="斜圆孔最小轴向深度 (mm)",
    )
    parser.add_argument(
        "--oblique-mount-adj-hops",
        type=int,
        default=2,
        help="斜圆孔孔壁到种子安装面的最大 BFS 跳数",
    )
    parser.add_argument(
        "--allow-slot-holes",
        action="store_true",
        help="种子邻域额外识别 BSpline/other 开放槽孔 (rule_slot.py)",
    )
    parser.add_argument(
        "--slot-mount-adj-hops",
        type=int,
        default=2,
        help="槽孔壁到种子安装面的最大 BFS 跳数",
    )
    parser.add_argument(
        "--mount-post-mode",
        choices=("off", "doghouse"),
        default="doghouse",
        help="Doghouse 定域: doghouse=几何签名快速识别(默认); off=全模型搜索",
    )
    parser.add_argument(
        "--auto-doghouse",
        action="store_true",
        help="从 doghouse prediction/annotation 自动分析安装面/安装孔并装配卡扣, 不需要 seed-idx",
    )
    parser.add_argument(
        "--prediction-json",
        default="",
        help="doghouse prediction/annotation JSON; 默认自动查找 STEP 同目录的预测/标注文件",
    )
    parser.add_argument(
        "--assembly-features-json",
        default="",
        help="已提取的 doghouse assembly features JSON; 指定后跳过特征提取",
    )
    parser.add_argument(
        "--assembly-features-output",
        default="",
        help="自动 doghouse 模式下保存提取到的安装面/安装孔 JSON",
    )
    parser.add_argument(
        "--recommend-output",
        default="",
        help="写出推荐 JSON，供 FreeCAD 插件/UI 使用",
    )
    parser.add_argument(
        "--placement-output",
        default="",
        help="写出选定卡扣位姿 JSON，供 FreeCAD 插件在当前文档预览",
    )
    parser.add_argument(
        "--no-assemble",
        action="store_true",
        help="只分析/推荐，不生成最终装配 STEP",
    )
    parser.add_argument(
        "--no-step-output",
        action="store_true",
        help="计算位姿但不写最终装配 STEP",
    )
    parser.add_argument(
        "--all-doghouses-same-clip",
        action="store_true",
        help="自动 doghouse 模式下所有 doghouse 使用同一种推荐卡扣",
    )
    parser.add_argument(
        "--doghouse-infer-output-dir",
        default="",
        help="无 prediction/annotation 时自动推理 doghouse 的中间输出目录",
    )
    parser.add_argument(
        "--infer-cpu",
        action="store_true",
        default=True,
        help="doghouse 自动推理使用 CPU",
    )
    parser.add_argument(
        "--auto-use-vf2",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="自动 doghouse 模式下使用 VF2/拓扑优先识别安装孔 (默认开启)",
    )
    args = parser.parse_args()
    flip_indices = {
        int(s.strip()) for s in args.flip_indices.split(",")
        if s.strip()
    }
    invert_direction_indices = {
        int(s.strip()) for s in args.invert_direction_indices.split(",")
        if s.strip()
    }
    clip_contact_faces = [
        int(s.strip()) for s in args.clip_contact_faces.split(",")
        if s.strip()
    ]

    CLIP_DIR = HERE.parent / "卡扣库"

    if args.auto_doghouse or args.prediction_json or args.assembly_features_json:
        print(f"{'='*60}")
        print(f"  STEP: {args.step}")
        print("  模式: 自动 doghouse 安装面/安装孔分析 + 卡扣装配")
        print(f"{'='*60}")

        print("\n[1] 加载 STEP ...")
        step_shape, all_faces = load_step(args.step)
        print(f"  {len(all_faces)} 面")

        print("\n[2] 自动分析 doghouse 安装面/安装孔 ...")
        assembly_features = _load_or_extract_assembly_features(args, all_faces)
        if assembly_features is None:
            return
        ok_count = sum(1 for inst in assembly_features.get("instances", []) if inst.get("status") == "ok")
        print(f"  doghouse 实例: {len(assembly_features.get('instances', []))}, 可装配: {ok_count}")
        hole_groups = assembly_features_to_hole_groups(assembly_features, all_faces)
        for i, h in enumerate(hole_groups):
            print(
                f"    [{i+1}] instance={h.get('instance_id', '-')} "
                f"孔壁={h.get('face_indices') or [h.get('face_idx')]} "
                f"→ 安装面 Face{h['mount_face_idx']+1} R={float(h['radius']):.2f}"
            )
        if not hole_groups:
            print("  ✗ 未生成可装配孔-安装面对")
            return

        print("\n[3] 读取卡扣库半径约束 ...")
        clip_defs_raw = collect_clip_defs(CLIP_DIR)
        if not clip_defs_raw:
            print("  ✗ 无可用卡扣定义")
            return
        clip_radii = [r for _, r, *_ in clip_defs_raw]
        print(f"  卡扣 BOLT_CYL 半径范围: {min(clip_radii):.2f} ~ {max(clip_radii):.2f}mm")

        recommendation_payload = build_clip_recommendation_payload(
            hole_groups,
            clip_defs_raw,
            tolerance=args.compatible_tolerance,
            source_step=args.step,
            all_holes_same_clip=args.all_doghouses_same_clip,
        )
        if args.recommend_output:
            _write_json(args.recommend_output, recommendation_payload)

        if args.placement_output:
            placement_payload = build_clip_placement_payload(
                all_faces=all_faces,
                hole_groups=hole_groups,
                clip_defs_raw=clip_defs_raw,
                clip_name=args.clip_name or recommendation_payload.get("selected_clip") or "",
                clip_contact_faces=clip_contact_faces,
                placement_mode=args.placement_mode,
                flip_indices=flip_indices,
                invert_direction_indices=invert_direction_indices,
                tolerance=args.compatible_tolerance,
                all_holes_same_clip=args.all_doghouses_same_clip,
            )
            _write_json(args.placement_output, placement_payload)

        if args.no_assemble or args.no_step_output:
            print("  跳过最终 STEP 装配输出")
            return

        assemble_hole_groups_to_step(
            step_shape=step_shape,
            all_faces=all_faces,
            step_path=args.step,
            hole_groups=hole_groups,
            clip_defs_raw=clip_defs_raw,
            clip_dir=CLIP_DIR,
            output=args.output,
            clip_name=args.clip_name,
            clip_contact_faces=clip_contact_faces,
            placement_mode=args.placement_mode,
            flip_indices=flip_indices,
            invert_direction_indices=invert_direction_indices,
            tolerance=args.compatible_tolerance,
        )
        return

    print(f"{'='*60}")
    print(f"  STEP: {args.step}")
    print(f"  种子面: Face{args.seed_idx+1}")
    print(f"{'='*60}")

    # 1. 读 STEP（TopologyExplorer 面序，与 rule_pillar / JSON face_idx 一致）
    print("\n[1] 加载 STEP ...")
    step_shape, all_faces, mount_idx, seed_hole_groups, _, mount_normal, face_adj = process_step(
        args.step,
        mount_idx=args.seed_idx,
        allow_oblique_holes=args.allow_oblique_holes,
        oblique_min_u_sum=args.oblique_min_u_sum,
        oblique_min_v_depth=args.oblique_min_v_depth,
        oblique_mount_adj_hops=args.oblique_mount_adj_hops,
        allow_slot_holes=args.allow_slot_holes,
        slot_mount_adj_hops=args.slot_mount_adj_hops,
    )
    print(f"  {len(all_faces)} 面")

    if not seed_hole_groups:
        print(
            f"\n  ✗ 种子面 Face{args.seed_idx + 1} 邻域未识别到孔，无法继续装配（已中止）"
        )
        print(
            "    说明：默认仅识别规则圆柱孔；"
            "可加 --allow-oblique-holes 尝试斜圆孔；"
            "可加 --allow-slot-holes 尝试 BSpline/open slot 槽孔。"
        )
        return

    # 2. 检测孔 / UV-Net
    print("\n[2] 检测孔 ...")
    uvnet_prediction = None
    if args.hole_detector != "rule":
        uvnet_json = Path(args.uvnet_json) if args.uvnet_json else None
        try:
            from uvnet_infer import predict_step_faces

            print(f"  UV-Net 推理: {args.uvnet_model}")
            uvnet_prediction = predict_step_faces(
                args.step,
                uvnet_json,
                checkpoint=args.uvnet_model,
                device_name=args.uvnet_device,
                use_graph_cache=args.uvnet_graph_cache,
                verbose=True,
            )
            pred_counts = {
                name: int(np.sum(uvnet_prediction.pred_ids == idx))
                for idx, name in enumerate(uvnet_prediction.label_names)
            }
            print(f"  UV-Net 预测统计: {pred_counts}")
            if len(uvnet_prediction.pred_ids) != len(all_faces):
                print(
                    f"  ⚠ UV-Net 面数 {len(uvnet_prediction.pred_ids)} "
                    f"与规则面数 {len(all_faces)} 不一致，"
                    "安装面/孔语义分数可能不可用"
                )
        except Exception as exc:
            print(f"  ⚠ UV-Net 推理失败, 回退规则流程: {exc}")

    seed_near_faces = _seed_near_face_indices(seed_hole_groups)
    print(f"  种子邻域检测到 {len(seed_hole_groups)} 个孔组 ({len(seed_near_faces)} 个孔壁面)")

    seed_c, seed_n = plane_info(all_faces[args.seed_idx])
    preferred_mount_normal = mount_normal
    seed_mount = None
    if seed_c is None or seed_n is None:
        print(f"  ⚠ seed face[{args.seed_idx}] 无法作为安装面法向参考, 使用 process_step 法向")
    else:
        seed_mount = (args.seed_idx, np.array(seed_c, dtype=float), np.array(seed_n, dtype=float))
        preferred_mount_normal = seed_mount[2]

    seed_hole_groups = enrich_hole_groups_with_mount(
        seed_hole_groups,
        all_faces,
        face_adj,
        seed_mount,
        seed_near_faces,
        preferred_mount_normal,
        uvnet_prediction,
        args.mount_face_mode,
        args.uvnet_min_mount_prob,
        args.seed_idx,
    )
    hole_groups = seed_hole_groups

    print("\n[2.5] 读取卡扣库半径约束 ...")
    clip_defs_raw = collect_clip_defs(CLIP_DIR)
    if not clip_defs_raw:
        print("  ✗ 无可用卡扣定义"); return
    clip_radii = [r for _, r, *_ in clip_defs_raw]
    print(f"  卡扣 BOLT_CYL 半径范围: {min(clip_radii):.2f} ~ {max(clip_radii):.2f}mm")

    # 同规格搜索
    if not hole_groups: return
    if args.target_radius is not None:
        hole_groups.sort(key=lambda g: abs(g['radius'] - args.target_radius))
        print(
            f"  指定目标半径 R={args.target_radius:.2f}mm, "
            f"选择孔组 R={hole_groups[0]['radius']:.2f}mm"
        )
    else:
        def _clip_radius_score(g):
            return min(abs(g['radius'] - cr) for cr in clip_radii)
        hole_groups.sort(key=lambda g: (_clip_radius_score(g), -g.get('u_sum', 0.0)))
        print(
            "  自动选择与卡扣库半径最匹配的孔组: "
            + ", ".join(
                f"R={g['radius']:.2f}(diff={_clip_radius_score(g):.2f})"
                for g in hole_groups
            )
        )
    target_r = float(args.target_radius) if args.target_radius is not None else hole_groups[0]['radius']
    part_json_path = None
    try:
        from step_geom_extract import resolve_uvnet_json_path

        part_json_path = resolve_uvnet_json_path(
            args.step, args.uvnet_json or None
        )
    except Exception:
        part_json_path = (
            Path(args.uvnet_json) if args.uvnet_json else Path(args.step).with_suffix(".json")
        )
    hole_depth_mm = 0.0
    hole_match_r = target_r
    if part_json_path is not None and part_json_path.exists():
        try:
            from clip_json_recommend import hole_specs_from_part_json

            json_specs = hole_specs_from_part_json(
                part_json_path, seed_idx=args.seed_idx
            )
            if json_specs:
                json_hole = min(
                    json_specs, key=lambda hs: abs(hs.radius_mm - target_r)
                )
                if abs(json_hole.radius_mm - target_r) < 0.51:
                    hole_match_r = float(json_hole.radius_mm)
                    hole_depth_mm = float(json_hole.depth_mm)
        except Exception:
            pass
        if hole_depth_mm > 0:
            print(f"  孔壁厚度(JSON): {hole_depth_mm:.2f}mm")
    seed_template = _seed_hole_mount_template(
        seed_hole_groups, seed_near_faces, seed_mount, target_radius=target_r,
    )
    if seed_template:
        print(
            f"  种子孔-安装面模板: R={seed_template.get('radius', '?')}mm, "
            f"安装面 Face{seed_template.get('mount_face_idx', args.seed_idx)+1}"
        )

    post_scope = None
    if args.mount_post_mode == "doghouse":
        from rule_mount_post import plan_mount_post_assembly

        print("\n[2.55] Doghouse 组件定域 ...")
        mp = plan_mount_post_assembly(
            args.step,
            args.seed_idx,
            target_radius=target_r,
            seed_hole_groups=seed_hole_groups,
            tolerance=args.tolerance,
            mode="doghouse",
            all_faces=all_faces,
            adj=face_adj,
            shape=step_shape,
        )
        anchor = mp.get("anchor_post")
        if anchor:
            post_scope = set(mp.get("scope_union") or [])
            print(
                f"  模板={anchor['template']} | 安装面 Face{anchor['mount_face_idx']+1} | "
                f"实例={len(mp.get('post_instances') or [])} | 组件域={len(post_scope)} 面"
            )
        else:
            print("  ⚠ 未匹配 doghouse 组件, 回退全模型孔搜索")

    if args.hole_search_mode == "compatible":
        label = "组件内" if post_scope else "全模型"
        print(f"\n[2.6] {label}搜索同规格孔 R={target_r:.2f}mm ...")
        if post_scope:
            from rule_mount_post import search_compatible_holes_in_scope

            hole_groups = search_compatible_holes_in_scope(
                all_faces,
                post_scope,
                target_r,
                tolerance=args.compatible_tolerance,
            )
        else:
            hole_groups = search_compatible_holes(
                all_faces, target_r, tolerance=args.compatible_tolerance,
            )
        if not hole_groups and uvnet_prediction is not None:
            from hole_detect_uvnet import detect_hole_groups_from_uvnet

            print("  规则兼容孔搜索为空, 尝试 UV-Net 全模型定孔 ...")
            hole_groups = detect_hole_groups_from_uvnet(
                all_faces,
                face_adj,
                uvnet_prediction,
                mount_normal=preferred_mount_normal,
                local_only=False,
                target_radius=target_r,
                radius_tolerance=args.compatible_tolerance,
            )
        if not hole_groups:
            print("  ✗ 未找到兼容孔"); return
        spec_tol = min(args.tolerance, 0.25) if args.target_radius is not None else min(
            args.compatible_tolerance, max(args.tolerance, 0.5)
        )
        before = len(hole_groups)
        hole_groups = [
            h for h in hole_groups if abs(float(h["radius"]) - target_r) <= spec_tol
        ]
        print(
            f"  同规格半径筛选 ±{spec_tol:.2f}mm: {before} → {len(hole_groups)} 个孔"
        )
        if post_scope:
            before_merge = len(hole_groups)
            hole_groups = merge_seed_holes_into_groups(
                seed_hole_groups, hole_groups, target_r, spec_tol,
            )
            if len(hole_groups) > before_merge:
                print(f"  补回种子邻域孔: {before_merge} → {len(hole_groups)} 个孔")
        if not hole_groups:
            print("  ✗ 同规格半径筛选后无可用孔"); return
    else:
        if post_scope:
            from rule_mount_post import search_same_spec_holes_in_scope

            print(f"\n[2.6] 组件内搜索同规格孔 R={target_r:.2f}mm ...")
            same_groups = search_same_spec_holes_in_scope(
                step_shape,
                all_faces,
                post_scope,
                target_r,
                tolerance=args.tolerance,
                mount_normal=mount_normal,
            )
        else:
            same_groups = search_same_spec_in_model(
                step_shape, target_r,
                tolerance=args.tolerance, mount_normal=mount_normal,
            )
        if not same_groups:
            print("  ✗ 未找到同规格孔"); return
        # 转换为统一格式: 每个 hole 一组
        hole_groups_flat = []
        for g in same_groups:
            best_fi = g.get('best_idx', g['face_indices'][0])
            c, a, _ = cyl_info(all_faces[best_fi])
            if c:
                hole_groups_flat.append({'center': c, 'axis': a, 'radius': g['radius'],
                                          'face_idx': best_fi})
        hole_groups = hole_groups_flat
    group_label = "兼容孔" if args.hole_search_mode == "compatible" else "同规格孔"
    print(f"  半径 R={target_r:.1f}mm, 规则找到 {len(hole_groups)} 个{group_label}:")
    for i, h in enumerate(hole_groups):
        face_ids = h.get("face_indices") or [h.get("face_idx")]
        near_seed = any(int(fi) in seed_near_faces for fi in face_ids if fi is not None)
        tag = " [种子邻域]" if near_seed else ""
        print(f"    [{i+1}] @ ({h['center'][0]:.1f},{h['center'][1]:.1f},{h['center'][2]:.1f}){tag}")

    print("\n[2.7] 为每个孔绑定局部安装面 (孔-安装面对) ...")
    hole_groups = enrich_hole_groups_with_mount(
        hole_groups,
        all_faces,
        face_adj,
        seed_mount,
        seed_near_faces,
        preferred_mount_normal,
        uvnet_prediction,
        args.mount_face_mode,
        args.uvnet_min_mount_prob,
        args.seed_idx,
    )
    for i, h in enumerate(hole_groups):
        if h.get("mount_valid"):
            print(
                f"    [{i+1}] 孔壁={h.get('face_indices') or [h.get('face_idx')]} "
                f"→ 安装面 Face{h['mount_face_idx']+1} ({h.get('mount_source')})"
            )
        else:
            print(f"    [{i+1}] ⚠ 未找到可用安装面")

    seed_mount_normal = (
        seed_template["mount_normal"] if seed_template else preferred_mount_normal
    )
    if uvnet_prediction is not None and args.hole_detector == "hybrid":
        try:
            from uvnet_infer import (
                filter_hole_mount_pairs_by_uvnet,
                summarize_hole_mount_pairs,
            )

            print("  UV-Net 孔-安装面对校验/过滤:")
            for summary in summarize_hole_mount_pairs(hole_groups, uvnet_prediction):
                print(
                    f"    [{summary['group']}] 孔={summary['faces']} "
                    f"HOLE_WALL={summary['hole_wall_count']}/{len(summary['faces'])} "
                    f"mean_p={summary['mean_hole_prob']:.3f} | "
                    f"安装面 Face{summary['mount_face_idx']+1 if summary['mount_face_idx'] is not None else '-'} "
                    f"MOUNT_FACE={'Y' if summary['mount_pred'] else 'N'} "
                    f"p={summary['mount_prob']:.3f} ({summary.get('mount_source', '')})"
                )
            kept, rejected = filter_hole_mount_pairs_by_uvnet(
                hole_groups,
                uvnet_prediction,
                min_mean_prob=args.uvnet_min_hole_mean_prob,
                min_max_prob=args.uvnet_min_hole_max_prob,
                min_mount_prob=args.uvnet_min_mount_prob,
                require_mount_pair=args.uvnet_require_mount_face,
                seed_mount_normal=seed_mount_normal,
                trusted_mount_face_indices={args.seed_idx},
            )
            for group in rejected:
                hole_summary = group.get("uvnet_hole_summary") or group.get("uvnet_summary") or {}
                mount_summary = group.get("uvnet_mount_summary") or {}
                reason = group.get("reject_reason") or {}
                print(
                    f"    ✗ 丢弃孔-安装面对 @ ({group['center'][0]:.1f},{group['center'][1]:.1f}): "
                    f"HOLE_WALL={hole_summary.get('hole_wall_count', '?')}/"
                    f"{len(hole_summary.get('faces', group.get('face_indices') or []))} "
                    f"mount=Face{(mount_summary.get('mount_face_idx') or -1)+1} "
                    f"reason={reason}"
                )
            hole_groups = kept
            print(f"  UV-Net 孔-安装面对过滤后保留 {len(hole_groups)} 组")
            if not hole_groups:
                print("  ✗ UV-Net 过滤后无可用孔-安装面对"); return
        except Exception as exc:
            print(f"  ⚠ UV-Net 孔-安装面对校验失败: {exc}")
    elif uvnet_prediction is not None:
        try:
            from uvnet_infer import summarize_hole_mount_pairs

            print("  UV-Net 孔-安装面对校验:")
            for summary in summarize_hole_mount_pairs(hole_groups, uvnet_prediction):
                print(
                    f"    [{summary['group']}] 孔={summary['faces']} "
                    f"HOLE_WALL={summary['hole_wall_count']}/{len(summary['faces'])} | "
                    f"安装面 Face{summary['mount_face_idx']+1 if summary['mount_face_idx'] is not None else '-'}"
                )
        except Exception as exc:
            print(f"  ⚠ UV-Net 孔-安装面对校验失败: {exc}")
    elif args.uvnet_require_mount_face:
        before = len(hole_groups)
        hole_groups = [h for h in hole_groups if h.get("mount_valid")]
        print(f"  规则模式安装面筛选: {before} → {len(hole_groups)} 组")
        if not hole_groups:
            print("  ✗ 无完整孔-安装面对"); return

    # 3. 确认孔-安装面对
    print("\n[3] 孔-安装面对确认 ...")
    for i, h in enumerate(hole_groups):
        if not h.get("mount_valid") or not h.get("mount"):
            print(f"  [{i+1}] ⚠ 缺少安装面, fallback 孔中心")
            h["placement_center"] = h["center"]
            continue
        m = h["mount"]
        mount_point = m[1]
        if "placement_center" not in h:
            h["placement_center"] = tuple(
                np.array(
                    opening_center_on_mount(h, mount_point, m[2]),
                    dtype=float,
                ).tolist()
            )
        pc = h["placement_center"]
        print(
            f"  [{i+1}] 孔 @ ({h['center'][0]:.1f},{h['center'][1]:.1f}) "
            f"→ Face{h['mount_face_idx']+1} ({h.get('mount_source')}), "
            f"孔口=({pc[0]:.1f},{pc[1]:.1f},{pc[2]:.1f})"
        )

    # 4. 从卡扣库推荐 — 直径差 1.5–3.0 mm + 厚度匹配
    print("\n[4] 从卡扣库匹配卡扣 ...")
    ranked = rank_clip_defs_for_hole(clip_defs_raw, hole_match_r, hole_depth_mm)
    for score, _, d_gap, stem, clip_r, diameter_gap, clip_depth, _, _ in ranked:
        depth_part = f"厚度={clip_depth:.2f}mm " if clip_depth > 0 else ""
        hole_depth_str = f"孔厚={hole_depth_mm:.2f} " if hole_depth_mm > 0 else ""
        print(
            f"    {stem:20s} R={clip_r:.2f}mm {depth_part}"
            f"(孔R={hole_match_r:.1f} {hole_depth_str}"
            f"直径差={diameter_gap:.2f}, 厚度差={d_gap:.2f}, score={score:.2f})"
        )

    if not ranked:
        print("  ✗ 无匹配卡扣")
        return
    _, _, _, best_name, clip_r, diameter_gap, _, clip_step, clip_json = ranked[0]
    if args.clip_name:
        forced = args.clip_name.strip()
        picked = next((r for r in ranked if r[3] == forced), None)
        if picked is None:
            print(f"  ✗ 未找到卡扣: {forced}")
            return
        _, _, _, best_name, clip_r, diameter_gap, _, clip_step, clip_json = picked
        print(f"\n  强制选择: {best_name} (R={clip_r:.2f}mm)")
    else:
        print(f"\n  选择: {best_name} (R={clip_r:.2f}mm, 直径差={diameter_gap:.2f}mm)")

    # 5. 加载
    print(f"\n[5] 加载卡扣文件 ...")
    clip_shape, clip_faces = load_step(clip_step)
    clip_json = remap_clip_bolt_cyl_json(clip_json, clip_faces, best_name)
    print(f"  ✓ {best_name} ({len(clip_faces)} 面)")

    # 6. 定位到每个孔
    hole_groups = align_hole_groups_to_seed_frame(hole_groups, seed_template)
    ref_hole_axis = seed_template.get("hole_axis") if seed_template else None
    ref_mount_normal = seed_template.get("mount_normal") if seed_template else None
    if ref_hole_axis is not None:
        print("  统一卡扣方向: 以种子孔轴/安装面法向为参考")
    print(f"\n[6] 装到 {len(hole_groups)} 个孔 ...")
    positioned = []
    for i, h in enumerate(hole_groups):
        hole_no = i + 1
        flip_side = hole_no in flip_indices
        invert_direction = hole_no in invert_direction_indices
        print(f"  [{hole_no}/{len(hole_groups)}] @ ({h['center'][0]:.1f},{h['center'][1]:.1f})...", end="")
        mount_info = h.get('mount', (None, h['center'], (0, 0, 1)))
        hole_anchor = h.get('placement_center', h['center'])
        if args.placement_mode == "original":
            trsf = position_clip_by_original_bolt(
                clip_shape, clip_json, hole_anchor, h['axis'], mount_info[1], mount_info[2],
                contact_face_indices=clip_contact_faces,
                pillar_faces=all_faces,
                preferred_mount_face_idx=mount_info[0],
                uvnet_prediction=uvnet_prediction,
                ref_hole_axis=ref_hole_axis,
                ref_mount_normal=ref_mount_normal,
                invert_direction=invert_direction,
            )
        else:
            trsf = position_clip(
                clip_shape, clip_json, mount_info[1], h['axis'], mount_info[2],
                flip_side=False, invert_direction=invert_direction,
            )
        if trsf is None:
            print(" ⚠ 跳过"); continue
        pos = BRepBuilderAPI_Transform(clip_shape, trsf, True).Shape()
        if flip_side:
            mp = mount_info[1]
            ha = np.array(h['axis'], dtype=float)
            ha /= np.linalg.norm(ha) + 1e-8
            flip_trsf = gp_Trsf()
            flip_trsf.SetRotation(
                gp_Ax1(gp_Pnt(mp[0], mp[1], mp[2]), gp_Dir(ha[0], ha[1], ha[2])),
                math.pi,
            )
            pos = BRepBuilderAPI_Transform(pos, flip_trsf, True).Shape()
        positioned.append(pos)
        suffix = ""
        if flip_side:
            suffix += " (绕轴翻转180°)"
        if invert_direction:
            suffix += " (安装方向反向)"
        print(" ✓" + suffix)

    # 7. 合并导出
    print(f"\n[7] 导出 STEP → {args.output} ...")
    b = BRep_Builder(); c = TopoDS_Compound(); b.MakeCompound(c)
    b.Add(c, step_shape)
    for p in positioned: b.Add(c, p)
    w = STEPControl_Writer()
    w.Transfer(c, STEPControl_AsIs)
    w.Write(args.output)
    part_name = Path(args.step).stem
    print(f"  ✓ {args.output} (1 个 {part_name} + {len(positioned)} 个卡扣)")


if __name__ == "__main__":
    main()