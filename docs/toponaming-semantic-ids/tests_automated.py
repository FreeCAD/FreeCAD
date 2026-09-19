# SPDX-License-Identifier: LGPL-2.1-or-later
"""Tests Automated — first-bar gate (in-script scoring).

Automated is the gate. Manual GUI is only a sanity check after this passes.

Run with FreeCADCmd (Windows pixi):

    pixi run --skip-deps -- .pixi/envs/default/Library/bin/FreeCADCmd.exe docs/toponaming-semantic-ids/tests_automated.py

Writes:
  %USERPROFILE%\\Desktop\\Tests_Automated_preSplit.FCStd
  %USERPROFILE%\\Desktop\\Tests_Automated.FCStd

Override output dir with env TESTS_OUT.

Required cases (exit 1 if any fail): 103 (101 prior cases + body_tip_seed + body_tip_feature_scope).
All Subtractive primitive isolates use the shared with-base Cut publisher. Keep pick_ellipsoid_unique_edge BSpline equator.
Do not change pick_loop3_edge, pd_boolean Fuse/Cut/Common construction, additive_cylinder (_is_closed_cylinder_seam), additive_sphere (_is_closed_sphere_seam), additive_cone (_is_closed_cone_seam), additive_box, additive_torus (_is_closed_torus_seam), additive_prism, additive_wedge, or Loop 3 construction.
  pocket_tip_seed          Fillet.Base on Pocket has stSeed
  pocket_feature_scope     LinkSub value is Pocket; STG1 identity.allocatedBy is Pocket id
  pocket_seed_stable       same stSeed hex after sketch split (EdgeN/shadow may jump)
  pad_u_outline_seed       Fillet.Base on Pad bottom outline (:U class) has stSeed
  pad_u_c1_keep            Python Base = (Pad, [EdgeN]) keeps that same seed
  pad_vertical_seed        Fillet.Base on a vertical Pad corner (Fillet002-class) has stSeed
  sketch_vertex_seed       SketchPadVert has STG1 Vertex-kind identities (rectangle → at least 4)
  attachment_support_seed  Sketch001 AttachmentSupport (Pad face) has stSeed
  chamfer_base_seed        Chamfer.Base on a uniquely-named Pad edge (NOT on a Fillet) has stSeed
  chamfer_feature_scope    that Chamfer Base seed allocatedBy is the linked Pad id (not leftover)
  draft_base_seed          Draft.Base on a uniquely-named Pad face (NOT on a Fillet) has stSeed
  draft_feature_scope      that Draft Base seed allocatedBy is the linked Pad id (not leftover)
  thickness_base_seed      Thickness.Base on a uniquely-named Pad cap face (NOT on a Fillet) has stSeed
  thickness_feature_scope  that Thickness Base seed allocatedBy is the linked Pad id (not leftover)
  revolution_base_seed     Fillet.Base on a uniquely-named Revolution edge has stSeed
  revolution_feature_scope that seed allocatedBy is the linked Revolution id (not leftover)
  hole_start_seed          Hole.StartReference on a uniquely-named Pad cap Face (NOT on a Fillet) has stSeed
  hole_feature_scope       that seed allocatedBy is the linked Pad id (not leftover)
  part_fillet_seed         Part::Fillet EdgeLinks on a uniquely-named Pad edge (NOT PartDesign::Fillet) has stSeed
  part_fillet_feature_scope that seed allocatedBy is the linked Pad id (not leftover)
  part_boolean_seed        Part::Fillet EdgeLinks on a uniquely-named Fuse edge (Part::Fuse of two Pads) has stSeed
  part_boolean_feature_scope that seed allocatedBy is the Fuse id (not a Pad leftover)
  part_extrusion_seed        Part::Fillet EdgeLinks on a uniquely-named Part::Extrusion edge has stSeed
  part_extrusion_feature_scope that seed allocatedBy is the Part::Extrusion id (not a source leftover)
  part_revolution_seed       Part::Fillet EdgeLinks on a uniquely-named Part::Revolution edge has stSeed
  part_revolution_feature_scope that seed allocatedBy is the Part::Revolution id (not a source leftover)
  part_mirroring_seed        Part::Fillet EdgeLinks on a uniquely-named Part::Mirroring edge has stSeed
  part_mirroring_feature_scope that seed allocatedBy is the Part::Mirroring id (not a source leftover)
  xlink_sub_seed           App::PropertyXLinkSub on FeaturePython to a uniquely-named Pad cap Face has stSeed; allocatedBy is that Pad id
  spreadsheet_face_seed    Spreadsheet::Sheet A1 =PadSpread.Shape.FaceN.Area has Cell stSeed; allocatedBy is that Pad id
  draft_facebinder_seed    Facebinder.Faces on a uniquely-named Pad cap Face (NOT on a Fillet) has stSeed
  draft_facebinder_feature_scope  that seed allocatedBy is the linked Pad id
  techdraw_dim_seed        DrawViewDimension References3D on a uniquely-named Pad edge has stSeed
  techdraw_dim_feature_scope that seed allocatedBy is the linked Pad id
  techdraw_hatch_seed      DrawHatch Source on a uniquely-named Pad cap Face has stSeed
  techdraw_hatch_feature_scope that seed allocatedBy is the linked Pad id
  linear_pattern_seed      Fillet.Base on a uniquely-named LinearPattern edge (not a Pad leftover) has stSeed
  linear_pattern_feature_scope that seed allocatedBy is the LinearPattern id
  polar_pattern_seed       Fillet.Base on a uniquely-named PolarPattern edge (not a Pad leftover) has stSeed
  polar_pattern_feature_scope that seed allocatedBy is the PolarPattern id
  mirrored_seed            Fillet.Base on a uniquely-named Mirrored edge (not a Pad leftover) has stSeed
  mirrored_feature_scope   that seed allocatedBy is the Mirrored id
  scaled_seed              Fillet.Base on a uniquely-named Scaled edge (not a Pad leftover) has stSeed
  scaled_feature_scope     that seed allocatedBy is the Scaled id
  multi_transform_seed     Fillet.Base on a uniquely-named MultiTransform edge (not a Pad leftover) has stSeed
  multi_transform_feature_scope that seed allocatedBy is the MultiTransform id
  loft_seed                Fillet.Base on AdditiveLoft unique edge has stSeed
  loft_feature_scope       that seed allocatedBy is AdditiveLoft getID()
  pipe_seed                Fillet.Base on AdditivePipe unique edge has stSeed
  pipe_feature_scope       that seed allocatedBy is AdditivePipe getID()
  helix_seed               Fillet.Base on AdditiveHelix unique edge has stSeed
  helix_feature_scope      that seed allocatedBy is AdditiveHelix getID()
  pd_boolean_seed          Fillet.Base on a uniquely-named PartDesign Boolean Fuse edge has stSeed
  pd_boolean_feature_scope that seed allocatedBy is the Boolean feature id (not a Pad leftover)
  additive_box_seed        Fillet.Base on a uniquely-named AdditiveBox edge (first solid, no Pad) has stSeed
  additive_box_feature_scope that seed allocatedBy is the AdditiveBox feature id (not a leftover)
  additive_cylinder_seed   Fillet.Base on a uniquely-named AdditiveCylinder edge (first solid, no Pad) has stSeed
  additive_cylinder_feature_scope that seed allocatedBy is the AdditiveCylinder feature id (not a leftover)
  additive_sphere_seed     Fillet.Base on a uniquely-named AdditiveSphere edge (first solid, no Pad) has stSeed
  additive_sphere_feature_scope that seed allocatedBy is the AdditiveSphere feature id (not a leftover)
  additive_cone_seed       Fillet.Base on a uniquely-named AdditiveCone edge (first solid, no Pad) has stSeed
  additive_cone_feature_scope that seed allocatedBy is the AdditiveCone feature id (not a leftover)
  additive_torus_seed      Fillet.Base on a uniquely-named AdditiveTorus edge (first solid, no Pad) has stSeed
  additive_torus_feature_scope that seed allocatedBy is the AdditiveTorus feature id (not a leftover)
  additive_prism_seed      Fillet.Base on a uniquely-named AdditivePrism edge (first solid, no Pad) has stSeed
  additive_prism_feature_scope that seed allocatedBy is the AdditivePrism feature id (not a leftover)
  additive_wedge_seed      Fillet.Base on a uniquely-named AdditiveWedge edge (first solid, no Pad) has stSeed
  additive_wedge_feature_scope that seed allocatedBy is the AdditiveWedge feature id (not a leftover)
  additive_ellipsoid_seed  Fillet.Base on a uniquely-named AdditiveEllipsoid edge (first solid, no Pad) has stSeed
  additive_ellipsoid_feature_scope that seed allocatedBy is the AdditiveEllipsoid feature id (not a leftover)
  subtractive_box_seed     Fillet.Base on a uniquely-named SubtractiveBox edge (with-base Cut) has stSeed
  subtractive_box_feature_scope that seed allocatedBy is the SubtractiveBox feature id (not Pad/Additive leftover)
  subtractive_cylinder_seed Fillet.Base on a uniquely-named SubtractiveCylinder curved rim (with-base Cut) has stSeed
  subtractive_cylinder_feature_scope that seed allocatedBy is the SubtractiveCylinder feature id
  subtractive_sphere_seed   Fillet.Base on a uniquely-named SubtractiveSphere curved rim (with-base Cut) has stSeed
  subtractive_sphere_feature_scope that seed allocatedBy is the SubtractiveSphere feature id
  subtractive_cone_seed      Fillet.Base on a uniquely-named SubtractiveCone rim has stSeed
  subtractive_cone_feature_scope that seed allocatedBy is the SubtractiveCone feature id
  subtractive_torus_seed     Fillet.Base on a uniquely-named SubtractiveTorus rim has stSeed
  subtractive_torus_feature_scope that seed allocatedBy is the SubtractiveTorus feature id
  subtractive_prism_seed     Fillet.Base on a uniquely-named SubtractivePrism edge has stSeed
  subtractive_prism_feature_scope that seed allocatedBy is the SubtractivePrism feature id
  subtractive_wedge_seed     Fillet.Base on a uniquely-named SubtractiveWedge edge has stSeed
  subtractive_wedge_feature_scope that seed allocatedBy is the SubtractiveWedge feature id
  subtractive_ellipsoid_seed Fillet.Base on a uniquely-named SubtractiveEllipsoid rim has stSeed
  subtractive_ellipsoid_feature_scope that seed allocatedBy is the SubtractiveEllipsoid feature id
  subtractive_loft_seed      Fillet.Base on a uniquely-named SubtractiveLoft edge has stSeed
  subtractive_loft_feature_scope that seed allocatedBy is the SubtractiveLoft feature id
  subtractive_pipe_seed      Fillet.Base on a uniquely-named SubtractivePipe edge has stSeed
  subtractive_pipe_feature_scope that seed allocatedBy is the SubtractivePipe feature id
  subtractive_helix_seed     Fillet.Base on a uniquely-named SubtractiveHelix edge has stSeed
  subtractive_helix_feature_scope that seed allocatedBy is the SubtractiveHelix feature id
  elementmap_st_output     SubtractiveLoft Shape Map sidecar (or ElementMap) contains a semantic ;:ST token
  pd_boolean_cut_seed      Fillet.Base on a uniquely-named PartDesign Boolean Cut edge has stSeed
  pd_boolean_cut_feature_scope that seed allocatedBy is the Boolean Cut feature id (not a Pad leftover)
  pd_boolean_common_seed   Fillet.Base on a uniquely-named PartDesign Boolean Common edge has stSeed
  pd_boolean_common_feature_scope that seed allocatedBy is the Boolean Common feature id (not a Pad leftover)
  pocket_through_split     new-body ThroughAll Pocket has a Pocket Split STG1 event on that Pocket id
  pocket_s3_remnant        Length Pocket Modified (S3) event keeps the uniquely-named Pad cap handle
  assembly_joint_seed      JointObject.Reference1 (PropertyXLinkSub) on a uniquely-named Pad cap Face has stSeed
  assembly_joint_feature_scope that seed allocatedBy is the linked Pad id (not leftover)
  body_tip_seed            PropertyXLinkSub on Body (not Pad) FaceN cache has stSeed via Tip projection
  body_tip_feature_scope   that seed allocatedBy is the Tip Pad id (not the Body id)

False pass: solid valid but seedless Base while this script required a seed.

Do not open Test_3 / Test_4_Unnamed. This must be a new Uid.
Chamfer/Draft/Thickness on Pad/Pocket is required. Chamfer-on-Fillet / Fillet-on-Fillet / Draft-on-Fillet / Thickness-on-Fillet still not required.
Hole-on-Fillet may stay seedless (allowed, not required). Hole.Profile is a Sketch LinkSub listed first — score StartReference (Face), not Profile.
Do not score PartDesign Fillet as part_fillet_*. Part-Fillet-on-Fillet may stay seedless.
Do not score Pad leftover as part_boolean_*. Ambiguous multi-image Fuse faces stay seedless (allowed).
Do not score Pad leftover as pd_boolean_*. Refine=False.
Do not score a leftover as additive_box_*. Separate Body; first solid AdditiveBox; Refine=False.
Do not score a leftover as additive_cylinder_*. Separate Body; first solid AdditiveCylinder; Refine=False.
Do not score a leftover as additive_sphere_*. Separate Body; first solid AdditiveSphere; Refine=False; hemisphere rim pick (_is_closed_sphere_seam).
Do not score a leftover as additive_cone_*. Separate Body; first solid AdditiveCone; Refine=False; truncated cone outer rim pick (_is_closed_cone_seam).
Do not score a leftover as additive_torus_*. Separate Body; first solid AdditiveTorus; Refine=False; partial Angle3 tube end-circle pick (_is_closed_torus_seam).
Do not score a leftover as additive_prism_*. Separate Body; first solid AdditivePrism; Refine=False; polygonal vertical outer via pick_vertical_outer_edge (Box class).
Do not score a leftover as additive_wedge_*. Separate Body; first solid AdditiveWedge; Refine=False; non-degenerate wedge vertical outer via pick_vertical_outer_edge (Box/Prism class).
Do not score a leftover as additive_ellipsoid_*. Separate Body; first solid AdditiveEllipsoid; Refine=False; hemisphere rim pick (pick_ellipsoid_unique_edge / _is_closed_ellipsoid_seam).
Do not score Pad/Additive leftover as subtractive_box_*. Separate Body; AdditiveBox base then SubtractiveBox overlap; Refine=False; Fillet.Base on SubtractiveBox. Cylinder/Sphere picks must be curved rims, not seams.
Do not score Pad leftover as pd_boolean_cut_*. Separate bodies; Refine=False; tool overlaps base; Type=Cut.
Do not score Pad leftover as pd_boolean_common_*. Separate bodies; Refine=False; substantial Pad overlap; Type=Common.
"""
from __future__ import annotations

import os
import sys
import math
import traceback
import zipfile
import xml.etree.ElementTree as ET

import FreeCAD as App
import Part
import Sketcher


FAILURES = []
SCORED = set()
REQUIRED_CASES = frozenset(
    [
    'additive_box_feature_scope',
    'additive_box_seed',
    'additive_cone_feature_scope',
    'additive_cone_seed',
    'additive_cylinder_feature_scope',
    'additive_cylinder_seed',
    'additive_ellipsoid_feature_scope',
    'additive_ellipsoid_seed',
    'additive_prism_feature_scope',
    'additive_prism_seed',
    'additive_sphere_feature_scope',
    'additive_sphere_seed',
    'additive_torus_feature_scope',
    'additive_torus_seed',
    'additive_wedge_feature_scope',
    'additive_wedge_seed',
    'assembly_joint_feature_scope',
    'assembly_joint_seed',
    'attachment_support_seed',
    'body_tip_feature_scope',
    'body_tip_seed',
    'chamfer_base_seed',
    'chamfer_feature_scope',
    'draft_base_seed',
    'draft_facebinder_feature_scope',
    'draft_facebinder_seed',
    'draft_feature_scope',
    'elementmap_st_output',
    'helix_feature_scope',
    'helix_seed',
    'hole_feature_scope',
    'hole_start_seed',
    'linear_pattern_feature_scope',
    'linear_pattern_seed',
    'loft_feature_scope',
    'loft_seed',
    'mirrored_feature_scope',
    'mirrored_seed',
    'multi_transform_feature_scope',
    'multi_transform_seed',
    'pad_u_c1_keep',
    'pad_u_outline_seed',
    'pad_vertical_seed',
    'part_boolean_feature_scope',
    'part_boolean_seed',
    'part_extrusion_feature_scope',
    'part_extrusion_seed',
    'part_fillet_feature_scope',
    'part_fillet_seed',
    'part_mirroring_feature_scope',
    'part_mirroring_seed',
    'part_revolution_feature_scope',
    'part_revolution_seed',
    'pd_boolean_common_feature_scope',
    'pd_boolean_common_seed',
    'pd_boolean_cut_feature_scope',
    'pd_boolean_cut_seed',
    'pd_boolean_feature_scope',
    'pd_boolean_seed',
    'pipe_feature_scope',
    'pipe_seed',
    'pocket_feature_scope',
    'pocket_s3_remnant',
    'pocket_seed_stable',
    'pocket_through_split',
    'pocket_tip_seed',
    'polar_pattern_feature_scope',
    'polar_pattern_seed',
    'revolution_base_seed',
    'revolution_feature_scope',
    'scaled_feature_scope',
    'scaled_seed',
    'sketch_vertex_seed',
    'spreadsheet_face_seed',
    'subtractive_box_feature_scope',
    'subtractive_box_seed',
    'subtractive_cone_feature_scope',
    'subtractive_cone_seed',
    'subtractive_cylinder_feature_scope',
    'subtractive_cylinder_seed',
    'subtractive_ellipsoid_feature_scope',
    'subtractive_ellipsoid_seed',
    'subtractive_helix_feature_scope',
    'subtractive_helix_seed',
    'subtractive_loft_feature_scope',
    'subtractive_loft_seed',
    'subtractive_pipe_feature_scope',
    'subtractive_pipe_seed',
    'subtractive_prism_feature_scope',
    'subtractive_prism_seed',
    'subtractive_sphere_feature_scope',
    'subtractive_sphere_seed',
    'subtractive_torus_feature_scope',
    'subtractive_torus_seed',
    'subtractive_wedge_feature_scope',
    'subtractive_wedge_seed',
    'techdraw_dim_feature_scope',
    'techdraw_dim_seed',
    'techdraw_hatch_feature_scope',
    'techdraw_hatch_seed',
    'thickness_base_seed',
    'thickness_feature_scope',
    'xlink_sub_seed',
    ]
)



