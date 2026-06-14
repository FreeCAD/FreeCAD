# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *   Copyright (c) 2024 Colin Rawlings <colin.d.rawlings@gmail.com>        *
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

__title__ = "Disambiguate Solid Selection"
__author__ = "Colin Rawlings"
__url__ = "https://www.freecad.org"

from typing import TYPE_CHECKING, Dict, List, Optional, Tuple

import copy

from PySide import QtGui
from PySide import QtCore

highlight_color_t = List[Tuple[float, float, float, float]]  # rgba value per face
highlight_map_t = Dict[Optional[str], highlight_color_t]

if TYPE_CHECKING:
    from Part import Face, PartFeature


def calculate_unhighlighted_color(parent_part: "PartFeature") -> highlight_color_t:
    """
    Cheap and cheerful function to dim object so that subsequent highlighting is apparent
    """

    initial_color = parent_part.ViewObject.DiffuseColor
    num_faces = len(parent_part.Shape.Faces)

    if len(initial_color) == num_faces:
        unhighlighted_color: highlight_color_t = initial_color
    else:
        unhighlighted_color: highlight_color_t = [initial_color[0]] * num_faces

    for face_idx in range(num_faces):
        rgba = unhighlighted_color[face_idx]
        unhighlighted_color[face_idx] = (
            rgba[0] * 0.5,
            rgba[1] * 0.5,
            rgba[2] * 0.5,
            rgba[3] * 0.5,
        )

    return unhighlighted_color


class HighlightContext:
    def __init__(self, parent_part: "PartFeature") -> None:
        self._parent_part = parent_part
        self._initial_color = parent_part.ViewObject.DiffuseColor
        self._unhighlighted_color = calculate_unhighlighted_color(parent_part)

    def _set_color(self, color: highlight_color_t) -> None:
        self._parent_part.ViewObject.DiffuseColor = color
        self._parent_part.ViewObject.update()

    def __enter__(self) -> None:
        self._set_color(self._unhighlighted_color)

    def __exit__(self, *arg, **kwargs) -> bool:
        self._set_color(self._initial_color)
        return False


def parent_face_index(parent_part: "PartFeature", face: "Face") -> int:
    """
    Return the index for the provided face in the parent_shape's
    list of faces.
    """

    return [face.isSame(f) for f in parent_part.Shape.Faces].index(True)


def solid_parent_faces_indices(parent_part: "PartFeature", solid_index: int) -> List[int]:
    """
    Return the parent's face indices for the faces bounding
    the given solid
    """

    solid = parent_part.Shape.Solids[solid_index]
    return [parent_face_index(parent_part, solid_face) for solid_face in solid.Faces]


def build_highlight_map(parent_part: "PartFeature", solid_indices: List[int]) -> highlight_map_t:
    """
    Build a mapping from solid name to face colors for highlighting a given selected solid.
    Indexing with None returns the unhighlighted coloring (see
    calculate_unhighlighted_color).
    """

    unhighlighted_color = calculate_unhighlighted_color(parent_part)

    # build the highlighting map
    highlight_colors_for_solid: highlight_map_t = {}
    highlight_colors_for_solid[None] = unhighlighted_color
    for idx in solid_indices:
        solid_faces = solid_parent_faces_indices(parent_part, idx)

        highlighted_colors = copy.deepcopy(unhighlighted_color)

        for index in solid_faces:
            rgba = unhighlighted_color[index]
            highlighted_colors[index] = (rgba[0], rgba[1], 0.99, rgba[3])

        highlight_colors_for_solid[solid_name_from_index(idx)] = highlighted_colors

    return highlight_colors_for_solid


def solid_name_from_index(solid_index: int) -> str:
    return f"Solid{solid_index+1}"


def disambiguate_solid_selection(
    parent_part: "PartFeature", solid_indices: List[int]
) -> Optional[str]:
    """
    @param solid_indices the indices which may be used to get a reference to the selected solid as
    parent_part.Solids[index].

    @return The name of the selected solid or None if the user cancels the selection
    """

    # Build menu
    menu_of_solids = QtGui.QMenu()
    label = menu_of_solids.addAction("Selected entity belongs to multiple solids, pick one…")
    label.setDisabled(True)

    for index in solid_indices:
        menu_of_solids.addAction(solid_name_from_index(index))

    # Configure highlighting callbacks
    last_action: list[QtGui.QAction] = []
    highlight_colors_for_solid = build_highlight_map(parent_part, solid_indices)

    def set_part_colors(solid_name: Optional[str]) -> None:
        if solid_name not in highlight_colors_for_solid:
            solid_name = None

        parent_part.ViewObject.DiffuseColor = highlight_colors_for_solid[solid_name]
        parent_part.ViewObject.update()

    def display_hover(action: QtGui.QAction) -> None:
        if last_action and last_action[0].text() == action.text():
            return

        set_part_colors(action.text())

        last_action.clear()
        last_action.append(action)

    def on_enter(arg__1: QtCore.QEvent) -> None:
        if not last_action:
            return

        set_part_colors(last_action[0].text())

    def on_leave(arg__1: QtCore.QEvent) -> None:
        set_part_colors(None)

    menu_of_solids.hovered.connect(display_hover)  # type: ignore
    menu_of_solids.enterEvent = on_enter
    menu_of_solids.leaveEvent = on_leave

    # Have user select desired solid
    with HighlightContext(parent_part):
        selected_action = menu_of_solids.exec_(QtGui.QCursor.pos())

    # Process selection
    if selected_action is None:
        return None

    return selected_action.text()
