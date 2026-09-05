# SPDX-License-Identifier: LGPL-2.1-or-later
"""Bounded subdivision mesh previews; no OCCT fitting or document recompute."""

from .cage import ControlCage
from .limits import check_sampling
from .topology import catmull_clark_step_details, catmull_clark_limit_points


def mesh_preview(obj):
    """Return preview points and polygons in the Form's local coordinates."""
    if getattr(obj, "MatchSupport", None) and getattr(obj, "MatchBoundary", ()):
        from .model import form_preview_object
        from .matching import apply_match_constraints
        obj = form_preview_object(obj, [(point.x, point.y, point.z) for point in
                                       list(obj.ControlPoints) + list(getattr(obj, "LocalControlPoints", ()))])
        apply_match_constraints(obj)
    cage = ControlCage.from_object(obj)
    vertices, faces = cage.vertices, cage.faces
    edges, corners = cage.edge_sharpness, cage.vertex_sharpness
    mesh = cells = None
    if getattr(obj, "TMeshData", "") or getattr(obj, "LocalEdgeInserts", ()):
        from .model import object_tmesh
        from .brep import _tmesh_refinement, _refined_parameter_cells
        mesh = object_tmesh(obj, cage)
        vertices, faces, cells, _indices, edges, corners = _tmesh_refinement(
            vertices, faces, mesh, edges, corners)
    levels = 1 if mesh is not None else 2
    check_sampling(len(faces), levels)
    for _ in range(levels):
        vertices, faces, _old, _edge, _face, edges, corners = catmull_clark_step_details(
            vertices, faces, edges, corners)
        if cells is not None:
            cells = _refined_parameter_cells(cells)
    vertices = catmull_clark_limit_points(vertices, faces, edges, corners)
    if mesh is not None:
        regions = {}
        for leaf in mesh.faces.values():
            us, vs = zip(*leaf.parameters)
            regions.setdefault(leaf.root, []).append((min(us), max(us), min(vs), max(vs)))
        visible = set()
        for root, root_cells in cells.items():
            for index, parameters in root_cells.items():
                u = sum(p[0] for p in parameters) / 4
                v = sum(p[1] for p in parameters) / 4
                if any(u0 <= u <= u1 and v0 <= v <= v1
                       for u0, u1, v0, v1 in regions.get(root, ())):
                    visible.add(index)
        faces = [face for index, face in enumerate(faces) if index in visible]
    return vertices, faces


class MotionPreview:
    """Own the temporary Coin mesh and restore native display on every exit."""

    def __init__(self, view_object):
        from pivy import coin
        self.view_object = view_object
        self.previous_switch = view_object.SwitchNode.whichChild.getValue()
        self.root = coin.SoSeparator()
        pick = coin.SoPickStyle()
        pick.style = coin.SoPickStyle.UNPICKABLE
        self.root.addChild(pick)
        color = coin.SoMaterial()
        color.diffuseColor = tuple(view_object.ShapeColor)[:3]
        self.root.addChild(color)
        self.coordinates = coin.SoCoordinate3()
        self.polygons = coin.SoIndexedFaceSet()
        self.root.addChild(self.coordinates)
        self.root.addChild(self.polygons)
        view_object.RootNode.addChild(self.root)
        view_object.SwitchNode.whichChild = coin.SO_SWITCH_NONE

    def update(self, obj):
        vertices, faces = mesh_preview(obj)
        indices = [index for face in faces for index in (*face, -1)]
        self.coordinates.point.setValues(0, len(vertices), vertices)
        self.coordinates.point.setNum(len(vertices))
        self.polygons.coordIndex.setValues(0, len(indices), indices)
        self.polygons.coordIndex.setNum(len(indices))

    def close(self):
        if self.root is None:
            return
        try:
            self.view_object.SwitchNode.whichChild = self.previous_switch
        finally:
            try:
                self.view_object.RootNode.removeChild(self.root)
            finally:
                self.root = self.coordinates = self.polygons = self.view_object = None
