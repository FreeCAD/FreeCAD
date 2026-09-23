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

"""Create a named work plane on a Job.

A work plane is a plain ``Part::LocalCoordinateSystem``. Everything the
feature needs from it already exists on that object: an attachment to the
geometry it was derived from, the map modes that describe a plane from a face
or from three points, an editable offset, a placement that recomputes when the
model moves, and a view provider that draws labelled axes. This module only
creates one, attaches it, and files it under the Job.
"""

import FreeCAD
import Path
import Path.Base.Util as PathUtil
import Path.Main.Workplane as PathWorkplane

from PySide.QtCore import QT_TRANSLATE_NOOP

if FreeCAD.GuiUp:
    import FreeCADGui

__title__ = "CAM Workplane Command"
__author__ = "sliptonic (Brad Collette)"
__url__ = "https://www.freecad.org"
__doc__ = "Command to create a named work plane on a Job."

translate = FreeCAD.Qt.translate


def _selectedFace():
    """Return (object, subname) for a single selected planar face, else (None, None)."""
    selection = FreeCADGui.Selection.getSelectionEx()
    if len(selection) != 1 or len(selection[0].SubElementNames) != 1:
        return (None, None)

    sel = selection[0]
    sub = sel.SubElementNames[0]
    if not sub.startswith("Face"):
        return (None, None)

    try:
        face = sel.Object.Shape.getElement(sub)
    except Exception:
        return (None, None)

    if not PathUtil.isPlanarFace(face):
        return (None, None)
    return (sel.Object, sub)


class CommandWorkplaneCreate:
    """Create a named work plane on the active Job."""

    def GetResources(self):
        return {
            # Not Std_CoordinateSystem: that is the LCS icon, and it already
            # appears in enough toolbars to be confusing next to this one.
            "Pixmap": "CAM_Area_Workplane",
            "MenuText": QT_TRANSLATE_NOOP("CAM_Workplane", "Work Plane"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "CAM_Workplane",
                "Create a named work plane on the Job, from a selected planar face "
                "or at the Job origin. Operations can share one work plane.",
            ),
        }

    def IsActive(self):
        if FreeCAD.ActiveDocument is None:
            return False
        return bool(self._jobs())

    @staticmethod
    def _jobs():
        import Path.Main.Job as PathJob

        return [
            o
            for o in FreeCAD.ActiveDocument.Objects
            if hasattr(o, "Proxy") and isinstance(o.Proxy, PathJob.ObjectJob)
        ]

    def Activated(self):
        jobs = self._jobs()
        if not jobs:
            Path.Log.error(translate("CAM", "No Job found to add a work plane to."))
            return

        base, sub = _selectedFace()
        job = jobs[0]
        if base is not None:
            # Prefer the Job whose model - original or clone - the face belongs to.
            for candidate in jobs:
                if PathWorkplane.resolveToJobModel(candidate, base) is not None:
                    job = candidate
                    break

        workplane = PathWorkplane.createWorkplane(job, base, sub)
        if base is None:
            Path.Log.info(
                translate(
                    "CAM",
                    "Created %s at the Job origin. Select a planar face before the "
                    "command to derive one from the model.",
                )
                % workplane.Label
            )
        FreeCADGui.Selection.clearSelection()
        FreeCADGui.Selection.addSelection(workplane)


if FreeCAD.GuiUp:
    FreeCADGui.addCommand("CAM_Workplane", CommandWorkplaneCreate())

FreeCAD.Console.PrintLog("Loading WorkplaneCmd… done\n")
