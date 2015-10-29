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

def makeCaeAnalysis(name):
    '''makeCaeAnalysis(name): makes a Fem Analysis object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemAnalysisPython", name)
    CaeAnalysis(obj)
    ViewProviderCaeAnalysis(obj.ViewObject)
    #FreeCAD.ActiveDocument.recompute()
    return obj

class CaeAnalysis:
    """The CaeAnalysis container object, serve CFD ,FEM, etc
    to-do: Gui dialog is needed to select category and solver
    """
    def __init__(self, obj):
        self.Type = "CaeAnalysis"
        self.Object=obj #keep a ref to the DocObj for nonGui usage
        obj.Proxy = self #link between App::DocumentObject to  this object
        obj.addProperty("App::PropertyString", "Category", "Analysis", "Cfd, Computional solid mechanics") #should be Enum
        obj.addProperty("App::PropertyString", "SolverName", "Analysis", "External solver unique name")
    """
    def setSolver(self, solverName):
        #should check available and show error!
        self.solver=makeCaeSolver(solverName)
    """    
    def getMesh(self):
        for i in self.Object.Member:
            if i.isDerivedFrom("Fem::FemMeshObject"):
                return i
        #python will return None by default
                
    def getSolver(self):
        for i in self.Object.Member:
            if i.isDerivedFrom("Fem::FemSolverObject"):
                return i
            
    def getConstraintGroup(self):
        group=[]
        for i in self.Object.Member:
            if i.isDerivedFrom("Fem::Constraint"):
                group.append(i)
        return group
        
    #following are the FeutureT standard methods
    def execute(self, obj):
        """updated Part should lead to recompute of mesh, if result_present"""
        return

    def onChanged(self, obj, prop):
        """updated Part should lead to recompute of mesh"""
        if prop in ["MaterialName"]:
            return #todo!

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state


class ViewProviderCaeAnalysis:
    """A View Provider for the CaeAnalysis container object
    doubleClicked() should activate AnalysisControlTaskView
    """
    def __init__(self, vobj):
        self.icon=":/icons/fem-analysis.svg"
        vobj.Proxy = self

    def getIcon(self):
        return self.icon
        
    def setIcon(self,icon):
        self.icon=icon

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
        FreeCAD.ActiveDocument.openTransaction("Create Cae Analysis")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.addModule("CaeAnalysis")
        _analysisName = analysisName if analysisName else solverName + "Analysis"
        FreeCADGui.doCommand("CaeAnalysis.makeCaeAnalysis('{}')".format(_analysisName))
        FreeCADGui.doCommand("FemGui.setActiveAnalysis(App.activeDocument().ActiveObject)")
        #create an solver and append into analysisObject
        FreeCADGui.addModule("CaeSolver")
        FreeCADGui.doCommand("CaeSolver.makeCaeSolver('{}')".format(solverName))
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Fem::FemMeshObject")):
                FreeCADGui.doCommand("FreeCAD.activeDocument().ActiveObject.Member = App.activeDocument().ActiveObject.Member + [App.activeDocument()." + sel[0].Name + "]")
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
        
class _CommandNewCfdAnalysis(FemCommands):
    "the Cfd Analysis command definition"
    def __init__(self):
        super(_CommandNewCfdAnalysis, self).__init__()
        self.resources = {'Pixmap': 'fem-cfd-analysis',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Cfd_Analysis", "New computional fluid dynamics analysis"),
                          'Accel': "N, A", #conflict with mechanical analysis?
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
        _CreateCaeAnalysis('Calculix', 'MechanicalAnalysis')
        
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