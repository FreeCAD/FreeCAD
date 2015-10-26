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

__title__ = "Command New CFD Analysis"
__author__ = "Qingfeng Xia"
__url__ = "http://www.freecadweb.org"

import FreeCAD
from FemCommands import FemCommands

registered_solvers={
"Calculix":{ "name":"Calculix", "catogory":"Fem", "module": "ccxFemSolver","version": (2,3,4), "reader":"","writer":""},
"OpenFoam":{ "name":"OpenFoam", "catogory":"Cfd", "module": "","version": (2,1,0), "reader":"","writer":""}
}

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore

def makeCaeAnalysis(name):  #<FemToCae> name changed!
    '''makeCaeAnalysis(name): makes a Fem Analysis object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemAnalysisPython", name)
    _CaeAnalysis(obj)
    _ViewProviderCaeAnalysis()
    #FreeCAD.ActiveDocument.recompute()
    return obj

class  _CaeAnalysis:
    """The CaeAnalysis container object, serve CFD ,FEM, etc
    to-do: Gui dialog is needed to select category and solver
    """
    def __init__(self, obj):
        self.Type = "CaeAnalysis"  
        obj.Proxy = self #link between App::DocumentObject to  this object
        obj.addProperty("App::PropertyString", "Category", "Analysis", "Cfd, Computional solid mechanics") #should be Enum
        obj.addProperty("App::PropertyString", "SolverName", "Analysis", "External solver unique name")
        #default solver name and category, late show a dialog/TaskView to show, set it
    """
    def setSolver(self, category, solverName):
        #should check available and show error!
        self.category=category
        self.solverName=solverName
        #
        self.solver=makeCaeSolver(self)
    """    
    def getMesh(self):
        if FreeCAD.GuiUp:
            for i in FemGui.getActiveAnalysis().Member:
                if i.isDerivedFrom("Fem::FemMeshObject"):
                    return i
        #python will return None by default
                
    def getSolver(self):
        if FreeCAD.GuiUp:
            for i in FemGui.getActiveAnalysis().Member:
                if i.isDerivedFrom("Fem::FemSolverObject"):
                    return i
            
    def getConstraintGroup(self):
        group=[]
        if FreeCAD.GuiUp:
            for i in FemGui.getActiveAnalysis().Member:
                if i.isDerivedFrom("Fem::Constraint"):
                    group.append(i)
        return group
        
    def execute(self, obj):
        return

    def onChanged(self, obj, prop):
        """updated Part should lead to recompute of mesh"""
        if prop in ["MaterialName"]:
            return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state


class _ViewProviderCaeAnalysis:
    """A View Provider for the CaeAnalysis container object
    doubleClicked() should activate JobControlTaskView
    """
    def __init__(self):
        self.icon=":/icons/fem-analysis.svg"

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
            taskd = _JobControlTaskPanel(self.Object)  #to-do
            FreeCADGui.Control.showDialog(taskd)
            pass
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None

def _create_cae_analysis(solverName):
        FreeCAD.ActiveDocument.openTransaction("Create Cae Analysis")
        FreeCADGui.addModule("FemGui")
        FreeCADGui.addModule("CaeAnalysis")

        FreeCADGui.doCommand("CaeAnalysis.makeCaeAnalysis('CaeAnalysis')")
        FreeCADGui.doCommand("FemGui.setActiveAnalysis(App.activeDocument().ActiveObject)")
        #create an solver and append into analysisObject
        FreeCADGui.doCommand("App.activeDocument().addObject('Fem::FemSolverObject', 'Solver')")
        FreeCADGui.doCommand("FemGui.getActiveAnalysis().Member = FemGui.getActiveAnalysis().Member + [App.activeDocument().ActiveObject]")
        sel = FreeCADGui.Selection.getSelection()
        if (len(sel) == 1):
            if(sel[0].isDerivedFrom("Fem::FemMeshObject")):
                FreeCADGui.doCommand("App.activeDocument().ActiveObject.Member = App.activeDocument().ActiveObject.Member + [App.activeDocument()." + sel[0].Name + "]")
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
        
class _CommandNewCaeAnalysis(FemCommands):
    "the Cfd Analysis command definition"
    def __init__(self):
        super(_CommandNewMechanicalAnalysis, self).__init__()
        self.resources = {'Pixmap': 'fem-analysis',
                          'MenuText': QtCore.QT_TRANSLATE_NOOP("Fem_Analysis", "New computional fluid dynamics analysis"),
                          'Accel': "N, A",
                          'ToolTip': QtCore.QT_TRANSLATE_NOOP("Fem_Analysis", "Create a new computional fluid dynamics analysis")}
        self.is_active = 'with_document'

    def Activated(self):
        _create_cae_analysis('OpenFoam')

if FreeCAD.GuiUp:
    FreeCADGui.addCommand('Fem_NewCaeAnalysis', _CommandNewCaeAnalysis())
