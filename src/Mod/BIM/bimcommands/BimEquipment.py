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

"""BIM equipment commands"""


import os
import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate
PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")



class Arch_Equipment:

    "the Arch Equipment command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_Equipment',
                'MenuText': QT_TRANSLATE_NOOP("Arch_Equipment","Equipment"),
                'Accel': "E, Q",
                'ToolTip': QT_TRANSLATE_NOOP("Arch_Equipment","Creates an equipment from a selected object (Part or Mesh)")}

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):

        s = FreeCADGui.Selection.getSelection()
        if not s:
            FreeCAD.Console.PrintError(translate("Arch","You must select a base shape object and optionally a mesh object"))
        else:
            base = ""
            mesh = ""
            if len(s) == 2:
                if hasattr(s[0],'Shape'):
                    base = s[0].Name
                elif s[0].isDerivedFrom("Mesh::Feature"):
                    mesh = s[0].Name
                if hasattr(s[1],'Shape'):
                    if mesh:
                        base = s[1].Name
                elif s[1].isDerivedFrom("Mesh::Feature"):
                    if base:
                        mesh = s[1].Name
            else:
                if hasattr(s[0],'Shape'):
                    base = s[0].Name
                elif s[0].isDerivedFrom("Mesh::Feature"):
                    mesh = s[0].Name
            FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create Equipment")))
            FreeCADGui.addModule("Arch")
            if base:
                base = "FreeCAD.ActiveDocument." + base
            FreeCADGui.doCommand("obj = Arch.makeEquipment(" + base + ")")
            if mesh:
                FreeCADGui.doCommand("obj.Mesh = FreeCAD.ActiveDocument." + mesh)
            FreeCADGui.addModule("Draft")
            FreeCADGui.doCommand("Draft.autogroup(obj)")
            FreeCAD.ActiveDocument.commitTransaction()
            FreeCAD.ActiveDocument.recompute()
            # get diffuse color info from base object
            if base and hasattr(s[0].ViewObject,"DiffuseColor"):
                FreeCADGui.doCommand("FreeCAD.ActiveDocument.Objects[-1].ViewObject.DiffuseColor = " + base + ".ViewObject.DiffuseColor")
        return


class Arch_3Views:

    # OBSOLETE

    "the Arch 3Views command definition"

    def GetResources(self):

        return {'Pixmap'  : 'Arch_3Views',
                'MenuText': QT_TRANSLATE_NOOP("Arch_3Views","3 views from mesh"),
                'ToolTip': QT_TRANSLATE_NOOP("Arch_3Views","Creates 3 views (top, front, side) from a mesh-based object")}

    def IsActive(self):

        return not FreeCAD.ActiveDocument is None

    def Activated(self):

        s = FreeCADGui.Selection.getSelection()
        if len(s) != 1:
            FreeCAD.Console.PrintError(translate("Arch","You must select exactly one base object"))
        else:
            obj = s[0]
            if not obj.isDerivedFrom("Mesh::Feature"):
                FreeCAD.Console.PrintError(translate("Arch","The selected object must be a mesh"))
            else:
                if obj.Mesh.CountFacets > 1000:
                    msgBox = QtGui.QMessageBox()
                    msgBox.setText(translate("Arch","This mesh has more than 1000 facets."))
                    msgBox.setInformativeText(translate("Arch","This operation can take a long time. Proceed?"))
                    msgBox.setStandardButtons(QtGui.QMessageBox.Ok | QtGui.QMessageBox.Cancel)
                    msgBox.setDefaultButton(QtGui.QMessageBox.Cancel)
                    ret = msgBox.exec_()
                    if ret == QtGui.QMessageBox.Cancel:
                        return
                elif obj.Mesh.CountFacets >= 500:
                    FreeCAD.Console.PrintWarning(translate("Arch","The mesh has more than 500 facets. This will take a couple of minutes..."))
                FreeCAD.ActiveDocument.openTransaction(str(translate("Arch","Create 3 views")))
                FreeCADGui.addModule("Arch")
                FreeCADGui.addModule("Part")
                FreeCADGui.doCommand("s1 = Arch.createMeshView(FreeCAD.ActiveDocument." + obj.Name + ",FreeCAD.Vector(0,0,-1),outeronly=False,largestonly=False)")
                FreeCADGui.doCommand("Part.show(s1)")
                FreeCADGui.doCommand("s2 = Arch.createMeshView(FreeCAD.ActiveDocument." + obj.Name + ",FreeCAD.Vector(1,0,0),outeronly=False,largestonly=False)")
                FreeCADGui.doCommand("Part.show(s2)")
                FreeCADGui.doCommand("s3 = Arch.createMeshView(FreeCAD.ActiveDocument." + obj.Name + ",FreeCAD.Vector(0,1,0),outeronly=False,largestonly=False)")
                FreeCADGui.doCommand("Part.show(s3)")
                FreeCAD.ActiveDocument.commitTransaction()
                FreeCAD.ActiveDocument.recompute()
        return




FreeCADGui.addCommand('Arch_Equipment',Arch_Equipment())
#FreeCADGui.addCommand('Arch_3Views',   Arch_3Views())
