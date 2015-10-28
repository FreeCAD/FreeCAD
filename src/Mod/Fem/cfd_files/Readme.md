==================
FoamCFDSolver
===================

### Software prerequisits for Testing

test on ubuntu 14.04

OpenFoam 2.x can be installed from repo, which will install the correct Paraview

Basically, I need several progam on PATH, like icoFoam, paraview, paraFoam
Can be easily done by source a bash script in your .bashrc

see [OpenFoam quick startup guide]()



## CFD Roadmap

### Phase I demonstration 

- CaeAnalysis: class should operate on diff category of solvers
- CaseSolver: extending Fem::FemSolverObject
- FoamCaseWriter: OpenFOAM case writer, the key part is meshing and case setup
- Fixed material as air or water
- Use fem::constraint Constraint mapping: 
    Force->Velocity (Displacement is missing), 
    Pressure->Pressure, Symmetry is missing (Pulley), 
    PressureOutlet (Gear), VelocityOutlet(Bearing),

- Use exteranal result viewer paraview
    CFD is cell base solution, while FEM is node base. It is not easy to reuse ResultObject 
    volPointInterpolation - interpolate volume field to point field;

### Phase II general usability

- FluidMaterail: discussion with community for standardizing Material model, 
   also design for other CAE analysis EletroMagnetics
- CFD boundary conditions

- More AnalysisType supported, 
   
- CFD mesh with viscous layer and hex meshcell supported
- Unsteady case support
- SolverControlTaskPanel: better solver parameter control


### Phase III  FSI 

- The best way to do that will be in Salome, 

- AnalysisCoupler.py
list of CaeAnalysis instances,  add_analysis()  add_time()
timeStep, currentTime,  adaptiveTimeStep=False
historicalTimes chain up all historical case data file. 

static multiple solvers are also possible

- feature request from FreeCADGui
NamedSelection: coupling of faces/interfaces
Part section: build3DFromCavity buildEnclosureBox


### Mesh (displacement) fixed there and save to new mesh? 

*************************************

FreeCADGui.addCommand() # gen menu and toolbar?
addPreferencePage()

openFoam stdout, plot ?

http://www.solidworks.co.uk/sw/products/simulation/flow-simulation.htm

myMesh->UNVToMesh(File.filePath().c_str());
myMesh->ExportUNV(File.filePath().c_str());

### adding time information to CaeSolver for unsteady test

convergence  criterion
currentTime=0


### TaskPanel for CFD solver


### Boundary condition and faceset mesh write 

http://docs.salome-platform.org/latest/extra/SALOME_4.1.2_SMDS_reference_guide.pdf

Python side,

Elmer, input format.
UNV import. SMESH




[/src/Mod/Fem/App/FemSetObject.h]
App::PropertyLink FemMesh;


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

## Export case setup/result and start external Visualisaton tool

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



