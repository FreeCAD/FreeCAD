# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 - Juergen Riegel <FreeCAD@juergen-riegel.net>      *
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

__title__ = "FemMaterial"
__author__ = "Juergen Riegel, Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

## \addtogroup FEM
#  @{

import FreeCAD
import _FemMaterial


def makeSolidMaterial(name):
    '''makeSolidMaterial(name): makes an FEM Material for solid
    '''
    obj = FreeCAD.ActiveDocument.addObject("App::MaterialObjectPython", name)
    _FemMaterial._FemMaterial(obj)
    obj.Category = 'Solid'
    if FreeCAD.GuiUp:
        import _ViewProviderFemMaterial
        _ViewProviderFemMaterial._ViewProviderFemMaterial(obj.ViewObject)
    # FreeCAD.ActiveDocument.recompute()
    return obj


def makeFluidMaterial(name):
    '''makeFluidMaterial(name): makes an FEM Material for fluid
    '''
    obj = FreeCAD.ActiveDocument.addObject("App::MaterialObjectPython", name)
    _FemMaterial._FemMaterial(obj)
    obj.Category = 'Fluid'
    if FreeCAD.GuiUp:
        import _ViewProviderFemMaterial
        _ViewProviderFemMaterial._ViewProviderFemMaterial(obj.ViewObject)
    # FreeCAD.ActiveDocument.recompute()
    return obj

makeFemMaterial = makeSolidMaterial  # alias to be compatible for FemTest.py
# @}
