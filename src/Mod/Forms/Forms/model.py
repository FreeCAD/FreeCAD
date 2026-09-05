# SPDX-License-Identifier: LGPL-2.1-or-later
# /**************************************************************************
#                                                                           *
#    Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
#                                                                           *
#    This file is part of FreeCAD.                                          *
#                                                                           *
#    FreeCAD is free software: you can redistribute it and/or modify it     *
#    under the terms of the GNU Lesser General Public License as            *
#    published by the Free Software Foundation, either version 2.1 of the   *
#    License, or (at your option) any later version.                        *
#                                                                           *
#    FreeCAD is distributed in the hope that it will be useful, but         *
#    WITHOUT ANY WARRANTY; without even the implied warranty of             *
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
#    Lesser General Public License for more details.                        *
#                                                                           *
#    You should have received a copy of the GNU Lesser General Public       *
#    License along with FreeCAD. If not, see                                *
#    <https://www.gnu.org/licenses/>.                                       *
#                                                                           *
# **************************************************************************/

"""Read the editable representation consistently across operations and conversion."""

from types import SimpleNamespace

from .placement import global_placement
from .tmesh import HierarchicalTMesh


def object_tmesh(obj, cage=None):
    """Read the authoritative mesh, migrating legacy insertion records once."""
    from .cage import ControlCage
    from .brep import LocalEdgeInsert
    cage = cage or ControlCage.from_object(obj)
    from .capabilities import validate_local_creases
    validate_local_creases(obj, [edge for edge, value in cage.edge_sharpness.items() if value > 0], 1)
    encoded = str(getattr(obj, "TMeshData", "") or "")
    if encoded:
        mesh = HierarchicalTMesh.decode(encoded)
        # The vector list remains a convenient FreeCAD property for editing.
        base_count = len(cage.vertices)
        for vertex_id, point in enumerate(cage.vertices):
            if vertex_id in mesh.vertices:
                mesh.set_vertex(vertex_id, point)
        for offset, point in enumerate(getattr(obj, "LocalControlPoints", ())):
            vertex_id = base_count + offset
            if vertex_id in mesh.vertices:
                mesh.set_vertex(vertex_id, (point.x, point.y, point.z))
        return mesh

    mesh = HierarchicalTMesh.from_quad_cage(cage.vertices, cage.faces)
    records = [LocalEdgeInsert.decode(value) for value in getattr(obj, "LocalEdgeInserts", ())]
    local_points = [(point.x, point.y, point.z) for point in getattr(obj, "LocalControlPoints", ())]
    face_lookup = {tuple(face): index for index, face in enumerate(cage.faces)}
    for record in records:
        face_id = face_lookup.get(tuple(record.face))
        if face_id is None or face_id not in mesh.faces:
            raise ValueError("A legacy Insert Edge face no longer exists")
        mesh, _new_ids, children = mesh.insert_edge(face_id, record.edge, record.position)
        seam = mesh.faces[children[0]].sides[2]
        for vertex_id, local_index in zip(seam, record.points):
            if local_index >= len(local_points):
                raise ValueError("A legacy Insert Edge endpoint is missing")
            mesh.set_vertex(vertex_id, local_points[local_index])
    return mesh



def form_preview_object(obj, points):
    """Build an unowned Form data object from replacement control points."""
    import FreeCAD as App
    import Part

    base_count = len(obj.ControlPoints)

    preview = SimpleNamespace()
    preview.ControlPoints = [App.Vector(*point) for point in points[:base_count]]
    preview.LocalControlPoints = [App.Vector(*point) for point in points[base_count:]]
    for name in (
        "ControlFaces",
        "VertexSharpness",
        "EdgeSharpness",
        "LocalEdgeInserts",
        "DissolvedEdges",
        "MatchBoundary",
        "MatchParameters",
        "MatchCornerVertices",
        "MatchCornerEdges",
    ):
        setattr(preview, name, list(getattr(obj, name, ())))
    for name in (
        "TMeshData",
        "MatchContinuity",
        "MatchTangentMode",
        "ConversionStatus",
    ):
        setattr(preview, name, str(getattr(obj, name, "") or ""))
    preview.Placement = global_placement(obj)
    preview.MatchSupport = getattr(obj, "MatchSupport", None)
    preview.FormType = str(getattr(obj, "FormType", ""))
    preview.BRepTolerance = obj.BRepTolerance
    preview.MaxRefinement = int(obj.MaxRefinement)
    preview.Shape = Part.Shape()
    preview.MaximumDeviation = 0.0
    preview.ConversionLevel = max(int(getattr(obj, "ConversionLevel", 1)), 1)
    return preview


