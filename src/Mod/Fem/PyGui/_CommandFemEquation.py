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


__title__ = "_CommandFemEquation"
__author__ = "Markus Hovorka"
__url__ = "http://www.freecadweb.org"


from PySide import QtCore

import FreeCAD as App
import FreeCADGui as Gui
import FemUtils


class _Base(QtCore.QObject):

    def getSpecifier(self):
        raise NotImplementedError()

    def Activated(self):
        s = Gui.Selection.getSelection()
        if len(s) == 1 and FemUtils.isDerivedFrom(s[0], "Fem::FemSolverObject"):
            App.ActiveDocument.openTransaction(
                "Add %s equation to %s"
                % (self.getSpecifier(), s[0].Label))
            Gui.doCommand(
                "App.ActiveDocument.%(obj)s.Proxy.addEquation("
                "App.ActiveDocument.%(obj)s, '%(name)s')"
                % {"obj": s[0].Name, "name": self.getSpecifier()})
            App.ActiveDocument.commitTransaction()
            App.ActiveDocument.recompute()

    def IsActive(self):
        s = Gui.Selection.getSelection()
        if len(s) == 1 and FemUtils.isDerivedFrom(s[0], "Fem::FemSolverObject"):
            return s[0].Proxy.isSupported(self.getSpecifier())
        return False


class Heat(_Base):

    def getSpecifier(self):
        return "Heat"

    def GetResources(self):
        return {
            'Pixmap': 'fem-equation-heat',
            'MenuText': "Heat Equation",
            'ToolTip': "Creates a FEM constraint body heat flux"
        }


class Elasticity(_Base):

    def getSpecifier(self):
        return "Elasticity"

    def GetResources(self):
        return {
            'Pixmap': 'fem-equation-elasticity',
            'MenuText': "Elasticity Equation",
            'ToolTip': "Creates a FEM constraint for elasticity"
        }


class Electrostatic(_Base):

    def getSpecifier(self):
        return "Electrostatic"

    def GetResources(self):
        return {
            'Pixmap': 'fem-equation-electrostatic',
            'MenuText': "Electrostatic Equation",
            'ToolTip': "Creates a FEM equation for electrostatic"
        }


class Flow(_Base):

    def getSpecifier(self):
        return "Flow"

    def GetResources(self):
        return {
            'Pixmap': 'fem-equation-flow',
            'MenuText': "Flow Equation",
            'ToolTip': "Creates a FEM constraint body heat flux"
        }


Gui.addCommand('FEM_AddEquationHeat', Heat())
Gui.addCommand('FEM_AddEquationElasticity', Elasticity())
Gui.addCommand('FEM_AddEquationElectrostatic', Electrostatic())
Gui.addCommand('FEM_AddEquationFlow', Flow())
