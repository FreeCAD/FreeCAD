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

"""GUI commands that modify Forms control-cage topology."""

import FreeCAD as App
import FreeCADGui as Gui
import Part
from PySide import QtCore

from Forms.feedback import MODELING_ERRORS, report_modeling_error
from Forms.operations import (
    bridge_boundaries,
    delete_faces,
    dissolve_edges,
    erase_and_fill,
    fill_holes,
    insert_edge_loop,
    set_edge_crease,
)


def _active_edit_session():
    from Forms.edit import active_form_session

    return active_form_session()


def _selected_control_elements(element_type):
    """Return mapped cage vertices for selected generated BRep elements."""
    session = _active_edit_session()
    if session is None:
        return {}
    elements = [
        tuple(indices)
        for kind, indices, _anchor in session._selected_control_targets(respect_symmetry=False)
        if kind == element_type
    ]
    return {session.obj: elements} if elements else {}


def _active_mapper():
    """Return the edit session's geometry-aware cached element mapper."""
    session = _active_edit_session()
    if session is None:
        return None
    try:
        return session._control_element_mapper()
    except (AttributeError, ValueError, RuntimeError):
        return None


def _selected_control_faces():
    result = {}
    mapper = _active_mapper()
    if mapper is None:
        return result
    for obj, elements in _selected_control_elements("Face").items():
        for vertices in elements:
            face_index = mapper.face_id(vertices)
            if face_index is not None:
                selected = result.setdefault(obj, set())
                if mapper.mesh is None:
                    selected.update(mapper.logical_face_groups[face_index])
                else:
                    selected.add(face_index)
    return result


def _selected_boundary_edges():
    result = {}
    mapper = _active_mapper()
    if mapper is None:
        return result
    for obj, elements in _selected_control_elements("Edge").items():
        boundary_edges = set(mapper.cage.boundary_edges)
        for vertices in elements:
            if len(vertices) != 2:
                continue
            edge = tuple(sorted(vertices))
            if edge in boundary_edges:
                result.setdefault(obj, set()).add(edge)
    return result


def _selected_control_edges():
    result = {}
    mapper = _active_mapper()
    if mapper is None:
        return result
    for obj, elements in _selected_control_elements("Edge").items():
        control_edges = set(mapper.cage.edge_counts())
        for vertices in elements:
            if len(vertices) != 2:
                continue
            edge = tuple(sorted(vertices))
            if edge in control_edges:
                result.setdefault(obj, set()).add(edge)
    return result


def _selected_dissolvable_edges():
    """Return selected internal seams that have not already been dissolved."""
    result = {}
    mapper = _active_mapper()
    if mapper is None:
        return result
    obj = mapper.obj
    if str(obj.FormType) == "Forms::Surface":
        return result
    dissolved = set()
    for encoded in getattr(obj, "DissolvedEdges", ()):
        try:
            dissolved.add(tuple(sorted(int(value) for value in str(encoded).split())))
        except (TypeError, ValueError):
            continue
    counts = mapper.mesh.edge_counts() if mapper.mesh is not None else mapper.cage.edge_counts()
    internal = {edge for edge, count in counts.items() if count == 2}.difference(dissolved)
    for selected_obj, elements in _selected_control_elements("Edge").items():
        for vertices in elements:
            edge = tuple(sorted(vertices))
            if len(edge) == 2 and edge in internal:
                result.setdefault(selected_obj, set()).add(edge)
    return result


