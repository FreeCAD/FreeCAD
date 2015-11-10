#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015 - Qingfeng Xia <qingfeng.xia()eng.ox.ac.uk> *
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

__title__ = "Command and Classes for New CAE Analysis"
__author__ = "Qingfeng Xia"
__url__ = "http://www.freecadweb.org"

import os.path

import FreeCAD
if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui


TurbulenceModelList = ["Laminar", "KE", "KW", "LES"]
AnalysisTypeList = []


class CaeSolver():
    """Fem::FemSolverObject 's Proxy python type
    add solver specific properties, methods and bring up SolverControlTaskPanel.
    After loaded from FCStd file, this python class is not instantiated.
    use CaeTools.getSolverPythonFromAnalysis() to get/create such an instance.
    """
    def __init__(self, obj):
        self.Type = "CfdSolver"
        self.Object = obj  # keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  # link between App::DocumentObject to  this object

        # general CFD properties, if there are not existant for newly create FemSolverObj, adProperty
        if not "Compressible" in obj.PropertiesList:
            # API: addProperty(self,type,name='',group='',doc='',attr=0,readonly=False,hidden=False)
            obj.addProperty("App::PropertyEnumeration", "TurbulenceModel", "CFD",
                            "Laminar,KE,KW,LES,")
            obj.TurbulenceModel = TurbulenceModelList
            obj.addProperty("App::PropertyBool", "Compressible", "CFD",
                            "Compressible air or Incompressible like liquid")
            obj.addProperty("App::PropertyBool", "ThermalAnalysisEnabled", "CFD",
                             "calc heat transfering")
            obj.addProperty("App::PropertyBool", "Transient", "CFD",
                            "static or transient analysis")
            # CurrentTime TimeStep StartTime, StopTime

            # adding solver specific properties
        # FemSolverObject standard properties should be set in _SetSolverInfo() of CaeSolver.py

    ########## CaeSolver API #####################
    def check_prerequisites(self, analysis_object):
        return ""

    def write_case(self, analysis_object=None):
        if analysis_object is None and FreeCAD.GuiUp:
            import FemGui
            analysis_object = FemGui.getActiveAnalysis()
        import FoamCaseWriter
        writer = FoamCaseWriter.FoamCaseWriter(analysis_object)
        writer.write_mesh()


    def generate_cmdline(self):
        return "icoFoam -help"  # try to use abs path for case name file/folder

    def edit_case_externally(self):
        case_path = self.Object.WorkingDir + os.path.sep + self.Object.InputCaseName
        if FreeCAD.GuiUp:
            QtGui.QDesktopServices.openUrl(QtCore.QUrl(case_path))

    def view_result_externally(self):
        return "paraFoam {}".format(self.Object.InputCaseName)

    ############ standard FeutureT methods ##########
    def execute(self, obj):
        """"this method is executed on object creation and whenever the document is recomputed"
        update Part or Mesh should NOT lead to recompution of the analysis automatically, time consuming"""
        return

    def onChanged(self, obj, prop):
        return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state


# this class could be moved into CaeSolver, as it can be shared by any solver
class ViewProviderCaeSolver:
    """A View Provider for the Solver object, base class for all derived solver
    derived solver should implement  a specific TaskPanel and set up solver and override setEdit()"""

    def __init__(self, vobj):
        vobj.Proxy = self

    def getIcon(self):
        """after load from FCStd file, self.icon does not exist, return constant path instead"""
        return ":/icons/fem-solver.svg"

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def doubleClicked(self, vobj):
        """after load from FCStd file, self.Object does not exist, use vobj.Object instead"""
        # from solver_specific module import _SolverControlTaskPanel
        taskd = _TaskPanelSolverControl(vobj.Object)
        FreeCADGui.Control.showDialog(taskd)
        return True

    def setEdit(self, vobj, mode):
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
        #QtGui.QMessageBox.critical(None, "This task panel is not implement yet", "Please edit in property editor ")
        pass

    def accept(self):
        return True

    def reject(self):
        return True
