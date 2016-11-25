# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Bernd Hahnebach <bernd@bimstatik.org>            *
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

__title__ = "Command constraint self weight"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package CommandConstraintSelfWeight
#  \ingroup FEM

import FreeCAD
from FemCommands import FemCommands
import FreeCADGui
from PySide import QtCore


class _CommandConstraintSelfWeight(FemCommands):
    "The Fem_ConstraintSelfWeight command definition"
    def __init__(self):
        super(_CommandConstraintSelfWeight, self).__init__()
        self.resources = {'Pixmap': 'fem-constraint-selfweight',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_ConstraintSelfWeight", "Constraint self weigt"),
                          'Accel': "C, W",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_ConstraintSelfWeight", "Creates a FEM constraint self weigt")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemConstraintSelfWeight")
        FreeCADGui.addModule("FemConstraintSelfWeight")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [FemConstraintSelfWeight.makeFemConstraintSelfWeight()]")


FreeCADGui.addCommand('Fem_ConstraintSelfWeight', _CommandConstraintSelfWeight())
