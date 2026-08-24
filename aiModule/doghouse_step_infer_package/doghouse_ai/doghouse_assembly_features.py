#!/usr/bin/env python3
"""Extract mount faces and mounting holes inside AI-detected doghouse instances."""

from __future__ import annotations

import argparse
import json
import math
import sys
from collections import defaultdict, deque
from pathlib import Path
from typing import Any

import numpy as np


HERE = Path(__file__).resolve().parent
TOOL_ROOT = HERE.parent
if str(TOOL_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOL_ROOT))


_FACE_GEOM_CACHE: dict[tuple[str, int], Any] = {}


def _face_cache_key(face, name: str) -> tuple[str, int]:
    return name, int(hash(face))


def _clear_face_geom_cache() -> None:
    _FACE_GEOM_CACHE.clear()


def _load_json(path: str | Path) -> dict:
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def _vec(values) -> list[float]:
    return [round(float(x), 6) for x in values]


def _surface_area(face) -> float:
    key = _face_cache_key(face, "area")
    if key in _FACE_GEOM_CACHE:
        return float(_FACE_GEOM_CACHE[key])

    from OCC.Core.BRepGProp import brepgprop
    from OCC.Core.GProp import GProp_GProps

    props = GProp_GProps()
    brepgprop.SurfaceProperties(face, props)
    area = float(props.Mass())
    _FACE_GEOM_CACHE[key] = area
    return area


def _surface_center(face) -> np.ndarray | None:
    key = _face_cache_key(face, "surface_center")
    if key in _FACE_GEOM_CACHE:
        return _FACE_GEOM_CACHE[key]

    from OCC.Core.BRepGProp import brepgprop
    from OCC.Core.GProp import GProp_GProps

    props = GProp_GProps()
    brepgprop.SurfaceProperties(face, props)
    c = props.CentreOfMass()
    center = np.asarray([c.X(), c.Y(), c.Z()], dtype=float)
    _FACE_GEOM_CACHE[key] = center
    return center


def _face_type(face) -> str:
    key = _face_cache_key(face, "face_type")
    if key in _FACE_GEOM_CACHE:
        return str(_FACE_GEOM_CACHE[key])

    from OCC.Core.BRepAdaptor import BRepAdaptor_Surface
    from OCC.Core.GeomAbs import (
        GeomAbs_BSplineSurface,
        GeomAbs_SurfaceOfExtrusion,
        GeomAbs_Cone,
        GeomAbs_Cylinder,
        GeomAbs_Plane,
        GeomAbs_Torus,
    )

    stype = BRepAdaptor_Surface(face, True).GetType()
    if stype == GeomAbs_Plane:
        face_type = "plane"
    elif stype == GeomAbs_Cylinder:
        face_type = "cylinder"
    elif stype == GeomAbs_Cone:
        face_type = "cone"
    elif stype == GeomAbs_Torus:
        face_type = "torus"
    elif stype == GeomAbs_BSplineSurface:
        face_type = "bspline"
    elif stype == GeomAbs_SurfaceOfExtrusion:
        face_type = "extrusion"
    else:
        face_type = "other"
    _FACE_GEOM_CACHE[key] = face_type
    return face_type


def _plane_info(face) -> tuple[np.ndarray | None, np.ndarray | None]:
    key = _face_cache_key(face, "plane_info")
    if key in _FACE_GEOM_CACHE:
        return _FACE_GEOM_CACHE[key]

    from recommend_and_assemble import plane_info

    center, normal = plane_info(face)
    if center is None or normal is None:
        result = (None, None)
        _FACE_GEOM_CACHE[key] = result
        return result
    n = np.asarray(normal, dtype=float)
    n_norm = float(np.linalg.norm(n))
    if n_norm <= 1e-8:
        result = (None, None)
        _FACE_GEOM_CACHE[key] = result
        return result
    result = (np.asarray(center, dtype=float), n / n_norm)
    _FACE_GEOM_CACHE[key] = result
    return result


def _cylinder_info(face) -> tuple[np.ndarray | None, np.ndarray | None, float | None]:
    key = _face_cache_key(face, "cylinder_info")
    if key in _FACE_GEOM_CACHE:
        return _FACE_GEOM_CACHE[key]

    from recommend_and_assemble import cyl_info

    center, axis, radius = cyl_info(face)
    if center is None or axis is None or radius is None:
        result = (None, None, None)
        _FACE_GEOM_CACHE[key] = result
        return result
    axis_arr = np.asarray(axis, dtype=float)
    axis_norm = float(np.linalg.norm(axis_arr))
    if axis_norm <= 1e-8:
        result = (None, None, None)
        _FACE_GEOM_CACHE[key] = result
        return result
    result = (np.asarray(center, dtype=float), axis_arr / axis_norm, float(radius))
    _FACE_GEOM_CACHE[key] = result
    return result


def _radius_u_v(face) -> tuple[float | None, float, float]:
    key = _face_cache_key(face, "radius_u_v")
    if key in _FACE_GEOM_CACHE:
        return _FACE_GEOM_CACHE[key]

    from OCC.Core.BRepAdaptor import BRepAdaptor_Surface
    from OCC.Core.GeomAbs import GeomAbs_Cone, GeomAbs_Cylinder

    adaptor = BRepAdaptor_Surface(face, True)
    stype = adaptor.GetType()
    radius = None
    if stype == GeomAbs_Cylinder:
        radius = float(adaptor.Cylinder().Radius())
    elif stype == GeomAbs_Cone:
        cone = adaptor.Cone()
        if abs(float(cone.SemiAngle())) < math.radians(5.0):
            radius = float(cone.RefRadius())
    u_range = abs(float(adaptor.LastUParameter()) - float(adaptor.FirstUParameter()))
    v_range = abs(float(adaptor.LastVParameter()) - float(adaptor.FirstVParameter()))
    result = (radius, u_range, v_range)
    _FACE_GEOM_CACHE[key] = result
    return result


def _face_vertices(face) -> np.ndarray:
    key = _face_cache_key(face, "vertices")
    if key in _FACE_GEOM_CACHE:
        return _FACE_GEOM_CACHE[key]

    from OCC.Core.BRep import BRep_Tool
    from OCC.Core.TopAbs import TopAbs_VERTEX
    from OCC.Core.TopExp import TopExp_Explorer

    pts = []
    exp = TopExp_Explorer(face, TopAbs_VERTEX)
    while exp.More():
        p = BRep_Tool.Pnt(exp.Current())
        pts.append([p.X(), p.Y(), p.Z()])
        exp.Next()
    arr = np.asarray(pts, dtype=float)
    _FACE_GEOM_CACHE[key] = arr
    return arr


def _fit_freeform_hole_axis(group: dict[str, Any], faces) -> tuple[np.ndarray | None, np.ndarray | None]:
    """Approximate freeform oblique-hole axis from wall vertices.

    For trimmed oblique hole walls, OCC may expose the surface as freeform. The
    vertex cloud still lies on a thin curved wall; the smallest PCA direction
    is a stable proxy for the hole/end-plane normal in the current data.
    """
    pts = []
    for fi in group.get("face_indices", []):
        fi = int(fi)
        if not (0 <= fi < len(faces)):
            continue
        pts_i = _face_vertices(faces[fi])
        if pts_i.size:
            pts.extend(pts_i.tolist())
    if len(pts) < 3:
        return None, None
    arr = np.asarray(pts, dtype=float)
    center = arr.mean(axis=0)
    x = arr - center
    try:
        vals, vecs = np.linalg.eigh(x.T @ x)
    except np.linalg.LinAlgError:
        return None, None
    axis = vecs[:, int(np.argmin(vals))]
    norm = float(np.linalg.norm(axis))
    if norm <= 1e-8:
        return None, None
    return center, axis / norm


def _outer_support_margin(
    faces,
    scope: set[int],
    center: np.ndarray,
    normal: np.ndarray,
) -> float:
    """Distance from this plane to the closest outer support side of local doghouse geometry.

    A mount face must be on the outside of the local doghouse, so the local
    geometry should lie almost entirely on one side of the candidate plane.
    Plane normal orientation is not trusted, so both sides are considered.
    """
    n = np.asarray(normal, dtype=float)
    nn = float(np.linalg.norm(n))
    if nn <= 1e-8:
        return float("inf")
    n = n / nn
    signed = []
    for fi in scope:
        if not (0 <= int(fi) < len(faces)):
            continue
        c = _surface_center(faces[int(fi)])
        if c is not None:
            signed.append(float(np.dot(c - center, n)))
    if not signed:
        return float("inf")
    # If the candidate is outermost, either the positive or negative extent
    # from this plane is near zero. Interior cap planes have meaningful extent
    # on both sides and therefore a larger margin.
    return min(max(signed), -min(signed))


def _dominant_plane_normal(faces) -> np.ndarray | None:
    """Use the largest model plane as the experimental bottom/shell reference."""
    best_area = -1.0
    best_normal = None
    for face in faces:
        if _face_type(face) != "plane":
            continue
        center, normal = _plane_info(face)
        if center is None or normal is None:
            continue
        area = _surface_area(face)
        if area > best_area:
            best_area = float(area)
            best_normal = np.asarray(normal, dtype=float)
    return best_normal


def _scope_dominant_plane_normal(faces, scope: set[int]) -> np.ndarray | None:
    """Largest plane normal inside one doghouse instance (not the whole part)."""
    best_area = -1.0
    best_normal = None
    for fi in scope:
        fi = int(fi)
        if not (0 <= fi < len(faces)):
            continue
        if _face_type(faces[fi]) != "plane":
            continue
        center, normal = _plane_info(faces[fi])
        if center is None or normal is None:
            continue
        area = _surface_area(faces[fi])
        if area > best_area:
            best_area = float(area)
            best_normal = np.asarray(normal, dtype=float)
    if best_normal is None:
        return None
    nn = float(np.linalg.norm(best_normal))
    if nn <= 1e-8:
        return None
    return best_normal / nn


def _scope_area_weighted_plane_normal(faces, scope: set[int]) -> np.ndarray | None:
    """Doghouse-region orientation from area-weighted plane normals (sign-aligned).

    Unlike the single largest face, this averages all planes in the instance so a
    tall sidewall cannot alone define "the" major direction when the mount top
    is also large (0981/pillar).
    """
    normals: list[np.ndarray] = []
    areas: list[float] = []
    for fi in scope:
        fi = int(fi)
        if not (0 <= fi < len(faces)) or _face_type(faces[fi]) != "plane":
            continue
        _c, normal = _plane_info(faces[fi])
        if normal is None:
            continue
        n = np.asarray(normal, dtype=float)
        nn = float(np.linalg.norm(n))
        if nn <= 1e-8:
            continue
        normals.append(n / nn)
        areas.append(float(_surface_area(faces[fi])))
    if not normals:
        return None
    # Seed with the largest plane, then flip others to the same hemisphere.
    order = sorted(range(len(normals)), key=lambda i: -areas[i])
    ref = normals[order[0]]
    acc = np.zeros(3, dtype=float)
    for i, n in enumerate(normals):
        if float(np.dot(n, ref)) < 0.0:
            n = -n
        acc += areas[i] * n
    an = float(np.linalg.norm(acc))
    if an <= 1e-8:
        return ref
    return acc / an


