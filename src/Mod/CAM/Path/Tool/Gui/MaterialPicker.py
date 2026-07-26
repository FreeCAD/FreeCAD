# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

"""
Reusable Material picker dialog filtered to the Machinability model.

Mirrors the Job editor's "Assign Stock Material" pattern (see
``Path/Main/Gui/Job.py:MaterialDialog``) but is parameterised for use in
the feeds & speeds flows where we need a (uuid, name) pair returned.

TODO (refactor): Job.MaterialDialog and this module are doing the same
thing — wrap MatGui::MaterialTreeWidget with a Machinability filter and
return a selected UUID. They should converge on one helper, with Job.py
importing from here (or a relocated shared module). Left as a follow-up
to keep the F&S Phase-1 PoC diff focused.
"""

from typing import Optional, Tuple

import FreeCAD
import FreeCADGui
import MatGui
import Materials
from PySide import QtWidgets

translate = FreeCAD.Qt.translate


class MachinabilityMaterialDialog(QtWidgets.QDialog):
    """
    Modal Material picker filtered to materials carrying the Machinability
    model. On Accept, ``selected_uuid`` and ``selected_name`` are populated.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self.selected_uuid: Optional[str] = None
        self.selected_name: Optional[str] = None

        self.setWindowTitle(translate("CAM_FeedsSpeeds", "Choose material"))

        self._material_tree = FreeCADGui.UiLoader().createWidget("MatGui::MaterialTreeWidget")
        self._tree_wrapper = MatGui.MaterialTreeWidget(self._material_tree)

        material_filter = Materials.MaterialFilter()
        material_filter.Name = "Machining Materials"
        material_filter.RequiredModels = [Materials.UUIDs().Machinability]
        self._tree_wrapper.setFilter(material_filter)
        self._tree_wrapper.selectFilter("Machining Materials")
        self._material_tree.onMaterial.connect(self._on_material)

        button_box = QtWidgets.QDialogButtonBox(
            QtWidgets.QDialogButtonBox.Ok | QtWidgets.QDialogButtonBox.Cancel
        )
        self._ok_button = button_box.button(QtWidgets.QDialogButtonBox.Ok)
        self._ok_button.setEnabled(False)
        button_box.accepted.connect(self.accept)
        button_box.rejected.connect(self.reject)

        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(self._material_tree)
        layout.addWidget(button_box)

    def _on_material(self, uuid: str) -> None:
        self.selected_uuid = uuid or None
        self.selected_name = lookup_material_name(self.selected_uuid)
        self._ok_button.setEnabled(self.selected_uuid is not None)


def lookup_material_name(uuid: Optional[str]) -> Optional[str]:
    """Resolve a material UUID to its display name, or None if not found."""
    if not uuid:
        return None
    try:
        material = Materials.MaterialManager().getMaterial(uuid)
    except Exception:
        return None
    if material is None:
        return None
    return getattr(material, "Name", None) or None


def pick_machinability_material(parent=None) -> Optional[Tuple[str, str]]:
    """
    Open the picker and return the chosen ``(uuid, name)`` pair, or
    ``None`` if the user cancelled.
    """
    dialog = MachinabilityMaterialDialog(parent)
    if dialog.exec_() != QtWidgets.QDialog.Accepted:
        return None
    if dialog.selected_uuid is None:
        return None
    return (dialog.selected_uuid, dialog.selected_name or "")
