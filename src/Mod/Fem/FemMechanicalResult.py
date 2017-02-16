#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2016 - Qingfeng Xia <qingfeng.xia()eng.ox.ac.uk> *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__title__ = "module to make a mechanical FEM result object"
__author__ = "Qingfeng Xia, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  \brief FreeCAD module to make mechanical result object in FEM workbench
#  @{

import FreeCAD
import _FemMechanicalResult


def makeFemMechanicalResult(result_obj_name):
    obj = FreeCAD.ActiveDocument.addObject('Fem::FemResultObjectPython', result_obj_name)
    _FemMechanicalResult._FemMechanicalResult(obj)
    if FreeCAD.GuiUp:
        from _ViewProviderFemMechanicalResult import _ViewProviderFemMechanicalResult
        _ViewProviderFemMechanicalResult(obj.ViewObject)
    return obj

#  @}