def _selected_crease_edges():
    """Return selected edges plus every boundary edge of selected faces."""
    result = {}
    mapper = _active_mapper()
    if mapper is None:
        return result
    valid_edges = (
        set(mapper.mesh.atomic_edges())
        if mapper.mesh is not None
        else set(mapper.cage.edge_counts())
    )
    for element_type in ("Edge", "Face"):
        for obj, elements in _selected_control_elements(element_type).items():
            selected = result.setdefault(obj, set())
            if element_type == "Edge":
                selected.update(
                    tuple(sorted(vertices))
                    for vertices in elements
                    if len(vertices) == 2 and tuple(sorted(vertices)) in valid_edges
                )
                continue
            for vertices in elements:
                face_id = mapper.face_id(vertices)
                if face_id is None:
                    continue
                if mapper.mesh is not None:
                    face = mapper.mesh.faces[face_id]
                    selected.update(
                        tuple(sorted((first, second)))
                        for side in face.sides
                        for first, second in zip(side, side[1:])
                    )
                else:
                    face = mapper.logical_faces[face_id]
                    selected.update(
                        tuple(sorted((start, face[(index + 1) % len(face)])))
                        for index, start in enumerate(face)
                    )
            if not selected:
                result.pop(obj, None)
    return result


def _selected_mapped_edges():
    result = {}
    for obj, elements in _selected_control_elements("Edge").items():
        for vertices in elements:
            if len(vertices) == 2:
                result.setdefault(obj, set()).add(tuple(sorted(vertices)))
    return result


def _selected_control_points():
    """Return every control affected by selected vertices, edges, or faces."""
    result = {}
    for element_type in ("Vertex", "Edge", "Face"):
        for obj, elements in _selected_control_elements(element_type).items():
            for vertices in elements:
                result.setdefault(obj, set()).update(vertices)
    return result


def _closed_wire_reference(source, names):
    """Return a persistent LinkSub reference for one selected closed wire."""
    if source is None or not hasattr(source, "Shape") or source.Shape.isNull():
        return None
    names = tuple(str(name) for name in names)
    try:
        if not names and source.Shape.ShapeType == "Wire":
            wire = source.Shape
        elif len(names) == 1 and names[0].startswith("Wire"):
            wire = source.Shape.getElement(names[0])
        elif names and all(name.startswith("Edge") for name in names):
            groups = Part.sortEdges([source.Shape.getElement(name) for name in names])
            if len(groups) != 1:
                return None
            wire = Part.Wire(groups[0])
        else:
            return None
    except (Part.OCCError, RuntimeError, ValueError, IndexError):
        return None
    if wire.ShapeType != "Wire" or not wire.isClosed():
        return None
    return source, list(names)


def _external_match_support(active_form, selection):
    """Resolve one face or closed wire selected on a separate shape object."""
    source = selection.Object
    if source == active_form or not hasattr(source, "Shape"):
        return None
    names = tuple(str(name) for name in selection.SubElementNames)
    if len(names) == 1 and names[0].startswith("Face"):
        try:
            face = source.Shape.getElement(names[0])
        except (Part.OCCError, RuntimeError, ValueError, IndexError):
            return None
        return (source, [names[0]]) if face.ShapeType == "Face" else None
    return _closed_wire_reference(source, names)


def _selected_match_inputs():
    """Return one Form opening and one external face or closed-wire support."""
    session = _active_edit_session()
    if session is None:
        return None
    mapper = _active_mapper()
    obj = session.obj
    base = getattr(obj, "BaseFeature", None)
    if mapper is None:
        return None

    boundary = set(mapper.cage.boundary_edges)
    edges = set()
    for kind, indices, _anchor in session._selected_control_targets(respect_symmetry=False):
        edge = tuple(sorted(indices))
        if kind == "Edge" and len(edge) == 2 and edge in boundary:
            edges.add(edge)

    supports = []
    internal_selection_keys = set()
    no_resolve_selections = Gui.Selection.getSelectionEx("", Gui.Selection.ResolveMode.NoResolve)
    for selection in no_resolve_selections:
        for raw_name in selection.SubElementNames:
            form_name = session._form_selection_subelement(
                obj.Document.Name, selection.Object.Name, raw_name
            )
            if form_name is None:
                continue
            internal_selection_keys.add((selection.Object.Name, str(raw_name)))
            if base is None:
                continue
            try:
                element = obj.Shape.getElement(str(form_name))
            except (Part.OCCError, RuntimeError, ValueError, IndexError):
                continue
            if element.ShapeType != "Face":
                continue
            matches = [
                index for index, face in enumerate(base.Shape.Faces, 1) if element.isPartner(face)
            ]
            if len(matches) == 1:
                supports.append((base, [f"Face{matches[0]}"]))

    # Keep standalone Forms and genuinely external support objects on the
    # normal resolved-selection path. Body-owned entries belonging to the
    # active additive Form were already handled above.
    for selection in Gui.Selection.getSelectionEx():
        if selection.Object == obj:
            continue
        if any(
            (selection.Object.Name, str(raw_name)) in internal_selection_keys
            for raw_name in selection.SubElementNames
        ):
            continue
        support = _external_match_support(obj, selection)
        if support is not None:
            supports.append(support)
    unique_supports = {(support[0].Name, tuple(support[1])): support for support in supports}
    if not edges or len(unique_supports) != 1:
        return None
    return obj, edges, next(iter(unique_supports.values()))


