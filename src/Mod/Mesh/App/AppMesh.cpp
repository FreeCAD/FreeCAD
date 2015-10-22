/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Python.h>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "Mesh.h"
#include "MeshPy.h"
#include "MeshPointPy.h"
#include "FacetPy.h"
#include "MeshFeaturePy.h"
#include "FeatureMeshImport.h"
#include "FeatureMeshExport.h"
#include "FeatureMeshTransform.h"
#include "FeatureMeshTransformDemolding.h"
#include "FeatureMeshCurvature.h"
#include "FeatureMeshSegmentByMesh.h"
#include "FeatureMeshSetOperations.h"
#include "FeatureMeshDefects.h"
#include "FeatureMeshSolid.h"

/* registration table  */
extern struct PyMethodDef Mesh_Import_methods[];


PyDoc_STRVAR(module_doc,
"The functions in this module allow working with mesh objects.\n"
"A set of functions are provided that allow to read in registered mesh file formats\n"
"to either an newly created or already exising document.\n"
"\n"
"open(string) -- Create a new document and a Mesh::Import feature to load the file into the document.\n"
"insert(string, string) -- Create a Mesh::Import feature to load the file into the given document.\n"
"mesh() -- Create an empty mesh object.\n"
"\n");

/* Python entry */
extern "C" {
void MeshExport initMesh() 
{
    PyObject* meshModule = Py_InitModule3("Mesh", Mesh_Import_methods, module_doc);   /* mod name, table ptr */
    Base::Console().Log("Loading Mesh module... done\n");

    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.

    // add mesh elements
    Base::Interpreter().addType(&Mesh::MeshPointPy  ::Type,meshModule,"MeshPoint");
    Base::Interpreter().addType(&Mesh::FacetPy      ::Type,meshModule,"Facet");
    Base::Interpreter().addType(&Mesh::MeshPy       ::Type,meshModule,"Mesh");
    Base::Interpreter().addType(&Mesh::MeshFeaturePy::Type,meshModule,"Feature");

    // init Type system
    Mesh::PropertyNormalList    ::init();
    Mesh::PropertyCurvatureList ::init();
    Mesh::PropertyMeshKernel    ::init();

    Mesh::MeshObject            ::init();

    Mesh::Feature               ::init();
    Mesh::FeaturePython         ::init();
    Mesh::Import                ::init();
    Mesh::Export                ::init();
    Mesh::Transform             ::init();
    Mesh::TransformDemolding    ::init();
    Mesh::Curvature             ::init();
    Mesh::SegmentByMesh         ::init();
    Mesh::SetOperations         ::init();
    Mesh::FixDefects            ::init();
    Mesh::HarmonizeNormals      ::init();
    Mesh::FlipNormals           ::init();
    Mesh::FixNonManifolds       ::init();
    Mesh::FixDuplicatedFaces    ::init();
    Mesh::FixDuplicatedPoints   ::init();
    Mesh::FixDegenerations      ::init();
    Mesh::FixDeformations       ::init();
    Mesh::FixIndices            ::init();
    Mesh::FillHoles             ::init();
    Mesh::RemoveComponents      ::init();

    Mesh::Sphere                ::init();
    Mesh::Ellipsoid             ::init();
    Mesh::Cylinder              ::init();
    Mesh::Cone                  ::init();
    Mesh::Torus                 ::init();
    Mesh::Cube                  ::init();
}


} // extern "C" 
