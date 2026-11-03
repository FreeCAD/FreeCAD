# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Nicholas Herrmann <nicholas.herrmann@proton.me>    *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""Build the neutral deck model (see writer.py) from a FreeCAD FEM analysis.

Supported:
  mesh        volumes: tetra4, tetra10 (native 10-node tetra), hexa8, penta6, pyra5
              and the second-order hexa20/penta15/pyra13, which are reduced to
              their corner nodes; faces (shell meshes): tria3/quad4 and
              tria6/quad8 (reduced to corner nodes)
  materials   linear elastic solids (several materials via their References)
  geometry    one shell thickness for face meshes
  constraints fixed, displacement (fixed components), force, pressure, self weight

When midside nodes are dropped their share of a nodal force is moved half to
each end node of their edge, so the total force is kept.
"""

__title__ = "FreeCAD FEM solver OpenRadioss model builder"
__author__ = "Nicholas Herrmann"
__url__ = "https://www.freecad.org"

## \addtogroup FEM
#  @{

import math

import FreeCAD

from femmesh import meshsetsgetter
from femmesh import meshtools
from femtools import membertools

# Midside node -> end nodes of its edge (0-based positions in the SMESH node order)
_MIDSIDE_EDGES = {
    # tria6
    6: {3: (0, 1), 4: (1, 2), 5: (2, 0)},
    # quad8
    8: {4: (0, 1), 5: (1, 2), 6: (2, 3), 7: (3, 0)},
    # pyra13
    13: {
        5: (0, 1),
        6: (1, 2),
        7: (2, 3),
        8: (3, 0),
        9: (0, 4),
        10: (1, 4),
        11: (2, 4),
        12: (3, 4),
    },
    # penta15
    15: {
        6: (0, 1),
        7: (1, 2),
        8: (2, 0),
        9: (3, 4),
        10: (4, 5),
        11: (5, 3),
        12: (0, 3),
        13: (1, 4),
        14: (2, 5),
    },
    # hexa20
    20: {
        8: (0, 1),
        9: (1, 2),
        10: (2, 3),
        11: (3, 0),
        12: (4, 5),
        13: (5, 6),
        14: (6, 7),
        15: (7, 4),
        16: (0, 4),
        17: (1, 5),
        18: (2, 6),
        19: (3, 7),
    },
}
# Number of corner nodes by total node count
_VOLUME_CORNERS = {4: 4, 10: 10, 5: 5, 13: 5, 6: 6, 15: 6, 8: 8, 20: 8}
_FACE_CORNERS = {3: 3, 6: 3, 4: 4, 8: 4}
TETRA10_ELFORM = 16

SUPPORTED_CONSTRAINTS = (
    "Fem::ConstraintFixed",
    "Fem::ConstraintDisplacement",
    "Fem::ConstraintForce",
    "Fem::ConstraintPressure",
    "Fem::ConstraintSelfWeight",
)


class ModelError(Exception):
    """The analysis cannot be translated (message is shown to the user)."""


def _vec(v):
    return (v.x, v.y, v.z)


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _centroid(points):
    n = len(points)
    return tuple(sum(p[k] for p in points) / n for k in range(3))


def _quantity(value, unit):
    return FreeCAD.Units.Quantity(value).getValueAs(unit).Value


def _number(value, unit):
    """A plain number or a Quantity as a float in ``unit``.

    Node load tables may hold dimensionless Quantities whose value is already
    in N; Quantities with a unit are converted.
    """
    if not hasattr(value, "Unit"):
        return float(value)
    if value.Unit == FreeCAD.Units.Unit():
        return value.Value
    return value.getValueAs(unit).Value


def _solid_element(nodes, coords):
    """(elform, nodes) of a volume element for *ELEMENT_SOLID (nodes as written)."""
    corners = list(nodes[: _VOLUME_CORNERS[len(nodes)]])
    if len(nodes) == 10:
        return TETRA10_ELFORM, list(nodes)  # native 10-node tetrahedron
    if len(corners) == 4:
        return 10, corners  # 4-node tetrahedron
    if len(corners) == 8:
        hexa = corners
    elif len(corners) == 6:
        hexa = [
            corners[0],
            corners[1],
            corners[2],
            corners[2],
            corners[3],
            corners[4],
            corners[5],
            corners[5],
        ]
    else:  # pyramid: quad base + apex
        hexa = corners[:4] + [corners[4]] * 4
    # A hexahedron's top nodes must lie on the positive side of its bottom face
    a, b, d = coords[hexa[0]], coords[hexa[1]], coords[hexa[3] if hexa[3] != hexa[2] else hexa[2]]
    top = _centroid([coords[n] for n in hexa[4:]])
    if _dot(_cross(_sub(b, a), _sub(d, a)), _sub(top, a)) < 0:
        hexa = hexa[4:] + hexa[:4]
    return 1, hexa


class ModelBuilder:
    def __init__(self, analysis, solver, mesh_obj=None):
        self.analysis = analysis
        self.solver = solver
        self.warnings = []
        meshes = [mesh_obj] if mesh_obj else membertools.get_mesh_to_solve(analysis)[0:1]
        if not meshes or meshes[0] is None:
            raise ModelError("The analysis has no mesh.")
        self.mesh_obj = meshes[0]
        self.femmesh = self.mesh_obj.FemMesh
        self.member = membertools.AnalysisMember(analysis)
        meshsetsgetter.MeshSetsGetter(analysis, solver, self.mesh_obj, self.member).get_mesh_sets()
        self.coords = {nid: _vec(v) for nid, v in self.femmesh.Nodes.items()}
        self.midside_to_ends = {}  # dropped midside node -> (end node, end node)
        self.kind = None

    # ------------------------------------------------------------------
    def build(self):
        femmesh = self.femmesh
        if femmesh.VolumeCount:
            self.kind, element_ids = "solid", list(femmesh.Volumes)
        elif femmesh.FaceCount:
            self.kind, element_ids = "shell", list(femmesh.Faces)
        else:
            raise ModelError("The mesh has neither volume nor face elements.")
        table = {eid: list(femmesh.getElementNodes(eid)) for eid in element_ids}
        if self.kind == "solid" and self.member.geos_shellthickness:
            self.warnings.append("Shell thickness ignored: the mesh has volume elements.")
        if self.member.geos_beamsection:
            self.warnings.append("Beam sections are not supported and were ignored.")

        parts = self.build_parts(table)
        additional = self.solver.AdditionalKeywords
        model = {
            "title": f"{self.analysis.Document.Label} - {self.analysis.Label}",
            "end_time": float(self.solver.EndTime.getValueAs("s")),
            "load_curve": "constant" if self.solver.LoadCurve == "Constant" else "ramp",
            "nodes": self.coords,
            "parts": parts,
            "fixed": self.fixed(),
            "node_forces": self.forces(),
            "pressures": self.pressures(table),
            "gravity": self.gravity(),
            "extra": additional.Text if additional else "",
            "mass_scaling_dt": max(0.0, float(self.solver.MassScalingMicroseconds)) * 1e-6,
        }
        model["output_interval"] = model["end_time"] / max(1, int(self.solver.OutputFrames))
        return model

    # ------------------------------------------------------------------
    def build_parts(self, table):
        materials = self.member.mats_linear
        if not materials:
            raise ModelError("The analysis has no linear elastic material.")
        if self.member.mats_nonlinear:
            self.warnings.append(
                "Nonlinear materials are ignored (only linear elastic is supported)."
            )
        if len(materials) > 1:
            meshtools.get_femelement_sets(self.femmesh, table, materials)
            assignments = [(m, list(m.get("FEMElements", []))) for m in materials]
        else:
            assignments = [(materials[0], list(table))]

        thickness = None
        if self.kind == "shell":
            if not self.member.geos_shellthickness:
                raise ModelError(
                    "A face (shell) mesh needs a shell thickness (Element geometry 2D)."
                )
            if len(self.member.geos_shellthickness) > 1:
                self.warnings.append(
                    "Several shell thicknesses: the first one is used for all shells."
                )
            thickness = _quantity(self.member.geos_shellthickness[0]["Object"].Thickness, "mm")

        parts, pid = [], 0
        for mat, element_ids in assignments:
            props = self.material_properties(mat["Object"])
            by_elform = {}
            for eid in element_ids:
                nodes = table[eid]
                if self.kind == "solid":
                    if len(nodes) not in _VOLUME_CORNERS:
                        raise ModelError(f"Unsupported volume element with {len(nodes)} nodes.")
                    elform, written = _solid_element(nodes, self.coords)
                else:
                    if len(nodes) not in _FACE_CORNERS:
                        raise ModelError(f"Unsupported face element with {len(nodes)} nodes.")
                    elform, written = 2, nodes[: _FACE_CORNERS[len(nodes)]]
                self.remember_midside(nodes, written)
                by_elform.setdefault(elform, []).append((eid, written))
            for elform, elements in sorted(by_elform.items()):
                pid += 1
                parts.append(
                    {
                        "pid": pid,
                        "title": f"{mat['Object'].Label}",
                        "kind": self.kind,
                        "elform": elform,
                        "thickness": thickness,
                        "material": props,
                        "elements": elements,
                    }
                )
        if not parts:
            raise ModelError("No mesh elements are assigned to a material.")
        if any(p["elform"] == TETRA10_ELFORM for p in parts):
            self.warnings.append(
                "10-node tetrahedra: in a cantilever test they deflected about 35 % more "
                "than with CalculiX. Use hexahedra or a fine 4-node tetrahedron mesh "
                "for quantitative results."
            )
        return parts

    def remember_midside(self, nodes, written):
        edges = _MIDSIDE_EDGES.get(len(nodes))
        if not edges or len(written) == len(nodes):
            return
        for position, (a, b) in edges.items():
            self.midside_to_ends[nodes[position]] = (nodes[a], nodes[b])

    def material_properties(self, material_obj):
        m = material_obj.Material
        try:
            return {
                "E": _quantity(m["YoungsModulus"], "MPa"),
                "nu": float(m["PoissonRatio"]),
                "rho": _quantity(m["Density"], "t/mm^3"),
            }
        except KeyError as e:
            raise ModelError(f"Material '{material_obj.Label}' has no {e.args[0]}.")

    # ------------------------------------------------------------------
    def fixed(self):
        result = [
            {"title": c["Object"].Label, "nodes": list(c["Nodes"]), "dofs": (1, 1, 1, 1, 1, 1)}
            for c in self.member.cons_fixed
        ]
        for c in self.member.cons_displacement:
            obj = c["Object"]
            dofs = tuple(
                int(bool(getattr(obj, name, False)))
                for name in ("xFix", "yFix", "zFix", "rotxFix", "rotyFix", "rotzFix")
            )
            prescribed = [
                axis
                for axis in ("x", "y", "z")
                if not getattr(obj, f"{axis}Free", True) and not getattr(obj, f"{axis}Fix", False)
            ]
            if prescribed:
                self.warnings.append(
                    f"{obj.Label}: prescribed displacements ({', '.join(prescribed)}) are "
                    "not supported; only fixed components are used."
                )
            if any(dofs):
                result.append({"title": obj.Label, "nodes": list(c["Nodes"]), "dofs": dofs})
        return result

    def forces(self):
        result = []
        for c in self.member.cons_force:
            direction = _vec(c["Object"].DirectionVector)
            loads = {}
            for _ref, node_loads in c["NodeLoadTable"]:
                for nid, load in node_loads.items():
                    load = _number(load, "N")
                    targets = [(nid, 1.0)]
                    if nid in self.midside_to_ends:
                        # dropped midside node: split onto its edge ends
                        a, b = self.midside_to_ends[nid]
                        targets = [(a, 0.5), (b, 0.5)]
                    for target, share in targets:
                        f = loads.setdefault(target, [0.0, 0.0, 0.0])
                        for k in range(3):
                            f[k] += direction[k] * load * share
            result.append(
                {"title": c["Object"].Label, "loads": {n: tuple(f) for n, f in loads.items()}}
            )
        return result

    def pressures(self, table):
        result = []
        for c in self.member.cons_pressure:
            obj = c["Object"]
            pressure = _quantity(obj.Pressure, "MPa")
            if getattr(obj, "Reversed", False):
                pressure = -pressure
            segments = []
            for ref_obj, sub_names in obj.References:
                for sub in sub_names:
                    face = ref_obj.Shape.getElement(sub)
                    face_nodes = set(self.femmesh.getNodesByFace(face))
                    segments.extend(self.face_segments(face, face_nodes, table))
            if not segments:
                self.warnings.append(f"{obj.Label}: no mesh faces found on its references.")
            result.append({"title": obj.Label, "pressure": pressure, "segments": segments})
        return result

    def face_segments(self, face, face_nodes, table):
        """Segments (corner node lists) on ``face``, ordered with outward normals."""
        segments = []
        for nodes in table.values():
            if face_nodes.isdisjoint(nodes):
                continue
            if self.kind == "solid":
                if len(nodes) == 10:
                    corners = nodes[:4]
                else:
                    corners = nodes[: _VOLUME_CORNERS[len(nodes)]][:8]
                on_face = [n for n in dict.fromkeys(corners) if n in face_nodes]
                if len(on_face) < 3:
                    continue
                outward = _sub(
                    _centroid([self.coords[n] for n in on_face]),
                    _centroid([self.coords[n] for n in dict.fromkeys(corners)]),
                )
            else:
                on_face = nodes[: _FACE_CORNERS[len(nodes)]]
                if not all(n in face_nodes for n in on_face):
                    continue
                center = _centroid([self.coords[n] for n in on_face])
                u, v = face.Surface.parameter(FreeCAD.Vector(*center))
                outward = _vec(face.normalAt(u, v))
            on_face = self.cyclic_order(on_face)
            a, b, c = (self.coords[n] for n in on_face[:3])
            if _dot(_cross(_sub(b, a), _sub(c, a)), outward) < 0:
                on_face = list(reversed(on_face))
            segments.append(on_face)
        return segments

    def cyclic_order(self, nodes):
        """Order 4 face nodes around their centroid (3 nodes are always cyclic)."""
        if len(nodes) != 4:
            return list(nodes)
        pts = [self.coords[n] for n in nodes]
        center = _centroid(pts)
        normal = _cross(_sub(pts[1], pts[0]), _sub(pts[2], pts[0]))
        if _dot(normal, normal) < 1e-24:
            normal = _cross(_sub(pts[2], pts[0]), _sub(pts[3], pts[0]))
        e1 = _sub(pts[0], center)
        e2 = _cross(normal, e1)
        angles = [
            (math.atan2(_dot(_sub(p, center), e2), _dot(_sub(p, center), e1)), n)
            for p, n in zip(pts, nodes)
        ]
        return [n for _angle, n in sorted(angles)]

    def gravity(self):
        if not self.member.cons_selfweight:
            return None
        if len(self.member.cons_selfweight) > 1:
            self.warnings.append("Several self weight constraints: only the first one is used.")
        obj = self.member.cons_selfweight[0]["Object"]
        g = _quantity(obj.GravityAcceleration, "mm/s^2")
        d = obj.GravityDirection
        return (g * d.x, g * d.y, g * d.z)


def build_model(analysis, solver, mesh_obj=None):
    """Return (model, warnings) for ``analysis``; raises ModelError."""
    builder = ModelBuilder(analysis, solver, mesh_obj)
    model = builder.build()
    return model, builder.warnings


##  @}