def _run_topology_operation(selections, transaction_name, operation):
    """Apply a GUI topology operation with one transaction per document."""
    documents = {obj.Document for obj in selections}
    own_transactions = [
        document for document in documents if document.getBookedTransactionID() == 0
    ]
    for document in own_transactions:
        document.openTransaction(transaction_name)
    try:
        for obj, targets in selections.items():
            operation(obj, targets)
        Gui.Selection.clearSelection()
        for document in documents:
            document.recompute()
        from Forms.edit import active_form_session

        for obj in selections:
            session = active_form_session(obj)
            if session is not None:
                session.topology_changed()
        for document in own_transactions:
            document.commitTransaction()
    except MODELING_ERRORS as error:
        for document in own_transactions:
            document.abortTransaction()
        return report_modeling_error(transaction_name, error)
    except Exception:
        for document in own_transactions:
            document.abortTransaction()
        raise
    return True


class CommandDeleteFaces:
    def GetResources(self):
        return {
            "Pixmap": "edit-delete",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_DeleteFaces", "Delete Faces"),
            "ToolTip": App.Qt.translate(
                "Forms_DeleteFaces",
                "Deletes selected control faces and creates open boundaries",
            ),
        }

    def IsActive(self):
        return App.ActiveDocument is not None and bool(_selected_control_faces())

    def Activated(self):
        selections = _selected_control_faces()
        if not selections:
            return
        _run_topology_operation(
            selections,
            App.Qt.translate("Forms_DeleteFaces", "Delete form faces"),
            delete_faces,
        )


class CommandDeleteEdges:
    def GetResources(self):
        return {
            "Pixmap": "edit-delete",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_DeleteEdges", "Delete Edges"),
            "ToolTip": App.Qt.translate(
                "Forms_DeleteEdges",
                "Dissolves selected internal control edges without opening the surface",
            ),
        }

    def IsActive(self):
        return App.ActiveDocument is not None and bool(_selected_dissolvable_edges())

    def Activated(self):
        selections = _selected_dissolvable_edges()
        if selections:
            _run_topology_operation(
                selections,
                App.Qt.translate("Forms_DeleteEdges", "Delete form edges"),
                dissolve_edges,
            )


class CommandEraseAndFill:
    def GetResources(self):
        return {
            "Pixmap": "Forms_EraseAndFill",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_EraseAndFill", "Erase and Fill"),
            "ToolTip": App.Qt.translate(
                "Forms_EraseAndFill",
                "Erases selected control faces and minimally rebuilds the exposed region",
            ),
        }

    def IsActive(self):
        return App.ActiveDocument is not None and bool(_selected_control_faces())

    def Activated(self):
        selections = _selected_control_faces()
        if selections:
            _run_topology_operation(
                selections,
                App.Qt.translate("Forms_EraseAndFill", "Erase and fill form faces"),
                erase_and_fill,
            )


