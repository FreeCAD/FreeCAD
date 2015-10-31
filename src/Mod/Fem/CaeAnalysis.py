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

import FreeCAD
from FemCommands import FemCommands

if FreeCAD.GuiUp:
    import FreeCADGui
    import FemGui
    from PySide import QtCore


def makeMechanicalAnalysis(name):
    '''makes a Fem MechAnalysis object'''
    return _CreateCaeAnalysis('Calculix', name)


def makeCaeAnalysis(name):
    '''makeCaeAnalysis(name): makes a Fem Analysis object,
    this should be marked for internal usage'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemAnalysisPython", name)
    CaeAnalysis(obj)
    ViewProviderCaeAnalysis(obj.ViewObject)
    #FreeCAD.ActiveDocument.recompute()
    return obj


class CaeAnalysis:
    """The CaeAnalysis container object, serve CFD ,FEM, etc
    This class should not have instance methods,
    since document reload, this class's instance does not exist!
    """
    def __init__(self, obj):
        self.Type = "CaeAnalysis"
        self.Object = obj  #  keep a ref to the DocObj for nonGui usage
        obj.Proxy = self  #  link between App::DocumentObject to  this object
        obj.addProperty("App::PropertyString", "Category", "Analysis", "Cfd, Computional solid mechanics")
        obj.addProperty("App::PropertyString", "SolverName", "Analysis", "External solver unique name")
        
        # added from Oct 30, 2015, this should be added into FemSolverPython object: ccxFemSolver, FemTools also needs change. 
        from FemTools import FemTools
        fem_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Fem")
        obj.addProperty("App::PropertyEnumeration", "AnalysisType", "Fem", "Type of the analysis")
        obj.AnalysisType = FemTools.known_analysis_types
        analysis_type = fem_prefs.GetInt("AnalysisType", 0)
        obj.AnalysisType = FemTools.known_analysis_types[analysis_type]
        obj.addProperty("App::PropertyPath", "WorkingDir", "Fem", "Working directory for calculations")
        obj.WorkingDir = fem_prefs.GetString("WorkingDir", "")

        obj.addProperty("App::PropertyIntegerConstraint", "NumberOfEigenmodes", "Fem", "Number of modes for frequency calculations")
        noe = fem_prefs.GetInt("NumberOfEigenmodes", 10)
        obj.NumberOfEigenmodes = (noe, 1, 100, 1)

        obj.addProperty("App::PropertyFloatConstraint", "EigenmodeLowLimit", "Fem", "Low frequency limit for eigenmode calculations")
        #Not yet in prefs, so it will always default to 0.0
        ell = fem_prefs.GetFloat("EigenmodeLowLimit", 0.0)
        obj.EigenmodeLowLimit = (ell, 0.0, 1000000.0, 10000.0)

        obj.addProperty("App::PropertyFloatConstraint", "EigenmodeHighLimit", "Fem", "High frequency limit for eigenmode calculations")
        ehl = fem_prefs.GetFloat("EigenmodeHighLimit", 1000000.0)
        obj.EigenmodeHighLimit = (ehl, 0.0, 1000000.0, 10000.0)

    # following are the FeutureT standard methods
    def execute(self, obj):
        """updated Part should lead to recompute of mesh, if result_present"""
        return

    def onChanged(self, obj, prop):
        """updated Part should lead to recompute of mesh"""
        if prop in ["MaterialName"]:
            return  # todo!

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state


class ViewProviderCaeAnalysis:
    """A View Provider for the CaeAnalysis container object
    doubleClicked() should activate AnalysisControlTaskView
    """
    #__class__.icon = ":/icons/fem-analysis.svg"
    def __init__(self, vobj):
        #self.icon=":/icons/fem-analysis.svg"
        vobj.Proxy = self

    #def getIcon(self):  # after read file, icon can not been found, this will fail to load this TaskPanel
    #    return self.icon

    def setIcon(self,icon):
        self.icon = icon

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object
        self.bubbles = None

    def updateData(self, obj, prop):
        return

    def onChanged(self, vobj, prop):
        return

    def doubleClicked(self, vobj):
        if not FemGui.getActiveAnalysis() == self.Object:
            if FreeCADGui.activeWorkbench().name() != 'FemWorkbench':
                FreeCADGui.activateWorkbench("FemWorkbench")
            FemGui.setActiveAnalysis(self.Object)
            return True
        else:
            import _AnalysisControlTaskPanel
            taskd = _AnalysisControlTaskPanel._AnalysisControlTaskPanel(self.Object)
            FreeCADGui.Control.showDialog(taskd)
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None


def _CreateCaeAnalysis(solverName, analysisName=None):
        """ todo: this function should be adapted to work without GUI later """
        FreeCAD.ActiveDocument.openTransaction("Create Cae Analysis")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.addModule("CaeAnalysis")
        _analysisName = analysisName if analysisName else solverName + "Analysis"
        FreeCADGui.doCommand("CaeAnalysis.makeCaeAnalysis('{}')".format(_analysisName))
        obj = FreeCAD.activeDocument().ActiveObject
        FreeCADGui.doCommand("FemGui.setActiveAnalysis(App.activeDocument().ActiveObject)")
        # create an solver and append into analysisObject
        FreeCADGui.addModule("CaeSolver")
        FreeCADGui.doCommand("CaeSolver.makeCaeSolver('{}')".format(solverName))
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Fem::FemMeshObject")):
                FreeCADGui.doCommand("FemGui.getActiveAnalysis().ActiveObject.Member = FemGui.getActiveAnalysis().Member + [App.activeDocument()." + sel[0].Name + "]")
            if(sel[0].isDerivedFrom("Part::Feature")):
                FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject', '" + sel[0].Name + "_Mesh')")
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Shape = App.activeDocument()." + sel[0].Name)
                FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
                #FreeCADGui.doCommand("Gui.activeDocument().hide('" + sel[0].Name + "')")
                #FreeCADGui.doCommand("App.activeDocument().ActiveObject.touch()")
                #FreeCADGui.doCommand("App.activeDocument().recompute()")
                FreeCADGui.doCommand("Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)")

        #FreeCAD.ActiveDocument.commitTransaction()
        FreeCADGui.Selection.clearSelection()
        return obj


class _CommandNewCfdAnalysis(FemCommands):
    "the Cfd Analysis command definition"
    def __init__(self):
        super(_CommandNewCfdAnalysis, self).__init__()
        self.resources = {'Pixmap': 'fem-cfd-analysis',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Cfd_Analysis", "New computional fluid dynamics analysis"),
                          'Accel': "N, A",  # conflict with mechanical analysis?
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Cfd_Analysis", "Create a new computional fluid dynamics analysis")}
        self.is_active = 'with_document'

    def Activated(self):
        _CreateCaeAnalysis('OpenFOAM')


class _CommandNewMechanicalAnalysis(FemCommands):
    "the Mechancial FEM Analysis command definition"
    def __init__(self):
        super(_CommandNewMechanicalAnalysis, self).__init__()
        self.resources = {'Pixmap': 'fem-mech-analysis',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Analysis", "Mechanical FEM Analysis"),
                          'Accel': "N, A",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Analysis", "Create a new Mechanical FEM Analysis")}
        self.is_active = 'with_document'

    def Activated(self):
        #default solverName  show a dialog to select solver if more than one solver
        import MechanicalAnalysis
        MechanicalAnalysis.makeMechanicalAnalysis('MechanicalAnalysis')


class _CommandAnalysisControl(FemCommands):
    "the Fem Analysis Job Control command definition"
    def __init__(self):
        super(_CommandAnalysisControl, self).__init__()
        self.resources = {'Pixmap': 'fem-new-analysis',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_AnalysisControl", "Start calculation"),
                          'Accel': "S, C",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_AnalysisControl", "Dialog to start the calculation of the anlysis")}
        self.is_active = 'with_analysis'

    def Activated(self):
        import _AnalysisControlTaskPanel
        import FemGui
        taskd = _AnalysisControlTaskPanel._AnalysisControlTaskPanel(FemGui.getActiveAnalysis())
        taskd.update()
        FreeCADGui.Control.showDialog(taskd)

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_NewCfdAnalysis', _CommandNewCfdAnalysis())
    FreeCADGui.addCommand('Fem_NewMechanicalAnalysis', _CommandNewMechanicalAnalysis())
    FreeCADGui.addCommand('Fem_AnalysisControl', _CommandAnalysisControl())