def _mount_parallel_to_scope_major(
    mount_normal,
    scope_major_normal: np.ndarray | None,
    *,
    min_dot: float = 0.50,
) -> tuple[bool, float]:
    """Soft gate: reject only mounts nearly orthogonal to doghouse-region orientation.

    True mounts on 0981/pillar are often not the single largest face, but they
    are not sidewalls orthogonal to the region. Hard 0.85 parallel-to-max-face
    incorrectly drops them.
    """
    if scope_major_normal is None:
        return True, 0.0
    n = np.asarray(mount_normal, dtype=float)
    nn = float(np.linalg.norm(n))
    if nn <= 1e-8:
        return False, 0.0
    n = n / nn
    align = abs(float(np.dot(n, scope_major_normal)))
    return align >= float(min_dot), align


def _hole_edge_in_mount_bbox_center(
    group: dict[str, Any],
    faces,
    mount_face,
    mount_center: np.ndarray,
    mount_normal: np.ndarray,
    *,
    center_ratio: float = 0.40,
    plane_tol: float = 3.0,
) -> tuple[bool, float]:
    """Require a hole-edge center to lie in the central UV bbox of the mount face.

    Returns (ok, centrality) where centrality in [0,1] is how close the best
    edge center is to the bbox midpoint (1 = exact center).
    """
    n = np.asarray(mount_normal, dtype=float)
    nn = float(np.linalg.norm(n))
    if nn <= 1e-8:
        return False, 0.0
    n = n / nn
    origin = np.asarray(mount_center, dtype=float)

    axis = group.get("axis")
    if axis is None:
        a = n
    else:
        a = np.asarray(axis, dtype=float)
        na = float(np.linalg.norm(a))
        a = n if na <= 1e-8 else a / na

    u = np.cross(a, n)
    if float(np.linalg.norm(u)) <= 1e-8:
        seed = np.array([1.0, 0.0, 0.0])
        if abs(float(np.dot(seed, n))) > 0.9:
            seed = np.array([0.0, 0.0, 1.0])
        u = np.cross(n, seed)
    u = u / float(np.linalg.norm(u))
    v = np.cross(n, u)
    v = v / float(np.linalg.norm(v))

    bounds = _face_plane_bounds(mount_face, origin, u, v)
    if bounds is None:
        return False, 0.0
    min_u, max_u, min_v, max_v = bounds
    width = max_u - min_u
    height = max_v - min_v
    if width <= 1e-8 or height <= 1e-8:
        return False, 0.0
    mid_u = 0.5 * (min_u + max_u)
    mid_v = 0.5 * (min_v + max_v)
    half_u = 0.5 * float(center_ratio) * width
    half_v = 0.5 * float(center_ratio) * height

    edge_centers = _hole_edge_circle_centers(group, faces)
    if not edge_centers:
        ctr = group.get("center")
        if ctr is None:
            return False, 0.0
        edge_centers = [np.asarray(ctr, dtype=float)]

    best_ok = False
    best_centrality = 0.0
    for ec in edge_centers:
        ec = np.asarray(ec, dtype=float)
        if abs(float(np.dot(ec - origin, n))) > float(plane_tol):
            continue
        pu = float(np.dot(ec - origin, u))
        pv = float(np.dot(ec - origin, v))
        in_center = (abs(pu - mid_u) <= half_u) and (abs(pv - mid_v) <= half_v)
        # Centrality: 1 at midpoint, 0 at bbox border (clamped).
        cu = 1.0 - min(1.0, abs(pu - mid_u) / (0.5 * width))
        cv = 1.0 - min(1.0, abs(pv - mid_v) / (0.5 * height))
        centrality = float(min(cu, cv))
        if centrality > best_centrality:
            best_centrality = centrality
        if in_center:
            best_ok = True
    return best_ok, best_centrality


def _local_parallel_reference_info(
    faces,
    scope: set[int],
    candidate_idx: int,
    center: np.ndarray,
    normal: np.ndarray,
    *,
    min_parallel_dot: float = 0.90,
) -> dict[str, Any]:
    """Find the local body plane that is parallel to a mount candidate."""
    n = np.asarray(normal, dtype=float)
    nn = float(np.linalg.norm(n))
    if nn <= 1e-8:
        return {}
    n = n / nn

    best = None
    candidate_area = _surface_area(faces[int(candidate_idx)])
    for fi in scope:
        fi = int(fi)
        if fi == int(candidate_idx) or not (0 <= fi < len(faces)):
            continue
        if _face_type(faces[fi]) != "plane":
            continue
        ref_center, ref_normal = _plane_info(faces[fi])
        if ref_center is None or ref_normal is None:
            continue
        dot = float(np.dot(n, ref_normal))
        alignment = abs(dot)
        if alignment < min_parallel_dot:
            continue
        area = _surface_area(faces[fi])
        # Prefer broad local body/shell faces over tiny construction caps.
        if area < max(25.0, min(candidate_area * 0.25, 150.0)):
            continue
        signed_distance = float(np.dot(np.asarray(center, dtype=float) - ref_center, n))
        distance = abs(signed_distance)
        key = (area, distance, alignment)
        if best is None or key > best[0]:
            best = (
                key,
                {
                    "face_idx": fi,
                    "area": float(area),
                    "parallel_alignment": alignment,
                    "signed_distance": signed_distance,
                    "distance": distance,
                    "normal_dot": dot,
                },
            )
    return best[1] if best is not None else {}


def _load_step_and_adjacency(step_path: str | Path):
    from recommend_and_assemble import build_face_adjacency, load_step

    _clear_face_geom_cache()
    shape, faces = load_step(str(step_path))
    adj = build_face_adjacency(shape, faces)
    return shape, faces, {int(k): {int(x) for x in v} for k, v in adj.items()}


def _scope_hops(seeds: set[int], adj: dict[int, set[int]], scope: set[int], max_hops: int) -> dict[int, int]:
    visited = {int(s): 0 for s in seeds if int(s) in scope}
    queue = deque(visited)
    while queue:
        cur = queue.popleft()
        depth = visited[cur]
        if depth >= max_hops:
            continue
        for nb in adj.get(cur, ()):
            nb = int(nb)
            if nb not in scope or nb in visited:
                continue
            visited[nb] = depth + 1
            queue.append(nb)
    return visited


def _collect_cylinder_candidates(
    faces,
    scope: set[int],
    *,
    min_radius: float,
    max_radius: float,
    min_v_depth: float,
) -> list[dict[str, Any]]:
    out = []
    for fi in sorted(scope):
        if not (0 <= fi < len(faces)):
            continue
        if _face_type(faces[fi]) not in {"cylinder", "cone"}:
            continue
        radius, u_range, v_range = _radius_u_v(faces[fi])
        if radius is None or not (min_radius <= radius <= max_radius):
            continue
        if v_range < min_v_depth:
            continue
        center, axis, _ = _cylinder_info(faces[fi])
        if center is None or axis is None:
            continue
        out.append(
            {
                "face_idx": int(fi),
                "center": center,
                "axis": axis,
                "radius": float(radius),
                "u": float(u_range),
                "v": float(v_range),
            }
        )
    return out


def _cluster_hole_candidates(
    candidates: list[dict[str, Any]],
    *,
    cluster_dist: float,
    radius_tolerance: float,
    min_u_sum: float,
) -> list[dict[str, Any]]:
    groups: list[list[dict[str, Any]]] = []
    for cand in candidates:
        placed = False
        for group in groups:
            ref = group[0]
            same_radius = abs(cand["radius"] - ref["radius"]) <= radius_tolerance
            same_place = float(np.linalg.norm(cand["center"] - ref["center"])) <= cluster_dist
            axis_dot = abs(float(np.dot(cand["axis"], ref["axis"])))
            if same_radius and same_place and axis_dot >= 0.80:
                group.append(cand)
                placed = True
                break
        if not placed:
            groups.append([cand])

    out = []
    for group in groups:
        u_sum = float(sum(c["u"] for c in group))
        if u_sum < min_u_sum:
            continue
        best = max(group, key=lambda c: c["u"])
        center = np.mean([c["center"] for c in group], axis=0)
        out.append(
            {
                "center": _vec(center),
                "axis": _vec(best["axis"]),
                "radius": round(float(best["radius"]), 6),
                "face_idx": int(best["face_idx"]),
                "best_idx": int(best["face_idx"]),
                "face_indices": sorted(int(c["face_idx"]) for c in group),
                "u_sum": round(u_sum, 6),
                "v_max": round(float(max(c["v"] for c in group)), 6),
                "source": "geometry",
            }
        )
    out.sort(key=lambda g: (-len(g["face_indices"]), -g["u_sum"], g["face_indices"][0]))
    return out


def _coaxial_through_hole(
    face_center,
    face_normal,
    near_groups: list[dict[str, Any]],
    *,
    parallel_tol: float = 0.9,
    max_end_gap: float = 6.0,
    max_lateral: float = 30.0,
) -> bool:
    """安装面判据: 面是否为「孔轴穿过的同轴端面」.

    通孔穿过的主体安装平面满足:
      - 法向与孔轴平行 (|n·axis| ≈ 1)
      - 孔中心到该面所在平面的距离很小 (端面, 落在孔两端的薄板范围内)
    用「沿法向到孔中心的距离」而非「质心到孔轴的横向距离」判定,
    因为主体大平面的质心通常远离孔轴, 横向距离不可靠.
    """
    fc = np.asarray(face_center, dtype=float)
    fn = np.asarray(face_normal, dtype=float)
    for g in near_groups:
        axis = g.get("axis")
        ctr = g.get("center")
        if axis is None or ctr is None:
            continue
        a = np.asarray(axis, dtype=float)
        c = np.asarray(ctr, dtype=float)
        na = float(np.linalg.norm(a))
        if na <= 1e-8:
            continue
        a = a / na
        if abs(float(np.dot(fn, a))) < parallel_tol:
            continue
        d = fc - c
        end_gap = abs(float(np.dot(d, fn)))
        lateral = float(np.linalg.norm(d - float(np.dot(d, a)) * a))
        if end_gap <= max_end_gap and lateral <= max_lateral:
            return True
    return False