class CommandInsertEdgeLoop:
    def GetResources(self):
        return {
            "Pixmap": "Forms_InsertEdge",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_InsertEdge", "Insert Edge Loop"),
            "ToolTip": App.Qt.translate(
                "Forms_InsertEdge",
                "Inserts a complete loop through the selected quad edge ring",
            ),
        }

    def IsActive(self):
        selections = _selected_control_edges()
        return (
            App.ActiveDocument is not None
            and len(selections) == 1
            and len(next(iter(selections.values()))) == 1
        )

    def Activated(self):
        selections = _selected_control_edges()
        if len(selections) != 1:
            return
        obj, edges = next(iter(selections.items()))
        if len(edges) != 1:
            return
        edge = next(iter(edges))
        _run_topology_operation(
            {obj: {edge}},
            App.Qt.translate("Forms_InsertEdge", "Insert form edge"),
            lambda target, selected: insert_edge_loop(target, next(iter(selected))),
        )


class CommandSetPivot:
    def GetResources(self):
        return {
            "Pixmap": "Forms_SetPivot",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_SetPivot", "Set Pivot"),
            "ToolTip": App.Qt.translate(
                "Forms_SetPivot",
                "Moves the transform pivot to a snapped point without changing the selection",
            ),
        }

    def IsActive(self):
        session = _active_edit_session()
        return (
            App.ActiveDocument is not None
            and session is not None
            and bool(session.selected)
            and not session.has_active_tool()
        )

    def Activated(self):
        session = _active_edit_session()
        if session is not None:
            session.start_set_pivot_tool()


class CommandInsertEdge:
    def GetResources(self):
        return {
            "Pixmap": "Forms_InsertEdge",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_InsertEdge", "Insert Edge"),
            "ToolTip": App.Qt.translate(
                "Forms_InsertEdge",
                "Inserts a localized parallel edge; select a full loop to insert a loop",
            ),
        }

    def IsActive(self):
        session = _active_edit_session()
        return (
            App.ActiveDocument is not None and session is not None and not session.has_active_tool()
        )

    def Activated(self):
        session = _active_edit_session()
        if session is None:
            return
        selections = _selected_control_edges()
        if len(selections) == 1:
            obj, edges = next(iter(selections.items()))
            if len(edges) > 1:
                _run_topology_operation(
                    {obj: edges},
                    App.Qt.translate("Forms_InsertEdge", "Insert form edge loop"),
                    lambda target, selected: insert_edge_loop(target, next(iter(selected))),
                )
                return
        session.start_insert_edge_tool()


class CommandInsertPoint:
    def GetResources(self):
        return {
            "Pixmap": "Forms_InsertPoint",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_InsertPoint", "Insert Point"),
            "ToolTip": App.Qt.translate(
                "Forms_InsertPoint",
                "Places points freely on control edges and joins consecutive points",
            ),
        }

    def IsActive(self):
        session = _active_edit_session()
        return (
            App.ActiveDocument is not None
            and session is not None
            and str(session.obj.FormType) != "Forms::Surface"
            and not session.has_active_tool()
        )

    def Activated(self):
        session = _active_edit_session()
        if session is not None:
            session.start_insert_point_tool()


class CommandThicken:
    def GetResources(self):
        return {
            "Pixmap": "Forms_Thicken",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_Thicken", "Thicken"),
            "ToolTip": App.Qt.translate(
                "Forms_Thicken",
                "Turns an open form surface into a closed editable form",
            ),
        }

    def IsActive(self):
        session = _active_edit_session()
        if App.ActiveDocument is None or session is None or session.has_active_tool():
            return False
        mapper = _active_mapper()
        if mapper is None:
            return False
        cage = mapper.cage
        return (
            not cage.is_closed
            and not bool(getattr(session.obj, "LocalEdgeInserts", ()))
            and not bool(str(getattr(session.obj, "TMeshData", "") or ""))
        )

    def Activated(self):
        session = _active_edit_session()
        if session is not None:
            session.start_thicken_tool()


class CommandFillHole:
    def GetResources(self):
        return {
            "Pixmap": "Forms_FillHole",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_FillHole", "Fill Hole"),
            "ToolTip": App.Qt.translate(
                "Forms_FillHole",
                "Fills the control-cage boundary containing the selected edge",
            ),
        }

    def IsActive(self):
        return App.ActiveDocument is not None and bool(_selected_boundary_edges())

    def Activated(self):
        selections = _selected_boundary_edges()
        if not selections:
            return
        _run_topology_operation(
            selections,
            App.Qt.translate("Forms_FillHole", "Fill form hole"),
            fill_holes,
        )


