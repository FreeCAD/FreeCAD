# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2015 Bernd Hahnebach <bernd@bimstatik.org>              *
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

__title__ = "FreeCAD FEM constraint tie ViewProvider for the document object"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## @package ViewProviderFemConstraintTie
#  \ingroup FEM
#  \brief FreeCAD FEM _ViewProviderFemConstraintTie

import FreeCAD
import FreeCADGui
import FemGui  # needed to display the icons in TreeView

# for the panel
from PySide import QtCore
from . import FemSelectionWidgets

False if FemGui.__name__ else True  # flake8, dummy FemGui usage


class _ViewProviderFemConstraintTie:
    "A View Provider for the FemConstraintTie object"

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-constraint-tie.svg"

    def attach(self, vobj):
        from pivy import coin
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.standard = coin.SoGroup()
        vobj.addDisplayMode(self.standard, "Default")

    def getDisplayModes(self, obj):
        return ["Default"]

    def getDefaultDisplayMode(self):
        return "Default"

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def setEdit(self, vobj, mode=0):
        return True

    def unsetEdit(self, vobj, mode=0):
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None