def _mount_axis_direction_score(face_normal, groups: list[dict[str, Any]]) -> float:
    """Soft prior for choosing the mounting side of a through-hole."""
    n = np.asarray(face_normal, dtype=float)
    nn = float(np.linalg.norm(n))
    if nn <= 1e-8:
        return 0.0
    n = n / nn
    best = 0.0
    for group in groups:
        axis = group.get("axis")
        if axis is None:
            continue
        a = np.asarray(axis, dtype=float)
        na = float(np.linalg.norm(a))
        if na <= 1e-8:
            continue
        score = float(np.dot(n, a / na))
        if group.get("axis_source") == "freeform_pca":
            score = abs(score)
        best = max(best, score)
    return best


def _mount_axis_abs_score(face_normal, groups: list[dict[str, Any]]) -> float:
    """Unsigned axis alignment used only to compensate reversed plane normals."""
    n = np.asarray(face_normal, dtype=float)
    nn = float(np.linalg.norm(n))
    if nn <= 1e-8:
        return 0.0
    n = n / nn
    best = 0.0
    for group in groups:
        axis = group.get("axis")
        if axis is None:
            continue
        a = np.asarray(axis, dtype=float)
        na = float(np.linalg.norm(a))
        if na <= 1e-8:
            continue
        best = max(best, abs(float(np.dot(n, a / na))))
    return best


def _point_in_face(face, point: np.ndarray, *, tol: float = 1e-3) -> bool:
    from OCC.Core.BRepClass import BRepClass_FaceClassifier
    from OCC.Core.gp import gp_Pnt
    from OCC.Core.TopAbs import TopAbs_IN, TopAbs_ON

    classifier = BRepClass_FaceClassifier()
    classifier.Perform(face, gp_Pnt(float(point[0]), float(point[1]), float(point[2])), float(tol))
    return classifier.State() in (TopAbs_IN, TopAbs_ON)


def _face_plane_bounds(face, origin: np.ndarray, u: np.ndarray, v: np.ndarray):
    from OCC.Core.BRep import BRep_Tool
    from OCC.Core.TopAbs import TopAbs_VERTEX
    from OCC.Core.TopExp import TopExp_Explorer

    coords = []
    exp = TopExp_Explorer(face, TopAbs_VERTEX)
    while exp.More():
        p = BRep_Tool.Pnt(exp.Current())
        q = np.asarray([p.X(), p.Y(), p.Z()], dtype=float) - origin
        coords.append((float(np.dot(q, u)), float(np.dot(q, v))))
        exp.Next()
    if not coords:
        return None
    arr = np.asarray(coords, dtype=float)
    return float(arr[:, 0].min()), float(arr[:, 0].max()), float(arr[:, 1].min()), float(arr[:, 1].max())


def _hole_edge_circle_centers(group: dict[str, Any], faces) -> list[np.ndarray]:
    """Estimate the two edge-circle centers of a cylindrical hole group."""
    ctr = group.get("center")
    axis = group.get("axis")
    if ctr is None or axis is None:
        return []
    c = np.asarray(ctr, dtype=float)
    a = np.asarray(axis, dtype=float)
    na = float(np.linalg.norm(a))
    if na <= 1e-8:
        return [c]
    a = a / na

    from OCC.Core.BRep import BRep_Tool
    from OCC.Core.TopAbs import TopAbs_VERTEX
    from OCC.Core.TopExp import TopExp_Explorer

    ts = []
    for fi in group.get("face_indices", []):
        fi = int(fi)
        if not (0 <= fi < len(faces)):
            continue
        exp = TopExp_Explorer(faces[fi], TopAbs_VERTEX)
        while exp.More():
            p = BRep_Tool.Pnt(exp.Current())
            q = np.asarray([p.X(), p.Y(), p.Z()], dtype=float)
            ts.append(float(np.dot(q - c, a)))
            exp.Next()
    if len(ts) < 2:
        return [c]
    return [c + min(ts) * a, c + max(ts) * a]


def _hole_contour_inside_mount_face(
    group: dict[str, Any],
    faces,
    mount_face,
    mount_center: np.ndarray,
    mount_normal: np.ndarray,
    *,
    samples: int = 12,
    tol: float = 0.05,
    plane_tol: float = 3.0,
) -> bool:
    """Require the mounting-hole contour to lie inside the candidate mount face."""
    axis = group.get("axis")
    if axis is None:
        return True

    n = np.asarray(mount_normal, dtype=float)
    nn = float(np.linalg.norm(n))
    if nn <= 1e-8:
        return True
    n = n / nn

    # Build an in-plane basis and compare the actual projected arc/edge bounds,
    # not a full-circle bound. Open C-shaped holes can have a full circle that
    # extends outside the mount face, while their real arc contour is inside.
    a = np.asarray(axis, dtype=float)
    if float(np.linalg.norm(a)) <= 1e-8:
        a = n
    a = a / float(np.linalg.norm(a))
    u = np.cross(a, n)
    if float(np.linalg.norm(u)) <= 1e-8:
        # Straight hole: pick any stable direction inside the mount plane.
        seed = np.array([1.0, 0.0, 0.0])
        if abs(float(np.dot(seed, n))) > 0.9:
            seed = np.array([0.0, 0.0, 1.0])
        u = np.cross(n, seed)
    u = u / float(np.linalg.norm(u))
    v = np.cross(n, u)
    v = v / float(np.linalg.norm(v))

    bounds = _face_plane_bounds(mount_face, np.asarray(mount_center, dtype=float), u, v)
    if bounds is None:
        return True
    min_u, max_u, min_v, max_v = bounds
    margin = 0.2

    origin = np.asarray(mount_center, dtype=float)
    # At least one real hole edge circle must lie on/near the candidate mount
    # plane. A side wall can receive the projected arc bbox, but its distance
    # to both edge circles is large, so it is not the mounting plane.
    edge_centers = _hole_edge_circle_centers(group, faces)
    if edge_centers:
        if not any(abs(float(np.dot(ec - origin, n))) <= float(plane_tol) for ec in edge_centers):
            return False
    pts = []
    for fi in group.get("face_indices", []):
        fi = int(fi)
        if not (0 <= fi < len(faces)):
            continue
        pts_i = _face_vertices(faces[fi])
        if pts_i.size:
            pts.extend(pts_i.tolist())
    if not pts:
        return True
    pts_arr = np.asarray(pts, dtype=float)
    plane_dist = np.asarray([float(np.dot(p - origin, n)) for p in pts_arr], dtype=float)
    abs_dist = np.abs(plane_dist)
    min_dist = float(abs_dist.min())
    if min_dist > float(plane_tol):
        return False

    # Only use the edge loop/arc closest to this mount plane. For oblique holes,
    # projecting the whole wall can be too large; the assembly rule is that one
    # circular arc edge projects inside the mount-face bbox.
    edge_pts = pts_arr[abs_dist <= min_dist + float(plane_tol)]
    if len(edge_pts) < 2:
        edge_pts = pts_arr[abs_dist <= min_dist + 1e-6]

    coords = []
    for q3 in edge_pts:
        p0 = q3 - float(np.dot(q3 - origin, n)) * n
        q2 = p0 - origin
        coords.append((float(np.dot(q2, u)), float(np.dot(q2, v))))
    arr = np.asarray(coords, dtype=float)
    arc_min_u, arc_max_u = float(arr[:, 0].min()), float(arr[:, 0].max())
    arc_min_v, arc_max_v = float(arr[:, 1].min()), float(arr[:, 1].max())
    return (
        arc_min_u >= min_u - margin
        and arc_max_u <= max_u + margin
        and arc_min_v >= min_v - margin
        and arc_max_v <= max_v + margin
    )


def _hole_group_u_sum(group: dict[str, Any], faces) -> float:
    total = 0.0
    for fi in group.get("face_indices", []):
        fi = int(fi)
        if 0 <= fi < len(faces):
            _radius, u_range, _v_range = _radius_u_v(faces[fi])
            total += float(u_range)
    return total


def _select_single_mounting_hole(groups: list[dict[str, Any]], faces) -> list[dict[str, Any]]:
    """Each doghouse mount face has one mounting hole; keep the strongest group."""
    if len(groups) <= 1:
        return groups

    best = max(
        groups,
        key=lambda g: (
            _hole_group_analytic_priority(g),
            _hole_group_u_sum(g, faces),
            float(g.get("radius", 0.0)),
            -len(g.get("face_indices", [])),
        ),
    )
    return [best]


def _hole_group_analytic_priority(group: dict[str, Any]) -> int:
    # Freeform VF2 groups are useful as a fallback, but they do not carry a
    # reliable hole axis. Prefer analytic cylinder/cone groups unless a
    # mount-aware endpoint check proves the freeform group is the true hole.
    if group.get("axis") is not None:
        return 1
    try:
        return 1 if float(group.get("radius", 0.0)) > 0.0 else 0
    except (TypeError, ValueError):
        return 0


def _select_single_mounting_hole_for_mount(
    groups: list[dict[str, Any]],
    faces,
    scope: set[int],
    adj: dict[int, set[int]],
    mount_idx: int,
) -> list[dict[str, Any]]:
    """Keep the hole group that best satisfies this mount's endpoint topology."""
    if len(groups) <= 1:
        return groups
    best = max(
        groups,
        key=lambda g: (
            _hole_group_analytic_priority(g),
            1 if _parallel_plane_link_info(faces, scope, adj, mount_idx, g).get("endpoint") else 0,
            _hole_group_u_sum(g, faces),
            float(g.get("radius", 0.0)),
            -len(g.get("face_indices", [])),
        ),
    )
    return [best]


