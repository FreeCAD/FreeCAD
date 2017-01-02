# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *
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

__title__ = "_CommandMaterial"
__author__ = "Juergen Riegel, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package CommandMaterial
#  \ingroup FEM

import FreeCAD
from FemCommands import FemCommands
import FreeCADGui
import FemGui
from PySide import QtCore


class _CommandMaterial(FemCommands):
    "the Fem_Material command definition"
    def __init__(self):
        super(_CommandMaterial, self).__init__()
        self.resources = {'Pixmap': 'fem-material',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Material", "FEM material"),
                          'Accel': "M, M",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Material", "Creates a FEM material")}
        self.is_active = 'with_analysis'

    def Activated(self):
        femDoc = FemGui.getActiveAnalysis().Document
        if FreeCAD.ActiveDocument is not femDoc:
            FreeCADGui.setActiveDocument(femDoc)
        FreeCAD.ActiveDocument.openTransaction("Create Material")
        FreeCADGui.addModule("FemMaterial")
        FreeCADGui.doCommand("FemMaterial.makeFemMaterial('FemMaterial')")
        FreeCADGui.doCommand("App.activeDocument()." + FemGui.getActiveAnalysis().Name + ".Member = App.activeDocument()." + FemGui.getActiveAnalysis().Name + ".Member + [App.ActiveDocument.ActiveObject]")
        FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")


FreeCADGui.addCommand('Fem_Material', _CommandMaterial())
