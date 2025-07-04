# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""BIM Truss command"""

import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Arch_Truss:

    "the Arch Truss command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Truss',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Truss","Truss"),
                'Accel': "T, U",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Truss","Creates a truss object from the selected line or from scratch")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        self.doc = FreeCAD.ActiveDocument
        sel = FreeCADGui.Selection.getSelection()
        if len(sel) > 1:
            FreeCAD.Console.PrintError(translate("Arch","Select only one base object or none")+"\n")
        elif len(sel) == 1:
            # build on selection
            basename = "FreeCAD.ActiveDocument."+FreeCADGui.Selection.getSelection()[0].Name
            self.createTruss(basename)
        else:
            # interactive line drawing
            import WorkingPlane

            FreeCAD.activeDraftCommand = self  # register as a Draft command for auto grid on/off
            self.points = []
            WorkingPlane.get_working_plane()
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.getPoint(callback=self.getPoint)

    def getPoint(self,point=None,obj=None):

        """Callback for clicks during interactive mode"""

        if point is None:
            # cancelled
            FreeCAD.activeDraftCommand = None
            FreeCADGui.Snapper.off()
            return
        self.points.append(point)
        if len(self.points) == 1:
            FreeCADGui.Snapper.getPoint(last=self.points[0],callback=self.getPoint)
        elif len(self.points) == 2:
            FreeCAD.activeDraftCommand = None
            FreeCADGui.Snapper.off()
            self.createTruss()

    def createTruss(self, basename=""):

        """Creates the truss"""

        FreeCADGui.Control.closeDialog()
        self.doc.openTransaction(translate("Arch","Create Truss"))
        FreeCADGui.addModule("Draft")
        FreeCADGui.addModule("Arch")
        if not basename:
            if self.points:
                cmd = "base = Draft.makeLine(FreeCAD."
                cmd += str(self.points[0])+",FreeCAD."+str(self.points[1])+")"
                FreeCADGui.doCommand(cmd)
                basename = "base"
        FreeCADGui.doCommand("obj = Arch.makeTruss("+basename+")")
        FreeCADGui.doCommand("Draft.autogroup(obj)")
        self.doc.commitTransaction()
        self.doc.recompute()


FreeCADGui.addCommand('Arch_Truss', Arch_Truss())