def _geometry_holes_for_mount_face(
    faces,
    scope: set[int],
    adj: dict[int, set[int]],
    mount: dict[str, Any],
    *,
    min_radius: float,
    max_radius: float,
    min_v_depth: float,
    cluster_dist: float,
    radius_tolerance: float,
    min_u_sum: float,
    hole_hops: int,
) -> list[dict[str, Any]]:
    """Fallback: one tangent-continuous hole group whose arc projects on mount."""
    mount_idx = int(mount["face_idx"])
    mount_center = np.asarray(mount.get("center", []), dtype=float)
    mount_normal = np.asarray(mount.get("normal", []), dtype=float)
    if mount_center.size != 3 or mount_normal.size != 3:
        return []
    groups = _cluster_hole_candidates(
        _collect_cylinder_candidates(
            faces,
            scope,
            min_radius=min_radius,
            max_radius=max_radius,
            min_v_depth=min_v_depth,
        ),
        cluster_dist=cluster_dist,
        radius_tolerance=radius_tolerance,
        min_u_sum=min(min_u_sum, math.pi / 2),
    )
    reach = _scope_hops({mount_idx}, adj, scope, hole_hops + 1)
    out: list[dict[str, Any]] = []
    for group in groups:
        if not ({int(x) for x in group.get("face_indices", [])} & set(reach)):
            continue
        best_idx = int(group.get("best_idx", group.get("face_idx", group["face_indices"][0])))
        face_indices = _continuous_hole_component(
            [int(fi) for fi in group.get("face_indices", [])],
            faces,
            adj,
            best_idx=best_idx,
        )
        if not face_indices:
            continue
        if best_idx not in face_indices:
            best_idx = int(face_indices[0])
        center, axis, radius = _cylinder_info(faces[best_idx])
        if center is None or axis is None or radius is None:
            continue
        copied = dict(group)
        copied["face_indices"] = face_indices
        copied["face_idx"] = best_idx
        copied["best_idx"] = best_idx
        copied["center"] = _vec(center)
        copied["axis"] = _vec(axis)
        copied["radius"] = round(float(radius), 6)
        copied["source"] = "geometry_mount_fallback"
        copied["mount_face_idx"] = mount_idx
        if _hole_contour_inside_mount_face(
            copied,
            faces,
            faces[mount_idx],
            mount_center,
            mount_normal,
        ):
            out.append(copied)
    return _select_single_mounting_hole(out, faces)


def _group_touching_mount(group: dict[str, Any], mount_idx: int, adj: dict[int, set[int]]) -> bool:
    for fi in group.get("face_indices", []):
        fi = int(fi)
        if fi in adj.get(mount_idx, ()):
            return True
        for nb in adj.get(mount_idx, ()):
            if fi in adj.get(int(nb), ()):
                return True
    return False


def _is_freeform_hole_seed(face) -> bool:
    """Broad seed for non-analytic hole walls used only before VF2 validation."""
    if _face_type(face) not in {"other", "bspline", "extrusion"}:
        return False
    try:
        _radius, u_range, v_range = _radius_u_v(face)
    except Exception:
        return False
    return float(u_range) >= 1.0 and float(v_range) >= 0.5


def _parallel_plane_link_info(
    faces,
    scope: set[int],
    adj: dict[int, set[int]],
    mount_idx: int,
    group: dict[str, Any],
    *,
    max_hops: int = 2,
    min_separation: float = 0.5,
) -> dict[str, Any]:
    """Check whether a hole group directly/fillet-connects two parallel planes."""
    mount_center, mount_normal = _plane_info(faces[int(mount_idx)])
    if mount_center is None or mount_normal is None:
        return {"endpoint": False, "partner_face_idx": None, "parallel_count": 0}
    seeds = {int(fi) for fi in group.get("face_indices", []) if int(fi) in scope}
    if not seeds:
        return {"endpoint": False, "partner_face_idx": None, "parallel_count": 0}
    reach = _scope_hops(seeds, adj, scope, max_hops)
    plane_faces = [
        int(fi)
        for fi, _dist in reach.items()
        if int(fi) != int(mount_idx) and _face_type(faces[int(fi)]) == "plane"
    ]
    mount_is_endpoint = int(mount_idx) in reach
    best_partner = None
    best_sep = 0.0
    parallel_count = 0
    for fi in plane_faces:
        center, normal = _plane_info(faces[fi])
        if center is None or normal is None:
            continue
        parallel = abs(float(np.dot(mount_normal, normal)))
        if parallel < 0.92:
            continue
        sep = abs(float(np.dot(center - mount_center, mount_normal)))
        if sep < min_separation:
            continue
        parallel_count += 1
        if sep > best_sep:
            best_sep = sep
            best_partner = fi
    endpoint = bool(mount_is_endpoint and best_partner is not None)
    return {
        "endpoint": endpoint,
        "partner_face_idx": best_partner,
        "parallel_count": int(parallel_count),
        "separation": round(float(best_sep), 6),
        "mount_reach_hops": int(reach.get(int(mount_idx), -1)),
    }


def _mount_candidates(
    faces,
    scope: set[int],
    adj: dict[int, set[int]],
    hole_groups: list[dict[str, Any]],
    role_by_face: dict[int, str],
    *,
    min_area: float,
    max_area: float,
    hole_hops: int,
    outer_support_tolerance: float = 3.0,
    experimental_freeform_endpoint: bool = True,
) -> list[dict[str, Any]]:
    hole_faces = {int(fi) for g in hole_groups for fi in g.get("face_indices", [])}
    out = []
    for fi in sorted(scope):
        if not (0 <= fi < len(faces)):
            continue
        if _face_type(faces[fi]) != "plane":
            continue
        area = _surface_area(faces[fi])
        if not (min_area <= area <= max_area):
            continue
        reach = _scope_hops({fi}, adj, scope, hole_hops)
        near_hole_faces = sorted(h for h in hole_faces if h in reach)
        near_groups = [
            g for g in hole_groups
            if set(int(x) for x in g.get("face_indices", [])) & set(near_hole_faces)
        ]
        if not near_groups:
            continue
        center, normal = _plane_info(faces[fi])
        if center is None or normal is None:
            continue
        centered_groups = []
        best_centrality = 0.0
        for g in near_groups:
            ok, centrality = _hole_edge_in_mount_bbox_center(
                g, faces, faces[fi], center, normal, center_ratio=0.60
            )
            best_centrality = max(best_centrality, centrality)
            if ok:
                centered_groups.append(g)
        if centered_groups:
            near_groups = centered_groups
        outer_margin = _outer_support_margin(faces, scope, center, normal)
        scope_align = 0.0
        small_plane_neighbors = sum(
            1
            for nb in adj.get(fi, ())
            if nb in scope and nb != fi and _face_type(faces[nb]) == "plane"
        )
        direct_touch = sum(1 for g in near_groups if _group_touching_mount(g, fi, adj))
        if experimental_freeform_endpoint:
            parallel_infos = [
                _parallel_plane_link_info(faces, scope, adj, int(fi), g)
                for g in near_groups
            ]
            parallel_endpoint = any(info.get("endpoint") for info in parallel_infos)
            parallel_count = max((int(info.get("parallel_count", 0)) for info in parallel_infos), default=0)
        else:
            parallel_endpoint = False
            parallel_count = 0
        # 严格 1 跳: 面是否直接与孔壁面共享边 (紧贴孔口的凹陷相邻小面).
        face_nbrs = adj.get(fi, set())
        rims_hole = any(
            int(hf) in face_nbrs
            for g in near_groups
            for hf in g.get("face_indices", [])
        )
        coaxial_through = _coaxial_through_hole(center, normal, near_groups)
        # 安装面是「孔轴穿过的同轴主体平面」, 与孔壁之间隔着倒角/开口 (≥2 跳);
        # 紧贴孔壁 (rims_hole, 1 跳) 的同轴面通常是孔口凹陷相邻小面 → 降权.
        if coaxial_through and not rims_hole:
            coaxial_bonus = 14.0
        elif coaxial_through:
            coaxial_bonus = 6.0
        else:
            coaxial_bonus = 0.0
        semantic_bonus = 12.0 if role_by_face.get(fi) == "mount" else 0.0
        axis_direction = _mount_axis_direction_score(normal, near_groups)
        axis_abs = _mount_axis_abs_score(normal, near_groups)
        axis_direction_bonus = 3.0 * axis_direction
        parallel_bonus = 10.0 if experimental_freeform_endpoint and parallel_endpoint else 0.0
        outer_endpoint_bonus = (
            max(0.0, 5.0 - float(outer_margin)) * 2.0
            if experimental_freeform_endpoint and parallel_endpoint and coaxial_through
            else 0.0
        )
        scope_parallel_bonus = 12.0 * float(scope_align)
        hole_center_bonus = 15.0 * float(best_centrality)
        score = (
            semantic_bonus
            + coaxial_bonus
            + axis_direction_bonus
            + parallel_bonus
            + outer_endpoint_bonus
            + scope_parallel_bonus
            + hole_center_bonus
            + 5.0 * len(near_groups)
            + min(area, 900.0) / 120.0
            + min(small_plane_neighbors, 4) * 1.2
        )
        out.append(
            {
                "face_idx": int(fi),
                "score": round(float(score), 6),
                "area": round(float(area), 6),
                "center": _vec(center),
                "normal": _vec(normal),
                "near_hole_faces": near_hole_faces,
                "near_hole_group_count": len(near_groups),
                "direct_hole_group_count": int(direct_touch),
                "rims_hole": bool(rims_hole),
                "coaxial_through_hole": bool(coaxial_through),
                "axis_direction_score": round(float(axis_direction), 6),
                "axis_abs_score": round(float(axis_abs), 6),
                "parallel_plane_link": bool(parallel_endpoint),
                "parallel_plane_count": int(parallel_count),
                "outer_endpoint_bonus": round(float(outer_endpoint_bonus), 6),
                "scope_major_parallel": round(float(scope_align), 6),
                "scope_parallel_bonus": round(float(scope_parallel_bonus), 6),
                "hole_bbox_centrality": round(float(best_centrality), 6),
                "hole_center_bonus": round(float(hole_center_bonus), 6),
                "outer_support_margin": round(float(outer_margin), 6),
                "role_hint": role_by_face.get(fi, "background"),
            }
        )
    out.sort(key=lambda c: (-c["score"], c["face_idx"]))
    return out


