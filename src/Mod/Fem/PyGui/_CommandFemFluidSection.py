# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2016 - Ofentse Kgoa <kgoaot@eskom.co.za>                *
# *   Based on the FemElementGeometry1D by Bernd Hahnebach                        *
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

__title__ = "_CommandFluidSection"
__author__ = "Ofentse Kgoa"
__url__ = "http://www.freecadweb.org"

## @package CommandFemFluidSection
#  \ingroup FEM

import FreeCAD
from FemCommands import FemCommands
import FreeCADGui
from PySide import QtCore


class _CommandFemFluidSection(FemCommands):
    "The FEM_FluidSection command definition"
    def __init__(self):
        super(_CommandFemFluidSection, self).__init__()
        self.resources = {'Pixmap': 'fem-fluid-section',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("FEM_FluidSection", "Fluid section for 1D flow"),
                          'Accel': "C, B",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("FEM_FluidSection", "Creates a FEM Fluid section for 1D flow")}
        self.is_active = 'with_analysis'

    def Activated(self):
        FreeCAD.ActiveDocument.openTransaction("Create FemFluidSection")
        FreeCADGui.addModule("ObjectsFem")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [ObjectsFem.makeFemFluidSection()]")


FreeCADGui.addCommand('FEM_FluidSection', _CommandFemFluidSection())
