# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Markus Hovorka <m.hovorka@live.de>               *
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

__title__ = "_CommandFemOpenParaview"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"

## @package CommandElmerFreeText
#  \ingroup FEM

import os.path
from PySide import QtCore

import FreeCAD as App
import FreeCADGui as Gui
from FemCommands import FemCommands
import FemGui
import FemToolsElmer
import Report


_PARAVIEW_BIN = "paraview"
_RESULT_NAME = "case0001.vtu"


class _CommandFemOpenParaview(FemCommands):
    """The Fem_SolverElmer command definition."""

    def __init__(self):
        super(_CommandFemOpenParaview, self).__init__()
        self.resources = {
                'Pixmap': 'fem-paraview',
                'MenuText': QtCore.QT_TRANSLATE_NOOP(
                    "Fem_OpenParaview", "Open in Paraview"),
                'Accel': "O, P",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP(
                    "Fem_OpenParaview", "Open result in paraview")
        }
        self.is_active = 'with_analysis'

    def Activated(self):
        analysis = FemGui.getActiveAnalysis()
        caseDir = FemToolsElmer.findCaseDir(analysis)
        if caseDir != "":
            resultPath = os.path.join(caseDir, _RESULT_NAME)
            if os.path.isfile(resultPath):
                QtCore.QProcess.startDetached(
                        _PARAVIEW_BIN, [resultPath])
                return
        report = Report.Data()
        report.appendError("solve_first")
        Report.display(report)
        

Gui.addCommand('FEM_OpenParaview', _CommandFemOpenParaview())
