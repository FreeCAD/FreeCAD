# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2024 Yorik van Havre <yorik@uncreated.net>              *
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

"""Misc Arch util commands"""


import os
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")



class Arch_CurtainWall:

    "the Arch Curtain Wall command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_CurtainWall',
                'MenuText': QT_TRANSLATE_NOOP("Arch_CurtainWall","Curtain Wall"),
                'Accel': "C, W",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_CurtainWall","Creates a curtain wall object from selected line or from scratch")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        sel = FreeCADGui.Selection.getSelection()
        if len(sel) > 1:
            FreeCAD.Console.PrintError(translate("Arch","Please select only one base object or none")+"\n")
        elif len(sel) == 1:
            # build on selection
            FreeCADGui.Control.closeDialog()
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Curtain Wall"))
            FreeCADGui.addModule("Draft")
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("obj = Arch.makeCurtainWall(FreeCAD.ActiveDocument."+FreeCADGui.Selection.getSelection()[0].Name+")")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
        else:
            # interactive line drawing
            self.points = []
            import WorkingPlane
            WorkingPlane.get_working_plane()
            if hasattr(FreeCADGui,"Snapper"):
                FreeCADGui.Snapper.getPoint(callback=self.getPoint)

    def getPoint(self,point=None,obj=None):

        """Callback for clicks during interactive mode"""

        if point is None:
            # cancelled
            return
        self.points.append(point)
        if len(self.points) == 1:
            FreeCADGui.Snapper.getPoint(last=self.points[0],callback=self.getPoint)
        elif len(self.points) == 2:
            FreeCADGui.Control.closeDialog()
            FreeCAD.ActiveDocument.openTransaction(translate("Arch","Create Curtain Wall"))
            FreeCADGui.addModule("Draft")
            FreeCADGui.addModule("Arch")
            FreeCADGui.doCommand("base = Draft.makeLine(FreeCAD."+str(self.points[0])+",FreeCAD."+str(self.points[1])+")")
            FreeCADGui.doCommand("obj = Arch.makeCurtainWall(base)")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()



FreeCADGui.addCommand('Arch_CurtainWall', Arch_CurtainWall())
