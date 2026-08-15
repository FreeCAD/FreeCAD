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

"""Public Python API for Forms objects."""


def create_box(document=None, name="FormBox"):
    """Create a parametric form box in *document*."""
    from .box import create_box as _create_box

    return _create_box(document, name)


def create_cylinder(document=None, name="FormCylinder"):
    """Create a parametric form cylinder in *document*."""
    from .primitives import create_cylinder as _create_cylinder

    return _create_cylinder(document, name)


def create_sphere(document=None, name="FormSphere"):
    """Create a parametric form sphere in *document*."""
    from .primitives import create_sphere as _create_sphere

    return _create_sphere(document, name)


def create_quadball(document=None, name="FormQuadball"):
    """Create a parametric cube-derived quadball in *document*."""
    from .primitives import create_quadball as _create_quadball

    return _create_quadball(document, name)


def create_pipe(document=None, path_object=None, name="FormPipe"):
    """Create a path-driven Form pipe from a wire-only object."""
    from .pipe import create_pipe as _create_pipe

    return _create_pipe(document, path_object, name)


def create_face(document=None, name="FormFace", profile=None):
    """Create a Form face, optionally initialized from a face or closed wire."""
    from .primitives import create_face as _create_face

    return _create_face(document, name, profile)


def create_torus(document=None, name="FormTorus"):
    """Create a parametric form torus in *document*."""
    from .primitives import create_torus as _create_torus

    return _create_torus(document, name)


def create_tube(document=None, name="FormTube"):
    """Create a parametric hollow form tube in *document*."""
    from .primitives import create_tube as _create_tube

    return _create_tube(document, name)


def create_surface(body, source, subelement, name="FormSurface"):
    """Create a Part Design Form Surface initialized from one selected face."""
    from .surface import create_surface as _create_surface

    return _create_surface(body, source, subelement, name)


def create_additive_form(body, base_feature, primitive="Box", name=None, placement=None, path_object=None):
    """Create an additive Forms primitive inside a Part Design Body."""
    from .additive import create_additive_form as _create_additive_form

    return _create_additive_form(body, base_feature, primitive, name, placement, path_object)


def create_subtractive_form(body, base_feature, primitive="Box", name=None, placement=None, path_object=None):
    """Create a subtractive Forms primitive inside a Part Design Body."""
    from .additive import create_subtractive_form as _create_subtractive_form

    return _create_subtractive_form(body, base_feature, primitive, name, placement, path_object)


def move_form_to_body(source, body):
    """Convert a standalone Form into the next additive feature of *body*."""
    from .additive import move_form_to_body as _move_form_to_body

    return _move_form_to_body(source, body)


def match_boundary(obj, boundary_edges, support, continuity="Connected"):
    """Match one Form opening to a face or closed-wire support."""
    from .additive import match_boundary as _match_boundary

    return _match_boundary(obj, boundary_edges, support, continuity)


def make_editable(obj):
    """Detach a parametric Forms primitive into an editable control cage."""
    if not getattr(obj, "FormType", "").startswith("Forms::"):
        raise TypeError("The object is not a Forms primitive")
    obj.CageMode = "Editable"
    obj.recompute()
    return obj


def delete_faces(obj, face_indices):
    """Delete indexed faces from a Forms object's control cage."""
    from .operations import delete_faces as _delete_faces

    return _delete_faces(obj, face_indices)


def dissolve_edges(obj, edges):
    """Dissolve internal control edges into merged surface faces."""
    from .operations import dissolve_edges as _dissolve_edges

    return _dissolve_edges(obj, edges)


def erase_and_fill(obj, face_indices):
    """Erase selected cage faces and minimally rebuild the exposed region."""
    from .operations import erase_and_fill as _erase_and_fill

    return _erase_and_fill(obj, face_indices)


def fill_holes(obj, boundary_edges, mode="automatic"):
    """Fill control-cage boundaries containing the given edges."""
    from .operations import fill_holes as _fill_holes

    return _fill_holes(obj, boundary_edges, mode)


def bridge_boundaries(obj, boundary_edges):
    """Bridge two selected equal-sized control-cage boundaries."""
    from .operations import bridge_boundaries as _bridge_boundaries

    return _bridge_boundaries(obj, boundary_edges)


def unweld_segment(obj, segment_edges, separate_forms=True):
    """Split a Form along a segment, optionally creating a second object."""
    from .operations import unweld_segment as _unweld_segment

    return _unweld_segment(obj, segment_edges, separate_forms)


def weld_boundaries(obj, first_edge, other, second_edge):
    """Join two Form openings without adding bridge faces."""
    from .operations import weld_boundaries as _weld_boundaries

    return _weld_boundaries(obj, first_edge, other, second_edge)


def thicken_surface(obj, distance, sharp=True):
    """Turn an open Form surface into a closed editable Form."""
    from .operations import thicken_surface as _thicken_surface

    return _thicken_surface(obj, distance, sharp)


def insert_edge_loop(obj, edge, position=0.5, mode="simple"):
    """Insert a complete loop through the selected control edge ring."""
    from .operations import insert_edge_loop as _insert_edge_loop

    return _insert_edge_loop(obj, edge, position, mode)


def insert_edge(obj, edge, position=0.5, side="left", mode="simple"):
    """Insert localized parallel edges beside one selected control edge."""
    from .operations import insert_edge as _insert_edge

    return _insert_edge(obj, edge, position, side, mode)


def insert_point_edges(obj, points):
    """Insert arbitrary edge points and join consecutive points."""
    from .operations import insert_point_edges as _insert_point_edges

    return _insert_point_edges(obj, points)


def subdivide_faces(obj, face_ids, u_divisions=2, v_divisions=2):
    """Dyadically subdivide selected logical Form faces."""
    from .operations import subdivide_faces as _subdivide_faces

    return _subdivide_faces(obj, face_ids, u_divisions, v_divisions)


def set_edge_crease(obj, edges, sharpness=10.0):
    """Set the semi-sharp value of selected control edges."""
    from .operations import set_edge_crease as _set_edge_crease

    return _set_edge_crease(obj, edges, sharpness)


def straighten_control_points(obj, indices, line=None, surface_points=False):
    """Straighten selected control or mapped surface points onto a line."""
    from .operations import straighten_control_points as _straighten_control_points

    return _straighten_control_points(obj, indices, line, surface_points)


def flatten_control_points(obj, indices):
    """Flatten selected controls to their best-fit plane."""
    from .operations import flatten_control_points as _flatten_control_points

    return _flatten_control_points(obj, indices)


__all__ = [
    "create_box",
    "create_cylinder",
    "create_sphere",
    "create_quadball",
    "create_pipe",
    "create_face",
    "create_torus",
    "create_tube",
    "create_surface",
    "create_additive_form",
    "create_subtractive_form",
    "move_form_to_body",
    "match_boundary",
    "make_editable",
    "delete_faces",
    "dissolve_edges",
    "erase_and_fill",
    "fill_holes",
    "bridge_boundaries",
    "unweld_segment",
    "weld_boundaries",
    "thicken_surface",
    "insert_edge",
    "insert_edge_loop",
    "subdivide_faces",
    "set_edge_crease",
    "straighten_control_points",
    "flatten_control_points",
]
