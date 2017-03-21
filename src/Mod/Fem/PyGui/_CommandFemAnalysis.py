# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013-2015 - Juergen Riegel <FreeCAD@juergen-riegel.net> *
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

__title__ = "Command New Analysis"
__author__ = "Juergen Riegel"
__url__ = "http://www.freecadweb.org"

## @package CommandFemAnalysis
#  \ingroup FEM

import FreeCAD
from .FemCommands import FemCommands
import FreeCADGui
from PySide import QtCore


class _CommandFemAnalysis(FemCommands):
    "the FEM_Analysis command definition"
    def __init__(self):
        super(_CommandFemAnalysis, self).__init__()
        self.resources = {'Pixmap': 'fem-analysis',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_Analysis", "Analysis container"),
                          'Accel': "N, A",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_Analysis", "Creates a analysis container with standard solver CalculiX")}
        self.is_active = 'with_document'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create Analysis")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("ObjectsFem.makeAnalysis('Analysis')")
        FreeCADGui.doCommand("FemGui.setActiveAnalysis(App.activeDocument().ActiveObject)")
        FreeCADGui.doCommand("ObjectsFem.makeSolverCalculix('CalculiX')")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Fem::FemMeshObject")):
                FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument()." + sel[0].Name + "]")
        FreeCADGui.Selection.clearSelection()

FreeCADGui.addCommand('FEM_Analysis', _CommandFemAnalysis())
