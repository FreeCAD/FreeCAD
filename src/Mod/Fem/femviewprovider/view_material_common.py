# ***************************************************************************
# *   Copyright (c) 2013 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
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

__title__ = "FreeCAD FEM material ViewProvider for the document object"
__author__ = "Juergen Riegel, Bernd Hahnebach, Qingfeng Xia"
__url__ = "https://www.freecad.org"

## @package view_material_common
#  \ingroup FEM
#  \brief FreeCAD FEM _ViewProviderFemMaterial
#  \brief view provider for common material object

import FreeCAD

from femtaskpanels import task_material_common
from . import view_base_femmaterial


class VPMaterialCommon(view_base_femmaterial.VPBaseFemMaterial):
    """
    A View Provider for the MaterialCommon object
    """

    def getIcon(self):
        if hasattr(self.Object, "Category"):
            if self.Object.Category == "Solid":
                return ":/icons/FEM_MaterialSolid.svg"
            elif self.Object.Category == "Fluid":
                return ":/icons/FEM_MaterialFluid.svg"
            else:
                return ""
        else:
            FreeCAD.Console.Error("Document object does not have Category property")
            return ""

    def setEdit(self, vobj, mode=0):
        return super().setEdit(vobj, mode, task_material_common._TaskPanel)

    def claimChildren(self):
        nonlin = self.Object.Nonlinear
        if nonlin:
            return [nonlin]
        else:
            return []
