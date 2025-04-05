# -*- coding: utf8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

"""The BIM Leader command"""


import FreeCAD
import FreeCADGui
from draftguitools import gui_lines  # Line tool from Draft

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

class BIM_Leader(gui_lines.Line):

    def __init__(self):
        super().__init__(mode="leader")

    def GetResources(self):
        return {
            "Pixmap": "BIM_Leader",
            "Accel": "L, E",
            "MenuText": QT_TRANSLATE_NOOP("BIM_Leader", "Leader"),
            "ToolTip": QT_TRANSLATE_NOOP(
                "BIM_Leader", "Creates a polyline with an arrow at its endpoint"
            ),
        }

    def Activated(self):
        super().Activated(
            name="Leader", icon="BIM_Leader", task_title=translate("BIM", "Leader")
        )

    def finish(self, closed=False, cont=False):
        import DraftVecUtils
        from draftutils import params
        self.end_callbacks(self.call)
        self.removeTemporaryObject()
        if len(self.node) > 1:
            rot, sup, pts, fil = self.getStrings()
            base = DraftVecUtils.toString(self.node[0])
            color = params.get_param("DefaultTextColor") | 0x000000FF
            cmd_list = [
                "pl = FreeCAD.Placement()",
                "pl.Rotation.Q = " + rot,
                "pl.Base = " + base,
                "points = " + pts,
                "leader = Draft.make_wire(points, placement=pl)",
                "leader.ViewObject.LineColor = " + str(color),
                "leader.ViewObject.EndArrow = True",
                "Draft.autogroup(leader)",
                "FreeCAD.ActiveDocument.recompute()",
            ]
            FreeCADGui.addModule("Draft")
            self.commit(translate("BIM", "Create Leader"), cmd_list)
        super(gui_lines.Line, self).finish()
        if self.ui and self.ui.continueMode:
            self.Activated()

FreeCADGui.addCommand("BIM_Leader", BIM_Leader())
