#!/usr/bin/env python3
"""Export doghouse inference geometry directly from STEP with pythonOCC."""

from __future__ import annotations

import math
from pathlib import Path


def _vec3(p):
    return [round(float(p.X()), 6), round(float(p.Y()), 6), round(float(p.Z()), 6)]


def _load_step(step_path: str | Path):
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


def _face_type_and_params(face):
    from OCC.Core.BRepAdaptor import BRepAdaptor_Surface
    from OCC.Core.GeomAbs import (
        GeomAbs_BezierSurface,
        GeomAbs_BSplineSurface,
        GeomAbs_Cone,
        GeomAbs_Cylinder,
        GeomAbs_Plane,
        GeomAbs_Sphere,
        GeomAbs_Torus,
    )

    adaptor = BRepAdaptor_Surface(face, True)
    stype = adaptor.GetType()
    face_type = "other"
    radius = 0.0
    semi_angle_deg = None

    if stype == GeomAbs_Plane:
        face_type = "plane"
    elif stype == GeomAbs_Cylinder:
        face_type = "cylinder"
        radius = adaptor.Cylinder().Radius()
    elif stype == GeomAbs_Cone:
        face_type = "cone"
        cone = adaptor.Cone()
        radius = cone.RefRadius()
        semi_angle_deg = round(math.degrees(float(cone.SemiAngle())), 6)
    elif stype == GeomAbs_Sphere:
        face_type = "sphere"
        radius = adaptor.Sphere().Radius()
    elif stype == GeomAbs_Torus:
        face_type = "torus"
        radius = adaptor.Torus().MajorRadius()
    elif stype == GeomAbs_BSplineSurface:
        face_type = "bspline"
    elif stype == GeomAbs_BezierSurface:
        face_type = "bezier"

    u_range = abs(float(adaptor.LastUParameter()) - float(adaptor.FirstUParameter()))
    v_range = abs(float(adaptor.LastVParameter()) - float(adaptor.FirstVParameter()))
    return face_type, float(radius), semi_angle_deg, u_range, v_range


def _surface_props(face):
    from OCC.Core.BRepGProp import brepgprop
    from OCC.Core.GProp import GProp_GProps

    props = GProp_GProps()
    brepgprop.SurfaceProperties(face, props)
    return float(props.Mass()), props.CentreOfMass()


def _bbox(face):
    from OCC.Core.Bnd import Bnd_Box
    from OCC.Core.BRepBndLib import brepbndlib

    box = Bnd_Box()
    brepbndlib.Add(face, box)
    xmin, ymin, zmin, xmax, ymax, zmax = box.Get()
    return {
        "min": [round(float(xmin), 6), round(float(ymin), 6), round(float(zmin), 6)],
        "max": [round(float(xmax), 6), round(float(ymax), 6), round(float(zmax), 6)],
    }


def _face_normal(face, centroid):
    try:
        from OCC.Core.BRep import BRep_Tool
        from OCC.Core.GeomLProp import GeomLProp_SLProps
        from OCC.Core.ShapeAnalysis import ShapeAnalysis_Surface
        from OCC.Core.TopAbs import TopAbs_REVERSED

        surf = BRep_Tool.Surface(face)
        uv = ShapeAnalysis_Surface(surf).ValueOfUV(centroid, 0.01)
        props = GeomLProp_SLProps(surf, uv.X(), uv.Y(), 1, 1e-6)
        if not props.IsNormalDefined():
            return [0.0, 0.0, 0.0]
        n = props.Normal()
        if face.Orientation() == TopAbs_REVERSED:
            n.Reverse()
        return [round(float(n.X()), 6), round(float(n.Y()), 6), round(float(n.Z()), 6)]
    except Exception:
        return [0.0, 0.0, 0.0]


def _sample_face_points(face, max_points: int) -> list[list[float]]:
    if max_points <= 0:
        return []
    try:
        from OCC.Core.BRepAdaptor import BRepAdaptor_Surface

        adaptor = BRepAdaptor_Surface(face, True)
        u0, u1 = float(adaptor.FirstUParameter()), float(adaptor.LastUParameter())
        v0, v1 = float(adaptor.FirstVParameter()), float(adaptor.LastVParameter())
        if not all(math.isfinite(x) for x in (u0, u1, v0, v1)):
            return []
        side = max(1, int(math.ceil(math.sqrt(max_points))))
        pts = []
        for ui in range(side):
            u = u0 if side == 1 else u0 + (u1 - u0) * ui / (side - 1)
            for vi in range(side):
                v = v0 if side == 1 else v0 + (v1 - v0) * vi / (side - 1)
                p = adaptor.Value(u, v)
                pts.append(_vec3(p))
                if len(pts) >= max_points:
                    return pts
        return pts
    except Exception:
        return []


def _edge_type(edge):
    try:
        curve = edge.Curve
        text = (str(curve) + " " + str(getattr(curve, "TypeId", ""))).lower()
        for name in ("line", "circle", "ellipse", "bspline"):
            if name in text:
                return name
    except Exception:
        pass
    return "other"


def _build_adjacency(shape, faces):
    from collections import defaultdict
    from OCC.Core.TopAbs import TopAbs_EDGE
    from OCC.Core.TopExp import TopExp_Explorer
    from OCC.Extend.TopologyUtils import TopologyExplorer

    topo = TopologyExplorer(shape)
    face_by_hash = {hash(face): idx for idx, face in enumerate(faces)}
    edge_pairs = {}
    exp = TopExp_Explorer(shape, TopAbs_EDGE)
    while exp.More():
        edge = exp.Current()
        incident = list(topo.faces_from_edge(edge))
        if len(incident) == 2:
            a = face_by_hash.get(hash(incident[0]))
            b = face_by_hash.get(hash(incident[1]))
            if a is not None and b is not None and a != b:
                key = tuple(sorted((int(a), int(b))))
                edge_pairs.setdefault(key, _edge_type(edge))
        exp.Next()
    return [
        {"a": a, "b": b, "edge_type": edge_type}
        for (a, b), edge_type in sorted(edge_pairs.items())
    ]


def build_geometry_from_step(
    step_path: str | Path,
    *,
    sample_points_per_face: int = 64,
) -> dict:
    """Return doghouse_inference_geometry.v1 from a STEP file."""
    step_path = Path(step_path)
    shape, faces = _load_step(step_path)

    rows = []
    for idx, face in enumerate(faces):
        area, centroid = _surface_props(face)
        face_type, radius, semi_angle_deg, u_range, v_range = _face_type_and_params(face)
        row = {
            "face_idx": idx,
            "face_name": f"Face{idx + 1}",
            "face_type": face_type,
            "area": round(float(area), 6),
            "centroid": _vec3(centroid),
            "bbox": _bbox(face),
            "normal": _face_normal(face, centroid),
            "radius": round(float(radius), 6),
            "semi_angle_deg": semi_angle_deg,
            "has_radius": 1 if radius > 0 else 0,
            "u_range": round(float(u_range), 6),
            "v_range": round(float(v_range), 6),
        }
        samples = _sample_face_points(face, sample_points_per_face)
        if samples:
            row["sample_points"] = samples
        rows.append(row)

    return {
        "schema": "doghouse_inference_geometry.v1",
        "source": str(step_path),
        "num_faces": len(rows),
        "index_base": 0,
        "sample_points_per_face": int(sample_points_per_face),
        "faces": rows,
        "adjacency_edges": _build_adjacency(shape, faces),
    }
