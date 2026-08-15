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

"""Keyboard and selection routing for a live Forms edit session."""

import FreeCADGui as Gui
from PySide import QtCore, QtWidgets

from .cage import canonical_subelement_name


class FormKeyFilter(QtCore.QObject):
    """Route editor shortcuts without replacing FreeCAD selection behavior."""

    def __init__(self, session):
        super().__init__()
        self.session = session

    def _has_selected_element(self, prefix):
        for selection in Gui.Selection.getSelectionEx():
            if selection.Object != self.session.obj:
                continue
            if any(
                canonical_subelement_name(name).startswith(prefix)
                for name in selection.SubElementNames
            ):
                return True
        return False

    @staticmethod
    def _focus_edits_text():
        focus = QtWidgets.QApplication.focusWidget()
        return isinstance(
            focus,
            (
                QtWidgets.QLineEdit,
                QtWidgets.QAbstractSpinBox,
                QtWidgets.QTextEdit,
                QtWidgets.QPlainTextEdit,
            ),
        )

    def eventFilter(self, _watched, event):
        session = self.session
        if (
            not session.cleaned
            and session.pivot_tool_active
            and event.type() == QtCore.QEvent.KeyPress
            and event.key() == QtCore.Qt.Key_Escape
            and not event.isAutoRepeat()
        ):
            session.stop_set_pivot_tool()
            return True
        if (
            not session.cleaned
            and session.straighten_tool_active
            and event.type() == QtCore.QEvent.KeyPress
            and event.key() == QtCore.Qt.Key_Escape
            and not event.isAutoRepeat()
        ):
            session.stop_straighten_tool()
            return True
        if (
            not session.cleaned
            and session.flatten_tool_active
            and event.type() == QtCore.QEvent.KeyPress
            and event.key() == QtCore.Qt.Key_Escape
            and not event.isAutoRepeat()
        ):
            session.stop_flatten_tool()
            return True
        if (
            not session.cleaned
            and session.match_tool_active
            and event.type() == QtCore.QEvent.KeyPress
            and event.key() == QtCore.Qt.Key_Escape
            and not event.isAutoRepeat()
        ):
            session.stop_match_tool()
            return True
        if (
            not session.cleaned
            and session.weld_tool_active
            and event.type() == QtCore.QEvent.KeyPress
            and event.key() == QtCore.Qt.Key_Escape
            and not event.isAutoRepeat()
        ):
            session.stop_weld_tool()
            return True
        if (
            not session.cleaned
            and session.thicken_tool_active
            and event.type() == QtCore.QEvent.KeyPress
            and event.key() == QtCore.Qt.Key_Escape
            and not event.isAutoRepeat()
        ):
            session.stop_thicken_tool(apply=False)
            return True
        if (
            not session.cleaned
            and session.surface_tool_active
            and event.type() == QtCore.QEvent.KeyPress
            and not event.isAutoRepeat()
        ):
            if event.key() == QtCore.Qt.Key_M:
                return session.toggle_surface_tool_orientation()
            if event.key() == QtCore.Qt.Key_Escape:
                session.stop_surface_tool()
                return True
        if (
            not session.cleaned
            and event.type() == QtCore.QEvent.ShortcutOverride
            and event.key() == QtCore.Qt.Key_Delete
            and not event.isAutoRepeat()
            and not self._focus_edits_text()
            and (
                self._has_selected_element("Face")
                or self._has_selected_element("Edge")
            )
        ):
            event.accept()
            Gui.runCommand(
                "Forms_DeleteFaces"
                if self._has_selected_element("Face")
                else "Forms_DeleteEdges"
            )
            return True
        return False


class FormSelectionGate:
    """Admit useful subelements of the form currently being edited."""

    def __init__(self, session):
        self.session = session

    def allow(self, document, obj, subelement):
        if self.session.cleaned or not subelement:
            return False
        object_name = getattr(obj, "Name", str(obj))
        subelement = self.session._form_selection_subelement(document, object_name, subelement)
        if subelement is None:
            return False
        subelement = canonical_subelement_name(subelement)
        if not subelement:
            return False
        selection_filter = (
            "Edge"
            if self.session.insert_point_tool_active or self.session.unweld_tool_active
            else self.session.selection_filter.currentData()
        )
        prefixes = {
            "Point": ("Vertex",),
            "Edge": ("Edge",),
            "Face": ("Face",),
            "All": ("Vertex", "Edge", "Face"),
        }
        return subelement.startswith(prefixes[selection_filter])

    def getGatedTypes(self, all_types):
        selection_filter = (
            "Edge"
            if self.session.insert_point_tool_active or self.session.unweld_tool_active
            else self.session.selection_filter.currentData()
        )
        requested = {
            "Point": {"Vertex"},
            "Edge": {"Edge"},
            "Face": {"Face"},
            "All": {"Vertex", "Edge", "Face"},
        }[selection_filter]
        return [element_type for element_type in all_types if element_type in requested]


__all__ = ["FormKeyFilter", "FormSelectionGate"]
