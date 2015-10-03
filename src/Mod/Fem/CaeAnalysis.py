
import FreeCAD
from FemResultControlTaskPanel import ResultControlTaskPanel
from JobControlTaskPanel import JobControlTaskPanel
from CaeSolver import makeCaeSolver

def makeCaeAnalysis(name):  #<FemToCae> name changed!
    '''makeCaeAnalysis(name): makes a Fem Analysis object'''
    obj = FreeCAD.ActiveDocument.addObject("Fem::FemAnalysisPython", name)
    CaeAnalysis(obj)  #
    if FreeCAD.GuiUp:
        ViewProviderCaeAnalysis()
    #FreeCAD.ActiveDocument.recompute()
    return obj

class  CaeAnalysis:
    """The CaeAnalysis container object, serve CFD ,FEM, etc
    to-do: Gui dialog is needed to select category and solver
    """
    def __init__(self, obj):
        self.Type = "FemAnalysis"  #<FemToCae>this is related with C++ code? if not change this type! 
        obj.Proxy = self #link between App::DocumentObject to  this object
        obj.addProperty("App::PropertyString", "OutputDir", "Base", "Directory where the jobs get generated")
        #default sover name and category, late show a dialog/TaskView to show, set it
        self.category="FEM"
        self.solverName="Calculix"
        obj.addProperty("App::PropertyString", "category", "Base", "desc")
        obj.addProperty("App::PropertyString", "solverName", "Base", "desc of this property")
        #
        #self.solver=makeCaeSolver(self)  # Solver property needs to be added to obj FemAnalysis C++ object 
    """
    def setSolver(self, category, solverName):
        #should check available and show error!
        self.category=category
        self.solverName=solverName
        #
        self.solver=makeCaeSolver(self)
    """    
    def getMesh(self):
        for i in FemGui.getActiveAnalysis().Member:
            if i.isDerivedFrom("Fem::FemMeshObject"):
                return i
        
    def execute(self, obj):
        return

    def onChanged(self, obj, prop):
        if prop in ["MaterialName"]:
            return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state


class ViewProviderCaeAnalysis:
    """A View Provider for the CaeAnalysis container object
    doubleClicked() is 
    """

    def __init__(self):
        #vobj.addProperty("App::PropertyLength", "BubbleSize", "Base", str(translate("Fem", "The size of the axis bubbles")))
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
            if FreeCADGui.activeWorkbench().name() != 'FemWorkbench': #<FemToCae> 
                FreeCADGui.activateWorkbench("FemWorkbench")  #<FemToCae> 
            FemGui.setActiveAnalysis(self.Object)
            return True
        else:
            taskd = JobControlTaskPanel(self.Object) 
            FreeCADGui.Control.showDialog(taskd)
        return True

    def __getstate__(self):
        return None

    def __setstate__(self, state):
        return None