def _build_local_vf2_aag(shape, faces, adj: dict[int, set[int]], scope: set[int], mount_idx: int):
    """Build a dense, doghouse-scoped AAG for VF2 hole verification."""
    from OCC.Core.BRep import BRep_Tool
    from OCC.Core.BRepAdaptor import BRepAdaptor_Surface
    from OCC.Core.BRepGProp import brepgprop
    from OCC.Core.GeomAbs import (
        GeomAbs_BSplineSurface,
        GeomAbs_SurfaceOfExtrusion,
        GeomAbs_Cone,
        GeomAbs_Cylinder,
        GeomAbs_Plane,
        GeomAbs_Torus,
    )
    from OCC.Core.GeomLProp import GeomLProp_SLProps
    from OCC.Core.GProp import GProp_GProps
    from OCC.Core.ShapeAnalysis import ShapeAnalysis_Surface
    from OCC.Core.gp import gp_Pnt
    from OCC.Extend.TopologyUtils import TopologyExplorer
    from rule_pillar import AAG, AAGEdge, AAGNode, _compute_dihedral_angle, _convexity_name, _edge_curve_type

    local_faces = sorted({int(mount_idx), *(int(x) for x in scope if 0 <= int(x) < len(faces))})
    orig_to_local = {orig: local for local, orig in enumerate(local_faces)}
    local_to_orig = {local: orig for orig, local in orig_to_local.items()}

    nodes = {}
    for local_idx, orig_idx in enumerate(local_faces):
        face = faces[orig_idx]
        sf = BRepAdaptor_Surface(face, True)
        ft = sf.GetType()
        radius = 0.0
        depth = 0.0
        has_radius = False
        u_range = 0.0
        is_true_cone = False
        try:
            u_range = abs(float(sf.LastUParameter()) - float(sf.FirstUParameter()))
            depth = abs(float(sf.LastVParameter()) - float(sf.FirstVParameter()))
        except Exception:
            u_range = 0.0
            depth = 0.0
        if ft == GeomAbs_Cylinder:
            radius = float(sf.Cylinder().Radius())
            has_radius = True
        elif ft == GeomAbs_Cone:
            try:
                cone = sf.Cone()
                if abs(float(cone.SemiAngle())) < math.radians(5):
                    radius = float(cone.RefRadius())
                    has_radius = True
                    is_true_cone = True
            except Exception:
                pass

        props = GProp_GProps()
        brepgprop.SurfaceProperties(face, props)
        type_map = {
            GeomAbs_Plane: "plane",
            GeomAbs_Cylinder: "cylinder",
            GeomAbs_Cone: "cone",
            GeomAbs_Torus: "torus",
            GeomAbs_BSplineSurface: "bspline",
            GeomAbs_SurfaceOfExtrusion: "extrusion",
        }
        face_type = type_map.get(ft, "other")

        normal = (0.0, 0.0, 0.0)
        if ft == GeomAbs_Plane:
            d = sf.Plane().Axis().Direction()
            normal = (d.X(), d.Y(), d.Z())
        else:
            try:
                c = props.CentreOfMass()
                surface = BRep_Tool.Surface(face)
                uv = ShapeAnalysis_Surface(surface).ValueOfUV(gp_Pnt(c.X(), c.Y(), c.Z()), 0.01)
                sl = GeomLProp_SLProps(surface, uv.X(), uv.Y(), 1, 1e-6)
                if sl.IsNormalDefined():
                    n = sl.Normal()
                    normal = (n.X(), n.Y(), n.Z())
            except Exception:
                pass

        nodes[local_idx] = AAGNode(
            local_idx,
            face_type,
            radius,
            depth,
            float(props.Mass()),
            has_radius,
            u_range,
            is_true_cone,
            normal,
        )

    local_adj = defaultdict(set)
    for orig_a in local_faces:
        local_a = orig_to_local[orig_a]
        for orig_b in adj.get(orig_a, ()):
            if orig_b in orig_to_local:
                local_b = orig_to_local[orig_b]
                if local_a != local_b:
                    local_adj[local_a].add(local_b)
                    local_adj[local_b].add(local_a)

    topo = TopologyExplorer(shape)
    face_by_hash = {hash(faces[orig]): orig for orig in local_faces}
    edge_list = []
    seen_pairs = set()
    for orig_a in local_faces:
        face_a = faces[orig_a]
        for edge in topo.edges_from_face(face_a):
            incident = list(topo.faces_from_edge(edge))
            if len(incident) != 2:
                continue
            pair_orig = [face_by_hash.get(hash(incident_face)) for incident_face in incident]
            if any(x is None for x in pair_orig):
                continue
            if len(pair_orig) != 2:
                continue
            a, b = pair_orig
            if a == b or a not in orig_to_local or b not in orig_to_local:
                continue
            pair = tuple(sorted((orig_to_local[a], orig_to_local[b])))
            if pair in seen_pairs:
                continue
            seen_pairs.add(pair)
            angle = _compute_dihedral_angle(faces[a], faces[b], edge)
            edge_list.append(
                AAGEdge(
                    pair[0],
                    pair[1],
                    _convexity_name(angle),
                    angle,
                    _edge_curve_type(edge),
                )
            )

    local_mount = orig_to_local[int(mount_idx)]
    aag = AAG(
        nodes,
        local_adj,
        edge_list,
        shape,
        [faces[orig] for orig in local_faces],
        local_mount,
        nodes[local_mount].normal,
    )
    return aag, local_mount, local_to_orig


def _filter_hole_groups_in_scope(groups: list[dict[str, Any]], scope: set[int]) -> list[dict[str, Any]]:
    """Keep VF2 hole groups whose faces stay inside the doghouse scope."""
    scope_set = {int(x) for x in scope}
    out: list[dict[str, Any]] = []
    for group in groups:
        face_indices = [int(fi) for fi in group.get("face_indices", [])]
        if face_indices and all(fi in scope_set for fi in face_indices):
            out.append(group)
    return out


def _same_tangent_hole_surface(face, ref_radius: float | None, ref_axis: np.ndarray | None) -> bool:
    """Face-level filter for one tangent-continuous mounting hole wall."""
    if _face_type(face) not in {"cylinder", "cone"}:
        return False
    radius, _u, _v = _radius_u_v(face)
    if ref_radius is not None:
        if radius is None or abs(float(radius) - float(ref_radius)) > 0.35:
            return False
    if ref_axis is not None:
        _center, axis, _radius = _cylinder_info(face)
        if axis is None:
            return False
        # Same hole wall can be split into several cylindrical/conical patches,
        # but their axes should remain almost identical.
        if abs(float(np.dot(axis, ref_axis))) < 0.95:
            return False
    return True


def _continuous_hole_component(
    face_indices: list[int],
    faces,
    adj: dict[int, set[int]],
    *,
    best_idx: int,
) -> list[int]:
    """Keep the tangent-connected component containing the representative hole face."""
    if not (0 <= int(best_idx) < len(faces)):
        return []
    _center, ref_axis, ref_radius = _cylinder_info(faces[int(best_idx)])
    if ref_axis is None:
        # Freeform hole walls (bspline/extrusion in OCC, "other" here) have no
        # analytic cylinder axis. Keep the topologically continuous freeform
        # component selected by VF2 and let projection checks validate it.
        candidates = {
            int(fi)
            for fi in face_indices
            if 0 <= int(fi) < len(faces)
            and _face_type(faces[int(fi)]) in {"other", "bspline", "extrusion"}
        }
    else:
        candidates = {
            int(fi)
            for fi in face_indices
            if 0 <= int(fi) < len(faces)
            and _same_tangent_hole_surface(faces[int(fi)], ref_radius, ref_axis)
        }
    if not candidates:
        return []
    components: list[set[int]] = []
    seen: set[int] = set()
    for start in sorted(candidates):
        if start in seen:
            continue
        comp = {start}
        queue = deque([start])
        seen.add(start)
        while queue:
            cur = queue.popleft()
            for nb in adj.get(cur, ()):
                nb = int(nb)
                if nb in candidates and nb not in seen:
                    seen.add(nb)
                    comp.add(nb)
                    queue.append(nb)
        components.append(comp)
    if int(best_idx) in candidates:
        selected = next(comp for comp in components if int(best_idx) in comp)
    else:
        selected = max(components, key=lambda comp: (len(comp), -min(comp)))
    return sorted(selected)


def _map_vf2_groups(
    groups,
    local_to_orig,
    faces,
    scope,
    adj,
    *,
    enrich_freeform_axis: bool = False,
) -> list[dict[str, Any]]:
    """Map local VF2 hole groups back to original face indices + enrich geometry."""

    mapped_groups = []
    for group in groups:
        mapped = dict(group)
        mapped["face_indices"] = [
            int(local_to_orig[int(fi)])
            for fi in group.get("face_indices", [])
            if int(fi) in local_to_orig
        ]
        if group.get("best_idx") is not None and int(group["best_idx"]) in local_to_orig:
            mapped["best_idx"] = int(local_to_orig[int(group["best_idx"])])
        if group.get("face_idx") is not None and int(group["face_idx"]) in local_to_orig:
            mapped["face_idx"] = int(local_to_orig[int(group["face_idx"])])
        if not mapped["face_indices"]:
            continue
        mapped_groups.append(mapped)

    out = []
    for group in _filter_hole_groups_in_scope(mapped_groups, scope):
        copied = dict(group)
        best_idx = int(copied.get("best_idx", copied.get("face_idx", copied["face_indices"][0])))
        copied["face_indices"] = _continuous_hole_component(
            [int(fi) for fi in copied.get("face_indices", [])],
            faces,
            adj,
            best_idx=best_idx,
        )
        if not copied["face_indices"]:
            continue
        if best_idx not in copied["face_indices"]:
            best_idx = int(copied["face_indices"][0])
        center, axis, radius = _cylinder_info(faces[best_idx])
        if center is not None and axis is not None:
            copied["center"] = _vec(center)
            copied["axis"] = _vec(axis)
        if radius is not None:
            copied["radius"] = round(float(radius), 6)
        elif enrich_freeform_axis:
            freeform_center, freeform_axis = _fit_freeform_hole_axis(copied, faces)
            if freeform_center is not None and freeform_axis is not None:
                copied["center"] = _vec(freeform_center)
                copied["axis"] = _vec(freeform_axis)
                copied["axis_source"] = "freeform_pca"
        copied["face_idx"] = best_idx
        copied["best_idx"] = best_idx
        copied["source"] = "vf2"
        out.append(copied)
    return out


def _vf2_holes_for_mount(
    step_path: str | Path,
    shape,
    faces,
    adj: dict[int, set[int]],
    mount_idx: int,
    scope: set[int],
    *,
    min_radius: float,
    min_u_sum: float,
) -> list[dict[str, Any]]:
    from rule_pillar import detect_holes_mcf_vf2

    aag, local_mount, local_to_orig = _build_local_vf2_aag(shape, faces, adj, scope, int(mount_idx))
    groups = detect_holes_mcf_vf2(
        aag,
        local_mount,
        min_radius=float(min_radius),
        min_u_sum=float(min_u_sum),
    )
    return _map_vf2_groups(groups, local_to_orig, faces, scope, adj)


def _hole_group_has_analytic_radius(
    group: dict[str, Any],
    faces,
    *,
    min_radius: float,
    max_radius: float,
) -> bool:
    """True if the hole group contains a parseable cylinder/cone in the fastener band.

    Drafted fastener holes (pillar) are OCC cones with RefRadius in-band; those
    count. Group-level radius alone does not count — VF2 may attach a numeric
    radius to extrusion/freeform walls that are not analytic round holes.
    """
    if str(group.get("axis_source", "")) == "freeform_pca":
        return False
    for fi in group.get("face_indices", []):
        fi = int(fi)
        if not (0 <= fi < len(faces)):
            continue
        if _face_type(faces[fi]) not in {"cylinder", "cone"}:
            continue
        radius, _u, _v = _radius_u_v(faces[fi])
        if radius is not None and float(min_radius) <= float(radius) <= float(max_radius):
            return True
    return False


