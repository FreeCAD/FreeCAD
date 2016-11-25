# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "_CommandShellThickness"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package CommandShellThickness
#  \ingroup FEM

import FreeCAD
from FemCommands import FemCommands
import FreeCADGui
from PySide import QtCore


class _CommandShellThickness(FemCommands):
    "The Fem_ShellThickness command definition"
    def __init__(self):
        super(_CommandShellThickness, self).__init__()
        self.resources = {'Pixmap': 'fem-shell-thickness',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_ShellThickness", "Shell plate thickness"),
                          'Accel': "C, S",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_ShellThickness", "Creates a FEM shell plate thickness")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemShellThickness")
        FreeCADGui.addModule("FemShellThickness")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [FemShellThickness.makeFemShellThickness()]")


FreeCADGui.addCommand('Fem_ShellThickness', _CommandShellThickness())
