### Aims of this pull request:

- genralization of CAE solver, sharing mesh and analysis class infra-strcture
- simplification of work for new solvers: A new solver will need only case-runner, case-writer, resultp-reader
- decoupling analysis and solver. 

### Achieved refactoring work

1. FemTools.py -> CaeSolver (general class to start external solver process) + ccxFemSolver (specific solver)
Significant refactoring(mainly renaming) work happends here, code logic is remain identical. 

2. MechanicalAnalysis -> CaeAnalysis (general class) + JobControlTaskView (general class) + FemResultTaskView + FemCommands.py
Just split a big class into several class, thereby, half code is reusable. 

### Read this post first on forum, design object and roadmap

[From FreeCAD's Fem workbench towards a full-fledged CAE workbench](http://forum.freecadweb.org/viewtopic.php?f=18&t=12654)

Mod/Cae files are now moved to Mod/Fem

https://github.com/qingfengxia/FreeCAD/tree/master/src/Mod/Fem


### CaeWorkbench to-do

1. catogory of solver dict, where is the best place to register?
when a new analysis is created, catogory and solver are selected in GUI dialog, then instantiated.

2. renaming Fem->Cae may be a huge job, just forget it

3. Currently, material, mesh, are members of CaeSolver
update_objects() extract tehm from AnalysisObject.
CaeAnalysis should be the object contains {solver, mesh, material, constraint}, so   ccxInpWriter.write_case_file() API can be simplified.

4. Fem should work in nonGui mode, i.e. key classes like, CaeSolver, CaeAnalysis, should work in both cmd and gui mode


**************************************************

### installation without compiling C++
Test python code, before these codes are merged into FemWorkbench

since the C++ is untouched, it is possible to just replace the python code with CAE version. 

it is conflicted with the current Fem module's python code
```
sudo mv /usr/lib/freecad/Mod/Fem /opt/Fem_backup
sudo ln -s /opt/FreeCAD/src/Mod/Cae /usr/lib/freecad/Mod/Fem
```

Error "No module named: MechanicalAnalysis" when do comand
`Gui.activateWorkbench("FemWorkbench")`

Gui/AppFemGui.cpp:    Base::Interpreter().loadModule("MechanicalAnalysis");


However, python code does not reference it, not sure about C++ code?
`find ./ -type f -exec grep -H 'MechanicalAnalysis' {} \;`

Temporal solution, activate this workbench for the second time : Gui.activateWorkbench("FemWorkbench")

### test with C++ compilation (merge with the existent Fem module)

replacesing all files under src/ModFem/*.* with new files under Mod/Cae, but leaves folders untouched "App, Gui, testfiles"

### test it by paste to python console of Gui

[step by step tutorial of Fem module](http://www.freecadweb.org/wiki/index.php?title=FEM_tutorial)

Once edit py file, do remember to "rm *.pyc" and restart Freecad from terminal!

This module can not load saved file and reload analysis, but macro is fine to quickly test GUI functions. 

Material selection is lost after replay the following commands

```
FreeCAD.newDocument("FemTest")
FreeCAD.setActiveDocument("FemTest")
active_doc = FreeCAD.ActiveDocument
box = active_doc.addObject("Part::Box", "Box")
active_doc.recompute()
#
Gui.activateWorkbench("FemWorkbench")
Gui.activateWorkbench("FemWorkbench")
import FemGui
import CaeAnalysis
App.activeDocument().addObject('Fem::FemMeshShapeNetgenObject', 'Box_Mesh')
App.activeDocument().ActiveObject.Shape = App.activeDocument().Box
Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name)
Gui.activeDocument().resetEdit()
#
FemGui.setActiveAnalysis(CaeAnalysis.makeCaeAnalysis('MechanicalAnalysis'))
App.activeDocument().ActiveObject.Member = App.activeDocument().ActiveObject.Member + [App.activeDocument().Box_Mesh]
import MechanicalMaterial
MechanicalMaterial.makeMechanicalMaterial('MechanicalMaterial')
App.activeDocument().MechanicalAnalysis.Member = App.activeDocument().MechanicalAnalysis.Member + [App.ActiveDocument.ActiveObject]
Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name,0)
#set material index
App.ActiveDocument.recompute()
Gui.activeDocument().resetEdit()
#
Gui.getDocument("FemTest").getObject("Box_Mesh").Visibility=False
Gui.getDocument("FemTest").getObject("Box").Visibility=True
#
App.activeDocument().addObject("Fem::ConstraintFixed","FemConstraintFixed")
App.activeDocument().MechanicalAnalysis.Member = App.activeDocument().MechanicalAnalysis.Member + [App.activeDocument().FemConstraintFixed]
App.ActiveDocument.recompute()
Gui.activeDocument().setEdit('FemConstraintFixed')
App.ActiveDocument.FemConstraintFixed.References = [(App.ActiveDocument.Box,"Face1")]
App.ActiveDocument.recompute()
Gui.activeDocument().resetEdit()
#
App.activeDocument().addObject("Fem::ConstraintForce","FemConstraintForce")
App.activeDocument().FemConstraintForce.Force = 0.0
App.activeDocument().MechanicalAnalysis.Member = App.activeDocument().MechanicalAnalysis.Member + [App.activeDocument().FemConstraintForce]
App.ActiveDocument.recompute()
Gui.activeDocument().setEdit('FemConstraintForce')
App.ActiveDocument.FemConstraintForce.Force = 500.0
App.ActiveDocument.FemConstraintForce.Direction = None
App.ActiveDocument.FemConstraintForce.Reversed = False
App.ActiveDocument.FemConstraintForce.References = [(App.ActiveDocument.Box,"Face5")]
App.ActiveDocument.recompute()
Gui.activeDocument().resetEdit()
#
```


### bug found

1. in MechanicalAnalysis.py
default material is none? a default material "Steel" can be set during makeMechanicalMaterial() ?
def choose_material(self, index): does not write/record to python console, 
FreeCAD.ActiveDocument.openTransaction("Create Material") # but where Transaction is closed???

Traceback (most recent call last):
  File "/usr/lib/freecad/Mod/Fem/MechanicalMaterial.py", line 111, in setEdit
    FreeCADGui.Control.showDialog(taskd)
<type 'exceptions.RuntimeError'>: Active task dialog found

>>> Gui.activeDocument().setEdit(App.ActiveDocument.ActiveObject.Name,0)
Previously used material cannot be found in material directories. Using transient material.


2. The code is tested until write_abaqus_file(), which does not return! 

3. Ubuntu 14.04.2 bug

qingfeng@qingfeng-ubuntu:/opt/FreeCAD/src/Mod/Fem$ freecad
FreeCAD 0.16, Libs: 0.16R5639 (Git)
© Juergen Riegel, Werner Mayer, Yorik van Havre 2001-2015
  #####                 ####  ###   ####  
  #                    #      # #   #   # 
  #     ##  #### ####  #     #   #  #   # 
  ####  # # #  # #  #  #     #####  #   # 
  #     #   #### ####  #    #     # #   # 
  #     #   #    #     #    #     # #   #  ##  ##  ##
  #     #   #### ####   ### #     # ####   ##  ##  ##

Traceback (most recent call last):
  File "/usr/lib/freecad/Mod/Fem/MechanicalMaterial.py", line 111, in setEdit
    FreeCADGui.Control.showDialog(taskd)
<type 'exceptions.RuntimeError'>: Active task dialog found
sh: 1: SMDS_MemoryLimit: not found


************************************************
## CFD To-do

### CaeAnalysis class should operate on diff category of solvers

Analysis Solver : update each other

FreeCADGui.addCommand() # gen menu and toolbar?
addPreferencePage()

openFoam stdout, plot ?

http://www.solidworks.co.uk/sw/products/simulation/flow-simulation.htm

### adding time information to CaeSolver 

convergence  criterion
currentTime=0


### TaskPanel for CFD solver


## Export case setup/result and start external Visualisaton tool


### CFD mesh conversion with NodeSet ?

<http://www.openfoam.org/features/mesh-conversion.php>

- gmshToFoam	Reads .msh file as written by Gmsh
- ideasUnvToFoam	I-Deas unv format mesh conversion- 
- tetgenToFoam	Converts .ele and .node and .face files, written by tetgen
-netgenNeutralToFoam	Converts neutral file format as written by Netgen v4.4

Convert the mesh file to openFoam: 
$ ideasUnvToFoam <root> <caseName> (Path) <meshFile>
meshCheck

mpirun
convert

### CFD result conversion back to FreeCAD

 foamDataToFluent Translates OpenFOAM data to Fluent format.
 foamToDX Translates OpenFOAM data to OpenDX format.
 foamToEnsight Translates OpenFOAM data to EnSight format.
 foamToEnsightParts Translates OpenFOAM data to Ensight format. An Ensight part is created for each cellZone and patch
 foamToFieldview Write out the OpenFOAM mesh in Fieldview-UNS format (binary). See Fieldview Release 8 Reference Manual - Appendix D (Unstructured Data Format) Borrows various from uns/write_binary_uns.c from FieldView dist.
 foamToFieldview9 Write out the OpenFOAM mesh in Version 3.0 Fieldview-UNS format (binary). See Fieldview Release 9 Reference Manual - Appendix D (Unstructured Data Format) Borrows various from uns/write_binary_uns.c from FieldView dist.
 foamToGMV Translates OpenFOAM data to GMV readable files.
 foamToTecplot360 Tecplot binary file format writer
 foamToVTK Legacy VTK file format writer. - handles volScalar, volVector, pointScalar, pointVector fields. - mesh topo changes. - both ascii and binary. - single time step writing. - write subset only. - automatic decomposition of cells; polygons on boundary undecomposed since handled by vtk


http://www.cfd-online.com/Forums/openfoam-meshing-open/151980-mesh-conversion-openfoam-polymesh-format-calculix.html#post543517
You need to decompose the polyhedral cells into shapes your software can understand. Paraview converter/module has a similar problem, so you can find code snippets there.


****************************************

## FSI 

### AnalysisCoupler.py
list of CaeAnalysis instances,  add_analysis()  add_time()
timeStep, currentTime,  adaptiveTimeStep=False
historicalTimes chain up all historical case data file. 

static multiple solvers are also possible

### feature request from FreeCADGui
NamedSelection: coupling of faces/interfaces
Part section: build3DFromCavity buildEnclosureBox

### Salome has yacs module can do this job


### Mesh (displacement) fixed there and save to new mesh? 