def _vf2_has_reliable_analytic_hole(
    holes: list[dict[str, Any]],
    faces,
    *,
    min_radius: float,
    max_radius: float,
) -> bool:
    """Rule path wins only when VF2/geometry produced a reliably parseable round hole."""
    return any(
        _hole_group_has_analytic_radius(g, faces, min_radius=min_radius, max_radius=max_radius)
        for g in holes
    )


def _vf2_supported_by_ai_structure(
    mount: dict[str, Any] | None,
    holes: list[dict[str, Any]],
    prediction: dict,
    *,
    mount_score_threshold: float,
    hole_score_threshold: float,
) -> bool:
    """True if AI structure agrees this VF2 result is a mount/hole pair.

    Without structure probabilities (legacy checkpoints), analytic VF2 keeps its
    old behavior. When structure heads are present, an analytic round hole only
    wins over AI fallback if either the mount face has high mount_prob or at
    least one hole-wall face has high hole_wall_prob.
    """
    rows = {
        int(r["face_idx"]): r
        for r in prediction.get("face_predictions", [])
        if "face_idx" in r
    }
    has_structure_probs = any(
        "mount_prob" in r or "hole_wall_prob" in r
        for r in rows.values()
    )
    if not has_structure_probs:
        return True
    if mount is not None:
        mount_idx = mount.get("face_idx")
        if mount_idx is not None:
            row = rows.get(int(mount_idx), {})
            if float(row.get("mount_prob", 0.0) or 0.0) >= float(mount_score_threshold):
                return True
    for g in holes:
        for fi in g.get("face_indices", []):
            row = rows.get(int(fi), {})
            if float(row.get("hole_wall_prob", 0.0) or 0.0) >= float(hole_score_threshold):
                return True
    return False


def _ai_structure_mount_and_holes(
    faces,
    adj: dict[int, set[int]],
    scope: set[int],
    prediction: dict,
    *,
    mount_min_area: float,
    mount_max_area: float,
    hole_hops: int = 2,
    mount_score_threshold: float = 0.35,
    hole_score_threshold: float = 0.35,
) -> tuple[dict[str, Any] | None, list[dict[str, Any]], list[dict[str, Any]], float]:
    """Select mount + through-hole walls from AI structure probs (no radius required).

    Mount: high mount_prob, plane, area band, prefer outermost (small outer margin).
    Hole walls: high hole_wall_prob faces within hole_hops of the mount (transition
    bridges allowed as ordinary adjacency).
    """
    face_rows = {
        int(r["face_idx"]): r
        for r in prediction.get("face_predictions", [])
        if "face_idx" in r
    }
    mount_cands: list[dict[str, Any]] = []
    for fi in sorted(scope):
        if _face_type(faces[fi]) != "plane":
            continue
        area = _surface_area(faces[fi])
        if not (mount_min_area <= area <= mount_max_area):
            continue
        center, normal = _plane_info(faces[fi])
        if center is None or normal is None:
            continue
        row = face_rows.get(int(fi), {})
        mount_p = float(row.get("mount_prob", 0.0) or 0.0)
        if mount_p < float(mount_score_threshold) and str(row.get("role")) != "mount":
            # Allow role=mount from thresholded infer even if raw prob slightly low.
            if mount_p < float(mount_score_threshold) * 0.5:
                continue
        outer_margin = _outer_support_margin(faces, scope, center, normal)
        score = 20.0 * mount_p + max(0.0, 8.0 - float(outer_margin)) + min(area, 900.0) / 150.0
        mount_cands.append(
            {
                "face_idx": int(fi),
                "score": round(float(score), 6),
                "area": round(float(area), 6),
                "center": _vec(center),
                "normal": _vec(normal),
                "mount_prob": round(mount_p, 6),
                "outer_support_margin": round(float(outer_margin), 6),
                "role_hint": str(row.get("role", "doghouse")),
                "source": "ai_structure",
            }
        )
    if not mount_cands:
        # Fallback: any plane in area band ranked by outer margin only.
        for fi in sorted(scope):
            if _face_type(faces[fi]) != "plane":
                continue
            area = _surface_area(faces[fi])
            if not (mount_min_area <= area <= mount_max_area):
                continue
            center, normal = _plane_info(faces[fi])
            if center is None or normal is None:
                continue
            row = face_rows.get(int(fi), {})
            mount_p = float(row.get("mount_prob", 0.0) or 0.0)
            outer_margin = _outer_support_margin(faces, scope, center, normal)
            score = 10.0 * mount_p + max(0.0, 8.0 - float(outer_margin)) + min(area, 900.0) / 150.0
            mount_cands.append(
                {
                    "face_idx": int(fi),
                    "score": round(float(score), 6),
                    "area": round(float(area), 6),
                    "center": _vec(center),
                    "normal": _vec(normal),
                    "mount_prob": round(mount_p, 6),
                    "outer_support_margin": round(float(outer_margin), 6),
                    "role_hint": str(row.get("role", "doghouse")),
                    "source": "ai_structure",
                }
            )
    if not mount_cands:
        return None, [], [], 0.0

    mount_cands.sort(key=lambda c: (-c["score"], c["face_idx"]))
    best = dict(mount_cands[0])
    mount_idx = int(best["face_idx"])

    hole_seeds = []
    for fi in sorted(scope):
        if fi == mount_idx:
            continue
        row = face_rows.get(int(fi), {})
        hole_p = float(row.get("hole_wall_prob", 0.0) or 0.0)
        if hole_p < float(hole_score_threshold) and not int(row.get("hole_wall", 0) or 0):
            continue
        hole_seeds.append((int(fi), hole_p))

    reach = _scope_hops({mount_idx}, adj, scope, int(hole_hops))
    near_walls = sorted(
        fi for fi, _p in hole_seeds if fi in reach and fi != mount_idx
    )
    # Grow connected components among near walls (and allow 1-hop bridges in scope).
    wall_set = set(near_walls)
    if wall_set:
        # Include high-prob walls that touch the component even if hop filter missed.
        for fi, hole_p in hole_seeds:
            if fi in wall_set:
                continue
            if any(nb in wall_set or nb == mount_idx for nb in adj.get(fi, ())):
                if hole_p >= float(hole_score_threshold):
                    wall_set.add(fi)

    components: list[list[int]] = []
    seen: set[int] = set()
    for seed in sorted(wall_set):
        if seed in seen:
            continue
        q = deque([seed])
        seen.add(seed)
        comp = []
        while q:
            cur = q.popleft()
            comp.append(cur)
            for nb in adj.get(cur, ()):
                nb = int(nb)
                if nb in wall_set and nb not in seen:
                    seen.add(nb)
                    q.append(nb)
        components.append(sorted(comp))

    holes: list[dict[str, Any]] = []
    if components:
        # Prefer the component with highest mean hole_wall_prob.
        def comp_score(comp: list[int]) -> float:
            probs = [
                float(face_rows.get(fi, {}).get("hole_wall_prob", 0.0) or 0.0) for fi in comp
            ]
            return float(np.mean(probs)) if probs else 0.0

        components.sort(key=lambda c: (-comp_score(c), -len(c), c[0]))
        best_comp = components[0]
        mean_p = comp_score(best_comp)
        # Soft radius if any analytic wall exists; else null.
        radius = None
        for fi in best_comp:
            if _face_type(faces[fi]) in {"cylinder", "cone"}:
                r, _u, _v = _radius_u_v(faces[fi])
                if r is not None:
                    radius = float(r)
                    break
        center = _surface_center(faces[best_comp[0]])
        holes = [
            {
                "face_indices": best_comp,
                "radius": None if radius is None else round(float(radius), 6),
                "center": _vec(center) if center is not None else None,
                "axis": None,
                "u_sum": None,
                "source": "ai_structure",
                "mean_hole_wall_prob": round(float(mean_p), 6),
                "mount_face_idx": mount_idx,
            }
        ]
        best["near_hole_faces"] = best_comp
        best["near_hole_group_count"] = 1
        conf = 0.5 * float(best.get("mount_prob", 0.0)) + 0.5 * float(mean_p)
    else:
        best["near_hole_faces"] = []
        best["near_hole_group_count"] = 0
        conf = float(best.get("mount_prob", 0.0))

    return best, holes, mount_cands, float(conf)