class CommandBridge:
    def GetResources(self):
        return {
            "Pixmap": "Forms_Bridge",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_Bridge", "Bridge"),
            "ToolTip": App.Qt.translate(
                "Forms_Bridge", "Bridges two selected equal-sized boundary loops"
            ),
        }

    def IsActive(self):
        selections = _selected_boundary_edges()
        if len(selections) != 1:
            return False
        obj, edges = next(iter(selections.items()))
        mapper = _active_mapper()
        if mapper is None:
            return False
        loops = mapper.cage.boundary_loops()
        touched = sum(
            bool(
                set(edges).intersection(
                    {
                        tuple(sorted((loop[index], loop[(index + 1) % len(loop)])))
                        for index in range(len(loop))
                    }
                )
            )
            for loop in loops
        )
        return touched == 2

    def Activated(self):
        selections = _selected_boundary_edges()
        if len(selections) == 1:
            _run_topology_operation(
                selections,
                App.Qt.translate("Forms_Bridge", "Bridge form boundaries"),
                bridge_boundaries,
            )


class CommandUnweld:
    def GetResources(self):
        return {
            "Pixmap": "Forms_Unweld",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_Unweld", "Unweld"),
            "ToolTip": App.Qt.translate(
                "Forms_Unweld", "Splits a closed Form along a hovered separating segment"
            ),
        }

    def IsActive(self):
        session = _active_edit_session()
        return (
            App.ActiveDocument is not None
            and session is not None
            and not session.has_active_tool()
            and str(session.obj.FormType) != "Forms::Surface"
            and str(session.obj.TypeId) == "Part::FeaturePython"
        )

    def Activated(self):
        session = _active_edit_session()
        if session is not None:
            session.start_unweld_tool()


class CommandWeld:
    def GetResources(self):
        return {
            "Pixmap": "Forms_Weld",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_Weld", "Weld"),
            "ToolTip": App.Qt.translate(
                "Forms_Weld", "Joins equal-sized openings from two Forms into one Form"
            ),
        }

    def IsActive(self):
        session = _active_edit_session()
        return (
            App.ActiveDocument is not None
            and session is not None
            and not session.has_active_tool()
            and str(session.obj.FormType) != "Forms::Surface"
            and str(session.obj.TypeId) == "Part::FeaturePython"
        )

    def Activated(self):
        session = _active_edit_session()
        if session is not None:
            session.start_weld_tool()


class CommandMatch:
    def GetResources(self):
        return {
            "Pixmap": "Forms_Match",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_Match", "Match"),
            "ToolTip": App.Qt.translate(
                "Forms_Match",
                "Matches a selected form opening to an external face or closed wire",
            ),
        }

    def IsActive(self):
        return App.ActiveDocument is not None and _selected_match_inputs() is not None

    def Activated(self):
        inputs = _selected_match_inputs()
        session = _active_edit_session()
        if inputs is None or session is None:
            return
        obj, edges, support = inputs
        session.start_match_tool(obj, edges, support)


class _CommandCreaseBase:
    Sharpness = 0.0
    CommandName = ""
    Label = ""
    Description = ""
    Pixmap = ""

    def GetResources(self):
        return {
            "Pixmap": self.Pixmap,
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate(self.CommandName, self.Label),
            "ToolTip": App.Qt.translate(self.CommandName, self.Description),
        }

    def IsActive(self):
        return App.ActiveDocument is not None and bool(_selected_crease_edges())

    def Activated(self):
        selections = _selected_crease_edges()
        if selections:
            _run_topology_operation(
                selections,
                App.Qt.translate(self.CommandName, self.Label),
                lambda obj, edges: set_edge_crease(obj, edges, self.Sharpness),
            )


