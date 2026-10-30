# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) 2026 Jesse Trutna <jesse@trutna.com>                    *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful,            *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Lesser General Public License for more details.                   *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public     *
# *   License along with FreeCAD; if not, write to the Free Software        *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""Command search popup for sketch edit mode (Sketcher_CommandSearch).

Lists every sketch tool, constraint and dimension in the shared search popup
(freecad.gui.CommandSearch); outside sketches, S searches the toolbars instead.
"""

import FreeCAD
import FreeCADGui
from PySide import QtCore

from freecad.gui import CommandSearch

translate = FreeCAD.Qt.translate

PARAM_PATH = "User parameter:BaseApp/Preferences/Mod/Sketcher/CommandSearch"

# Commands that make no sense from the search box in edit mode.
EXCLUDED = {
    "Sketcher_CommandSearch",
    "Sketcher_CopyClipboard",
    "Sketcher_Cut",
    "Sketcher_Paste",
    "Sketcher_CreatePolylineLegacy",
    "Sketcher_EditSketch",
    "Sketcher_LeaveGroup",
    "Sketcher_MapSketch",
    "Sketcher_MergeSketches",
    "Sketcher_MirrorSketch",
    "Sketcher_NewSketch",
    "Sketcher_ProfilesHexagon1",
    "Sketcher_ReorientSketch",
    "Sketcher_ValidateSketch",
}


def _category(name, menu_text):
    n = name[len("Sketcher_") :]
    if "Dimension" in menu_text:
        return translate("Sketcher_CommandSearch", "Dimension")
    if n.startswith("Constrain"):
        return translate("Sketcher_CommandSearch", "Constraint")
    if n.startswith("Create"):
        return translate("Sketcher_CommandSearch", "Create")
    if n.startswith("Select"):
        return translate("Sketcher_CommandSearch", "Select")
    if n.startswith("BSpline"):
        return translate("Sketcher_CommandSearch", "B-Spline")
    if n.startswith(("View", "ArcOverlay", "SwitchVirtualSpace")):
        return translate("Sketcher_CommandSearch", "View")
    if n in ("LeaveSketch", "CancelSketch", "StopOperation"):
        return translate("Sketcher_CommandSearch", "Sketch")
    return translate("Sketcher_CommandSearch", "Modify")


def _entries():
    entries = []
    seen = set()
    for name in sorted(FreeCADGui.listCommands()):
        if not name.startswith("Sketcher_") or name in EXCLUDED:
            continue
        if name.startswith("Sketcher_Comp"):
            continue  # toolbar drop-downs; their members are listed individually
        cmd = FreeCADGui.Command.get(name)
        actions = cmd.getAction()
        if not actions:
            continue  # not exposed anywhere in the UI
        title = cmd.getInfo()["menuText"]
        # The same title can belong to more than one command; keep one of each.
        if title in seen:
            continue
        seen.add(title)
        entries.append(
            CommandSearch.Entry(
                name,
                title,
                _category(name, title),
                actions[0].icon(),
                lambda n=name: FreeCADGui.runCommand(n),
                cmd.isActive(),
            )
        )
    return entries


def show():
    return CommandSearch.show(
        _entries(),
        PARAM_PATH,
        translate("Sketcher_CommandSearch", "Search sketch tools and constraints…"),
        translate("Sketcher_CommandSearch", "Select suitable geometry first"),
    )


class _CommandSearch:
    def GetResources(self):
        return {
            "Pixmap": "edit-find",
            "MenuText": QtCore.QT_TRANSLATE_NOOP("Sketcher_CommandSearch", "Search Commands…"),
            "ToolTip": QtCore.QT_TRANSLATE_NOOP(
                "Sketcher_CommandSearch",
                "Opens a search box to find and start any sketch tool or constraint",
            ),
            "Accel": "S",
            "CmdType": "ForEdit",
        }

    def IsActive(self):
        doc = FreeCADGui.ActiveDocument
        vp = doc.getInEdit() if doc else None
        return bool(vp) and vp.Object.isDerivedFrom("Sketcher::SketchObject")

    def Activated(self):
        # Look the module up at call time so a reloaded module takes effect.
        import SketcherCommandSearch

        SketcherCommandSearch.show()


if "Sketcher_CommandSearch" not in FreeCADGui.listCommands():
    FreeCADGui.addCommand("Sketcher_CommandSearch", _CommandSearch())