def _vf2_mount_and_holes(
    shape,
    faces,
    adj: dict[int, set[int]],
    scope: set[int],
    role_by_face: dict[int, str],
    *,
    min_radius: float,
    max_radius: float,
    min_u_sum: float,
    oblique_min_u_sum: float,
    mount_min_area: float,
    mount_max_area: float,
    hole_hops: int,
    experimental_freeform_endpoint: bool = True,
    bottom_normal: np.ndarray | None = None,
) -> tuple[dict[str, Any] | None, list[dict[str, Any]], list[dict[str, Any]]]:
    """拓扑优先: 候选安装面驱动 VF2 孔识别 (含斜孔门限), 打分选最优安装面+孔.

    打破 `几何孔聚类 → 安装面 → VF2` 的死锁: 斜孔弧段不共轴、单段 u 很小,
    几何共轴聚类会失败; 这里改为枚举候选安装平面, 各自用拓扑子图匹配
    (rule_pillar.detect_holes_mcf_vf2, 按半径签名聚合 + 边约束) 找孔.
    检出门限放到斜孔水平 (oblique_min_u_sum), 由 VF2 拓扑校验拒伪.
    """
    from rule_pillar import detect_holes_mcf_vf2

    if not scope:
        return None, [], []

    # scope 内紧固件尺寸的解析孔壁。实验分支才额外加入 freeform 疑似孔壁。
    hole_faces = set()
    for fi in scope:
        face_type = _face_type(faces[fi])
        if face_type in {"cylinder", "cone"}:
            radius, _u, _v = _radius_u_v(faces[fi])
            if radius is not None and min_radius <= radius <= max_radius:
                hole_faces.add(int(fi))
        elif experimental_freeform_endpoint and _is_freeform_hole_seed(faces[fi]):
            hole_faces.add(int(fi))
    if not hole_faces:
        return None, [], []

    # 局部 AAG 只建一次 (seed 仅用于元数据, detect 按传入 seed_idx 搜索)
    seed_any = next(iter(scope))
    aag, _lm, local_to_orig = _build_local_vf2_aag(shape, faces, adj, scope, int(seed_any))
    orig_to_local = {orig: local for local, orig in local_to_orig.items()}

    detect_floor = float(min(min_u_sum, oblique_min_u_sum))
    plane_reach = _scope_hops(hole_faces, adj, scope, hole_hops + 1)
    seed_planes = [
        int(fi)
        for fi in plane_reach
        if _face_type(faces[int(fi)]) == "plane"
        and int(fi) in orig_to_local
    ]
    candidates: list[dict[str, Any]] = []
    for fi in sorted(seed_planes):
        area = _surface_area(faces[fi])
        if not (mount_min_area <= area <= mount_max_area):
            continue
        center, normal = _plane_info(faces[fi])
        if center is None or normal is None:
            continue
        groups = detect_holes_mcf_vf2(
            aag,
            orig_to_local[fi],
            min_radius=float(min_radius),
            min_u_sum=detect_floor,
        )
        mapped = _map_vf2_groups(
            groups,
            local_to_orig,
            faces,
            scope,
            adj,
            enrich_freeform_axis=experimental_freeform_endpoint,
        )
        # detect_holes_mcf_vf2 无半径上限, 剔除超过紧固件规格的大圆柱 (非孔特征).
        mapped = [g for g in mapped if float(g.get("radius", 0.0)) <= max_radius + 1e-6]
        if not mapped:
            continue
        outer_margin = _outer_support_margin(faces, scope, center, normal)
        mapped = [
            g for g in mapped
            if _hole_contour_inside_mount_face(g, faces, faces[fi], center, normal)
        ]
        if not mapped:
            continue
        # Prefer analytic cylinder/cone holes (incl. drafted cones) over freeform.
        analytic_mapped = [
            g
            for g in mapped
            if _hole_group_has_analytic_radius(
                g, faces, min_radius=min_radius, max_radius=max_radius
            )
        ]
        if analytic_mapped:
            mapped = analytic_mapped
        # Prefer holes whose edge sits in the central UV bbox; if none qualify,
        # keep contour-valid holes (0981 analytic arcs can sit off-center).
        centered = []
        best_centrality = 0.0
        for g in mapped:
            ok, centrality = _hole_edge_in_mount_bbox_center(
                g, faces, faces[fi], center, normal, center_ratio=0.60
            )
            best_centrality = max(best_centrality, centrality)
            if ok:
                centered.append(g)
        if centered:
            mapped = centered
        if experimental_freeform_endpoint:
            mapped = _select_single_mounting_hole_for_mount(mapped, faces, scope, adj, int(fi))
        else:
            mapped = _select_single_mounting_hole(mapped, faces)
        scope_align = 0.0
        face_nbrs = adj.get(fi, set())
        rims_hole = any(
            int(hf) in face_nbrs
            for g in mapped
            for hf in g.get("face_indices", [])
        )
        coaxial_through = _coaxial_through_hole(center, normal, mapped)
        if experimental_freeform_endpoint:
            parallel_infos = [
                _parallel_plane_link_info(faces, scope, adj, int(fi), g)
                for g in mapped
            ]
            parallel_endpoint = any(info.get("endpoint") for info in parallel_infos)
            parallel_count = max((int(info.get("parallel_count", 0)) for info in parallel_infos), default=0)
        else:
            parallel_endpoint = False
            parallel_count = 0
        # Coaxial bonus only when the hole mouth is reasonably centered on the
        # mount face; otherwise nearby cylinders can steal the mount (0981/712).
        if coaxial_through and best_centrality >= 0.35 and not rims_hole:
            coaxial_bonus = 14.0
        elif coaxial_through and best_centrality >= 0.35:
            coaxial_bonus = 6.0
        elif coaxial_through:
            coaxial_bonus = 2.0
        else:
            coaxial_bonus = 0.0
        semantic_bonus = 12.0 if role_by_face.get(fi) == "mount" else 0.0
        axis_direction = _mount_axis_direction_score(normal, mapped)
        axis_abs = _mount_axis_abs_score(normal, mapped)
        axis_direction_bonus = 3.0 * axis_direction
        parallel_bonus = 10.0 if experimental_freeform_endpoint and parallel_endpoint else 0.0
        outer_endpoint_bonus = (
            max(0.0, 5.0 - float(outer_margin)) * 2.0
            if experimental_freeform_endpoint and parallel_endpoint and coaxial_through
            else 0.0
        )
        if experimental_freeform_endpoint and bottom_normal is not None:
            bottom_alignment = float(np.dot(normal, bottom_normal))
        else:
            bottom_alignment = 0.0
        bottom_alignment_bonus = 6.0 * bottom_alignment
        local_ref = (
            _local_parallel_reference_info(faces, scope, int(fi), center, normal)
            if experimental_freeform_endpoint
            else {}
        )
        local_ref_distance = float(local_ref.get("distance", 0.0))
        local_ref_alignment = float(local_ref.get("parallel_alignment", 0.0))
        local_ref_bonus = (
            min(local_ref_distance, 20.0) / 2.0
            + max(0.0, 6.0 - float(outer_margin)) * 1.5
            if local_ref
            else 0.0
        )
        hole_center_bonus = 15.0 * float(best_centrality)
        # Prefer analytic cylinder/cone groups when selecting the hole for a
        # candidate, but do not add a large mount-ranking bonus: nearby
        # non-mounting cylinders (0981) can otherwise beat the true mount that
        # only has extrusion/freeform walls.
        small_plane_neighbors = sum(
            1
            for nb in adj.get(fi, ())
            if nb in scope and nb != fi and _face_type(faces[nb]) == "plane"
        )
        score = (
            semantic_bonus
            + coaxial_bonus
            + axis_direction_bonus
            + parallel_bonus
            + outer_endpoint_bonus
            + bottom_alignment_bonus
            + local_ref_bonus
            + hole_center_bonus
            + 5.0 * len(mapped)
            + min(area, 900.0) / 120.0
            + min(small_plane_neighbors, 4) * 1.2
        )
        candidates.append(
            {
                "face_idx": int(fi),
                "score": round(float(score), 6),
                "area": round(float(area), 6),
                "center": _vec(center),
                "normal": _vec(normal),
                "near_hole_faces": sorted(int(hf) for g in mapped for hf in g.get("face_indices", [])),
                "near_hole_group_count": len(mapped),
                "rims_hole": bool(rims_hole),
                "coaxial_through_hole": bool(coaxial_through),
                "has_analytic_hole": bool(analytic_mapped),
                "axis_direction_score": round(float(axis_direction), 6),
                "axis_abs_score": round(float(axis_abs), 6),
                "parallel_plane_link": bool(parallel_endpoint),
                "parallel_plane_count": int(parallel_count),
                "outer_endpoint_bonus": round(float(outer_endpoint_bonus), 6),
                "bottom_alignment_score": round(float(bottom_alignment), 6),
                "bottom_alignment_bonus": round(float(bottom_alignment_bonus), 6),
                "hole_bbox_centrality": round(float(best_centrality), 6),
                "hole_center_bonus": round(float(hole_center_bonus), 6),
                "local_reference_face_idx": int(local_ref["face_idx"]) if local_ref else None,
                "local_reference_parallel_score": round(float(local_ref_alignment), 6),
                "local_reference_distance": round(float(local_ref_distance), 6),
                "local_reference_bonus": round(float(local_ref_bonus), 6),
                "outer_support_margin": round(float(outer_margin), 6),
                "role_hint": role_by_face.get(fi, "background"),
                "_holes": mapped,
            }
        )

    if not candidates:
        return None, [], []

    candidates.sort(key=lambda c: (-c["score"], c["face_idx"]))
    best = candidates[0]
    holes = [dict(h, mount_face_idx=int(best["face_idx"])) for h in best.pop("_holes")]
    mount_rows = []
    for cand in candidates:
        row = dict(cand)
        row.pop("_holes", None)
        mount_rows.append(row)
    return best, holes, mount_rows


