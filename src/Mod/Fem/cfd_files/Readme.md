
1) I am not aware the Fem/App/CMakeList.txt also keep a list of py files.

which I think Fem/CMakeList.txt should do

SET(FemScripts_SRCS


2) multiple analysis , is possible with careful operation

select the new geometry and add new mesh and then new analysis. 
double click to activate one Analysis and then click toolbar show result 

3) UnitTest failed at compring inp file, but this file is good to solve 

Comparing /home/qingfeng/FreeCAD/build/Mod/Fem/test_files/cube_static.inp to /tmp/FEM_static/Mesh.inp


4) test without make install may failed, I tested after make install, 
GUI paste example is working. 

5) best way to load module by name, work for python 2 and python 3, also for nonGUI mode? 
    #Solver factory pattern: eval is not compatible with python 3? how about imp? 
    #eval("from {} import ViewProviderCaeSolver, CaeSolver".format(solverInfo["Module"]))
    if solverInfo["SolverName"] == "Calculix":
        from ccxFemSolver import ViewProviderCaeSolver, CaeSolver
    else:
        from FoamCfdSolver import ViewProviderCaeSolver, CaeSolver

6) ViewProvider failed after saved document is loaded, can not find icon since Proxy instance is not there
[code] 
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
[/code]

7) best way to get cross-platform tmp folder?

//#include <QDesktopServices> // which is in QtGui, Qt5 has <QStandardPaths> in QtCore
//const char* workingDir = QDesktopServices::storageLocation(QDesktopServices::TempLocation).toAscii(); 
//For QString.toUtf8();  I got wrong encoding in GUI, leading to failure, not sure about toAscii(); 
