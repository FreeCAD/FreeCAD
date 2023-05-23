# ***************************************************************************
# *   Copyright (c) 2016 Ofentse Kgoa <kgoaot@eskom.co.za>                  *
# *   Copyright (c) 2018 Bernd Hahnebach <bernd@bimstatik.org>              *
# *   Based on the FemElementGeometry1D by Bernd Hahnebach                  *
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

__title__ = "FreeCAD FEM element fluid 1D ViewProvider for the document object"
__author__ = "Ofentse Kgoa, Bernd Hahnebach"
__url__ = "https://www.freecad.org"

## @package view_element_fluid1D
#  \ingroup FEM
#  \brief view provider for element fluid 1D object

from femtaskpanels import task_element_fluid1D
from . import view_base_femconstraint


class VPElementFluid1D(view_base_femconstraint.VPBaseFemConstraint):
    """
    A View Provider for the ElementFluid1D object
    """

    def setEdit(self, vobj, mode=0):
        view_base_femconstraint.VPBaseFemConstraint.setEdit(
            self,
            vobj,
            mode,
            task_element_fluid1D._TaskPanel
        )