def desktop_dir() -> str:
    env = os.environ.get("TESTS_OUT") or os.environ.get("TEST5_OUT")
    if env:
        return env
    return os.path.join(os.environ.get("USERPROFILE", os.path.expanduser("~")), "Desktop")


def add_rectangle(sketch, corner, lengths):
    hmin, hmax = corner[0], corner[0] + lengths[0]
    vmin, vmax = corner[1], corner[1] + lengths[1]
    i = int(sketch.GeometryCount)
    sketch.addGeometry(Part.LineSegment(App.Vector(hmin, vmax), App.Vector(hmax, vmax, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(hmax, vmax, 0), App.Vector(hmax, vmin, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(hmax, vmin, 0), App.Vector(hmin, vmin, 0)))
    sketch.addGeometry(Part.LineSegment(App.Vector(hmin, vmin, 0), App.Vector(hmin, vmax, 0)))
    sketch.addConstraint(Sketcher.Constraint("Coincident", i + 0, 2, i + 1, 1))
    sketch.addConstraint(Sketcher.Constraint("Coincident", i + 1, 2, i + 2, 1))
    sketch.addConstraint(Sketcher.Constraint("Coincident", i + 2, 2, i + 3, 1))
    sketch.addConstraint(Sketcher.Constraint("Coincident", i + 3, 2, i + 0, 1))
    sketch.addConstraint(Sketcher.Constraint("Horizontal", i + 0))
    sketch.addConstraint(Sketcher.Constraint("Horizontal", i + 2))
    sketch.addConstraint(Sketcher.Constraint("Vertical", i + 1))
    sketch.addConstraint(Sketcher.Constraint("Vertical", i + 3))
    sketch.addConstraint(Sketcher.Constraint("DistanceX", i + 2, 2, corner[0]))
    sketch.addConstraint(Sketcher.Constraint("DistanceY", i + 2, 2, corner[1]))
    sketch.addConstraint(Sketcher.Constraint("Distance", i + 1, lengths[1]))
    sketch.addConstraint(Sketcher.Constraint("Distance", i + 0, lengths[0]))
    return i, i + 2


def add_circle(sketch, center, radius):
    i = int(sketch.GeometryCount)
    sketch.addGeometry(Part.Circle(App.Vector(center[0], center[1], 0), App.Vector(0, 0, 1), radius), False)
    sketch.addConstraint(Sketcher.Constraint("Radius", i, radius))
    sketch.addConstraint(Sketcher.Constraint("DistanceX", i, 3, center[0]))
    sketch.addConstraint(Sketcher.Constraint("DistanceY", i, 3, center[1]))
    return i


def face_name_max_z(shape) -> str:
    best_i, best_z = 1, None
    for i, face in enumerate(shape.Faces, start=1):
        z = face.CenterOfMass.z
        if best_z is None or z > best_z:
            best_z, best_i = z, i
    return "Face%d" % best_i


def _mapped_is_pad_xtr_leftover(mapped: str) -> bool:
    """Bare Pad :G;XTR leftover (no LPT/PPT/PMR/FUS token)."""
    if not mapped:
        return False
    if ":G;XTR" not in mapped:
        return False
    for token in (
        ";LPT", ";PPT", ";PMR", ";FUS", ";LFT", ";PIP", ";HLX", ";RVL", ";PBF", ";PBX", ";PCY",
        ":LPT", ":PPT", ":PMR", ":FUS", ":LFT", ":PIP", ":HLX", ":RVL", ":PBF", ":PBX", ":PCY",

    ):
        if token in mapped:
            return False
    return True


def pick_vertical_outer_edge(shape, pad_center_xy, skip_pad_xtr=False) -> str:
    """Farthest vertical edge from pad_center_xy (copy / mirror instance).

    skip_pad_xtr: Loop 2 — prefer an edge whose mapped name is not a bare Pad
    :G;XTR leftover so Fillet.Base is not a Pad name looked up on the pattern.
    """
    cx, cy = pad_center_xy
    candidates = []
    n = shape.countElement("Edge")
    for i in range(1, n + 1):
        name = "Edge%d" % i
        edge = shape.getElement(name)
        verts = edge.Vertexes
        if len(verts) < 2:
            continue
        p1, p2 = verts[0].Point, verts[1].Point
        if abs(p1.x - p2.x) > 1e-6 or abs(p1.y - p2.y) > 1e-6:
            continue
        if abs(p1.z - p2.z) < 1e-6:
            continue
        mx, my = 0.5 * (p1.x + p2.x), 0.5 * (p1.y + p2.y)
        r2 = (mx - cx) ** 2 + (my - cy) ** 2
        leftover = False
        if skip_pad_xtr:
            leftover = _mapped_is_pad_xtr_leftover(mapped_name_for_edge(shape, name))
        candidates.append((r2, name, leftover))
    if not candidates:
        raise RuntimeError("No vertical outer edge on %s" % shape)
    pool = candidates
    if skip_pad_xtr:
        non_left = [c for c in candidates if not c[2]]
        if non_left:
            pool = non_left
    best = max(pool, key=lambda c: c[0])
    return best[1]


def pick_outer_circular_edge(shape) -> str:
    """360° Revolution has circular edges, not Pad-style Z-parallel lines."""
    best = None
    best_r = -1.0
    n = shape.countElement("Edge")
    for i in range(1, n + 1):
        name = "Edge%d" % i
        edge = shape.getElement(name)
        try:
            curve = edge.Curve
            radius = float(abs(curve.Radius))
        except Exception:
            continue
        if radius > best_r:
            best_r = radius
            best = name
    if not best:
        raise RuntimeError("No circular edge on %s" % shape)
    return best


def pick_longest_edge(shape, skip_pad_xtr=False) -> str:
    best = None
    best_len = -1.0
    n = shape.countElement("Edge")
    for i in range(1, n + 1):
        name = "Edge%d" % i
        if skip_pad_xtr and _mapped_is_pad_xtr_leftover(mapped_name_for_edge(shape, name)):
            continue
        edge = shape.getElement(name)
        length = -1.0
        try:
            length = float(edge.Length)
        except Exception:
            verts = edge.Vertexes
            if len(verts) >= 2:
                length = verts[0].Point.distanceToPoint(verts[1].Point)
        if length > best_len:
            best_len = length
            best = name
    if not best:
        raise RuntimeError("No edge on %s" % shape)
    return best


def pick_loop3_edge(shape) -> str:
    # Loop 3: skip :G;XTR leftovers. Prefer a Z-parallel outer edge, else longest.
    try:
        com = shape.CenterOfMass
        return pick_vertical_outer_edge(shape, (com.x, com.y), skip_pad_xtr=True)
    except Exception:
        return pick_longest_edge(shape, skip_pad_xtr=True)


def _is_closed_cylinder_seam(edge) -> bool:
    """True for the vertical Line seam on a 360° MakeCylinder solid.

    That edge is Z-parallel and unique but not C0-continuous, so PartDesign
    Fillet rejects it. Partial-arc cylinders use vertical boundary edges that
    are fillet-safe; only skip the closed solid seam.
    """
    verts = edge.Vertexes
    if len(verts) != 2:
        return False
    try:
        if type(edge.Curve).__name__ != "Line":
            return False
    except Exception:
        return False
    p1, p2 = verts[0].Point, verts[1].Point
    if abs(p1.x - p2.x) > 1e-6 or abs(p1.y - p2.y) > 1e-6:
        return False
    if abs(p1.z - p2.z) < 1e-6:
        return False
    return True


def pick_cylinder_unique_edge(shape) -> str:
    """Fillet-safe unique edge on AdditiveCylinder (I13).

    Do not use pick_loop3_edge. A 360° cylinder has a vertical seam that is
    unique but not fillet-able; prefer the outer circular rim in that case.
    Partial-arc cylinders keep the farthest vertical boundary edge.
    """
    com = shape_center_of_mass(shape)
    try:
        vertical = pick_vertical_outer_edge(shape, (com.x, com.y))
        edge = shape.getElement(vertical)
        if _is_closed_cylinder_seam(edge):
            return pick_outer_circular_edge(shape)
        return vertical
    except RuntimeError:
        return pick_outer_circular_edge(shape)


def _is_closed_sphere_seam(edge) -> bool:
    """True for the meridian seam on a 360° MakeSphere solid of revolution.

    That edge can be unique but is not C0-continuous at the seam, so
    PartDesign Fillet rejects it (mirror of _is_closed_cylinder_seam).
    Prefer a unique horizontal circular rim (equator / parallel cut).
    Meridian: circular curve whose vertices differ in Z (or a single-vertex
    closed meridian through a pole). Horizontal rims keep constant Z.
    """
    try:
        if type(edge.Curve).__name__ != "Circle":
            return False
    except Exception:
        return False
    verts = edge.Vertexes
    if len(verts) == 0:
        return False
    if len(verts) == 1:
        return True
    zs = [v.Point.z for v in verts]
    return abs(max(zs) - min(zs)) > 1e-6


def pick_sphere_unique_edge(shape) -> str:
    """Fillet-safe unique edge on AdditiveSphere (I13).

    Do not use pick_loop3_edge. A 360° sphere of revolution has a meridian
    seam that may be unique but not fillet-able; prefer a unique horizontal
    circular rim (equator / parallel) — mirror of pick_cylinder_unique_edge.
    Construction uses a hemisphere (Angle2=0) so the equator is present.
    """
    best = None
    best_r = -1.0
    n = shape.countElement("Edge")
    for i in range(1, n + 1):
        name = "Edge%d" % i
        edge = shape.getElement(name)
        if _is_closed_sphere_seam(edge):
            continue
        try:
            curve = edge.Curve
            radius = float(abs(curve.Radius))
        except Exception:
            continue
        if radius > best_r:
            best_r = radius
            best = name
    if best:
        return best
    # Fallback: unique equator / largest circular even if seam filter empty.
    try:
        return pick_outer_circular_edge(shape)
    except RuntimeError as exc:
        raise RuntimeError("No unique Fillet-safe Sphere edge on %s" % shape) from exc


def pick_subtractive_cylinder_unique_edge(shape) -> str:
    """Pick a curved cylinder-cut rim with :M;CUT (stSeed path), not G3-only CUT."""
    best = None
    best_r = -1.0
    for i in range(1, shape.countElement("Edge") + 1):
        name = "Edge%d" % i
        edge = shape.getElement(name)
        mapped = mapped_name_for_edge(shape, name)
        if not mapped or ":M;CUT" not in mapped:
            continue
        if _is_closed_cylinder_seam(edge):
            continue
        try:
            curve = edge.Curve
            if type(curve).__name__ not in ("Circle", "Ellipse"):
                continue
            radius = float(abs(curve.Radius))
        except Exception:
            continue
        if radius > best_r:
            best_r, best = radius, name
    if best:
        return best
    raise RuntimeError("No :M;CUT curved cylinder-cut edge on %s" % shape)


def pick_subtractive_cut_named_edge(shape) -> str:
    """Pick a with-base Cut edge that carries :M;CUT (fillet + stSeed path)."""
    for i in range(1, shape.countElement("Edge") + 1):
        name = "Edge%d" % i
        mapped = mapped_name_for_edge(shape, name)
        if mapped and ":M;CUT" in mapped:
            return name
    raise RuntimeError("No :M;CUT edge on subtractive cut shape %s" % shape)


def pick_subtractive_torus_unique_edge(shape) -> str:
    """SubtractiveTorus tube rim with :M;CUT; smallest Circle (tube Radius2 class).

    Do not use pick_subtractive_cut_named_edge here — the first :M;CUT hit is often
    a Line (Edge5 class). Fillet on r=1 tube circles needs radius < tube radius.
    """
    best = None
    best_r = None
    for i in range(1, shape.countElement("Edge") + 1):
        name = "Edge%d" % i
        edge = shape.getElement(name)
        mapped = mapped_name_for_edge(shape, name)
        if not mapped or ":M;CUT" not in mapped:
            continue
        try:
            curve = edge.Curve
            if type(curve).__name__ != "Circle":
                continue
            radius = float(abs(curve.Radius))
        except Exception:
            continue
        if best_r is None or radius < best_r:
            best_r, best = radius, name
    if best:
        return best
    raise RuntimeError("No :M;CUT Circle edge on subtractive torus %s" % shape)


def pick_subtractive_sphere_unique_edge(shape) -> str:
    """Pick fillet-safe subtractive-sphere rim; prefer :M;CUT equator (Edge1 class)."""
    try:
        return pick_subtractive_cut_named_edge(shape)
    except RuntimeError:
        pass
    best = None
    best_r = -1.0
    for i in range(1, shape.countElement("Edge") + 1):
        name = "Edge%d" % i
        edge = shape.getElement(name)
        if _is_closed_sphere_seam(edge):
            continue
        verts = edge.Vertexes
        if verts:
            zs = [v.Point.z for v in verts]
            if abs(max(zs) - min(zs)) > 1e-4:
                continue
        try:
            curve = edge.Curve
            if type(curve).__name__ not in ("Circle", "Ellipse"):
                continue
            radius = float(abs(curve.Radius))
        except Exception:
            continue
        if radius > best_r:
            best_r, best = radius, name
    if not best:
        raise RuntimeError("No fillet-safe curved sphere-cut edge on %s" % shape)
    return best


def _is_closed_cone_seam(edge) -> bool:
    """True for the vertical Line seam on a 360° MakeCone solid.

    Mirror of _is_closed_cylinder_seam — that edge is unique but not
    C0-continuous, so PartDesign Fillet rejects it. Prefer unique base/top
    circular rims (truncated cone: Radius1 != Radius2, both > 0).
    """
    return _is_closed_cylinder_seam(edge)



def _is_closed_ellipsoid_seam(edge) -> bool:
    """True for the meridian seam on a 360° MakeSphere+GTransform ellipsoid.

    Mirror of _is_closed_sphere_seam: after unequal Z-scale the meridian may be
    Circle or Ellipse with Z-varying vertices. Prefer the unique equatorial
    circular/elliptical rim (hemisphere Angle2=0).
    """
    try:
        cname = type(edge.Curve).__name__
        if cname not in ("Circle", "Ellipse"):
            return False
    except Exception:
        return False
    verts = edge.Vertexes
    if len(verts) == 0:
        return False
    if len(verts) == 1:
        return True
    zs = [v.Point.z for v in verts]
    return abs(max(zs) - min(zs)) > 1e-6


def pick_ellipsoid_unique_edge(shape) -> str:
    """Fillet-safe unique edge on AdditiveEllipsoid (I13).

    Do not use pick_loop3_edge. GTransform from MakeSphere yields BSpline
    rims, not Circle/Ellipse. Construction uses a hemisphere (Angle2=0) so the
    equator is a unique closed horizontal rim at constant Z (Edge1 class).
    Open meridians and pole/degenerate closed edges are not fillet-able.
    """
    best = None
    best_r = -1.0
    n = shape.countElement("Edge")
    for i in range(1, n + 1):
        name = "Edge%d" % i
        edge = shape.getElement(name)
        if not edge.isClosed():
            continue
        verts = edge.Vertexes
        if not verts:
            continue
        zs = [v.Point.z for v in verts]
        if abs(max(zs) - min(zs)) > 1e-4:
            continue
        try:
            edge.Curve
        except Exception:
            continue
        rxy = max(math.hypot(v.Point.x, v.Point.y) for v in verts)
        if rxy > best_r:
            best_r = rxy
            best = name
    if best:
        return best
    # Legacy Circle/Ellipse path if OCCT exposes analytic equator.
    for i in range(1, n + 1):
        name = "Edge%d" % i
        edge = shape.getElement(name)
        if _is_closed_ellipsoid_seam(edge):
            continue
        try:
            curve = edge.Curve
            cname = type(curve).__name__
            if cname == "Circle":
                radius = float(abs(curve.Radius))
            elif cname == "Ellipse":
                radius = float(abs(curve.MajorRadius))
            else:
                continue
        except Exception:
            continue
        if radius > best_r:
            best_r = radius
            best = name
    if best:
        return best
    try:
        return pick_outer_circular_edge(shape)
    except RuntimeError as exc:
        raise RuntimeError("No unique Fillet-safe Ellipsoid edge on %s" % shape) from exc


def pick_cone_unique_edge(shape) -> str:
    """Fillet-safe unique edge on AdditiveCone (I13).

    Do not use pick_loop3_edge. A 360° truncated cone has a vertical side
    seam that may be unique but not fillet-able; prefer the unique outer
    circular rim (larger of base/top) — mirror of pick_cylinder_unique_edge
    / pick_sphere_unique_edge (equator). Construction uses Radius1 != Radius2
    both > 0 so two circular rims exist; pick unique outer.
    """
    best = None
    best_r = -1.0
    n = shape.countElement("Edge")
    for i in range(1, n + 1):
        name = "Edge%d" % i
        edge = shape.getElement(name)
        if _is_closed_cone_seam(edge):
            continue
        try:
            curve = edge.Curve
            if type(curve).__name__ != "Circle":
                continue
            radius = float(abs(curve.Radius))
        except Exception:
            continue
        if radius > best_r:
            best_r = radius
            best = name
    if best:
        return best
    # Fallback: largest circular even if seam filter empty.
    try:
        return pick_outer_circular_edge(shape)
    except RuntimeError as exc:
        raise RuntimeError("No unique Fillet-safe Cone edge on %s" % shape) from exc


def _is_closed_torus_seam(edge) -> bool:
    """Meridian-seam skip for AdditiveTorus Fillet pick (documented helper).

    On a full 360° MakeTorus the meridian seam can be unique but not
    C0-continuous, so PartDesign Fillet rejects it (mirror of
    _is_closed_sphere_seam / _is_closed_cone_seam). Construction uses
    Angle3 < 360 so unique tube cross-section end circles exist and are the
    Fillet targets — those closed circles often have a single Vertex in OCC,
    so a verts==1 heuristic would wrongly skip them. This predicate therefore
    returns False for the partial-torus gate; pick_torus_unique_edge prefers
    the smallest Circle radius (tube Radius2) over major equator
    (Radius1±Radius2). Keep the helper so call sites mirror cone/sphere.
    """
    _ = edge
    return False


def pick_torus_unique_edge(shape) -> str:
    """Fillet-safe unique edge on AdditiveTorus (I13).

    Do not use pick_loop3_edge. A full 360° torus often has a meridian seam
    that may be unique but not fillet-able. Prefer a unique circular tube
    cross-section end rim — construction uses Angle3 < 360 (mirror of
    hemisphere Sphere Angle2=0 / truncated Cone) so end-circle edges exist.
    Among Circle edges, prefer the smallest radius (tube Radius2), not the
    major equator (Radius1±Radius2).
    """
    best = None
    best_r = None
    n = shape.countElement("Edge")
    for i in range(1, n + 1):
        name = "Edge%d" % i
        edge = shape.getElement(name)
        if _is_closed_torus_seam(edge):
            continue
        try:
            curve = edge.Curve
            if type(curve).__name__ != "Circle":
                continue
            radius = float(abs(curve.Radius))
        except Exception:
            continue
        if best_r is None or radius < best_r:
            best_r = radius
            best = name
    if best:
        return best
    try:
        return pick_outer_circular_edge(shape)
    except RuntimeError as exc:
        raise RuntimeError("No unique Fillet-safe Torus edge on %s" % shape) from exc


def pick_vertical_side_face(shape) -> str:
    best = None
    best_area = -1.0
    for i, face in enumerate(shape.Faces, start=1):
        try:
            n = face.normalAt(0, 0)
        except Exception:
            continue
        if abs(n.z) > 1e-5:
            continue
        area = face.Area
        if area > best_area:
            best_area = area
            best = "Face%d" % i
    if not best:
        raise RuntimeError("No vertical side face on %s" % shape)
    return best


def pick_bottom_outline_edge(shape) -> str:
    min_z = None
    for v in shape.Vertexes:
        z = v.Point.z
        if min_z is None or z < min_z:
            min_z = z
    if min_z is None:
        raise RuntimeError("No vertices on %s" % shape)
    best = None
    best_len = -1.0
    n = shape.countElement("Edge")
    for i in range(1, n + 1):
        name = "Edge%d" % i
        edge = shape.getElement(name)
        verts = edge.Vertexes
        if len(verts) < 2:
            continue
        p1, p2 = verts[0].Point, verts[1].Point
        if abs(p1.z - min_z) > 1e-5 or abs(p2.z - min_z) > 1e-5:
            continue
        if abs(p1.z - p2.z) > 1e-5:
            continue
        length = p1.distanceToPoint(p2)
        if length > best_len:
            best_len = length
            best = name
    if not best:
        raise RuntimeError("No bottom outline edge on %s" % shape)
    return best


def mapped_name_for_edge(shape, edge_name) -> str:
    try:
        rev = shape.ElementReverseMap
        if isinstance(rev, dict) and edge_name in rev:
            return str(rev[edge_name])
    except Exception:
        pass
    try:
        em = shape.ElementMap
        if isinstance(em, dict):
            for mapped, indexed in em.items():
                if str(indexed) == edge_name or str(indexed).endswith("." + edge_name):
                    return str(mapped)
    except Exception:
        pass
    return ""


def shape_center_of_mass(shape):
    """Center for pick helpers; Part.Compound may lack CenterOfMass in FreeCADCmd."""
    for attr in ("CenterOfMass", "CenterOfGravity"):
        try:
            p = getattr(shape, attr)
            if callable(p):
                p = p()
            return p
        except Exception:
            pass
    bb = shape.BoundBox
    return App.Vector(
        0.5 * (bb.XMin + bb.XMax),
        0.5 * (bb.YMin + bb.YMax),
        0.5 * (bb.ZMin + bb.ZMax),
    )


def require_ok(obj, label):
    if not obj.isValid():
        raise RuntimeError("%s invalid: %s" % (label, obj.Name))


def check(name, cond, detail=""):
    SCORED.add(name)
    if cond:
        print("TESTS_CASE PASS", name, detail)
        return True
    print("TESTS_CASE FAIL", name, detail)
    FAILURES.append(name)
    return False


def decode_hex_payload(hex_text: str) -> str:
    if not hex_text:
        return ""
    try:
        return bytes.fromhex(hex_text).decode("utf-8", errors="replace")
    except Exception:
        return ""

def elementmap_has_st_token(path: str, object_name: str) -> bool:
    """True when SubtractiveLoft (or named object) persisted a ;:ST ElementMap stamp.

    Shape ElementMap lives in FCStd sidecar ``Object.Shape.Map.txt`` (and
    AddSubShape), not inline in Document.xml Object tostring — Save() writes a
    ``file=`` reference. Prefer product stamp in the real Map sidecar; also
    accept inline ElementMap2 / Object XML for forceXML layouts.
    """
    prefix = object_name + "."
    with zipfile.ZipFile(path) as z:
        for name in z.namelist():
            base = name.rsplit("/", 1)[-1]
            if base.startswith(prefix) and base.endswith(".Map.txt"):
                if b";:ST" in z.read(name):
                    return True
        root = ET.fromstring(z.read("Document.xml"))
        for obj in root.findall(".//Object"):
            if obj.get("name") == object_name:
                if ";:ST" in ET.tostring(obj, encoding="unicode"):
                    return True
    return False


def parse_fcstd(path: str):
    with zipfile.ZipFile(path) as z:
        xml = z.read("Document.xml")
    root = ET.fromstring(xml)
    objects = {}
    for obj in root.findall(".//Object"):
        name = obj.get("name")
        if not name:
            continue
        rec = objects.setdefault(name, {"id": None, "type": None, "links": []})
        if obj.get("id"):
            rec["id"] = int(obj.get("id"))
        if obj.get("type"):
            rec["type"] = obj.get("type")
        for prop in obj.findall(".//Property"):
            pname = prop.get("name") or ""
            for ls in prop.findall("LinkSub"):
                subs = []
                for sub in ls.findall("Sub"):
                    subs.append(
                        {
                            "value": sub.get("value") or "",
                            "shadow": sub.get("shadow") or "",
                            "stSeed": sub.get("stSeed") or "",
                            "stKind": sub.get("stKind") or "",
                            "stFallback": sub.get("stFallback") or "",
                        }
                    )
                rec["links"].append(
                    {
                        "value": ls.get("value") or "",
                        "subs": subs,
                        "kind": "LinkSub",
                        "prop": pname,
                    }
                )
            for lsl in prop.findall("LinkSubList"):
                subs = []
                for link in lsl.findall("Link"):
                    subs.append(
                        {
                            "value": link.get("sub") or "",
                            "shadow": link.get("shadow") or "",
                            "stSeed": link.get("stSeed") or "",
                            "stKind": link.get("stKind") or "",
                            "stFallback": link.get("stFallback") or "",
                            "obj": link.get("obj") or "",
                        }
                    )
                rec["links"].append(
                    {
                        "value": (subs[0].get("obj") if len(subs) == 1 else ""),
                        "subs": subs,
                        "kind": "LinkSubList",
                        "prop": pname,
                    }
                )
            for xl in prop.findall("XLink"):
                subs = []
                if xl.get("sub") is not None:
                    subs.append(
                        {
                            "value": xl.get("sub") or "",
                            "shadow": xl.get("shadow") or "",
                            "stSeed": xl.get("stSeed") or "",
                            "stKind": xl.get("stKind") or "",
                            "stFallback": xl.get("stFallback") or "",
                        }
                    )
                for sub in xl.findall("Sub"):
                    subs.append(
                        {
                            "value": sub.get("value") or "",
                            "shadow": sub.get("shadow") or "",
                            "stSeed": sub.get("stSeed") or "",
                            "stKind": sub.get("stKind") or "",
                            "stFallback": sub.get("stFallback") or "",
                        }
                    )
                rec["links"].append(
                    {
                        "value": xl.get("name") or "",
                        "subs": subs,
                        "kind": "XLink",
                        "prop": pname,
                    }
                )
        att = []
        for prop in obj.findall(".//Property"):
            if prop.get("name") != "AttachmentSupport":
                continue
            for lsl in prop.findall(".//LinkSubList"):
                for link in lsl.findall("Link"):
                    att.append(
                        {
                            "value": link.get("sub") or "",
                            "shadow": link.get("shadow") or "",
                            "stSeed": link.get("stSeed") or "",
                            "stKind": link.get("stKind") or "",
                            "stFallback": link.get("stFallback") or "",
                            "obj": link.get("obj") or "",
                        }
                    )
            for ls in prop.findall(".//LinkSub"):
                for sub in ls.findall("Sub"):
                    att.append(
                        {
                            "value": sub.get("value") or "",
                            "shadow": sub.get("shadow") or "",
                            "stSeed": sub.get("stSeed") or "",
                            "stKind": sub.get("stKind") or "",
                            "stFallback": sub.get("stFallback") or "",
                            "obj": ls.get("value") or "",
                        }
                    )
        rec["attachment_support"] = att
        cells = []
        for cell in obj.findall(".//Cell"):
            cells.append(
                {
                    "address": cell.get("address") or "",
                    "content": cell.get("content") or "",
                    "stSeed": cell.get("stSeed") or "",
                    "stKind": cell.get("stKind") or "",
                    "stFallback": cell.get("stFallback") or "",
                }
            )
        rec["cells"] = cells
    stg = ""
    sg = root.find(".//SemanticGraph")
    if sg is not None:
        stg = decode_hex_payload(sg.get("payload") or "")
    return objects, stg


def first_link(objects, obj_name):
    rec = objects.get(obj_name) or {}
    links = rec.get("links") or []
    if not links:
        return None, []
    return links[0].get("value") or "", links[0].get("subs") or []


def named_link(prop_name):
    """first_link restricted to a Property name (Faces / References3D / Source)."""

    def finder(objects, obj_name):
        rec = objects.get(obj_name) or {}
        for link in rec.get("links") or []:
            if (link.get("prop") or "") != prop_name:
                continue
            val = link.get("value") or ""
            subs = link.get("subs") or []
            if not val and subs:
                val = subs[0].get("obj") or ""
            return val, subs
        return first_link(objects, obj_name)

    return finder


def first_face_link(objects, obj_name):
    """Hole.Profile is typically listed first; StartReference is the Face LinkSub."""
    rec = objects.get(obj_name) or {}
    for link in rec.get("links") or []:
        face_subs = []
        for sub in link.get("subs") or []:
            val = sub.get("value") or sub.get("stFallback") or ""
            if val.startswith("Face"):
                face_subs.append(sub)
        if face_subs:
            return link.get("value") or "", face_subs
    return None, []


def attachment_support_slots(objects, obj_name):
    rec = objects.get(obj_name) or {}
    slots = list(rec.get("attachment_support") or [])
    if slots:
        return slots
    for link in rec.get("links") or []:
        if link.get("kind") == "LinkSubList":
            return list(link.get("subs") or [])
    return []


def parse_identities(stg: str):
    out = {}
    for line in stg.splitlines():
        if not line.startswith("identity "):
            continue
        parts = line.split()
        if len(parts) < 4:
            continue
        try:
            handle = int(parts[1])
            allocated_by = int(parts[3])
        except ValueError:
            continue
        out[handle] = allocated_by
    return out


def parse_fillet_event_features(stg: str):
    features = []
    for line in stg.splitlines():
        if not line.startswith("event "):
            continue
        parts = line.split()
        if len(parts) < 5:
            continue
        if parts[3] == "Fillet":
            try:
                features.append(int(parts[4]))
            except ValueError:
                pass
    return features


def parse_chamfer_event_features(stg: str):
    features = []
    for line in stg.splitlines():
        if not line.startswith("event "):
            continue
        parts = line.split()
        if len(parts) < 5:
            continue
        if parts[3] == "Chamfer":
            try:
                features.append(int(parts[4]))
            except ValueError:
                pass
    return features


def parse_draft_event_features(stg: str):
    features = []
    for line in stg.splitlines():
        if not line.startswith("event "):
            continue
        parts = line.split()
        if len(parts) < 5:
            continue
        if parts[3] == "Draft":
            try:
                features.append(int(parts[4]))
            except ValueError:
                pass
    return features


def parse_opcode_event_features(stg: str, opcode: str):
    features = []
    for line in stg.splitlines():
        if not line.startswith("event "):
            continue
        parts = line.split()
        if len(parts) < 5:
            continue
        if parts[3] == opcode:
            try:
                features.append(int(parts[4]))
            except ValueError:
                pass
    return features


# STG1 event line (SemanticGraph::serialize):
#   event <id> <kind> <op> <feature> <eval> <role> <published>
# kind is EventKind unsigned (SemanticTopology.h): 0 Modified, 1 Generated, 2 Split, ...
EVENT_KIND_MODIFIED = 0
EVENT_KIND_GENERATED = 1
EVENT_KIND_SPLIT = 2


def parse_identity_rows(stg: str):
    """STG1: identity <handle> <kindChar> <allocatedBy> <eval> <role>."""
    rows = []
    for line in stg.splitlines():
        if not line.startswith("identity "):
            continue
        parts = line.split()
        if len(parts) < 4:
            continue
        kind = parts[2]
        try:
            handle = int(parts[1])
            if kind.isalpha():
                allocated_by = int(parts[3])
            else:
                allocated_by = int(parts[2])
                kind = ""
        except ValueError:
            continue
        rows.append({"handle": handle, "kind": kind.upper(), "allocated_by": allocated_by})
    return rows


def parse_output_rows(stg: str):
    """STG1: output <eventId> <handle> <kindChar> <allocatedBy> ..."""
    rows = []
    for line in stg.splitlines():
        if not line.startswith("output "):
            continue
        parts = line.split()
        if len(parts) < 4:
            continue
        try:
            ev = int(parts[1])
            handle = int(parts[2])
        except ValueError:
            continue
        kind = parts[3] if len(parts) > 3 else ""
        allocated_by = None
        if len(parts) > 4 and kind.isalpha():
            try:
                allocated_by = int(parts[4])
            except ValueError:
                pass
        rows.append(
            {
                "event": ev,
                "handle": handle,
                "kind": kind.upper() if kind.isalpha() else "",
                "allocated_by": allocated_by,
            }
        )
    return rows


def count_sketch_vertex_identities(objects, stg, sketch_name):
    """Vertex-kind identities allocatedBy SketchPadVert (rectangle → ≥4).

    Prefer identity kindChar V. If identity lines lack kind, parse Generated
    Vertex outputs / Sketch.v notes on that sketch feature id.
    """
    rec = objects.get(sketch_name) or {}
    sketch_id = rec.get("id")
    detail = ["sketch=%s id=%s" % (sketch_name, sketch_id)]
    if sketch_id is None:
        return 0, "; ".join(detail)
    handles = []
    for row in parse_identity_rows(stg):
        if row["allocated_by"] != sketch_id:
            continue
        if row["kind"] == "V":
            handles.append(row["handle"])
    if handles:
        detail.append("identity V handles=%s count=%d" % (handles, len(handles)))
        return len(handles), "; ".join(detail)
    events = parse_stg_events(stg)
    by_ev = {}
    for o in parse_output_rows(stg):
        by_ev.setdefault(o["event"], []).append(o)
    found = []
    for e in events:
        if e["feature"] != sketch_id or e["kind"] != EVENT_KIND_GENERATED:
            continue
        outs = by_ev.get(e["id"]) or []
        vertex_out = [o for o in outs if o["kind"] == "V"]
        if vertex_out or str(e["op"]).startswith("Sketch.v"):
            for o in vertex_out or outs:
                found.append(o["handle"])
    detail.append("event Generated Vertex handles=%s count=%d" % (found, len(found)))
    return len(found), "; ".join(detail)



def parse_stg_events(stg: str):
    events = []
    for line in stg.splitlines():
        if not line.startswith("event "):
            continue
        parts = line.split()
        if len(parts) < 5:
            continue
        try:
            events.append(
                {
                    "id": int(parts[1]),
                    "kind": int(parts[2]),
                    "op": parts[3],
                    "feature": int(parts[4]),
                }
            )
        except ValueError:
            continue
    return events


def parse_stg_event_handles(stg: str, tag: str):
    """input/output <eventId> <handle> <kindChar> <allocatedBy> ..."""
    by_event = {}
    prefix = tag + " "
    for line in stg.splitlines():
        if not line.startswith(prefix):
            continue
        parts = line.split()
        if len(parts) < 3:
            continue
        try:
            ev = int(parts[1])
            handle = int(parts[2])
        except ValueError:
            continue
        by_event.setdefault(ev, []).append(handle)
    return by_event


def parse_thickness_event_features(stg: str):
    features = []
    for line in stg.splitlines():
        if not line.startswith("event "):
            continue
        parts = line.split()
        if len(parts) < 5:
            continue
        if parts[3] == "Thickness":
            try:
                features.append(int(parts[4]))
            except ValueError:
                pass
    return features



def body_origin_plane(body, role_suffix):
    """Return this Body's Origin plane (XY_Plane / XZ_Plane / …).

    On multi-Body documents, ``doc.XY_Plane`` resolves to the *first* Body's
    Origin feature and is out of GeoFeatureGroup scope for later Bodies
    (Draft NeutralPlane / Sketch AttachmentSupport warnings on main).
    """
    origin = getattr(body, "Origin", None)
    if origin is None:
        raise RuntimeError("%s has no Origin" % getattr(body, "Name", body))
    feats = list(origin.OriginFeatures)
    for feat in feats:
        name = getattr(feat, "Name", "") or ""
        if name.endswith(role_suffix) or name.endswith("_" + role_suffix):
            return feat
    # FreeCAD Origin order: X/Y/Z axes then XY/XZ/YZ planes.
    order = {"XY_Plane": 3, "XZ_Plane": 4, "YZ_Plane": 5}
    idx = order.get(role_suffix)
    if idx is not None and len(feats) > idx:
        return feats[idx]
    raise RuntimeError("%s Origin missing %s" % (body.Name, role_suffix))


def body_xy_plane(body):
    return body_origin_plane(body, "XY_Plane")


def body_xz_plane(body):
    return body_origin_plane(body, "XZ_Plane")


def make_pad_body(doc, body_name, sketch_name, pad_name, refine, recomputes, corner=(0.0, 0.0)):
    body = doc.addObject("PartDesign::Body", body_name)
    sketch = doc.addObject("Sketcher::SketchObject", sketch_name)
    sketch.AttachmentSupport = (body_xy_plane(body), [""])
    sketch.MapMode = "FlatFace"
    body.addObject(sketch)
    add_rectangle(sketch, corner, (10.0, 10.0))
    doc.recompute()
    pad = doc.addObject("PartDesign::Pad", pad_name)
    body.addObject(pad)
    pad.Profile = sketch
    pad.Length = 10.0
    pad.Type = "Length"
    try:
        pad.Refine = refine
    except Exception:
        pass
    doc.recompute()
    require_ok(pad, pad_name)
    for _ in range(max(0, recomputes - 1)):
        pad.touch()
        doc.recompute()
        require_ok(pad, pad_name + " recompute")
    return body, sketch, pad


def add_fillet(doc, body, name, support, edge, radius=1.0):
    fillet = doc.addObject("PartDesign::Fillet", name)
    body.addObject(fillet)
    fillet.Base = (support, [edge])
    fillet.Radius = radius
    doc.recompute()
    require_ok(fillet, name)
    return fillet


def add_part_fillet(doc, name, support, edge, radius=1.0):
    """Part WB Fillet (not PartDesign::Fillet). Edges assignment syncs EdgeLinks."""
    fillet = doc.addObject("Part::Fillet", name)
    edge_index = int(str(edge).replace("Edge", ""))
    fillet.Base = support
    fillet.Edges = [(edge_index, radius, radius)]
    doc.recompute()
    require_ok(fillet, name)
    return fillet


def add_chamfer(doc, body, name, support, edge, size=1.0):
    chamfer = doc.addObject("PartDesign::Chamfer", name)
    body.addObject(chamfer)
    chamfer.Base = (support, [edge])
    chamfer.Size = size
    doc.recompute()
    require_ok(chamfer, name)
    return chamfer


def add_draft(doc, body, name, support, face, angle=5.0):
    draft = doc.addObject("PartDesign::Draft", name)
    body.addObject(draft)
    draft.Base = (support, [face])
    draft.Angle = angle
    try:
        draft.NeutralPlane = (body_xy_plane(body), [""])
    except Exception:
        pass
    doc.recompute()
    require_ok(draft, name)
    return draft


def add_thickness(doc, body, name, support, face, value=1.0):
    thick = doc.addObject("PartDesign::Thickness", name)
    body.addObject(thick)
    thick.Base = (support, [face])
    thick.Value = value
    try:
        thick.Reversed = True
    except Exception:
        pass
    doc.recompute()
    require_ok(thick, name)
    return thick


def _freeze_recomputes(doc):
    try:
        doc.RecomputesFrozen = True
        return True
    except Exception:
        return False


def _unfreeze_recomputes(doc, frozen):
    if not frozen:
        return
    try:
        doc.RecomputesFrozen = False
    except Exception:
        pass


def _origin_link(doc, origin_name, sketch, sketch_sub):
    obj = getattr(doc, origin_name, None)
    if obj is not None:
        return (obj, [""])
    return (sketch, [sketch_sub])


def add_linear_pattern(doc, body, name, pad, sketch, occurrences=2, length=20.0):
    """Isolate-3: configure like TestLinearPattern.py, then Body.addObject.

    Upstream sets Originals/Direction/Length/Occurrences before Body.addObject.
    Isolate-2 added the pattern to the Body first; that is the one lever this round.
    Polar/Mirrored stay parked.
    """
    frozen = _freeze_recomputes(doc)
    try:
        lp = doc.addObject("PartDesign::LinearPattern", name)
        lp.Originals = [pad]
        try:
            lp.TransformMode = "Features"
        except Exception:
            try:
                lp.TransformMode = 0
            except Exception:
                pass
        lp.Direction = _origin_link(doc, "X_Axis", sketch, "H_Axis")
        lp.Occurrences = occurrences
        try:
            lp.Length = length
        except Exception:
            pass
        try:
            lp.Refine = False
        except Exception:
            pass
        body.addObject(lp)
        try:
            lp.BaseFeature = pad
        except Exception:
            pass
    finally:
        _unfreeze_recomputes(doc, frozen)
    doc.recompute()
    require_ok(lp, name)
    return lp


def add_polar_pattern(doc, body, name, pad, sketch, occurrences=2, angle=180.0):
    """Same order as add_linear_pattern / TestLinearPattern.py (configure, then Body.addObject)."""
    frozen = _freeze_recomputes(doc)
    try:
        pp = doc.addObject("PartDesign::PolarPattern", name)
        pp.Originals = [pad]
        try:
            pp.TransformMode = "Features"
        except Exception:
            try:
                pp.TransformMode = 0
            except Exception:
                pass
        pp.Axis = _origin_link(doc, "Z_Axis", sketch, "N_Axis")
        pp.Occurrences = occurrences
        try:
            pp.Angle = angle
        except Exception:
            pass
        try:
            pp.Refine = False
        except Exception:
            pass
        body.addObject(pp)
        try:
            pp.BaseFeature = pad
        except Exception:
            pass
    finally:
        _unfreeze_recomputes(doc, frozen)
    doc.recompute()
    require_ok(pp, name)
    return pp


def add_mirrored(doc, body, name, pad, sketch):
    """Same order as add_linear_pattern / add_polar_pattern (configure, then Body.addObject)."""
    frozen = _freeze_recomputes(doc)
    try:
        mir = doc.addObject("PartDesign::Mirrored", name)
        mir.Originals = [pad]
        try:
            mir.TransformMode = "Features"
        except Exception:
            try:
                mir.TransformMode = 0
            except Exception:
                pass
        mir.MirrorPlane = _origin_link(doc, "YZ_Plane", sketch, "V_Axis")
        try:
            mir.Refine = False
        except Exception:
            pass
        body.addObject(mir)
        try:
            mir.BaseFeature = pad
        except Exception:
            pass
    finally:
        _unfreeze_recomputes(doc, frozen)
    doc.recompute()
    require_ok(mir, name)
    return mir


def add_scaled(doc, body, name, pad, factor=1.6, occurrences=2):
    frozen = _freeze_recomputes(doc)
    try:
        scaled = doc.addObject("PartDesign::Scaled", name)
        scaled.Originals = [pad]
        try:
            scaled.TransformMode = "Features"
        except Exception:
            scaled.TransformMode = 0
        scaled.Factor = factor
        scaled.Occurrences = occurrences
        try:
            scaled.Refine = False
        except Exception:
            pass
        body.addObject(scaled)
        try:
            scaled.BaseFeature = pad
        except Exception:
            pass
    finally:
        _unfreeze_recomputes(doc, frozen)
    doc.recompute()
    require_ok(scaled, name)
    return scaled


def add_multi_transform_scaled(doc, body, name, pad, factor=1.5, occurrences=2):
    frozen = _freeze_recomputes(doc)
    try:
        scaled = doc.addObject("PartDesign::Scaled", name + "Scale")
        scaled.Factor = factor
        scaled.Occurrences = occurrences
        body.addObject(scaled)
        multi = doc.addObject("PartDesign::MultiTransform", name)
        multi.Originals = [pad]
        try:
            multi.TransformMode = "Features"
        except Exception:
            multi.TransformMode = 0
        multi.Transformations = [scaled]
        try:
            multi.Refine = False
        except Exception:
            pass
        body.addObject(multi)
        try:
            multi.BaseFeature = pad
        except Exception:
            pass
    finally:
        _unfreeze_recomputes(doc, frozen)
    doc.recompute()
    require_ok(multi, name)
    return multi


def add_additive_loft(doc, body, name, profile, section):
    loft = doc.addObject("PartDesign::AdditiveLoft", name)
    body.addObject(loft)
    loft.Profile = profile
    loft.Sections = [section]
    try:
        loft.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(loft, name)
    return loft


def add_additive_pipe(doc, body, name, profile, spine):
    pipe = doc.addObject("PartDesign::AdditivePipe", name)
    body.addObject(pipe)
    pipe.Profile = profile
    pipe.Spine = spine
    try:
        pipe.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(pipe, name)
    return pipe


def add_additive_helix(doc, body, name, profile, sketch):
    helix = doc.addObject("PartDesign::AdditiveHelix", name)
    body.addObject(helix)
    helix.Profile = profile
    helix.ReferenceAxis = (sketch, ["V_Axis"])
    helix.Pitch = 50.0
    helix.Height = 150.0
    helix.Turns = 3.0
    helix.Angle = 0.0
    helix.Mode = 0
    helix.LeftHanded = 0
    try:
        helix.Reversed = 0
    except Exception:
        pass
    try:
        helix.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(helix, name)
    return helix

def add_sweep_sketch(doc, body, name, plane, first, second):
    sketch = doc.addObject("Sketcher::SketchObject", name)
    body.addObject(sketch)
    sketch.AttachmentSupport = (plane, [""])
    sketch.MapMode = "FlatFace"
    add_rectangle(sketch, first, second)
    doc.recompute()
    return sketch


def add_subtractive_loft(doc, body, name, profile, section, base):
    loft = doc.addObject("PartDesign::SubtractiveLoft", name)
    body.addObject(loft)
    loft.Profile = profile
    loft.Sections = [section]
    try:
        loft.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(loft, name)
    return loft


def add_subtractive_pipe(doc, body, name, profile, spine, base):
    pipe = doc.addObject("PartDesign::SubtractivePipe", name)
    body.addObject(pipe)
    pipe.Profile = profile
    pipe.Spine = spine
    try:
        pipe.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(pipe, name)
    return pipe


def add_subtractive_helix(doc, body, name, profile, sketch, base):
    helix = doc.addObject("PartDesign::SubtractiveHelix", name)
    body.addObject(helix)
    helix.Profile = profile
    helix.ReferenceAxis = (sketch, ["V_Axis"])
    helix.Pitch = 50.0
    helix.Height = 150.0
    helix.Turns = 3.0
    helix.Angle = 0.0
    helix.Mode = 0
    helix.LeftHanded = 0
    try:
        helix.Reversed = 0
    except Exception:
        pass
    try:
        helix.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(helix, name)
    return helix


def add_sketch_on_cap_pocket(doc, body, pad, sketch_name, pocket_name, pocket_type, length=5.0, radius=1.5):
    top = face_name_max_z(pad.Shape)
    print("TESTS pick %s.Profile support = %s.%s Type=%s" % (pocket_name, pad.Name, top, pocket_type))
    sketch = doc.addObject("Sketcher::SketchObject", sketch_name)
    sketch.AttachmentSupport = (pad, [top])
    sketch.MapMode = "FlatFace"
    body.addObject(sketch)
    add_circle(sketch, (5.0, 5.0), radius)
    doc.recompute()
    pocket = doc.addObject("PartDesign::Pocket", pocket_name)
    body.addObject(pocket)
    pocket.Profile = sketch
    pocket.Type = pocket_type
    pocket.Length = length
    doc.recompute()
    if not pocket.isValid() and pocket_type == "ThroughAll":
        try:
            pocket.Reversed = True
        except Exception:
            pass
        doc.recompute()
    require_ok(pocket, pocket_name)
    return sketch, pocket, top


def score_fillet_seed(objects, stg, fillet_name, expected_link, case_seed, case_scope=None, link_finder=None):
    finder = link_finder or first_link
    link_val, subs = finder(objects, fillet_name)
    sub = subs[0] if subs else {}
    seed = (sub.get("stSeed") or "").strip()
    shadow = sub.get("shadow") or ""
    fallback = sub.get("stFallback") or sub.get("value") or ""
    detail = "link=%s sub=%s stSeed=%s shadow=%s" % (link_val, fallback, seed or "(none)", shadow)
    if not seed:
        detail += "  false pass if solid OK: seedless Base"
    check(case_seed, bool(seed), detail)
    if case_scope:
        identities = parse_identities(stg)
        rec = objects.get(expected_link) or {}
        expected_id = rec.get("id")
        handle = None
        try:
            handle = int(seed, 16) if seed else None
        except ValueError:
            handle = None
        allocated = identities.get(handle) if handle is not None else None
        scope_ok = (
            link_val == expected_link
            and expected_id is not None
            and allocated == expected_id
        )
        check(
            case_scope,
            scope_ok,
            "link=%s expected_id=%s identity.handle=%s allocatedBy=%s"
            % (link_val, expected_id, handle, allocated),
        )
    return seed


def make_subtractive_primitive(
    doc, body_name, base_name, feature_type, feature_name, values, picker, fillet_radius=1.0
):
    body = doc.addObject("PartDesign::Body", body_name)
    base = doc.addObject("PartDesign::AdditiveBox", base_name)
    body.addObject(base)
    base.Length = 10.0
    base.Width = 10.0
    base.Height = 10.0
    try:
        base.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(base, base_name)
    feature = doc.addObject("PartDesign::" + feature_type, feature_name)
    body.addObject(feature)
    for prop, value in values.items():
        setattr(feature, prop, value)
    try:
        feature.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(feature, feature_name)
    edge = picker(feature.Shape)
    mapped = mapped_name_for_edge(feature.Shape, edge)
    print("TESTS pick Fillet" + feature_name + ".Base = " + feature_name + ".", edge, "mapped=", mapped)
    add_fillet(doc, body, "Fillet" + feature_name, feature, edge, radius=fillet_radius)
    return feature, edge


def main():
    out_dir = desktop_dir()
    os.makedirs(out_dir, exist_ok=True)
    pre_path = os.path.join(out_dir, "Tests_Automated_preSplit.FCStd")
    post_path = os.path.join(out_dir, "Tests_Automated.FCStd")

    doc = App.newDocument("Tests_Automated")

    body = doc.addObject("PartDesign::Body", "Body")
    sketch = doc.addObject("Sketcher::SketchObject", "Sketch")
    sketch.AttachmentSupport = (body_xy_plane(body), [""])
    sketch.MapMode = "FlatFace"
    body.addObject(sketch)
    _first, bottom_id = add_rectangle(sketch, (0.0, 0.0), (10.0, 10.0))
    doc.recompute()

    pad = doc.addObject("PartDesign::Pad", "Pad")
    body.addObject(pad)
    pad.Profile = sketch
    pad.Length = 10.0
    pad.Type = "Length"
    doc.recompute()
    require_ok(pad, "Pad")

    top_face = face_name_max_z(pad.Shape)
    sketch001 = doc.addObject("Sketcher::SketchObject", "Sketch001")
    sketch001.AttachmentSupport = (pad, [top_face])
    sketch001.MapMode = "FlatFace"
    body.addObject(sketch001)
    add_circle(sketch001, (5.0, 5.0), 2.0)
    doc.recompute()

    pocket = doc.addObject("PartDesign::Pocket", "Pocket")
    body.addObject(pocket)
    pocket.Profile = sketch001
    pocket.Length = 12.0
    pocket.Type = "Length"
    doc.recompute()
    require_ok(pocket, "Pocket")

    pad_com = pad.Shape.CenterOfMass
    pocket_edge = pick_vertical_outer_edge(pocket.Shape, (pad_com.x, pad_com.y))
    print("TESTS pick Fillet.Base = Pocket.", pocket_edge)
    fillet = add_fillet(doc, body, "Fillet", pocket, pocket_edge)
    print("TESTS Fillet.Base after set:", fillet.Base)

    body_u, _sk_u, pad_u = make_pad_body(
        doc, "BodyPadU", "SketchPadU", "PadU", refine=True, recomputes=5
    )
    u_edge = pick_bottom_outline_edge(pad_u.Shape)
    u_map = mapped_name_for_edge(pad_u.Shape, u_edge)
    print("TESTS pick FilletPadU.Base = PadU.", u_edge, "mapped=", u_map)
    fillet_u = add_fillet(doc, body_u, "FilletPadU", pad_u, u_edge)

    body_v, _sk_v, pad_v = make_pad_body(
        doc, "BodyPadVert", "SketchPadVert", "PadVert", refine=True, recomputes=1
    )
    v_com = pad_v.Shape.CenterOfMass
    v_edge = pick_vertical_outer_edge(pad_v.Shape, (v_com.x, v_com.y))
    v_map = mapped_name_for_edge(pad_v.Shape, v_edge)
    print("TESTS pick FilletPadVert.Base = PadVert.", v_edge, "mapped=", v_map)
    add_fillet(doc, body_v, "FilletPadVert", pad_v, v_edge)

    body_c, _sk_c, pad_c = make_pad_body(
        doc, "BodyPadChamfer", "SketchPadChamfer", "PadChamfer", refine=True, recomputes=1
    )
    c_com = pad_c.Shape.CenterOfMass
    c_edge = pick_vertical_outer_edge(pad_c.Shape, (c_com.x, c_com.y))
    c_map = mapped_name_for_edge(pad_c.Shape, c_edge)
    print("TESTS pick ChamferPadVert.Base = PadChamfer.", c_edge, "mapped=", c_map)
    add_chamfer(doc, body_c, "ChamferPadVert", pad_c, c_edge)

    body_d, _sk_d, pad_d = make_pad_body(
        doc, "BodyPadDraft", "SketchPadDraft", "PadDraft", refine=True, recomputes=1
    )
    d_face = pick_vertical_side_face(pad_d.Shape)
    print("TESTS pick DraftPadVert.Base = PadDraft.", d_face)
    add_draft(doc, body_d, "DraftPadVert", pad_d, d_face, angle=5.0)

    body_t, _sk_t, pad_t = make_pad_body(
        doc, "BodyPadThickness", "SketchPadThickness", "PadThickness", refine=True, recomputes=1
    )
    t_face = face_name_max_z(pad_t.Shape)
    print("TESTS pick ThicknessPadCap.Base = PadThickness.", t_face)
    add_thickness(doc, body_t, "ThicknessPadCap", pad_t, t_face, value=1.0)

    body_r = doc.addObject("PartDesign::Body", "BodyRevolution")
    sketch_r = doc.addObject("Sketcher::SketchObject", "SketchRevolution")
    sketch_r.AttachmentSupport = (body_xy_plane(body_r), [""])
    sketch_r.MapMode = "FlatFace"
    body_r.addObject(sketch_r)
    add_rectangle(sketch_r, (10.0, 0.0), (5.0, 10.0))
    doc.recompute()
    rev = doc.addObject("PartDesign::Revolution", "Revolution")
    body_r.addObject(rev)
    rev.Profile = sketch_r
    rev.Angle = 360.0
    try:
        rev.ReferenceAxis = (sketch_r, ["V_Axis"])
    except Exception:
        try:
            rev.ReferenceAxis = (doc.Y_Axis, [""])
        except Exception:
            pass
    doc.recompute()
    require_ok(rev, "Revolution")
    r_edge = pick_outer_circular_edge(rev.Shape)
    print("TESTS pick FilletRevolution.Base = Revolution.", r_edge)
    add_fillet(doc, body_r, "FilletRevolution", rev, r_edge)

    body_h, _sk_h, pad_h = make_pad_body(
        doc, "BodyPadHole", "SketchPadHole", "PadHole", refine=True, recomputes=1
    )
    h_top = face_name_max_z(pad_h.Shape)
    print("TESTS pick HolePadCap.StartReference = PadHole.", h_top)
    sketch_hole = doc.addObject("Sketcher::SketchObject", "SketchHole")
    sketch_hole.AttachmentSupport = (pad_h, [h_top])
    sketch_hole.MapMode = "FlatFace"
    body_h.addObject(sketch_hole)
    add_circle(sketch_hole, (5.0, 5.0), 1.0)
    doc.recompute()
    hole = doc.addObject("PartDesign::Hole", "HolePadCap")
    body_h.addObject(hole)
    hole.Profile = sketch_hole
    hole.Diameter = 3.0
    try:
        hole.DepthType = "ThroughAll"
    except Exception:
        hole.DepthType = "Dimension"
        hole.Depth = 12.0
    hole.StartType = "Reference"
    hole.StartReference = (pad_h, [h_top])
    doc.recompute()
    require_ok(hole, "HolePadCap")

    body_pf, _sk_pf, pad_pf = make_pad_body(
        doc, "BodyPadPartFillet", "SketchPadPartFillet", "PadPartFillet", refine=True, recomputes=1
    )
    pf_com = pad_pf.Shape.CenterOfMass
    pf_edge = pick_vertical_outer_edge(pad_pf.Shape, (pf_com.x, pf_com.y))
    pf_map = mapped_name_for_edge(pad_pf.Shape, pf_edge)
    print("TESTS pick PartFilletPad.EdgeLinks = PadPartFillet.", pf_edge, "mapped=", pf_map)
    add_part_fillet(doc, "PartFilletPad", pad_pf, pf_edge)

    body_ba, _sk_ba, pad_ba = make_pad_body(
        doc, "BodyPadBoolA", "SketchPadBoolA", "PadBoolA", refine=True, recomputes=1
    )
    body_bb, _sk_bb, pad_bb = make_pad_body(
        doc, "BodyPadBoolB", "SketchPadBoolB", "PadBoolB", refine=True, recomputes=1,
        corner=(6.0, 0.0),
    )
    fuse = doc.addObject("Part::Fuse", "FusePads")
    fuse.Base = pad_ba
    fuse.Tool = pad_bb
    try:
        fuse.Refine = True
    except Exception:
        pass
    doc.recompute()
    require_ok(fuse, "FusePads")
    fuse_com = shape_center_of_mass(fuse.Shape)
    fuse_edge = pick_vertical_outer_edge(fuse.Shape, (fuse_com.x, fuse_com.y))
    fuse_map = mapped_name_for_edge(fuse.Shape, fuse_edge)
    print("TESTS pick PartFilletFuse.EdgeLinks = FusePads.", fuse_edge, "mapped=", fuse_map)
    add_part_fillet(doc, "PartFilletFuse", fuse, fuse_edge)


    print("TESTS Loop 5 Part WB Extrusion first isolate")
    extrusion_face = doc.addObject("Part::Feature", "ExtrusionFace")
    extrusion_wire = Part.makePolygon([
        App.Vector(0.0, 0.0, 0.0),
        App.Vector(10.0, 0.0, 0.0),
        App.Vector(10.0, 10.0, 0.0),
        App.Vector(0.0, 10.0, 0.0),
        App.Vector(0.0, 0.0, 0.0),
    ])
    extrusion_face.Shape = Part.Face(extrusion_wire)
    extrusion = doc.addObject("Part::Extrusion", "PartExtrusion")
    extrusion.Base = extrusion_face
    extrusion.Dir = App.Vector(0.0, 0.0, 10.0)
    extrusion.LengthFwd = 10.0
    extrusion.Solid = True
    doc.recompute()
    require_ok(extrusion, "PartExtrusion")
    extrusion_com = shape_center_of_mass(extrusion.Shape)
    extrusion_edge = pick_vertical_outer_edge(
        extrusion.Shape, (extrusion_com.x, extrusion_com.y)
    )
    extrusion_map = mapped_name_for_edge(extrusion.Shape, extrusion_edge)
    print(
        "TESTS pick PartFilletExtrusion.EdgeLinks = PartExtrusion.",
        extrusion_edge,
        "mapped=",
        extrusion_map,
    )
    add_part_fillet(doc, "PartFilletExtrusion", extrusion, extrusion_edge)

    print("TESTS Loop 5 Part WB Revolution isolate")
    revolution_face = doc.addObject("Part::Feature", "PartRevolutionFace")
    revolution_wire = Part.makePolygon([
        App.Vector(15.0, 0.0, 0.0),
        App.Vector(20.0, 0.0, 0.0),
        App.Vector(20.0, 0.0, 8.0),
        App.Vector(15.0, 0.0, 8.0),
        App.Vector(15.0, 0.0, 0.0),
    ])
    revolution_face.Shape = Part.Face(revolution_wire)
    revolution = doc.addObject("Part::Revolution", "PartRevolution")
    revolution.Source = revolution_face
    revolution.Base = App.Vector(0.0, 0.0, 0.0)
    revolution.Axis = App.Vector(0.0, 0.0, 1.0)
    revolution.Angle = 360.0
    revolution.Solid = True
    doc.recompute()
    require_ok(revolution, "PartRevolution")
    revolution_edge = pick_outer_circular_edge(revolution.Shape)
    revolution_map = mapped_name_for_edge(revolution.Shape, revolution_edge)
    print(
        "TESTS pick PartFilletRevolution.EdgeLinks = PartRevolution.",
        revolution_edge,
        "mapped=",
        revolution_map,
    )
    add_part_fillet(doc, "PartFilletRevolution", revolution, revolution_edge, radius=0.5)

    print("TESTS Loop 5 Part WB Mirroring isolate")
    mirroring = doc.addObject("Part::Mirroring", "PartMirroring")
    mirroring.Source = extrusion
    mirroring.Base = App.Vector(20.0, 0.0, 0.0)
    mirroring.Normal = App.Vector(1.0, 0.0, 0.0)
    doc.recompute()
    require_ok(mirroring, "PartMirroring")
    mirroring_com = shape_center_of_mass(mirroring.Shape)
    mirroring_edge = pick_vertical_outer_edge(
        mirroring.Shape, (mirroring_com.x, mirroring_com.y)
    )
    mirroring_map = mapped_name_for_edge(mirroring.Shape, mirroring_edge)
    print(
        "TESTS pick PartFilletMirroring.EdgeLinks = PartMirroring.",
        mirroring_edge,
        "mapped=",
        mirroring_map,
    )
    add_part_fillet(doc, "PartFilletMirroring", mirroring, mirroring_edge, radius=0.5)
    body_x, _sk_x, pad_x = make_pad_body(
        doc, "BodyPadXLink", "SketchPadXLink", "PadXLink", refine=True, recomputes=1
    )
    x_face = face_name_max_z(pad_x.Shape)
    print("TESTS pick XLinkHolder.Ref = PadXLink.", x_face)
    holder = doc.addObject("App::FeaturePython", "XLinkHolder")
    holder.addProperty("App::PropertyXLinkSub", "Ref")
    holder.Ref = (pad_x, [x_face])
    doc.recompute()

    body_sp, _sk_sp, pad_sp = make_pad_body(
        doc, "BodyPadSpread", "SketchPadSpread", "PadSpread", refine=True, recomputes=1
    )
    sp_face = face_name_max_z(pad_sp.Shape)
    print("TESTS pick SpreadFace.A1 = PadSpread.Shape.%s.Area" % sp_face)
    try:
        import Spreadsheet  # noqa: F401
    except Exception as exc:
        raise RuntimeError("Spreadsheet WB must import in FreeCADCmd: %s: %s" % (type(exc).__name__, exc))
    sheet = doc.addObject("Spreadsheet::Sheet", "SpreadFace")
    sheet.set("A1", "=PadSpread.Shape.%s.Area" % sp_face)
    doc.recompute()

    body_fb, _sk_fb, pad_fb = make_pad_body(
        doc, "BodyPadFacebinder", "SketchPadFacebinder", "PadFacebinder", refine=True, recomputes=1
    )
    fb_face = face_name_max_z(pad_fb.Shape)
    print("TESTS pick FacebinderPad.Faces = PadFacebinder.", fb_face)
    try:
        from draftobjects.facebinder import Facebinder as _Facebinder
    except Exception as exc:
        raise RuntimeError("Draft Facebinder must import in FreeCADCmd: %s: %s" % (type(exc).__name__, exc))
    fb = doc.addObject("Part::FeaturePython", "FacebinderPad")
    _Facebinder(fb)
    fb.Faces = [(pad_fb, (fb_face,))]
    doc.recompute()

    body_td, _sk_td, pad_td = make_pad_body(
        doc, "BodyPadTdDim", "SketchPadTdDim", "PadTdDim", refine=True, recomputes=1
    )
    td_com = pad_td.Shape.CenterOfMass
    td_edge = pick_vertical_outer_edge(pad_td.Shape, (td_com.x, td_com.y))
    print("TESTS pick TdDim.References3D = PadTdDim.", td_edge)
    try:
        import TechDraw  # noqa: F401
    except Exception as exc:
        raise RuntimeError("TechDraw WB must import in FreeCADCmd: %s: %s" % (type(exc).__name__, exc))
    dim = doc.addObject("TechDraw::DrawViewDimension", "TdDim")
    try:
        dim.setPropertyStatus("References3D", "-ReadOnly")
    except Exception:
        pass
    try:
        dim.References3D = [(pad_td, td_edge)]
    except Exception:
        dim.References3D = [(pad_td, (td_edge,))]
    doc.recompute()

    body_th, _sk_th, pad_th = make_pad_body(
        doc, "BodyPadTdHatch", "SketchPadTdHatch", "PadTdHatch", refine=True, recomputes=1
    )
    th_face = face_name_max_z(pad_th.Shape)
    print("TESTS pick TdHatch.Source = PadTdHatch.", th_face)
    hatch = doc.addObject("TechDraw::DrawHatch", "TdHatch")
    hatch.Source = (pad_th, [th_face])
    doc.recompute()

    body_aj, _sk_aj, pad_aj = make_pad_body(
        doc, "BodyPadAsmJoint", "SketchPadAsmJoint", "PadAsmJoint", refine=True, recomputes=1
    )
    aj_face = face_name_max_z(pad_aj.Shape)
    print("TESTS pick AsmJoint.Reference1 = PadAsmJoint.", aj_face)
    try:
        import JointObject as _JointObject
    except Exception as exc:
        raise RuntimeError(
            "Assembly JointObject must import in FreeCADCmd: %s: %s" % (type(exc).__name__, exc)
        )
    assembly_aj = doc.addObject("Assembly::AssemblyObject", "AssemblyJointRoot")
    jointgroup_aj = assembly_aj.newObject("Assembly::JointGroup", "JointsAsm")
    try:
        assembly_aj.addObject(body_aj)
    except Exception:
        pass
    joint_aj = jointgroup_aj.newObject("App::FeaturePython", "AsmJoint")
    _JointObject.Joint(joint_aj, 0)
    # Headless consume path (TestCore pattern): PropertyXLinkSub dual-write only.
    # Detach avoids Placement solve; Reference2 left unset (findPlacement tolerates None).
    joint_aj.Detach1 = True
    joint_aj.Reference1 = (pad_aj, [aj_face])
    doc.recompute()

    # Body-tip projection: user-facing cache is Body.FaceN; seed comes from Tip Pad.
    body_bt, _sk_bt, pad_bt = make_pad_body(
        doc, "BodyTipProj", "SketchBodyTip", "PadBodyTip", refine=True, recomputes=1
    )
    bt_face = face_name_max_z(pad_bt.Shape)
    print("TESTS pick BodyTipHolder.Ref = BodyTipProj.", bt_face)
    holder_bt = doc.addObject("App::FeaturePython", "BodyTipHolder")
    holder_bt.addProperty("App::PropertyXLinkSub", "Ref")
    holder_bt.Ref = (body_bt, [bt_face])
    doc.recompute()

    print("TESTS isolate-6 Mirrored unpark: LinearPattern setup order; full Loop 2")
    body_lp, sk_lp, pad_lp = make_pad_body(
        doc, "BodyPadLinear", "SketchPadLinear", "PadLinear", refine=True, recomputes=1
    )
    lp = add_linear_pattern(doc, body_lp, "LinearPatternPad", pad_lp, sk_lp)
    lp_com = shape_center_of_mass(pad_lp.Shape)
    lp_edge = pick_vertical_outer_edge(lp.Shape, (lp_com.x, lp_com.y), skip_pad_xtr=True)
    lp_map = mapped_name_for_edge(lp.Shape, lp_edge)
    print("TESTS pick FilletLinear.Base = LinearPatternPad.", lp_edge, "mapped=", lp_map)
    add_fillet(doc, body_lp, "FilletLinear", lp, lp_edge)

    body_pp, sk_pp, pad_pp = make_pad_body(
        doc, "BodyPadPolar", "SketchPadPolar", "PadPolar", refine=True, recomputes=1,
        corner=(20.0, 0.0),
    )
    pp = add_polar_pattern(doc, body_pp, "PolarPatternPad", pad_pp, sk_pp)
    pp_com = shape_center_of_mass(pad_pp.Shape)
    pp_edge = pick_vertical_outer_edge(pp.Shape, (pp_com.x, pp_com.y), skip_pad_xtr=True)
    pp_map = mapped_name_for_edge(pp.Shape, pp_edge)
    print("TESTS pick FilletPolar.Base = PolarPatternPad.", pp_edge, "mapped=", pp_map)
    add_fillet(doc, body_pp, "FilletPolar", pp, pp_edge)

    body_mi, sk_mi, pad_mi = make_pad_body(
        doc, "BodyPadMirror", "SketchPadMirror", "PadMirror", refine=True, recomputes=1,
        corner=(10.0, 0.0),
    )
    mi = add_mirrored(doc, body_mi, "MirroredPad", pad_mi, sk_mi)
    mi_com = shape_center_of_mass(pad_mi.Shape)
    mi_edge = pick_vertical_outer_edge(mi.Shape, (mi_com.x, mi_com.y), skip_pad_xtr=True)
    mi_map = mapped_name_for_edge(mi.Shape, mi_edge)
    print("TESTS pick FilletMirror.Base = MirroredPad.", mi_edge, "mapped=", mi_map)
    add_fillet(doc, body_mi, "FilletMirror", mi, mi_edge)

    print("TESTS isolate Wave 4 Scaled and MultiTransform semantic emit")
    body_scaled, sk_scaled, pad_scaled = make_pad_body(
        doc, "BodyPadScaled", "SketchPadScaled", "PadScaled", refine=True, recomputes=1,
        corner=(30.0, 0.0),
    )
    scaled = add_scaled(doc, body_scaled, "ScaledPad", pad_scaled)
    scaled_com = shape_center_of_mass(scaled.Shape)
    scaled_edge = pick_vertical_outer_edge(scaled.Shape, (scaled_com.x, scaled_com.y), skip_pad_xtr=True)
    scaled_map = mapped_name_for_edge(scaled.Shape, scaled_edge)
    print("TESTS pick FilletScaled.Base = ScaledPad.", scaled_edge, "mapped=", scaled_map)
    add_fillet(doc, body_scaled, "FilletScaled", scaled, scaled_edge)

    body_multi, sk_multi, pad_multi = make_pad_body(
        doc, "BodyPadMultiTransform", "SketchPadMultiTransform", "PadMultiTransform",
        refine=True, recomputes=1, corner=(50.0, 0.0),
    )
    multi = add_multi_transform_scaled(doc, body_multi, "MultiTransformPad", pad_multi)
    multi_com = shape_center_of_mass(multi.Shape)
    multi_edge = pick_vertical_outer_edge(multi.Shape, (multi_com.x, multi_com.y), skip_pad_xtr=True)
    multi_map = mapped_name_for_edge(multi.Shape, multi_edge)
    print("TESTS pick FilletMultiTransform.Base = MultiTransformPad.", multi_edge, "mapped=", multi_map)
    add_fillet(doc, body_multi, "FilletMultiTransform", multi, multi_edge)

    print("TESTS isolate Loop 3 Helix unpark; Loft/Pipe retained")
    body_loft = doc.addObject("PartDesign::Body", "BodyLoft")
    sketch_loft_p = doc.addObject("Sketcher::SketchObject", "SketchLoftProfile")
    sketch_loft_p.AttachmentSupport = (body_xy_plane(body_loft), [""])
    sketch_loft_p.MapMode = "FlatFace"
    body_loft.addObject(sketch_loft_p)
    add_rectangle(sketch_loft_p, (0.0, 0.0), (10.0, 10.0))
    doc.recompute()
    sketch_loft_s = doc.addObject("Sketcher::SketchObject", "SketchLoftSection")
    body_loft.addObject(sketch_loft_s)
    sketch_loft_s.MapMode = "FlatFace"
    sketch_loft_s.AttachmentSupport = (body_xz_plane(body_loft), [""])
    doc.recompute()
    add_rectangle(sketch_loft_s, (0.0, 10.0), (10.0, 10.0))
    doc.recompute()
    loft = add_additive_loft(doc, body_loft, "AdditiveLoft", sketch_loft_p, sketch_loft_s)
    loft_edge = pick_loop3_edge(loft.Shape)
    loft_map = mapped_name_for_edge(loft.Shape, loft_edge)
    print("TESTS pick FilletLoft.Base = AdditiveLoft.", loft_edge, "mapped=", loft_map)
    add_fillet(doc, body_loft, "FilletLoft", loft, loft_edge)

    body_pipe = doc.addObject("PartDesign::Body", "BodyPipe")
    sketch_pipe_p = doc.addObject("Sketcher::SketchObject", "SketchPipeProfile")
    sketch_pipe_p.AttachmentSupport = (body_xy_plane(body_pipe), [""])
    sketch_pipe_p.MapMode = "FlatFace"
    body_pipe.addObject(sketch_pipe_p)
    add_rectangle(sketch_pipe_p, (0.0, 0.0), (10.0, 10.0))
    doc.recompute()
    sketch_pipe_s = doc.addObject("Sketcher::SketchObject", "SketchPipeSpine")
    body_pipe.addObject(sketch_pipe_s)
    sketch_pipe_s.MapMode = "FlatFace"
    sketch_pipe_s.AttachmentSupport = (body_xz_plane(body_pipe), [""])
    doc.recompute()
    sketch_pipe_s.addGeometry(
        Part.LineSegment(App.Vector(0.0, 0.0, 0), App.Vector(0, 20.0, 0)), False
    )
    sketch_pipe_s.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
    sketch_pipe_s.addConstraint(Sketcher.Constraint("PointOnObject", 0, 2, -2))
    sketch_pipe_s.addConstraint(Sketcher.Constraint("DistanceY", 0, 1, 0, 2, 20.0))
    doc.recompute()
    pipe = add_additive_pipe(doc, body_pipe, "AdditivePipe", sketch_pipe_p, sketch_pipe_s)
    pipe_edge = pick_loop3_edge(pipe.Shape)
    pipe_map = mapped_name_for_edge(pipe.Shape, pipe_edge)
    print("TESTS pick FilletPipe.Base = AdditivePipe.", pipe_edge, "mapped=", pipe_map)
    add_fillet(doc, body_pipe, "FilletPipe", pipe, pipe_edge)

    body_helix = doc.addObject("PartDesign::Body", "BodyHelix")
    sketch_helix = doc.addObject("Sketcher::SketchObject", "SketchHelix")
    sketch_helix.AttachmentSupport = (body_xy_plane(body_helix), [""])
    sketch_helix.MapMode = "FlatFace"
    body_helix.addObject(sketch_helix)
    add_rectangle(sketch_helix, (0.0, 0.0), (5.0, 5.0))
    doc.recompute()
    helix = add_additive_helix(doc, body_helix, "AdditiveHelix", sketch_helix, sketch_helix)
    helix_edge = pick_loop3_edge(helix.Shape)
    helix_map = mapped_name_for_edge(helix.Shape, helix_edge)
    print("TESTS pick FilletHelix.Base = AdditiveHelix.", helix_edge, "mapped=", helix_map)
    add_fillet(doc, body_helix, "FilletHelix", helix, helix_edge)

    print("TESTS isolate Loop 4 Fuse; Box/Cut/Common parked")
    body_pd_a, _sk_pd_a, pad_pd_a = make_pad_body(
        doc, "BodyPDBoolA", "SketchPDBoolA", "PadPDBoolA", refine=False, recomputes=1
    )
    body_pd_b, _sk_pd_b, pad_pd_b = make_pad_body(
        doc, "BodyPDBoolB", "SketchPDBoolB", "PadPDBoolB", refine=False, recomputes=1,
        corner=(6.0, 0.0),
    )
    pd_bool = doc.addObject("PartDesign::Boolean", "BooleanFusePads")
    body_pd_a.addObject(pd_bool)
    pd_bool.Type = "Fuse"
    try:
        pd_bool.Refine = False
    except Exception:
        pass
    pd_bool.Group = [body_pd_b]
    doc.recompute()
    require_ok(pd_bool, "BooleanFusePads")
    pd_com = shape_center_of_mass(pd_bool.Shape)
    pd_edge = pick_vertical_outer_edge(pd_bool.Shape, (pd_com.x, pd_com.y), skip_pad_xtr=True)
    pd_map = mapped_name_for_edge(pd_bool.Shape, pd_edge)
    print("TESTS pick FilletPDBoolean.Base = BooleanFusePads.", pd_edge, "mapped=", pd_map)
    add_fillet(doc, body_pd_a, "FilletPDBoolean", pd_bool, pd_edge)

    print("TESTS isolate AdditiveBox first-solid; Cut/Common/Cylinder parked")
    body_box = doc.addObject("PartDesign::Body", "BodyAdditiveBox")
    box = doc.addObject("PartDesign::AdditiveBox", "AdditiveBox")
    body_box.addObject(box)
    box.Length = 10.0
    box.Width = 10.0
    box.Height = 10.0
    try:
        box.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(box, "AdditiveBox")
    box_com = shape_center_of_mass(box.Shape)
    box_edge = pick_vertical_outer_edge(box.Shape, (box_com.x, box_com.y))
    box_map = mapped_name_for_edge(box.Shape, box_edge)
    print("TESTS pick FilletAdditiveBox.Base = AdditiveBox.", box_edge, "mapped=", box_map)
    add_fillet(doc, body_box, "FilletAdditiveBox", box, box_edge)

    print("TESTS isolate AdditiveCylinder first-solid retained")
    body_cyl = doc.addObject("PartDesign::Body", "BodyAdditiveCylinder")
    cyl = doc.addObject("PartDesign::AdditiveCylinder", "AdditiveCylinder")
    body_cyl.addObject(cyl)
    cyl.Radius = 5.0
    cyl.Height = 10.0
    cyl.Angle = 360.0
    try:
        cyl.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(cyl, "AdditiveCylinder")
    cyl_edge = pick_cylinder_unique_edge(cyl.Shape)
    cyl_map = mapped_name_for_edge(cyl.Shape, cyl_edge)
    print("TESTS pick FilletAdditiveCylinder.Base = AdditiveCylinder.", cyl_edge, "mapped=", cyl_map)
    add_fillet(doc, body_cyl, "FilletAdditiveCylinder", cyl, cyl_edge)

    print("TESTS isolate Boolean Cut retained; Common next")
    body_cut_a, _sk_cut_a, pad_cut_a = make_pad_body(
        doc, "BodyPDCutA", "SketchPDCutA", "PadPDCutA", refine=False, recomputes=1
    )
    body_cut_b, _sk_cut_b, pad_cut_b = make_pad_body(
        doc, "BodyPDCutB", "SketchPDCutB", "PadPDCutB", refine=False, recomputes=1,
        corner=(6.0, 0.0),
    )
    pd_cut = doc.addObject("PartDesign::Boolean", "BooleanCutPads")
    body_cut_a.addObject(pd_cut)
    pd_cut.Type = "Cut"
    try:
        pd_cut.Refine = False
    except Exception:
        pass
    pd_cut.Group = [body_cut_b]
    doc.recompute()
    require_ok(pd_cut, "BooleanCutPads")
    cut_com = shape_center_of_mass(pd_cut.Shape)
    cut_edge = pick_vertical_outer_edge(pd_cut.Shape, (cut_com.x, cut_com.y), skip_pad_xtr=True)
    cut_map = mapped_name_for_edge(pd_cut.Shape, cut_edge)
    print("TESTS pick FilletPDCut.Base = BooleanCutPads.", cut_edge, "mapped=", cut_map)
    add_fillet(doc, body_cut_a, "FilletPDCut", pd_cut, cut_edge)

    print("TESTS isolate Boolean Common retained; Sphere next")
    # Substantial overlap (more than Cut corner=6): Common needs non-empty intersection volume.
    body_com_a, _sk_com_a, pad_com_a = make_pad_body(
        doc, "BodyPDCommonA", "SketchPDCommonA", "PadPDCommonA", refine=False, recomputes=1
    )
    body_com_b, _sk_com_b, pad_com_b = make_pad_body(
        doc, "BodyPDCommonB", "SketchPDCommonB", "PadPDCommonB", refine=False, recomputes=1,
        corner=(3.0, 0.0),
    )
    pd_common = doc.addObject("PartDesign::Boolean", "BooleanCommonPads")
    body_com_a.addObject(pd_common)
    pd_common.Type = "Common"
    try:
        pd_common.Refine = False
    except Exception:
        pass
    pd_common.Group = [body_com_b]
    doc.recompute()
    require_ok(pd_common, "BooleanCommonPads")
    common_com = shape_center_of_mass(pd_common.Shape)
    common_edge = pick_vertical_outer_edge(
        pd_common.Shape, (common_com.x, common_com.y), skip_pad_xtr=True
    )
    common_map = mapped_name_for_edge(pd_common.Shape, common_edge)
    print(
        "TESTS pick FilletPDCommon.Base = BooleanCommonPads.",
        common_edge,
        "mapped=",
        common_map,
    )
    add_fillet(doc, body_com_a, "FilletPDCommon", pd_common, common_edge)

    print("TESTS isolate AdditiveSphere first-solid retained")
    # Hemisphere (Angle2=0): unique equatorial circular rim for Fillet.
    # Full sphere poles/meridian seam are often non-unique or not fillet-able.
    body_sph = doc.addObject("PartDesign::Body", "BodyAdditiveSphere")
    sph = doc.addObject("PartDesign::AdditiveSphere", "AdditiveSphere")
    body_sph.addObject(sph)
    sph.Radius = 5.0
    sph.Angle1 = -90.0
    sph.Angle2 = 0.0
    sph.Angle3 = 360.0
    try:
        sph.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(sph, "AdditiveSphere")
    sph_edge = pick_sphere_unique_edge(sph.Shape)
    sph_map = mapped_name_for_edge(sph.Shape, sph_edge)
    print("TESTS pick FilletAdditiveSphere.Base = AdditiveSphere.", sph_edge, "mapped=", sph_map)
    add_fillet(doc, body_sph, "FilletAdditiveSphere", sph, sph_edge)

    print("TESTS isolate AdditiveCone first-solid retained")
    # Truncated cone (Radius1 != Radius2, both > 0): two circular rims;
    # pick unique outer via pick_cone_unique_edge (skips _is_closed_cone_seam).
    body_cone = doc.addObject("PartDesign::Body", "BodyAdditiveCone")
    cone = doc.addObject("PartDesign::AdditiveCone", "AdditiveCone")
    body_cone.addObject(cone)
    cone.Radius1 = 3.0
    cone.Radius2 = 6.0
    cone.Height = 10.0
    cone.Angle = 360.0
    try:
        cone.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(cone, "AdditiveCone")
    cone_edge = pick_cone_unique_edge(cone.Shape)
    cone_map = mapped_name_for_edge(cone.Shape, cone_edge)
    print("TESTS pick FilletAdditiveCone.Base = AdditiveCone.", cone_edge, "mapped=", cone_map)
    add_fillet(doc, body_cone, "FilletAdditiveCone", cone, cone_edge)

    print("TESTS isolate AdditiveTorus first-solid retained")
    # Partial torus (Angle3 < 360): unique tube cross-section end circles for
    # Fillet. Full 360° meridian seam may be non-unique or not fillet-able.
    # pick_torus_unique_edge prefers smallest Circle radius (tube Radius2).
    body_tor = doc.addObject("PartDesign::Body", "BodyAdditiveTorus")
    tor = doc.addObject("PartDesign::AdditiveTorus", "AdditiveTorus")
    body_tor.addObject(tor)
    tor.Radius1 = 10.0
    tor.Radius2 = 2.0
    tor.Angle1 = -180.0
    tor.Angle2 = 180.0
    tor.Angle3 = 180.0
    try:
        tor.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(tor, "AdditiveTorus")
    tor_edge = pick_torus_unique_edge(tor.Shape)
    tor_map = mapped_name_for_edge(tor.Shape, tor_edge)
    print("TESTS pick FilletAdditiveTorus.Base = AdditiveTorus.", tor_edge, "mapped=", tor_map)
    add_fillet(doc, body_tor, "FilletAdditiveTorus", tor, tor_edge)

    print("TESTS isolate AdditivePrism first-solid retained")
    # Regular hexagon prism (polygonal like Box): vertical outer edges via
    # pick_vertical_outer_edge (same helper as AdditiveBox). Centered prism
    # verticals are equidistant from COM — max picks first farthest (stable).
    # Avoid ambiguous multi-image edges (I13).
    body_pri = doc.addObject("PartDesign::Body", "BodyAdditivePrism")
    pri = doc.addObject("PartDesign::AdditivePrism", "AdditivePrism")
    body_pri.addObject(pri)
    pri.Polygon = 6
    pri.Circumradius = 5.0
    pri.Height = 10.0
    try:
        pri.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(pri, "AdditivePrism")
    pri_com = shape_center_of_mass(pri.Shape)
    pri_edge = pick_vertical_outer_edge(pri.Shape, (pri_com.x, pri_com.y))
    pri_map = mapped_name_for_edge(pri.Shape, pri_edge)
    print("TESTS pick FilletAdditivePrism.Base = AdditivePrism.", pri_edge, "mapped=", pri_map)
    add_fillet(doc, body_pri, "FilletAdditivePrism", pri, pri_edge)

    print("TESTS isolate AdditiveWedge first-solid retained")
    # Non-degenerate tapered wedge (all faces present). OCCT MakeWedge Ax2 Z-up:
    # bottom XZ at Ymin (Xmax×Zmax), top XZ at Ymax (X2max×Z2max).
    # Params: Xmin/Ymin/Zmin=0; Xmax=Ymax=Zmax=10; X2min=Z2min=0; X2max=5; Z2max=10.
    # Vertical edges (||Z) at (0,0), (10,0), (0,10), (5,10) — unique farthest from
    # COM via pick_vertical_outer_edge (same helper as Box/Prism). Avoid I13 multi-image.
    body_wed = doc.addObject("PartDesign::Body", "BodyAdditiveWedge")
    wed = doc.addObject("PartDesign::AdditiveWedge", "AdditiveWedge")
    body_wed.addObject(wed)
    wed.Xmin = 0.0
    wed.Ymin = 0.0
    wed.Zmin = 0.0
    wed.Xmax = 10.0
    wed.Ymax = 10.0
    wed.Zmax = 10.0
    wed.X2min = 0.0
    wed.Z2min = 0.0
    wed.X2max = 5.0
    wed.Z2max = 10.0
    try:
        wed.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(wed, "AdditiveWedge")
    wed_com = shape_center_of_mass(wed.Shape)
    wed_edge = pick_vertical_outer_edge(wed.Shape, (wed_com.x, wed_com.y))
    wed_map = mapped_name_for_edge(wed.Shape, wed_edge)
    print("TESTS pick FilletAdditiveWedge.Base = AdditiveWedge.", wed_edge, "mapped=", wed_map)
    add_fillet(doc, body_wed, "FilletAdditiveWedge", wed, wed_edge)

    print("TESTS isolate AdditiveEllipsoid first-solid")
    # Hemisphere (Angle2=0): unique equatorial circular/elliptical rim for Fillet.
    # Ellipsoid = MakeSphere(Radius2) + GTransform scales (Radius1 Z, Radius3 Y).
    # Radius1 != Radius2 so solid is curved like Sphere but non-spherical; Radius3=0
    # keeps Y=X so equator stays Circle (pick_ellipsoid_unique_edge also accepts Ellipse).
    body_ell = doc.addObject("PartDesign::Body", "BodyAdditiveEllipsoid")
    ell = doc.addObject("PartDesign::AdditiveEllipsoid", "AdditiveEllipsoid")
    body_ell.addObject(ell)
    ell.Radius1 = 3.0
    ell.Radius2 = 5.0
    ell.Radius3 = 0.0
    ell.Angle1 = -90.0
    ell.Angle2 = 0.0
    ell.Angle3 = 360.0
    try:
        ell.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(ell, "AdditiveEllipsoid")
    ell_edge = pick_ellipsoid_unique_edge(ell.Shape)
    ell_map = mapped_name_for_edge(ell.Shape, ell_edge)
    print("TESTS pick FilletAdditiveEllipsoid.Base = AdditiveEllipsoid.", ell_edge, "mapped=", ell_map)
    add_fillet(doc, body_ell, "FilletAdditiveEllipsoid", ell, ell_edge)


    print("TESTS isolate SubtractiveBox with-base Cut")
    # Base AdditiveBox then overlapping SubtractiveBox (offset pocket). Separate Body;
    # Refine=False. Fillet.Base on SubtractiveBox — allocatedBy must be SubtractiveBox id.
    # AttachmentOffset shifts tool so Cut leaves unique new edges (not Additive leftovers).
    body_sbox = doc.addObject("PartDesign::Body", "BodySubtractiveBox")
    sbox_base = doc.addObject("PartDesign::AdditiveBox", "AdditiveBoxForSub")
    body_sbox.addObject(sbox_base)
    sbox_base.Length = 10.0
    sbox_base.Width = 10.0
    sbox_base.Height = 10.0
    try:
        sbox_base.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(sbox_base, "AdditiveBoxForSub")
    sbox = doc.addObject("PartDesign::SubtractiveBox", "SubtractiveBox")
    body_sbox.addObject(sbox)
    sbox.Length = 6.0
    sbox.Width = 6.0
    sbox.Height = 6.0
    sbox.AttachmentOffset = App.Placement(App.Vector(4.0, 0.0, 0.0), App.Rotation())
    try:
        sbox.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(sbox, "SubtractiveBox")
    sbox_com = shape_center_of_mass(sbox.Shape)
    sbox_edge = pick_vertical_outer_edge(
        sbox.Shape, (sbox_com.x, sbox_com.y), skip_pad_xtr=True
    )
    sbox_map = mapped_name_for_edge(sbox.Shape, sbox_edge)
    print(
        "TESTS pick FilletSubtractiveBox.Base = SubtractiveBox.",
        sbox_edge,
        "mapped=",
        sbox_map,
    )

    print("TESTS isolate SubtractiveCylinder with-base Cut")
    body_scyl = doc.addObject("PartDesign::Body", "BodySubtractiveCylinder")
    scyl_base = doc.addObject("PartDesign::AdditiveBox", "AdditiveBoxForSubCylinder")
    body_scyl.addObject(scyl_base)
    scyl_base.Length = 10.0
    scyl_base.Width = 10.0
    scyl_base.Height = 10.0
    try:
        scyl_base.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(scyl_base, "AdditiveBoxForSubCylinder")
    scyl = doc.addObject("PartDesign::SubtractiveCylinder", "SubtractiveCylinder")
    body_scyl.addObject(scyl)
    scyl.Radius = 2.0
    scyl.Height = 12.0
    scyl.Angle = 360.0
    scyl.AttachmentOffset = App.Placement(App.Vector(5.0, 5.0, -1.0), App.Rotation())
    try:
        scyl.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(scyl, "SubtractiveCylinder")
    scyl_edge = pick_subtractive_cylinder_unique_edge(scyl.Shape)
    scyl_map = mapped_name_for_edge(scyl.Shape, scyl_edge)
    print("TESTS pick FilletSubtractiveCylinder.Base = SubtractiveCylinder.", scyl_edge, "mapped=", scyl_map)
    add_fillet(doc, body_scyl, "FilletSubtractiveCylinder", scyl, scyl_edge)

    print("TESTS isolate SubtractiveSphere with-base Cut")
    body_ssph = doc.addObject("PartDesign::Body", "BodySubtractiveSphere")
    ssph_base = doc.addObject("PartDesign::AdditiveBox", "AdditiveBoxForSubSphere")
    body_ssph.addObject(ssph_base)
    ssph_base.Length = 10.0
    ssph_base.Width = 10.0
    ssph_base.Height = 10.0
    try:
        ssph_base.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(ssph_base, "AdditiveBoxForSubSphere")
    ssph = doc.addObject("PartDesign::SubtractiveSphere", "SubtractiveSphere")
    body_ssph.addObject(ssph)
    ssph.Radius = 4.0
    ssph.Angle1 = -90.0
    ssph.Angle2 = 0.0
    ssph.Angle3 = 360.0
    ssph.AttachmentOffset = App.Placement(App.Vector(5.0, 5.0, 8.0), App.Rotation())
    try:
        ssph.Refine = False
    except Exception:
        pass
    doc.recompute()
    require_ok(ssph, "SubtractiveSphere")
    ssph_edge = pick_subtractive_sphere_unique_edge(ssph.Shape)
    ssph_map = mapped_name_for_edge(ssph.Shape, ssph_edge)
    print("TESTS pick FilletSubtractiveSphere.Base = SubtractiveSphere.", ssph_edge, "mapped=", ssph_map)
    add_fillet(doc, body_ssph, "FilletSubtractiveSphere", ssph, ssph_edge)

    add_fillet(doc, body_sbox, "FilletSubtractiveBox", sbox, sbox_edge)

    print("TESTS isolate remaining subtractive primitives through shared Cut")
    make_subtractive_primitive(
        doc, "BodySubtractiveCone", "AdditiveBoxForSubCone",
        "SubtractiveCone", "SubtractiveCone",
        {"Radius1": 2.0, "Radius2": 3.0, "Height": 6.0, "Angle": 360.0,
         "AttachmentOffset": App.Placement(App.Vector(5.0, 5.0, 2.0), App.Rotation())},
        pick_cone_unique_edge,
    )
    make_subtractive_primitive(
        doc, "BodySubtractiveTorus", "AdditiveBoxForSubTorus",
        "SubtractiveTorus", "SubtractiveTorus",
        {"Radius1": 3.0, "Radius2": 1.0, "Angle1": -180.0, "Angle2": 180.0,
         "Angle3": 180.0,
         "AttachmentOffset": App.Placement(App.Vector(5.0, 5.0, 5.0), App.Rotation())},
        pick_subtractive_torus_unique_edge,
        fillet_radius=0.3,
    )
    make_subtractive_primitive(
        doc, "BodySubtractivePrism", "AdditiveBoxForSubPrism",
        "SubtractivePrism", "SubtractivePrism",
        {"Polygon": 6, "Circumradius": 3.0, "Height": 6.0,
         "AttachmentOffset": App.Placement(App.Vector(5.0, 5.0, 2.0), App.Rotation())},
        lambda shape: pick_vertical_outer_edge(shape, (5.0, 5.0)),
    )
    make_subtractive_primitive(
        doc, "BodySubtractiveWedge", "AdditiveBoxForSubWedge",
        "SubtractiveWedge", "SubtractiveWedge",
        {"Xmin": 0.0, "Ymin": 0.0, "Zmin": 0.0,
         "Xmax": 6.0, "Ymax": 6.0, "Zmax": 6.0,
         "X2min": 0.0, "Z2min": 0.0, "X2max": 3.0, "Z2max": 6.0,
         "AttachmentOffset": App.Placement(App.Vector(2.0, 2.0, 2.0), App.Rotation())},
        lambda shape: pick_vertical_outer_edge(shape, (5.0, 5.0)),
    )
    make_subtractive_primitive(
        doc, "BodySubtractiveEllipsoid", "AdditiveBoxForSubEllipsoid",
        "SubtractiveEllipsoid", "SubtractiveEllipsoid",
        {"Radius1": 2.0, "Radius2": 3.0, "Radius3": 0.0,
         "Angle1": -90.0, "Angle2": 0.0, "Angle3": 360.0,
         "AttachmentOffset": App.Placement(App.Vector(5.0, 5.0, 6.0), App.Rotation())},
        pick_subtractive_cut_named_edge,
    )
    print("TESTS isolate subtractive Loft/Pipe/Helix semantic emit")
    body_sloft, _sk_sloft, _base_sloft = make_pad_body(
        doc, "BodySubtractiveLoft", "SketchSubtractiveLoftBase", "PadSubtractiveLoft",
        refine=False, recomputes=1
    )
    sloft_profile = add_sweep_sketch(
        doc, body_sloft, "SketchSubtractiveLoftProfile", body_xy_plane(body_sloft), (1.0, 1.0), (7.0, 7.0)
    )
    sloft_section = add_sweep_sketch(
        doc, body_sloft, "SketchSubtractiveLoftSection", body_xz_plane(body_sloft), (1.0, 4.0), (7.0, 10.0)
    )
    sloft = add_subtractive_loft(
        doc, body_sloft, "SubtractiveLoft", sloft_profile, sloft_section, _base_sloft
    )
    sloft_edge = pick_loop3_edge(sloft.Shape)
    sloft_map = mapped_name_for_edge(sloft.Shape, sloft_edge)
    print("TESTS pick FilletSubtractiveLoft.Base = SubtractiveLoft.", sloft_edge, "mapped=", sloft_map)
    add_fillet(doc, body_sloft, "FilletSubtractiveLoft", sloft, sloft_edge)

    body_spipe, _sk_spipe, _base_spipe = make_pad_body(
        doc, "BodySubtractivePipe", "SketchSubtractivePipeBase", "PadSubtractivePipe",
        refine=False, recomputes=1
    )
    # Circle profile + line spine (TestPipe-class): rectangle sweep is ~zero-volume
    # invalid for with-base Cut boolean on Windows OCCT.
    spipe_profile = doc.addObject("Sketcher::SketchObject", "SketchSubtractivePipeProfile")
    body_spipe.addObject(spipe_profile)
    spipe_profile.AttachmentSupport = (body_xy_plane(body_spipe), [""])
    spipe_profile.MapMode = "FlatFace"
    add_circle(spipe_profile, (5.0, 5.0), 1.5)
    doc.recompute()
    spipe_spine = doc.addObject("Sketcher::SketchObject", "SketchSubtractivePipeSpine")
    body_spipe.addObject(spipe_spine)
    spipe_spine.MapMode = "FlatFace"
    spipe_spine.AttachmentSupport = (body_xz_plane(body_spipe), [""])
    doc.recompute()
    spipe_spine.addGeometry(
        Part.LineSegment(App.Vector(5.0, 0.0, 0), App.Vector(5.0, 10.0, 0)), False
    )
    spipe_spine.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
    spipe_spine.addConstraint(Sketcher.Constraint("PointOnObject", 0, 2, -2))
    spipe_spine.addConstraint(Sketcher.Constraint("DistanceY", 0, 1, 0, 2, 10.0))
    doc.recompute()
    spipe = add_subtractive_pipe(
        doc, body_spipe, "SubtractivePipe", spipe_profile, spipe_spine, _base_spipe
    )
    spipe_edge = pick_loop3_edge(spipe.Shape)
    spipe_map = mapped_name_for_edge(spipe.Shape, spipe_edge)
    print("TESTS pick FilletSubtractivePipe.Base = SubtractivePipe.", spipe_edge, "mapped=", spipe_map)
    add_fillet(doc, body_spipe, "FilletSubtractivePipe", spipe, spipe_edge)

    body_shellix, _sk_shellix, _base_shellix = make_pad_body(
        doc, "BodySubtractiveHelix", "SketchSubtractiveHelixBase", "PadSubtractiveHelix",
        refine=False, recomputes=1
    )
    shellix_profile = add_sweep_sketch(
        doc, body_shellix, "SketchSubtractiveHelixProfile", body_xy_plane(body_shellix), (2.0, 2.0), (5.0, 5.0)
    )
    shellix = add_subtractive_helix(
        doc, body_shellix, "SubtractiveHelix", shellix_profile, shellix_profile, _base_shellix
    )
    shellix_edge = pick_loop3_edge(shellix.Shape)
    shellix_map = mapped_name_for_edge(shellix.Shape, shellix_edge)
    print("TESTS pick FilletSubtractiveHelix.Base = SubtractiveHelix.", shellix_edge, "mapped=", shellix_map)
    add_fillet(doc, body_shellix, "FilletSubtractiveHelix", shellix, shellix_edge)

    body_split, _sk_split, pad_split = make_pad_body(
        doc, "BodyPocketSplit", "SketchPadSplit", "PadSplit", refine=True, recomputes=1
    )
    add_sketch_on_cap_pocket(
        doc, body_split, pad_split, "SketchPocketSplit", "PocketThrough", "ThroughAll", length=12.0
    )

    body_s3, _sk_s3, pad_s3 = make_pad_body(
        doc, "BodyPocketS3", "SketchPadS3", "PadS3", refine=True, recomputes=1
    )
    add_sketch_on_cap_pocket(
        doc, body_s3, pad_s3, "SketchPocketS3", "PocketS3", "Length", length=5.0
    )

    doc.saveAs(pre_path)
    print("TESTS wrote", pre_path)
    objects_pre, stg_pre = parse_fcstd(pre_path)

    score_fillet_seed(
        objects_pre, stg_pre, "Fillet", "Pocket",
        "pocket_tip_seed", "pocket_feature_scope",
    )
    seed_u = score_fillet_seed(
        objects_pre, stg_pre, "FilletPadU", "PadU", "pad_u_outline_seed"
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletPadVert", "PadVert", "pad_vertical_seed"
    )
    n_v, v_detail = count_sketch_vertex_identities(objects_pre, stg_pre, "SketchPadVert")
    check("sketch_vertex_seed", n_v >= 4, v_detail)
    score_fillet_seed(
        objects_pre, stg_pre, "ChamferPadVert", "PadChamfer",
        "chamfer_base_seed", "chamfer_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "DraftPadVert", "PadDraft",
        "draft_base_seed", "draft_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "ThicknessPadCap", "PadThickness",
        "thickness_base_seed", "thickness_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletRevolution", "Revolution",
        "revolution_base_seed", "revolution_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "HolePadCap", "PadHole",
        "hole_start_seed", "hole_feature_scope",
        first_face_link,
    )
    score_fillet_seed(
        objects_pre, stg_pre, "PartFilletPad", "PadPartFillet",
        "part_fillet_seed", "part_fillet_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "PartFilletFuse", "FusePads",
        "part_boolean_seed", "part_boolean_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "PartFilletExtrusion", "PartExtrusion",
        "part_extrusion_seed", "part_extrusion_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "PartFilletRevolution", "PartRevolution",
        "part_revolution_seed", "part_revolution_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "PartFilletMirroring", "PartMirroring",
        "part_mirroring_seed", "part_mirroring_feature_scope",
    )

    att_slots = attachment_support_slots(objects_pre, "Sketch001")
    att_details = []
    att_seed = ""
    for slot in att_slots:
        seed = (slot.get("stSeed") or "").strip()
        objn = slot.get("obj") or ""
        val = slot.get("value") or slot.get("stFallback") or ""
        att_details.append("%s.%s stSeed=%s" % (objn, val, seed or "(none)"))
        if seed:
            att_seed = seed
    check(
        "attachment_support_seed",
        bool(att_seed),
        "; ".join(att_details) or "Sketch001 AttachmentSupport missing / seedless LinkSubList",
    )

    xlink_val, xlink_subs = first_link(objects_pre, "XLinkHolder")
    xsub = xlink_subs[0] if xlink_subs else {}
    xseed = (xsub.get("stSeed") or "").strip()
    xfallback = xsub.get("stFallback") or xsub.get("value") or ""
    xidentities = parse_identities(stg_pre)
    xpad_id = (objects_pre.get("PadXLink") or {}).get("id")
    xhandle = None
    try:
        xhandle = int(xseed, 16) if xseed else None
    except ValueError:
        xhandle = None
    xallocated = xidentities.get(xhandle) if xhandle is not None else None
    check(
        "xlink_sub_seed",
        bool(xseed) and xlink_val == "PadXLink" and xpad_id is not None and xallocated == xpad_id,
        "link=%s sub=%s stSeed=%s allocatedBy=%s expected_id=%s"
        % (xlink_val, xfallback, xseed or "(none)", xallocated, xpad_id),
    )

    score_fillet_seed(
        objects_pre, stg_pre, "AsmJoint", "PadAsmJoint",
        "assembly_joint_seed", "assembly_joint_feature_scope",
        named_link("Reference1"),
    )

    bt_val, bt_subs = first_link(objects_pre, "BodyTipHolder")
    bt_sub = bt_subs[0] if bt_subs else {}
    bt_seed = (bt_sub.get("stSeed") or "").strip()
    bt_fallback = bt_sub.get("stFallback") or bt_sub.get("value") or ""
    bt_ids = parse_identities(stg_pre)
    bt_pad_id = (objects_pre.get("PadBodyTip") or {}).get("id")
    bt_body_id = (objects_pre.get("BodyTipProj") or {}).get("id")
    bt_handle = None
    try:
        bt_handle = int(bt_seed, 16) if bt_seed else None
    except ValueError:
        bt_handle = None
    bt_allocated = bt_ids.get(bt_handle) if bt_handle is not None else None
    check(
        "body_tip_seed",
        bool(bt_seed) and bt_val == "BodyTipProj",
        "link=%s sub=%s stSeed=%s (FaceN cache on Body; seed from Tip)"
        % (bt_val, bt_fallback, bt_seed or "(none)"),
    )
    check(
        "body_tip_feature_scope",
        bool(bt_seed)
        and bt_val == "BodyTipProj"
        and bt_pad_id is not None
        and bt_allocated == bt_pad_id
        and bt_allocated != bt_body_id,
        "link=%s pad_id=%s body_id=%s allocatedBy=%s"
        % (bt_val, bt_pad_id, bt_body_id, bt_allocated),
    )

    spread_rec = objects_pre.get("SpreadFace") or {}
    spread_cells = spread_rec.get("cells") or []
    a1 = {}
    for cell in spread_cells:
        if (cell.get("address") or "") == "A1":
            a1 = cell
            break
    sseed = (a1.get("stSeed") or "").strip()
    sfallback = a1.get("stFallback") or a1.get("content") or ""
    spad_id = (objects_pre.get("PadSpread") or {}).get("id")
    shandle = None
    try:
        shandle = int(sseed, 16) if sseed else None
    except ValueError:
        shandle = None
    sallocated = xidentities.get(shandle) if shandle is not None else None
    check(
        "spreadsheet_face_seed",
        bool(sseed) and spad_id is not None and sallocated == spad_id,
        "cell=A1 content=%s stSeed=%s allocatedBy=%s expected_id=%s"
        % (sfallback, sseed or "(none)", sallocated, spad_id),
    )

    score_fillet_seed(
        objects_pre, stg_pre, "FacebinderPad", "PadFacebinder",
        "draft_facebinder_seed", "draft_facebinder_feature_scope",
        named_link("Faces"),
    )
    score_fillet_seed(
        objects_pre, stg_pre, "TdDim", "PadTdDim",
        "techdraw_dim_seed", "techdraw_dim_feature_scope",
        named_link("References3D"),
    )
    score_fillet_seed(
        objects_pre, stg_pre, "TdHatch", "PadTdHatch",
        "techdraw_hatch_seed", "techdraw_hatch_feature_scope",
        named_link("Source"),
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletLinear", "LinearPatternPad",
        "linear_pattern_seed", "linear_pattern_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletPolar", "PolarPatternPad",
        "polar_pattern_seed", "polar_pattern_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletMirror", "MirroredPad",
        "mirrored_seed", "mirrored_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletScaled", "ScaledPad",
        "scaled_seed", "scaled_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletMultiTransform", "MultiTransformPad",
        "multi_transform_seed", "multi_transform_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletLoft", "AdditiveLoft",
        "loft_seed", "loft_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletPipe", "AdditivePipe",
        "pipe_seed", "pipe_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletHelix", "AdditiveHelix",
        "helix_seed", "helix_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletPDBoolean", "BooleanFusePads",
        "pd_boolean_seed", "pd_boolean_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletAdditiveBox", "AdditiveBox",
        "additive_box_seed", "additive_box_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletAdditiveCylinder", "AdditiveCylinder",
        "additive_cylinder_seed", "additive_cylinder_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletPDCut", "BooleanCutPads",
        "pd_boolean_cut_seed", "pd_boolean_cut_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletPDCommon", "BooleanCommonPads",
        "pd_boolean_common_seed", "pd_boolean_common_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletAdditiveSphere", "AdditiveSphere",
        "additive_sphere_seed", "additive_sphere_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletAdditiveCone", "AdditiveCone",
        "additive_cone_seed", "additive_cone_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletAdditiveTorus", "AdditiveTorus",
        "additive_torus_seed", "additive_torus_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletAdditivePrism", "AdditivePrism",
        "additive_prism_seed", "additive_prism_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletAdditiveWedge", "AdditiveWedge",
        "additive_wedge_seed", "additive_wedge_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletAdditiveEllipsoid", "AdditiveEllipsoid",
        "additive_ellipsoid_seed", "additive_ellipsoid_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletSubtractiveBox", "SubtractiveBox",
        "subtractive_box_seed", "subtractive_box_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletSubtractiveCylinder", "SubtractiveCylinder",
        "subtractive_cylinder_seed", "subtractive_cylinder_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletSubtractiveSphere", "SubtractiveSphere",
        "subtractive_sphere_seed", "subtractive_sphere_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletSubtractiveCone", "SubtractiveCone",
        "subtractive_cone_seed", "subtractive_cone_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletSubtractiveTorus", "SubtractiveTorus",
        "subtractive_torus_seed", "subtractive_torus_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletSubtractivePrism", "SubtractivePrism",
        "subtractive_prism_seed", "subtractive_prism_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletSubtractiveWedge", "SubtractiveWedge",
        "subtractive_wedge_seed", "subtractive_wedge_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletSubtractiveEllipsoid", "SubtractiveEllipsoid",
        "subtractive_ellipsoid_seed", "subtractive_ellipsoid_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletSubtractiveLoft", "SubtractiveLoft",
        "subtractive_loft_seed", "subtractive_loft_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletSubtractivePipe", "SubtractivePipe",
        "subtractive_pipe_seed", "subtractive_pipe_feature_scope",
    )
    score_fillet_seed(
        objects_pre, stg_pre, "FilletSubtractiveHelix", "SubtractiveHelix",
        "subtractive_helix_seed", "subtractive_helix_feature_scope",
    )
    check(
        "elementmap_st_output",
        elementmap_has_st_token(pre_path, "SubtractiveLoft"),
        "SubtractiveLoft Document.xml output contains a semantic ;:ST token",
    )

    fillet_u.Base = (pad_u, [u_edge])
    doc.recompute()
    require_ok(fillet_u, "FilletPadU after C1 setValue")
    c1_path = os.path.join(out_dir, "Tests_Automated_c1.FCStd")
    doc.saveAs(c1_path)
    objects_c1, _stg_c1 = parse_fcstd(c1_path)
    _link, c1_subs = first_link(objects_c1, "FilletPadU")
    seed_c1 = ((c1_subs[0].get("stSeed") if c1_subs else "") or "").strip()
    check(
        "pad_u_c1_keep",
        bool(seed_u) and seed_c1 == seed_u,
        "before=%s after=%s" % (seed_u or "(none)", seed_c1 or "(none)"),

    )

    g = sketch.Geometry[bottom_id]
    mid = App.Vector(
        0.5 * (g.StartPoint.x + g.EndPoint.x),
        0.5 * (g.StartPoint.y + g.EndPoint.y),
        0.0,
    )
    sketch.split(bottom_id, mid)
    doc.recompute()
    require_ok(fillet, "Fillet after split")
    print("TESTS Fillet.Base after split:", fillet.Base)

    doc.saveAs(post_path)
    print("TESTS wrote", post_path)
    objects_post, stg_post = parse_fcstd(post_path)
    _plink, psubs_pre = first_link(objects_pre, "Fillet")
    _qlink, psubs_post = first_link(objects_post, "Fillet")
    seed_pre = ((psubs_pre[0].get("stSeed") if psubs_pre else "") or "").strip()
    seed_post = ((psubs_post[0].get("stSeed") if psubs_post else "") or "").strip()
    check(
        "pocket_seed_stable",
        bool(seed_pre) and seed_pre == seed_post,
        "pre=%s post=%s sub_pre=%s sub_post=%s"
        % (
            seed_pre or "(none)",
            seed_post or "(none)",
            (psubs_pre[0].get("value") if psubs_pre else ""),
            (psubs_post[0].get("value") if psubs_post else ""),
        ),
    )

    fillet_feats = parse_fillet_event_features(stg_post)
    print("TESTS Fillet STG1 events on features:", fillet_feats)
    chamfer_feats = parse_chamfer_event_features(stg_post)
    print("TESTS Chamfer STG1 events on features:", chamfer_feats)
    draft_feats = parse_draft_event_features(stg_post)
    print("TESTS Draft STG1 events on features:", draft_feats)
    thickness_feats = parse_thickness_event_features(stg_post)
    print("TESTS Thickness STG1 events on features:", thickness_feats)
    revolution_feats = parse_opcode_event_features(stg_post, "Revolution")
    print("TESTS Revolution STG1 events on features:", revolution_feats)
    loft_feats = parse_opcode_event_features(stg_post, "Loft")
    print("TESTS Loft STG1 events on features:", loft_feats)
    print("TESTS loftDiag is the C++ Console line from AdditiveLoft execute (Generated vs loftIndexOnPublished)")
    pipe_feats = parse_opcode_event_features(stg_post, "Pipe")
    print("TESTS Pipe STG1 events on features:", pipe_feats)
    helix_feats = parse_opcode_event_features(stg_post, "Helix")
    print("TESTS Helix STG1 events on features:", helix_feats)
    boolean_feats = parse_opcode_event_features(stg_post, "Boolean")
    print("TESTS Boolean STG1 events on features:", boolean_feats)
    box_feats = parse_opcode_event_features(stg_post, "AdditiveBox")
    print("TESTS AdditiveBox STG1 events on features:", box_feats)
    cyl_feats = parse_opcode_event_features(stg_post, "AdditiveCylinder")
    print("TESTS AdditiveCylinder STG1 events on features:", cyl_feats)
    pri_feats = parse_opcode_event_features(stg_post, "AdditivePrism")
    print("TESTS AdditivePrism STG1 events on features:", pri_feats)
    wed_feats = parse_opcode_event_features(stg_post, "AdditiveWedge")
    print("TESTS AdditiveWedge STG1 events on features:", wed_feats)
    ell_feats = parse_opcode_event_features(stg_post, "AdditiveEllipsoid")
    print("TESTS AdditiveEllipsoid STG1 events on features:", ell_feats)
    sbox_feats = parse_opcode_event_features(stg_post, "SubtractiveBox")
    print("TESTS SubtractiveBox STG1 events on features:", sbox_feats)
    hole_feats = parse_opcode_event_features(stg_post, "Hole")
    print("TESTS Hole STG1 events on features:", hole_feats)
    stg_events = parse_stg_events(stg_post)
    pocket_feats = [e["feature"] for e in stg_events if e["op"] == "Pocket"]
    print("TESTS Pocket STG1 events on features:", pocket_feats)
    pocket_inputs = parse_stg_event_handles(stg_post, "input")
    pocket_outputs = parse_stg_event_handles(stg_post, "output")

    through_id = (objects_post.get("PocketThrough") or {}).get("id")
    through_splits = [
        e for e in stg_events
        if e["op"] == "Pocket" and e["kind"] == EVENT_KIND_SPLIT and e["feature"] == through_id
    ]
    check(
        "pocket_through_split",
        through_id is not None and bool(through_splits),
        "pocket_id=%s split_event_ids=%s pocket_events=%s"
        % (
            through_id,
            [e["id"] for e in through_splits],
            [(e["feature"], e["kind"], e["id"]) for e in stg_events if e["op"] == "Pocket"],
        ),
    )

    s3_id = (objects_post.get("PocketS3") or {}).get("id")
    cap_slots = attachment_support_slots(objects_post, "SketchPocketS3")
    cap_seed = ""
    cap_detail = []
    for slot in cap_slots:
        seed = (slot.get("stSeed") or "").strip()
        objn = slot.get("obj") or ""
        val = slot.get("value") or slot.get("stFallback") or ""
        cap_detail.append("%s.%s stSeed=%s" % (objn, val, seed or "(none)"))
        if seed:
            cap_seed = seed
    cap_handle = None
    try:
        cap_handle = int(cap_seed, 16) if cap_seed else None
    except ValueError:
        cap_handle = None
    remnant_ok = False
    remnant_events = []
    for e in stg_events:
        if e["op"] != "Pocket" or e["kind"] != EVENT_KIND_MODIFIED:
            continue
        if s3_id is None or e["feature"] != s3_id:
            continue
        remnant_events.append(e["id"])
        handles = list(pocket_inputs.get(e["id"]) or []) + list(pocket_outputs.get(e["id"]) or [])
        if cap_handle is not None and cap_handle in handles:
            remnant_ok = True
            break
    check(
        "pocket_s3_remnant",
        remnant_ok,
        "pocket_id=%s cap_handle=%s modified_event_ids=%s %s"
        % (
            s3_id,
            cap_handle,
            remnant_events,
            "; ".join(cap_detail) or "SketchPocketS3 AttachmentSupport missing / seedless",
        ),
    )

    # Fail-closed: every REQUIRED_CASES id must be scored (harness gap → FAIL).
    # Keep the module docstring "Required cases" list in lockstep with REQUIRED_CASES.
    missing = sorted(REQUIRED_CASES - SCORED)
    if missing:
        for name in missing:
            check(name, False, "required case never scored (harness gap)")
    unexpected = sorted(SCORED - REQUIRED_CASES)
    if unexpected:
        print("TESTS_WARN unexpected case ids:", " ".join(unexpected))

    if FAILURES:
        print("TESTS_FAIL", " ".join(FAILURES))
        return 1
    if len(SCORED) != len(REQUIRED_CASES):
        print(
            "TESTS_FAIL scored_count=%s required_count=%s"
            % (len(SCORED), len(REQUIRED_CASES))
        )
        return 1
    print("TESTS_OK")
    return 0


if __name__ == "__main__":
    try:
        rc = main()
    except Exception:
        traceback.print_exc()
        rc = 1
    sys.exit(rc)