def extract_assembly_features(
    step_path: str | Path,
    prediction: dict,
    *,
    use_vf2: bool = False,
    vf2_required: bool = False,
    min_radius: float = 2.0,
    max_radius: float = 6.0,
    radius_tolerance: float = 0.5,
    min_u_sum: float = math.pi,
    oblique_min_u_sum: float = math.pi / 2,
    min_v_depth: float = 0.3,
    mount_min_area: float = 35.0,
    mount_max_area: float = 2000.0,
    hole_hops: int = 2,
    cluster_dist: float = 20.0,
    experimental_freeform_endpoint: bool = True,
    prefer_ai_structure_fallback: bool = True,
    ai_mount_score_threshold: float = 0.35,
    ai_hole_score_threshold: float = 0.35,
    # Backward-compatible aliases from older AI-hole wiring.
    prefer_ai_holes: bool | None = None,
    ai_hole_min_confidence: float = 0.35,
) -> dict:
    """Extract assembly-oriented features inside predicted doghouse components.

    Routing: VF2/analytic round holes win when reliable; otherwise AI structure
    roles (mount_prob + hole_wall_prob) select mount and through-hole walls
    without requiring parseable radius.
    """
    if prefer_ai_holes is not None:
        prefer_ai_structure_fallback = bool(prefer_ai_holes)
    shape, faces, adj = _load_step_and_adjacency(step_path)
    bottom_normal = _dominant_plane_normal(faces) if experimental_freeform_endpoint else None
    role_by_face = {
        int(row["face_idx"]): str(row.get("role", "background"))
        for row in prediction.get("face_predictions", [])
    }

    instances = []
    for inst in prediction.get("doghouse_instances", []):
        iid = int(inst.get("instance_id", len(instances) + 1))
        scope = {int(fi) for fi in inst.get("faces", []) if 0 <= int(fi) < len(faces)}
        if not scope:
            instances.append(
                {
                    "instance_id": iid,
                    "status": "empty_scope",
                    "scope_faces": [],
                    "mount_face": None,
                    "hole_groups": [],
                }
            )
            continue

        mount = None
        holes = []
        mount_rows = []
        hole_method = "local_brep_cluster"
        used_reliable_vf2 = False

        # 拓扑优先: 候选安装面驱动 VF2 (含斜孔), 破解几何聚类死锁.
        if use_vf2:
            topo_mount, topo_holes, topo_rows = _vf2_mount_and_holes(
                shape,
                faces,
                adj,
                scope,
                role_by_face,
                min_radius=min_radius,
                max_radius=max_radius,
                min_u_sum=min_u_sum,
                oblique_min_u_sum=oblique_min_u_sum,
                mount_min_area=mount_min_area,
                mount_max_area=mount_max_area,
                hole_hops=hole_hops,
                experimental_freeform_endpoint=experimental_freeform_endpoint,
                bottom_normal=bottom_normal,
            )
            if (
                topo_mount is not None
                and _vf2_has_reliable_analytic_hole(
                    topo_holes,
                    faces,
                    min_radius=min_radius,
                    max_radius=max_radius,
                )
                and _vf2_supported_by_ai_structure(
                    topo_mount,
                    topo_holes,
                    prediction,
                    mount_score_threshold=ai_mount_score_threshold,
                    hole_score_threshold=ai_hole_score_threshold,
                )
            ):
                mount, holes, mount_rows = topo_mount, topo_holes, topo_rows
                hole_method = "vf2_topo"
                used_reliable_vf2 = True
            elif topo_mount is not None and not prefer_ai_structure_fallback:
                # Legacy: keep VF2 even with freeform-only holes when AI fallback off.
                mount, holes, mount_rows = topo_mount, topo_holes, topo_rows
                hole_method = "vf2_topo"
            # else: VF2 returned freeform-only / unreliable holes → leave mount
            # unset so AI structure fallback can run.

        # AI structure fallback when analytic round holes are unavailable.
        if mount is None and prefer_ai_structure_fallback:
            ai_mount, ai_holes, ai_rows, ai_conf = _ai_structure_mount_and_holes(
                faces,
                adj,
                scope,
                prediction,
                mount_min_area=mount_min_area,
                mount_max_area=mount_max_area,
                hole_hops=hole_hops,
                mount_score_threshold=ai_mount_score_threshold,
                hole_score_threshold=ai_hole_score_threshold,
            )
            if ai_mount is not None and float(ai_conf) >= float(ai_hole_min_confidence):
                mount, holes, mount_rows = ai_mount, ai_holes, ai_rows
                hole_method = "ai_structure"

        # 几何法兜底 (未启用 VF2, 或拓扑/AI 未命中).
        if mount is None:
            geo_candidates = _collect_cylinder_candidates(
                faces,
                scope,
                min_radius=min_radius,
                max_radius=max_radius,
                min_v_depth=min_v_depth,
            )
            geometry_holes = _cluster_hole_candidates(
                geo_candidates,
                cluster_dist=cluster_dist,
                radius_tolerance=radius_tolerance,
                min_u_sum=min_u_sum,
            )
            mount_rows = _mount_candidates(
                faces,
                scope,
                adj,
                geometry_holes,
                role_by_face,
                min_area=mount_min_area,
                max_area=mount_max_area,
                hole_hops=hole_hops,
                experimental_freeform_endpoint=experimental_freeform_endpoint,
            )
            mount = mount_rows[0] if mount_rows else None
            holes = geometry_holes
            hole_method = "local_brep_cluster"
            if mount is not None and use_vf2:
                vf2_holes = _vf2_holes_for_mount(
                    step_path,
                    shape,
                    faces,
                    adj,
                    int(mount["face_idx"]),
                    scope,
                    min_radius=min_radius,
                    min_u_sum=min_u_sum,
                )
                if vf2_holes or vf2_required:
                    holes = vf2_holes
                    hole_method = "vf2"
            if mount is not None:
                near = set(int(x) for x in mount.get("near_hole_faces", []))
                mount_center = np.asarray(mount.get("center", []), dtype=float)
                mount_normal = np.asarray(mount.get("normal", []), dtype=float)
                holes = [
                    dict(g, mount_face_idx=int(mount["face_idx"]))
                    for g in holes
                    if not near or near & {int(x) for x in g.get("face_indices", [])}
                    if mount_center.size == 3
                    and mount_normal.size == 3
                    and _hole_contour_inside_mount_face(
                        g, faces, faces[int(mount["face_idx"])], mount_center, mount_normal
                    )
                ]
                holes = _select_single_mounting_hole(holes, faces)
                if not holes:
                    holes = _geometry_holes_for_mount_face(
                        faces,
                        scope,
                        adj,
                        mount,
                        min_radius=min_radius,
                        max_radius=max_radius,
                        min_v_depth=min_v_depth,
                        cluster_dist=cluster_dist,
                        radius_tolerance=radius_tolerance,
                        min_u_sum=min_u_sum,
                        hole_hops=hole_hops,
                    )

        if mount is None:
            status = "no_mount"
        elif holes:
            status = "ok"
        elif hole_method == "ai_structure":
            status = "mount_only"
        else:
            status = "no_hole"
        instances.append(
            {
                "instance_id": iid,
                "status": status,
                "scope_faces": sorted(scope),
                "mount_face": mount,
                "mount_candidates": mount_rows[:5],
                "hole_groups": holes,
                "hole_method": hole_method,
                "vf2_reliable_analytic": bool(used_reliable_vf2),
            }
        )

    return {
        "schema": "doghouse_assembly_features.v1",
        "source_step": str(step_path),
        "source_prediction_schema": prediction.get("schema"),
        "method": {
            "doghouse_scope": "ai_prediction",
            "mount_face": "vf2_then_ai_structure",
            "hole": "vf2_analytic_or_ai_structure" if use_vf2 else "local_brep_or_ai_structure",
            "vf2_required": bool(vf2_required),
            "prefer_ai_structure_fallback": bool(prefer_ai_structure_fallback),
            "vf2_mount_priority": True,
            "experimental_freeform_endpoint": bool(experimental_freeform_endpoint),
        },
        "instances": instances,
    }


def export_assembly_colored_step(
    step_path: str | Path,
    assembly_features: dict,
    output_step: str | Path,
) -> None:
    """Export colored STEP for visual inspection of mount and hole faces."""
    from OCC.Core.Interface import Interface_Static
    from OCC.Core.Quantity import Quantity_Color, Quantity_TOC_RGB
    from OCC.Core.STEPCAFControl import STEPCAFControl_Writer
    from OCC.Core.STEPControl import STEPControl_AsIs
    from OCC.Core.TDocStd import TDocStd_Document
    from OCC.Core.XCAFDoc import XCAFDoc_ColorSurf, XCAFDoc_DocumentTool

    doghouse_faces: set[int] = set()
    mount_faces: set[int] = set()
    hole_faces: set[int] = set()
    for inst in assembly_features.get("instances", []):
        doghouse_faces.update(int(fi) for fi in inst.get("scope_faces", []))
        mount = inst.get("mount_face") or {}
        if mount.get("face_idx") is not None:
            mount_faces.add(int(mount["face_idx"]))
        for group in inst.get("hole_groups", []):
            hole_faces.update(int(fi) for fi in group.get("face_indices", []))

    shape, faces = _load_step_faces_for_color(step_path)
    doc = TDocStd_Document("XCAF")
    label = doc.Main()
    shape_tool = XCAFDoc_DocumentTool.ShapeTool(label)
    color_tool = XCAFDoc_DocumentTool.ColorTool(label)
    shape_tool.AddShape(shape)

    gray = Quantity_Color(0.72, 0.72, 0.72, Quantity_TOC_RGB)
    doghouse = Quantity_Color(1.0, 0.55, 0.55, Quantity_TOC_RGB)
    mount = Quantity_Color(0.0, 0.85, 0.15, Quantity_TOC_RGB)
    hole = Quantity_Color(0.0, 0.35, 1.0, Quantity_TOC_RGB)

    for idx, face in enumerate(faces):
        color = gray
        if idx in doghouse_faces:
            color = doghouse
        if idx in hole_faces:
            color = hole
        if idx in mount_faces:
            color = mount
        color_tool.SetColor(face, color, XCAFDoc_ColorSurf)

    output_step = Path(output_step)
    output_step.parent.mkdir(parents=True, exist_ok=True)
    Interface_Static.SetCVal("write.step.unit", "MM")
    Interface_Static.SetCVal("write.step.schema", "AP214CD")
    writer = STEPCAFControl_Writer()
    writer.Transfer(doc, STEPControl_AsIs)
    writer.Write(str(output_step))

    print(f"colored_step: {output_step}")
    print(f"mount_faces: {sorted(mount_faces)}")
    print(f"hole_faces: {sorted(hole_faces)}")


def _load_step_faces_for_color(step_path: str | Path):
    from OCC.Core.STEPControl import STEPControl_Reader
    from OCC.Core.TopAbs import TopAbs_FACE
    from OCC.Core.TopExp import TopExp_Explorer

    reader = STEPControl_Reader()
    if reader.ReadFile(str(step_path)) != 1:
        raise RuntimeError(f"failed to read STEP: {step_path}")
    reader.TransferRoots()
    shape = reader.OneShape()
    faces = []
    exp = TopExp_Explorer(shape, TopAbs_FACE)
    while exp.More():
        faces.append(exp.Current())
        exp.Next()
    return shape, faces


def _build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    parser.add_argument("--step", required=True)
    parser.add_argument("--prediction-json", required=True)
    parser.add_argument("--output", help="Optional JSON output for debugging or downstream use")
    parser.add_argument("--output-step", help="Optional colored STEP output for visual inspection")
    parser.add_argument("--use-vf2", action="store_true")
    parser.add_argument("--vf2-required", action="store_true")
    parser.add_argument("--min-radius", type=float, default=2.0)
    parser.add_argument("--max-radius", type=float, default=6.0)
    parser.add_argument("--radius-tolerance", type=float, default=0.5)
    parser.add_argument("--min-u-sum", type=float, default=math.pi)
    parser.add_argument("--hole-hops", type=int, default=2)
    parser.add_argument(
        "--experimental-freeform-endpoint",
        action=argparse.BooleanOptionalAction,
        default=True,
        help=(
            "Enable freeform oblique-hole endpoint topology rules "
            "(default: enabled; use --no-experimental-freeform-endpoint for legacy rules)."
        ),
    )
    return parser


def main() -> int:
    parser = _build_arg_parser()
    args = parser.parse_args()
    if not args.output and not args.output_step:
        raise ValueError("Either --output or --output-step is required")

    prediction = _load_json(args.prediction_json)
    result = extract_assembly_features(
        args.step,
        prediction,
        use_vf2=args.use_vf2,
        vf2_required=args.vf2_required,
        min_radius=args.min_radius,
        max_radius=args.max_radius,
        radius_tolerance=args.radius_tolerance,
        min_u_sum=args.min_u_sum,
        hole_hops=args.hole_hops,
        experimental_freeform_endpoint=args.experimental_freeform_endpoint,
    )
    if args.output:
        output = Path(args.output)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"saved: {output}")
    if args.output_step:
        export_assembly_colored_step(args.step, result, args.output_step)
    ok_count = sum(1 for inst in result["instances"] if inst["status"] == "ok")
    print(f"instances: {len(result['instances'])}, ok: {ok_count}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
