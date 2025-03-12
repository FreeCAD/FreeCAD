#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2021 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************


"""This module contains FreeCAD commands for the Draft workbench"""

import os
import FreeCAD
from draftguitools import gui_base
from draftutils import params
from draftutils.todo import todo
from draftutils.translate import QT_TRANSLATE_NOOP, translate


class Draft_Hatch(gui_base.GuiCommandNeedsSelection):


    def GetResources(self):

        return {"Pixmap": "Draft_Hatch",
                "MenuText": QT_TRANSLATE_NOOP("Draft_Hatch", "Hatch"),
                "Accel": "H, A",
                "ToolTip": QT_TRANSLATE_NOOP("Draft_Hatch", "Creates hatches on the faces of a selected object")}

    def Activated(self):

        import FreeCADGui

        if FreeCADGui.Selection.getSelection():
            FreeCADGui.Control.showDialog(Draft_Hatch_TaskPanel(FreeCADGui.Selection.getSelection()[0]))
        else:
            FreeCAD.Console.PrintError(translate("Draft", "You must choose a base object before using this command") + "\n")


class Draft_Hatch_TaskPanel:


    def __init__(self,baseobj):

        import FreeCADGui
        from PySide import QtCore,QtGui
        import Draft_rc

        self.baseobj = baseobj
        self.form = FreeCADGui.PySideUic.loadUi(":/ui/dialogHatch.ui")
        self.form.setWindowIcon(QtGui.QIcon(":/icons/Draft_Hatch.svg"))
        self.form.File.fileNameChanged.connect(self.onFileChanged)
        self.form.File.setFileName(params.get_param("HatchPatternFile"))
        pat = params.get_param("HatchPatternName")
        if pat in [self.form.Pattern.itemText(i) for i in range(self.form.Pattern.count())]:
            self.form.Pattern.setCurrentText(pat)
        self.form.Scale.setValue(params.get_param("HatchPatternScale"))
        self.form.Rotation.setValue(params.get_param("HatchPatternRotation"))
        todo.delay(self.form.setFocus, None)  # Make sure using Esc works.

    def accept(self):

        import FreeCADGui

        params.set_param("HatchPatternFile", self.form.File.property("fileName"))
        params.set_param("HatchPatternName", self.form.Pattern.currentText())
        params.set_param("HatchPatternScale", self.form.Scale.value())
        params.set_param("HatchPatternRotation", self.form.Rotation.value())
        if hasattr(self.baseobj,"File") and hasattr(self.baseobj,"Pattern"):
            # modify existing hatch object
            o = "FreeCAD.ActiveDocument.getObject(\""+self.baseobj.Name+"\")"
            FreeCADGui.doCommand(o+".File=\""+self.form.File.property("fileName")+"\"")
            FreeCADGui.doCommand(o+".Pattern=\""+self.form.Pattern.currentText()+"\"")
            FreeCADGui.doCommand(o+".Scale="+str(self.form.Scale.value()))
            FreeCADGui.doCommand(o+".Rotation="+str(self.form.Rotation.value()))
        else:
            # create new hatch object
            FreeCAD.ActiveDocument.openTransaction("Create Hatch")
            FreeCADGui.addModule("Draft")
            cmd = "Draft.make_hatch("
            cmd += "baseobject=FreeCAD.ActiveDocument.getObject(\""+self.baseobj.Name
            cmd += "\"),filename=\""+self.form.File.property("fileName")
            cmd += "\",pattern=\""+self.form.Pattern.currentText()
            cmd += "\",scale="+str(self.form.Scale.value())
            cmd += ",rotation="+str(self.form.Rotation.value())+")"
            FreeCADGui.doCommand(cmd)
            FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.doCommand("FreeCAD.ActiveDocument.recompute()")
        self.reject()

    def reject(self):

        import FreeCADGui

        FreeCADGui.Control.closeDialog()
        FreeCADGui.ActiveDocument.resetEdit()
        FreeCAD.ActiveDocument.recompute()

    def onFileChanged(self,filename):

        pat = self.form.Pattern.currentText()
        self.form.Pattern.clear()
        patterns = self.getPatterns(filename)
        self.form.Pattern.addItems(patterns)
        if pat in patterns:
            self.form.Pattern.setCurrentText(pat)

    def getPatterns(self,filename):

        """returns a list of pattern names found in a PAT file"""
        patterns = []
        if os.path.exists(filename):
            with open(filename) as patfile:
                for line in patfile:
                    if line.startswith("*"):
                        patterns.append(line.split(",")[0][1:])
        return patterns

if FreeCAD.GuiUp:
    import FreeCADGui
    FreeCADGui.addCommand("Draft_Hatch",Draft_Hatch())
