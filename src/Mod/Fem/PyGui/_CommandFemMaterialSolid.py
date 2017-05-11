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

__title__ = "_CommandSolidMaterial"
__author__ = "Juergen Riegel, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package CommandFemMaterialSolid
#  \ingroup FEM

import FreeCAD
from .FemCommands import FemCommands
import FreeCADGui
import FemGui
from PySide import QtCore


class _CommandFemMaterialSolid(FemCommands):
    "the FEM_MaterialSolid command definition"
    def __init__(self):
        super(_CommandFemMaterialSolid, self).__init__()
        self.resources = {'Pixmap': 'fem-material',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_MaterialSolid", "FEM material for solid"),
                          'Accel': "M, M",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_MaterialSolid", "Creates a FEM material for solid")}
        self.is_active = 'with_analysis'

    def Activated(self):
        femDoc = FemGui.getActiveAnalysis().Document
        if FreeCAD.ActiveDocument is not femDoc:
            FreeCADGui.setActiveDocument(femDoc)
        FreeCAD.ActiveDocument.openTransaction("Create Solid Material")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeMaterialSolid('SolidMaterial')")
        FreeCADGui.doCommand("App.activeDocument()." + FemGui.getActiveAnalysis().Name + ".Member = App.activeDocument()." + FemGui.getActiveAnalysis().Name + ".Member + [App.ActiveDocument.ActiveObject]")
        FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")


FreeCADGui.addCommand('FEM_MaterialSolid', _CommandFemMaterialSolid())
