# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2017 - Markus Hovorka <m.hovorka@live.de>               *
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


__title__ = "AddConstraintElectrostaticPotential"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"

## @package CommandFemConstraintElectrostaticPotential
#  \ingroup FEM

import FreeCAD
from .FemCommands import FemCommands
import FreeCADGui
from PySide import QtCore


class _CommandFemConstraintElectrostaticPotential(FemCommands):
    "The FEM_ConstraintElectrostaticPotential command definition"
    def __init__(self):
        super(_CommandFemConstraintElectrostaticPotential, self).__init__()
        self.resources = {
            'Pixmap': 'fem-constraint-electrostatic-potential',
            'MenuText': QtCore.QT_TRANSLATE_NOOP(
                "FEM_ConstraintElectrostaticPotential",
                "Constraint Potenial"),
            'ToolTip': QtCore.QT_TRANSLATE_NOOP(
                "FEM_ConstraintElectrostaticPotential",
                "Creates a FEM constraint electrostatic potential")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction(
            "Create FemConstraintElectrostaticPotential")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand(
            "FemGui.getActiveAnalysis().Member += "
            "[ObjectsFem.makeConstraintElectrostaticPotential(FreeCAD.ActiveDocument)]")


FreeCADGui.addCommand('FEM_ConstraintElectrostaticPotential', _CommandFemConstraintElectrostaticPotential())
