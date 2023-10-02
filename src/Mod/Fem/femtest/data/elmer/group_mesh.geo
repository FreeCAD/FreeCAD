// geo file for meshing with Gmsh meshing software created by FreeCAD

// enable multi-core processing
General.NumThreads = X;

// open brep geometry
Merge "tmp0TVZbM.brep";

// group data
Physical Surface("Face1") = {1};
Physical Surface("Face2") = {2};
Physical Surface("Face6") = {6};
Physical Volume("Solid1") = {1};

// Characteristic Length
// no boundary layer settings for this mesh
// min, max Characteristic Length
Mesh.CharacteristicLengthMax = 1e+22;
Mesh.CharacteristicLengthMin = 8.0;
Mesh.MeshSizeFromCurvature = 12; // number of elements per 2*pi radians, 0 to deactivate

// optimize the mesh
Mesh.Optimize = 1;
Mesh.OptimizeNetgen = 0;
// High-order meshes optimization (0=none, 1=optimization, 2=elastic+optimization, 3=elastic, 4=fast curving)
Mesh.HighOrderOptimize = 0;

// mesh order
Mesh.ElementOrder = 2;
Mesh.SecondOrderLinear = 0; // Second order nodes are created by linear interpolation instead by curvilinear

// mesh algorithm, only a few algorithms are usable with 3D boundary layer generation
// 2D mesh algorithm (1=MeshAdapt, 2=Automatic, 5=Delaunay, 6=Frontal, 7=BAMG, 8=DelQuad, 9=Packing Parallelograms)
Mesh.Algorithm = 2;
// 3D mesh algorithm (1=Delaunay, 2=New Delaunay, 4=Frontal, 7=MMG3D, 9=R-tree, 10=HTX)
Mesh.Algorithm3D = 1;

// meshing
Geometry.Tolerance = 1e-06; // set geometrical tolerance (also used for merging nodes)
Mesh  3;
Coherence Mesh; // Remove duplicate vertices

// save
// Ignore Physical definitions and save all elements;
Mesh.SaveAll = 1;
Save "tmpjVhNNb.unv";


// **********************************************************************
// Gmsh documentation:
// https://gmsh.info/doc/texinfo/gmsh.html#Mesh
//
// We do not check if something went wrong, like negative jacobians etc. You can run Gmsh manually yourself:
//
// to see full Gmsh log, run in bash:
// /usr/bin/gmsh - /tmp/tmputZ_uU.geo
//
// to run Gmsh and keep file in Gmsh GUI (with log), run in bash:
// /usr/bin/gmsh /tmp/tmputZ_uU.geo
