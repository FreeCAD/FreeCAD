#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - FreeCAD Developers                               *
#*   Author (c) 2015 - Qingfeng Xia <qingfeng xia eng.ox.ac.uk>                    *
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

"""
A dump class to let Calculix solver fit into the CaeSolver Framework
Some of FemTools() information retrieval function could be moved in, if not all
"""

import FreeCAD
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore


class CaeSolver():
    """The Fem::FemSolver 's Proxy python type,
    add solver specific properties, methods and bring up SolverControlTaskPanel.
    After loaded from FCStd file, this python class is not instantiated.
    use CaeTools.getSolverPythonFromAnalysis() to get/create such an instance.
    """
    def __init__(self, obj):
        self.Type = "CaeAnalysis"
        self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to  this object

    # following are the standard FeaturePython methods
    def execute(self, obj):
        return

    def onChanged(self, obj, prop):
        """updated Part should lead to recompution of the analysis # to-do """
        return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state


class ViewProviderCaeSolver:
    """A View Provider for the Solver object, base class for all derived solver
    derived solver should implement  a specific TaskPanel and set up solver and override setEdit()
    """
    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        return ":/icons/fem-solver.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def doubleClicked(self, vobj):
        # from import _SolverControlTaskPanel
        taskd = _TaskPanelSolverControl(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def setEdit(self, vobj, mode):
        # import module if it is defined in another file
        taskd = _TaskPanelSolverControl(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def unsetEdit(self, vobj, mode):
        #FreeCADGui.Control.closeDialog()
        return

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


class _TaskPanelSolverControl:
    def __init__(self, solver_object):
        self.form = FreeCADGui.PySideUic.loadUi(FreeCAD.getHomePath() + "Mod/Fem/TaskPanelSolverControl.ui")
        #QtGui.QMessageBox.critical(None, "Not implement yet", "Please edit proper in property editor")
        pass

    def accept(self):
        return True

    def reject(self):
        return True