class CommandCrease(_CommandCreaseBase):
    Sharpness = 10.0
    CommandName = "Forms_Crease"
    Label = QtCore.QT_TRANSLATE_NOOP("Forms_Crease", "Crease")
    Description = QtCore.QT_TRANSLATE_NOOP(
        "Forms_Crease", "Makes selected control edges or face boundaries fully sharp"
    )
    Pixmap = "Forms_Crease"


class CommandUncrease(_CommandCreaseBase):
    Sharpness = 0.0
    CommandName = "Forms_Uncrease"
    Label = QtCore.QT_TRANSLATE_NOOP("Forms_Uncrease", "Uncrease")
    Description = QtCore.QT_TRANSLATE_NOOP(
        "Forms_Uncrease", "Removes sharpness from selected control edges or face boundaries"
    )
    Pixmap = "Forms_Uncrease"


class CommandStraighten:
    def GetResources(self):
        return {
            "Pixmap": "Forms_Straighten",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_Straighten", "Straighten"),
            "ToolTip": App.Qt.translate(
                "Forms_Straighten", "Aligns selected control points onto a chosen line"
            ),
        }

    def IsActive(self):
        session = _active_edit_session()
        selections = _selected_control_points()
        return (
            App.ActiveDocument is not None
            and session is not None
            and not session.has_active_tool()
            and bool(selections.get(session.obj))
        )

    def Activated(self):
        session = _active_edit_session()
        selections = _selected_control_points()
        if session is not None and selections.get(session.obj):
            session.start_straighten_tool(selections[session.obj])


class CommandFlatten:
    def GetResources(self):
        return {
            "Pixmap": "Forms_Flatten",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_Flatten", "Flatten"),
            "ToolTip": App.Qt.translate(
                "Forms_Flatten", "Flattens selected controls onto a chosen plane"
            ),
        }

    def IsActive(self):
        session = _active_edit_session()
        selections = _selected_control_points()
        return (
            App.ActiveDocument is not None
            and session is not None
            and not session.has_active_tool()
            and len(selections.get(session.obj, ())) >= 3
        )

    def Activated(self):
        session = _active_edit_session()
        selections = _selected_control_points()
        if session is not None and len(selections.get(session.obj, ())) >= 3:
            session.start_flatten_tool(selections[session.obj])


class CommandSubdivide:
    def GetResources(self):
        return {
            "Pixmap": "Forms_Subdivide",
            "CmdType": "ForEdit",
            "MenuText": App.Qt.translate("Forms_Subdivide", "Subdivide"),
            "ToolTip": App.Qt.translate(
                "Forms_Subdivide",
                "Subdivides selected logical faces into a 2 by 2 control grid",
            ),
        }

    def IsActive(self):
        session = _active_edit_session()
        return (
            App.ActiveDocument is not None and session is not None and not session.has_active_tool()
        )

    def Activated(self):
        session = _active_edit_session()
        if session is not None:
            session.start_subdivide_tool()


Gui.addCommand("Forms_DeleteFaces", CommandDeleteFaces())
Gui.addCommand("Forms_DeleteEdges", CommandDeleteEdges())
Gui.addCommand("Forms_EraseAndFill", CommandEraseAndFill())
Gui.addCommand("Forms_SetPivot", CommandSetPivot())
Gui.addCommand("Forms_InsertEdge", CommandInsertEdge())
Gui.addCommand("Forms_InsertPoint", CommandInsertPoint())
Gui.addCommand("Forms_InsertEdgeLoop", CommandInsertEdgeLoop())
Gui.addCommand("Forms_Thicken", CommandThicken())
Gui.addCommand("Forms_FillHole", CommandFillHole())
Gui.addCommand("Forms_Bridge", CommandBridge())
Gui.addCommand("Forms_Unweld", CommandUnweld())
Gui.addCommand("Forms_Weld", CommandWeld())
Gui.addCommand("Forms_Match", CommandMatch())
Gui.addCommand("Forms_Crease", CommandCrease())
Gui.addCommand("Forms_Uncrease", CommandUncrease())
Gui.addCommand("Forms_Straighten", CommandStraighten())
Gui.addCommand("Forms_Flatten", CommandFlatten())
Gui.addCommand("Forms_Subdivide", CommandSubdivide())
