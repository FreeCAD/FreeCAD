// geo file for meshing with GMSH meshing software created by FreeCAD

// open brep geometry
Merge "/tmp/tmp0TVZbM.brep";

// group data
Physical Surface("Face1") = {1};
Physical Surface("Face2") = {2};
Physical Surface("Face6") = {6};
Physical Volume("Solid1") = {1};

// Characteristic Length
// no boundary layer settings for this mesh
// min, max Characteristic Length
Mesh.CharacteristicLengthMax = 1e+22;
Mesh.CharacteristicLengthMin = 9.0;

// optimize the mesh
Mesh.Optimize = 1;
Mesh.OptimizeNetgen = 0;
Mesh.HighOrderOptimize = 0;  // for more HighOrderOptimize parameter check http://gmsh.info/doc/texinfo/gmsh.html

// mesh order
Mesh.ElementOrder = 2;

// mesh algorithm, only a few algorithms are usable with 3D boundary layer generation
// 2D mesh algorithm (1=MeshAdapt, 2=Automatic, 5=Delaunay, 6=Frontal, 7=BAMG, 8=DelQuad)
Mesh.Algorithm = 2;
// 3D mesh algorithm (1=Delaunay, 2=New Delaunay, 4=Frontal, 5=Frontal Delaunay, 6=Frontal Hex, 7=MMG3D, 9=R-tree)
Mesh.Algorithm3D = 1;

// meshing
Geometry.Tolerance = 1e-06; // set geometrical tolerance (also used for merging nodes)
Mesh  3;
Coherence Mesh; // Remove duplicate vertices

// save
Mesh.Format = 2;
// Ignore Physical definitions and save all elements;
Mesh.SaveAll = 1;
Save "/tmp/tmpjVhNNb.unv";


//////////////////////////////////////////////////////////////////////
// GMSH documentation:
// http://gmsh.info/doc/texinfo/gmsh.html#Mesh
//
// We do not check if something went wrong, like negative jacobians etc. You can run GMSH manually yourself: 
//
// to see full GMSH log, run in bash:
// /usr/local/bin/gmsh - /tmp/tmputZ_uU.geo
//
// to run GMSH and keep file in GMSH GUI (with log), run in bash:
// /usr/local/bin/gmsh /tmp/tmputZ_uU.geo
