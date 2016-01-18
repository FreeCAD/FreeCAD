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

__title__ = "FemShellThickness"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"


import FreeCAD
import FemGui
import _FemShellThickness


def makeFemShellThickness(thickness=20.0, name="ShellThickness"):
    '''makeFemShellThickness([thickness], [name]): creates an shellthickness object to define a plate thickness'''
    obj = FemGui.getActiveAnalysis().Document.addObject("Fem::FeaturePython", name)
    _FemShellThickness._FemShellThickness(obj)
    obj.Thickness = thickness
    if FreeCAD.GuiUp:
        import _ViewProviderFemShellThickness
        _ViewProviderFemShellThickness._ViewProviderFemShellThickness(obj.ViewObject)
    return obj
