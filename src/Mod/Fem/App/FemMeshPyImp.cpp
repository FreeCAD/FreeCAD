/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2009     *
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
#include <algorithm>
#include <stdexcept>

#include <Base/VectorPy.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/QuantityPy.h>

#include <SMESH_Gen.hxx>
#include <SMESH_Group.hxx>
#include <SMESH_Mesh.hxx>
#include <SMESHDS_Group.hxx>
#include <SMDSAbs_ElementType.hxx>
#include <SMDS_MeshElement.hxx>
#include <SMDS_VolumeTool.hxx>
#include <SMESHDS_Mesh.hxx>

#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>

#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Part/App/TopoShapeSolidPy.h>
#include <Mod/Part/App/TopoShapeFacePy.h>
#include <Mod/Part/App/TopoShapeEdgePy.h>
#include <Mod/Part/App/TopoShapeVertexPy.h>
#include <Mod/Part/App/TopoShape.h>

#include "Mod/Fem/App/FemMesh.h"

// inclusion of the generated files (generated out of FemMeshPy.xml)
#include "FemMeshPy.h"
#include "FemMeshPy.cpp"
#include "HypothesisPy.h"

using namespace Fem;

// returns a string which represents the object e.g. when printed in python
std::string FemMeshPy::representation(void) const
{
    std::stringstream str;
    getFemMeshPtr()->getSMesh()->Dump(str);
    return str.str();
}

PyObject *FemMeshPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of FemMeshPy and the Twin object
    return new FemMeshPy(new FemMesh);
}

// constructor method
int FemMeshPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject *pcObj=0;
    if (!PyArg_ParseTuple(args, "|O", &pcObj))     // convert args: Python->C
        return -1;                             // NULL triggers exception

    try {
        // if no mesh is given
        if (!pcObj) return 0;
        if (PyObject_TypeCheck(pcObj, &(FemMeshPy::Type))) {
            getFemMeshPtr()->operator= (*static_cast<FemMeshPy*>(pcObj)->getFemMeshPtr());
        }
        else {
            PyErr_Format(PyExc_TypeError, "Cannot create a FemMesh out of a '%s'",
                pcObj->ob_type->tp_name);
            return -1;
        }
    }
    catch (const Base::Exception &e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError,e.what());
        return -1;
    }
    catch (const std::exception &e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError,e.what());
        return -1;
    }
    catch (const Py::Exception&) {
        return -1;
    }

    return 0;
}


// ===== Methods ============================================================

PyObject* FemMeshPy::setShape(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &pcObj))
        return 0;

    try {
        TopoDS_Shape shape = static_cast<Part::TopoShapePy*>(pcObj)->getTopoShapePtr()->getShape();
        getFemMeshPtr()->getSMesh()->ShapeToMesh(shape);
    }
    catch (const std::exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return 0;
    }
    Py_Return;
}

PyObject* FemMeshPy::addHypothesis(PyObject *args)
{
    PyObject* hyp;
    PyObject* shp=0;
    // Since we have not a common base class for the Python binding of the
    // hypotheses classes we cannot pass a certain Python type
    if (!PyArg_ParseTuple(args, "O|O!",&hyp, &(Part::TopoShapePy::Type), &shp))
        return 0;

    TopoDS_Shape shape;
    if (shp == 0)
        shape = getFemMeshPtr()->getSMesh()->GetShapeToMesh();
    else
        shape = static_cast<Part::TopoShapePy*>(shp)->getTopoShapePtr()->getShape();

    try {
        Py::Object obj(hyp);
        Fem::Hypothesis attr(obj.getAttr("this"));
        SMESH_HypothesisPtr thesis = attr.extensionObject()->getHypothesis();
        getFemMeshPtr()->addHypothesis(shape, thesis);
    }
    catch (const Py::Exception&) {
        return 0;
    }
    catch (const std::exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return 0;
    }
    Py_Return;
}

PyObject* FemMeshPy::setStandardHypotheses(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        getFemMeshPtr()->setStandardHypotheses();
    }
    catch (const std::exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return 0;
    }
    Py_Return;
}

PyObject* FemMeshPy::compute(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        getFemMeshPtr()->compute();
    }
    catch (const std::exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return 0;
    }
    Py_Return;
}

PyObject* FemMeshPy::addNode(PyObject *args)
{
    double x,y,z;
    int i = -1;
    if (PyArg_ParseTuple(args, "ddd",&x,&y,&z)){
        try {
            SMESH_Mesh* mesh = getFemMeshPtr()->getSMesh();
            SMESHDS_Mesh* meshDS = mesh->GetMeshDS();
            SMDS_MeshNode* node = meshDS->AddNode(x,y,z);
            if (!node)
                throw std::runtime_error("Failed to add node");
            return Py::new_reference_to(Py::Long(node->GetID()));
        }
        catch (const std::exception& e) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
            return 0;
        }
    }
    PyErr_Clear();

    if (PyArg_ParseTuple(args, "dddi",&x,&y,&z,&i)){
        try {
            SMESH_Mesh* mesh = getFemMeshPtr()->getSMesh();
            SMESHDS_Mesh* meshDS = mesh->GetMeshDS();
            SMDS_MeshNode* node = meshDS->AddNodeWithID(x,y,z,i);
            if (!node)
                throw std::runtime_error("Failed to add node");
            return Py::new_reference_to(Py::Long(node->GetID()));
        }
        catch (const std::exception& e) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
            return 0;
        }
    }
    PyErr_SetString(PyExc_TypeError, "addNode() accepts:\n"
        "-- addNode(x,y,z)\n"
        "-- addNode(x,y,z,ElemId)\n");
    return 0;
}

PyObject* FemMeshPy::addEdge(PyObject *args)
{
    SMESH_Mesh* mesh = getFemMeshPtr()->getSMesh();
    SMESHDS_Mesh* meshDS = mesh->GetMeshDS();

    int n1,n2;
    if (PyArg_ParseTuple(args, "ii",&n1,&n2)) {
        try {
            const SMDS_MeshNode* node1 = meshDS->FindNode(n1);
            const SMDS_MeshNode* node2 = meshDS->FindNode(n2);
            if (!node1 || !node2)
                throw std::runtime_error("Failed to get node of the given indices");
            SMDS_MeshEdge* edge = meshDS->AddEdge(node1, node2);
            if (!edge)
                throw std::runtime_error("Failed to add edge");
            return Py::new_reference_to(Py::Long(edge->GetID()));
        }
        catch (const std::exception& e) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
            return 0;
        }
    }
    PyErr_Clear();

    PyObject *obj;
    int ElementId=-1;
    if (PyArg_ParseTuple(args, "O!|i", &PyList_Type, &obj, &ElementId))
    {
        Py::List list(obj);
        std::vector<const SMDS_MeshNode*> Nodes;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
            Py::Long NoNr(*it);
#else
            Py::Int NoNr(*it);
#endif
            const SMDS_MeshNode* node = meshDS->FindNode(NoNr);
            if (!node)
                throw std::runtime_error("Failed to get node of the given indices");
            Nodes.push_back(node);
        }

        SMDS_MeshEdge* edge=0;
        if(ElementId != -1) {
            switch(Nodes.size()){
                case 2:
                    edge = meshDS->AddEdgeWithID(Nodes[0],Nodes[1],ElementId);
                    if (!edge)
                        throw std::runtime_error("Failed to add edge with given ElementId");
                    break;
                case 3:
                    edge = meshDS->AddEdgeWithID(Nodes[0],Nodes[1],Nodes[2],ElementId);
                    if (!edge)
                        throw std::runtime_error("Failed to add edge with given ElementId");
                    break;
                default:
                    throw std::runtime_error("Unknown node count, [2|3] are allowed"); //unknown edge type
            }
        }
        else {
            switch(Nodes.size()){
            case 2:
                edge = meshDS->AddEdge(Nodes[0],Nodes[1]);
                if (!edge)
                    throw std::runtime_error("Failed to add edge");
                break;
            case 3:
                edge = meshDS->AddEdge(Nodes[0],Nodes[1],Nodes[2]);
                if (!edge)
                    throw std::runtime_error("Failed to add edge");
                break;
            default:
                throw std::runtime_error("Unknown node count, [2|3] are allowed"); //unknown edge type
            }
        }
#if PY_MAJOR_VERSION >= 3
        return Py::new_reference_to(Py::Long(edge->GetID()));
#else
        return Py::new_reference_to(Py::Int(edge->GetID()));
#endif
    }
    PyErr_SetString(PyExc_TypeError, "addEdge accepts:\n"
        "-- int,int\n"
        "-- [2|3],[int]\n");
    return 0;
}

PyObject* FemMeshPy::addFace(PyObject *args)
{
    SMESH_Mesh* mesh = getFemMeshPtr()->getSMesh();
    SMESHDS_Mesh* meshDS = mesh->GetMeshDS();

    int n1,n2,n3;
    if (PyArg_ParseTuple(args, "iii",&n1,&n2,&n3))
    {
        // old form, deprecated
        try {
            const SMDS_MeshNode* node1 = meshDS->FindNode(n1);
            const SMDS_MeshNode* node2 = meshDS->FindNode(n2);
            const SMDS_MeshNode* node3 = meshDS->FindNode(n3);
            if (!node1 || !node2 || !node3)
                throw std::runtime_error("Failed to get node of the given indices");
            SMDS_MeshFace* face = meshDS->AddFace(node1, node2, node3);
            if (!face)
                throw std::runtime_error("Failed to add face");
            return Py::new_reference_to(Py::Long(face->GetID()));
        }
        catch (const std::exception& e) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
            return 0;
        }
    }
    PyErr_Clear();

    PyObject *obj;
    int ElementId=-1;
    if (PyArg_ParseTuple(args, "O!|i", &PyList_Type, &obj, &ElementId))
    {
        Py::List list(obj);
        std::vector<const SMDS_MeshNode*> Nodes;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
            Py::Long NoNr(*it);
#else
            Py::Int NoNr(*it);
#endif
            const SMDS_MeshNode* node = meshDS->FindNode(NoNr);
            if (!node)
                throw std::runtime_error("Failed to get node of the given indices");
            Nodes.push_back(node);
        }

        SMDS_MeshFace* face=0;
        if(ElementId != -1) {
            switch(Nodes.size()){
                case 3:
                    face = meshDS->AddFaceWithID(Nodes[0],Nodes[1],Nodes[2],ElementId);
                    if (!face)
                        throw std::runtime_error("Failed to add triangular face with given ElementId");
                    break;
                case 4:
                    face = meshDS->AddFaceWithID(Nodes[0],Nodes[1],Nodes[2],Nodes[3],ElementId);
                    if (!face)
                        throw std::runtime_error("Failed to add face with given ElementId");
                    break;
                case 6:
                    face = meshDS->AddFaceWithID(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],ElementId);
                    if (!face)
                        throw std::runtime_error("Failed to add face with given ElementId");
                    break;
                case 8:
                    face = meshDS->AddFaceWithID(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7],ElementId);
                    if (!face)
                        throw std::runtime_error("Failed to add face with given ElementId");
                    break;
                default:
                    throw std::runtime_error("Unknown node count, [3|4|6|8] are allowed"); //unknown face type
            }
        }else{
            switch(Nodes.size()){
                case 3:
                    face = meshDS->AddFace(Nodes[0],Nodes[1],Nodes[2]);
                    if (!face)
                        throw std::runtime_error("Failed to add triangular face");
                    break;
                case 4:
                    face = meshDS->AddFace(Nodes[0],Nodes[1],Nodes[2],Nodes[3]);
                    if (!face)
                        throw std::runtime_error("Failed to add face");
                    break;
                case 6:
                    face = meshDS->AddFace(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5]);
                    if (!face)
                        throw std::runtime_error("Failed to add face");
                    break;
                case 8:
                    face = meshDS->AddFace(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7]);
                    if (!face)
                        throw std::runtime_error("Failed to add face");
                    break;
                default:
                    throw std::runtime_error("Unknown node count, [4|5|6|8] are allowed"); //unknown face type
            }
        }

        return Py::new_reference_to(Py::Long(face->GetID()));

    }
    PyErr_SetString(PyExc_TypeError, "addFace accepts:\n"
        "-- int,int,int\n"
        "-- [3|4|6|8 int],[int]\n");
    return 0;
}

PyObject* FemMeshPy::addQuad(PyObject *args)
{
    int n1,n2,n3,n4;
    if (!PyArg_ParseTuple(args, "iiii",&n1,&n2,&n3,&n4))
        return 0;

    try {
        SMESH_Mesh* mesh = getFemMeshPtr()->getSMesh();
        SMESHDS_Mesh* meshDS = mesh->GetMeshDS();
        const SMDS_MeshNode* node1 = meshDS->FindNode(n1);
        const SMDS_MeshNode* node2 = meshDS->FindNode(n2);
        const SMDS_MeshNode* node3 = meshDS->FindNode(n3);
        const SMDS_MeshNode* node4 = meshDS->FindNode(n4);
        if (!node1 || !node2 || !node3 || !node4)
            throw std::runtime_error("Failed to get node of the given indices");
        SMDS_MeshFace* face = meshDS->AddFace(node1, node2, node3, node4);
        if (!face)
            throw std::runtime_error("Failed to add quad");
        return Py::new_reference_to(Py::Long(face->GetID()));
    }
    catch (const std::exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return 0;
    }
}

PyObject* FemMeshPy::addVolume(PyObject *args)
{
    SMESH_Mesh* mesh = getFemMeshPtr()->getSMesh();
    SMESHDS_Mesh* meshDS = mesh->GetMeshDS();

    int n1,n2,n3,n4;
    if (PyArg_ParseTuple(args, "iiii",&n1,&n2,&n3,&n4))
    {
        try {
            const SMDS_MeshNode* node1 = meshDS->FindNode(n1);
            const SMDS_MeshNode* node2 = meshDS->FindNode(n2);
            const SMDS_MeshNode* node3 = meshDS->FindNode(n3);
            const SMDS_MeshNode* node4 = meshDS->FindNode(n4);
            if (!node1 || !node2 || !node3 || !node4)
                throw std::runtime_error("Failed to get node of the given indices");
            SMDS_MeshVolume* vol = meshDS->AddVolume(node1, node2, node3, node4);
            if (!vol)
                throw std::runtime_error("Failed to add volume");
            return Py::new_reference_to(Py::Long(vol->GetID()));
        }
        catch (const std::exception& e) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
            return 0;
        }
    }
    PyErr_Clear();

    PyObject *obj;
    int ElementId=-1;
    if (PyArg_ParseTuple(args, "O!|i", &PyList_Type, &obj, &ElementId))
    {
        Py::List list(obj);
        std::vector<const SMDS_MeshNode*> Nodes;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
            Py::Long NoNr(*it);
#else
            Py::Int NoNr(*it);
#endif
            const SMDS_MeshNode* node = meshDS->FindNode(NoNr);
            if (!node)
                throw std::runtime_error("Failed to get node of the given indices");
            Nodes.push_back(node);
        }

        SMDS_MeshVolume* vol=0;
        if(ElementId != -1) {
            switch(Nodes.size()){
                case 4:
                    vol = meshDS->AddVolumeWithID(Nodes[0],Nodes[1],Nodes[2],Nodes[3],ElementId);
                    if (!vol)
                        throw std::runtime_error("Failed to add Tet4 volume with given ElementId");
                    break;
                case 5:
                    vol = meshDS->AddVolumeWithID(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],ElementId);
                    if (!vol)
                        throw std::runtime_error("Failed to add Pyra5 volume with given ElementId");
                    break;
                case 6:
                    vol = meshDS->AddVolumeWithID(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],ElementId);
                    if (!vol)
                        throw std::runtime_error("Failed to add Penta6 volume with given ElementId");
                    break;
                case 8:
                    vol = meshDS->AddVolumeWithID(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7],ElementId);
                    if (!vol)
                        throw std::runtime_error("Failed to add Hexa8 volume with given ElementId");
                    break;
                case 10:
                    vol = meshDS->AddVolumeWithID(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7],Nodes[8],Nodes[9],ElementId);
                    if (!vol)
                        throw std::runtime_error("Failed to add Tet10 volume with given ElementId");
                    break;
                case 13:
                    vol = meshDS->AddVolumeWithID(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7],Nodes[8],Nodes[9],Nodes[10],Nodes[11],Nodes[12],ElementId);
                    if (!vol)
                        throw std::runtime_error("Failed to add Pyra13 volume with given ElementId");
                    break;
                case 15:
                    vol = meshDS->AddVolumeWithID(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7],Nodes[8],Nodes[9],Nodes[10],Nodes[11],Nodes[12],Nodes[13],Nodes[14],ElementId);
                    if (!vol)
                        throw std::runtime_error("Failed to add Penta15 volume with given ElementId");
                    break;
                case 20:
                    vol = meshDS->AddVolumeWithID(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7],Nodes[8],Nodes[9],Nodes[10],Nodes[11],Nodes[12],Nodes[13],Nodes[14],Nodes[15],Nodes[16],Nodes[17],Nodes[18],Nodes[19],ElementId);
                    if (!vol)
                        throw std::runtime_error("Failed to add Hexa20 volume with given ElementId");
                    break;
                default: throw std::runtime_error("Unknown node count, [4|5|6|8|10|13|15|20] are allowed"); //unknown volume type
            }
        }else{
            switch(Nodes.size()){
                case 4:
                    vol = meshDS->AddVolume(Nodes[0],Nodes[1],Nodes[2],Nodes[3]);
                    if (!vol)
                        throw std::runtime_error("Failed to add Tet4 volume");
                    break;
                case 5:
                    vol = meshDS->AddVolume(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4]);
                    if (!vol)
                        throw std::runtime_error("Failed to add Pyra5 volume");
                    break;
                case 6:
                    vol = meshDS->AddVolume(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5]);
                    if (!vol)
                        throw std::runtime_error("Failed to add Penta6 volume");
                    break;
                case 8:
                    vol = meshDS->AddVolume(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7]);
                    if (!vol)
                        throw std::runtime_error("Failed to add Hexa8 volume");
                    break;
                case 10:
                    vol = meshDS->AddVolume(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7],Nodes[8],Nodes[9]);
                    if (!vol)
                        throw std::runtime_error("Failed to add Tet10 volume");
                    break;
                case 13:
                    vol = meshDS->AddVolume(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7],Nodes[8],Nodes[9],Nodes[10],Nodes[11],Nodes[12]);
                    if (!vol)
                        throw std::runtime_error("Failed to add Pyra13 volume");
                    break;
                case 15:
                    vol = meshDS->AddVolume(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7],Nodes[8],Nodes[9],Nodes[10],Nodes[11],Nodes[12],Nodes[13],Nodes[14]);
                    if (!vol)
                        throw std::runtime_error("Failed to add Penta15 volume");
                    break;
                case 20:
                    vol = meshDS->AddVolume(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7],Nodes[8],Nodes[9],Nodes[10],Nodes[11],Nodes[12],Nodes[13],Nodes[14],Nodes[15],Nodes[16],Nodes[17],Nodes[18],Nodes[19]);
                    if (!vol)
                        throw std::runtime_error("Failed to add Hexa20 volume");
                    break;
                default: throw std::runtime_error("Unknown node count, [4|5|6|8|10|13|15|20] are allowed"); //unknown volume type
            }
        }

        return Py::new_reference_to(Py::Long(vol->GetID()));

    }
    PyErr_SetString(PyExc_TypeError, "addVolume accepts:\n"
        "-- int,int,int,int\n"
        "-- [4|5|6|8|10|13|15|20 int],[int]\n");
    return 0;
}

PyObject* FemMeshPy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    const FemMesh& mesh = *getFemMeshPtr();
    return new FemMeshPy(new FemMesh(mesh));
}

PyObject* FemMeshPy::read(PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return 0;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    try {
        getFemMeshPtr()->read(EncodedName.c_str());
    }
    catch (const std::exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return 0;
    }
    Py_Return;
}

PyObject* FemMeshPy::write(PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return 0;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    try {
        getFemMeshPtr()->write(EncodedName.c_str());
    }
    catch (const std::exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return 0;
    }
    Py_Return;
}

PyObject* FemMeshPy::writeABAQUS(PyObject *args)
{
    char* Name;
    int elemParam;
    PyObject* groupParam;
    if (!PyArg_ParseTuple(args, "etiO!","utf-8",&Name,&elemParam,&PyBool_Type,&groupParam))
        return 0;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);
    bool grpParam = PyObject_IsTrue(groupParam) ? true : false;

    try {
        getFemMeshPtr()->writeABAQUS(EncodedName.c_str(), elemParam, grpParam);
    }
    catch (const std::exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return 0;
    }
    Py_Return;
}

PyObject* FemMeshPy::setTransform(PyObject *args)
{
    PyObject* ptr;
    if (!PyArg_ParseTuple(args, "O!", &(Base::PlacementPy::Type), &ptr))
        return 0;

    try {
        Base::Placement* placement = static_cast<Base::PlacementPy*>(ptr)->getPlacementPtr();
        Base::Matrix4D mat = placement->toMatrix();
        getFemMeshPtr()->transformGeometry(mat);
    }
    catch (const std::exception& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());
        return 0;
    }
    Py_Return;
}


PyObject* FemMeshPy::getFacesByFace(PyObject *args)
{
    PyObject *pW;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapeFacePy::Type), &pW))
         return 0;

    try {
        const TopoDS_Shape& sh = static_cast<Part::TopoShapeFacePy*>(pW)->getTopoShapePtr()->getShape();
        if (sh.IsNull()) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Face is empty");
            return 0;
        }

        const TopoDS_Face& fc = TopoDS::Face(sh);

        Py::List ret;
        std::list<int> resultSet = getFemMeshPtr()->getFacesByFace(fc);
        for (std::list<int>::const_iterator it = resultSet.begin();it!=resultSet.end();++it) {
#if PY_MAJOR_VERSION >= 3
            ret.append(Py::Long(*it));
#else
            ret.append(Py::Int(*it));
#endif
        }

        return Py::new_reference_to(ret);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.GetMessageString());
        return 0;
    }
}


PyObject* FemMeshPy::getEdgesByEdge(PyObject *args)
{
    PyObject *pW;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapeEdgePy::Type), &pW))
         return 0;

    try {
        const TopoDS_Shape& sh = static_cast<Part::TopoShapeEdgePy*>(pW)->getTopoShapePtr()->getShape();
        if (sh.IsNull()) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Edge is empty");
            return 0;
        }

        const TopoDS_Edge& fc = TopoDS::Edge(sh);

        Py::List ret;
        std::list<int> resultSet = getFemMeshPtr()->getEdgesByEdge(fc);
        for (std::list<int>::const_iterator it = resultSet.begin();it!=resultSet.end();++it) {
#if PY_MAJOR_VERSION >= 3
            ret.append(Py::Long(*it));
#else
            ret.append(Py::Int(*it));
#endif
        }

        return Py::new_reference_to(ret);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.GetMessageString());
        return 0;
    }
}

PyObject* FemMeshPy::getVolumesByFace(PyObject *args)
{
    PyObject *pW;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapeFacePy::Type), &pW))
         return 0;

    try {
        const TopoDS_Shape& sh = static_cast<Part::TopoShapeFacePy*>(pW)->getTopoShapePtr()->getShape();
        if (sh.IsNull()) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Face is empty");
            return 0;
        }

        const TopoDS_Face& fc = TopoDS::Face(sh);

        Py::List ret;
        std::list<std::pair<int, int> > resultSet = getFemMeshPtr()->getVolumesByFace(fc);
        for (std::list<std::pair<int, int> >::const_iterator it = resultSet.begin();it!=resultSet.end();++it) {
            Py::Tuple vol_face(2);
            vol_face.setItem(0, Py::Long(it->first));
            vol_face.setItem(1, Py::Long(it->second));
            ret.append(vol_face);
        }

        return Py::new_reference_to(ret);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.GetMessageString());
        return 0;
    }
}

PyObject* FemMeshPy::getccxVolumesByFace(PyObject *args)
{
    PyObject *pW;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapeFacePy::Type), &pW))
         return 0;

    try {
        const TopoDS_Shape& sh = static_cast<Part::TopoShapeFacePy*>(pW)->getTopoShapePtr()->getShape();
        if (sh.IsNull()) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Face is empty");
            return 0;
        }

        const TopoDS_Face& fc = TopoDS::Face(sh);

        Py::List ret;
        std::map<int, int> resultSet = getFemMeshPtr()->getccxVolumesByFace(fc);
        for (std::map<int, int>::const_iterator it = resultSet.begin();it!=resultSet.end();++it) {
            Py::Tuple vol_face(2);
            vol_face.setItem(0, Py::Long(it->first));
            vol_face.setItem(1, Py::Long(it->second));
            ret.append(vol_face);
        }

        return Py::new_reference_to(ret);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.GetMessageString());
        return 0;
    }
}

PyObject* FemMeshPy::getNodeById(PyObject *args)
{
    int id;
    if (!PyArg_ParseTuple(args, "i", &id))
        return 0;

    Base::Matrix4D Mtrx = getFemMeshPtr()->getTransform();
    const SMDS_MeshNode* aNode = getFemMeshPtr()->getSMesh()->GetMeshDS()->FindNode(id);

    if(aNode){
        Base::Vector3d vec(aNode->X(),aNode->Y(),aNode->Z());
        vec = Mtrx * vec;
        return new Base::VectorPy( vec );
    }else{
        PyErr_SetString(Base::BaseExceptionFreeCADError, "No valid ID");
        return 0;
    }
}

PyObject* FemMeshPy::getNodesBySolid(PyObject *args)
{
    PyObject *pW;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapeSolidPy::Type), &pW))
         return 0;

    try {
        const TopoDS_Shape& sh = static_cast<Part::TopoShapeSolidPy*>(pW)->getTopoShapePtr()->getShape();
        const TopoDS_Solid& fc = TopoDS::Solid(sh);
        if (sh.IsNull()) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Solid is empty");
            return 0;
        }
        Py::List ret;
        std::set<int> resultSet = getFemMeshPtr()->getNodesBySolid(fc);
        for (std::set<int>::const_iterator it = resultSet.begin();it!=resultSet.end();++it)
            ret.append(Py::Long(*it));

        return Py::new_reference_to(ret);

    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.GetMessageString());
        return 0;
    }
}

PyObject* FemMeshPy::getNodesByFace(PyObject *args)
{
    PyObject *pW;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapeFacePy::Type), &pW))
         return 0;

    try {
        const TopoDS_Shape& sh = static_cast<Part::TopoShapeFacePy*>(pW)->getTopoShapePtr()->getShape();
        const TopoDS_Face& fc = TopoDS::Face(sh);
        if (sh.IsNull()) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Face is empty");
            return 0;
        }
        Py::List ret;
        std::set<int> resultSet = getFemMeshPtr()->getNodesByFace(fc);
        for (std::set<int>::const_iterator it = resultSet.begin();it!=resultSet.end();++it)
            ret.append(Py::Long(*it));

        return Py::new_reference_to(ret);

    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.GetMessageString());
        return 0;
    }
}

PyObject* FemMeshPy::getNodesByEdge(PyObject *args)
{
    PyObject *pW;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapeEdgePy::Type), &pW))
         return 0;

    try {
        const TopoDS_Shape& sh = static_cast<Part::TopoShapeEdgePy*>(pW)->getTopoShapePtr()->getShape();
        const TopoDS_Edge& fc = TopoDS::Edge(sh);
        if (sh.IsNull()) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Edge is empty");
            return 0;
        }
        Py::List ret;
        std::set<int> resultSet = getFemMeshPtr()->getNodesByEdge(fc);
        for (std::set<int>::const_iterator it = resultSet.begin();it!=resultSet.end();++it)
            ret.append(Py::Long(*it));

        return Py::new_reference_to(ret);

    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.GetMessageString());
        return 0;
    }
}

PyObject* FemMeshPy::getNodesByVertex(PyObject *args)
{
    PyObject *pW;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapeVertexPy::Type), &pW))
         return 0;

    try {
        const TopoDS_Shape& sh = static_cast<Part::TopoShapeVertexPy*>(pW)->getTopoShapePtr()->getShape();
        const TopoDS_Vertex& fc = TopoDS::Vertex(sh);
        if (sh.IsNull()) {
            PyErr_SetString(Base::BaseExceptionFreeCADError, "Vertex is empty");
            return 0;
        }
        Py::List ret;
        std::set<int> resultSet = getFemMeshPtr()->getNodesByVertex(fc);
        for (std::set<int>::const_iterator it = resultSet.begin();it!=resultSet.end();++it)
            ret.append(Py::Long(*it));

        return Py::new_reference_to(ret);

    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.GetMessageString());
        return 0;
    }
}

PyObject* FemMeshPy::getElementNodes(PyObject *args)
{
    int id;
    if (!PyArg_ParseTuple(args, "i", &id))
         return 0;

    try {
        std::list<int> resultSet = getFemMeshPtr()->getElementNodes(id);
        Py::Tuple ret(resultSet.size());
        int index = 0;
        for (std::list<int>::const_iterator it = resultSet.begin();it!=resultSet.end();++it)
            ret.setItem(index++, Py::Long(*it));

        return Py::new_reference_to(ret);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.GetMessageString());
        return 0;
    }
}

PyObject* FemMeshPy::getGroupName(PyObject *args)
{
    int id;
    if (!PyArg_ParseTuple(args, "i", &id))
         return 0;
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromString(getFemMeshPtr()->getSMesh()->GetGroup(id)->GetName());
#else
    return PyString_FromString(getFemMeshPtr()->getSMesh()->GetGroup(id)->GetName());
#endif
}

PyObject* FemMeshPy::getGroupElementType(PyObject *args)
{
    int id;
    if (!PyArg_ParseTuple(args, "i", &id))
         return 0;

    SMDSAbs_ElementType aElementType = getFemMeshPtr()->getSMesh()->GetGroup(id)->GetGroupDS()->GetType();
    const char* typeString = "";
    switch(aElementType) {
        case SMDSAbs_All            : typeString = "All"; break;
        case SMDSAbs_Node           : typeString = "Node"; break;
        case SMDSAbs_Edge           : typeString = "Edge"; break;
        case SMDSAbs_Face           : typeString = "Face"; break;
        case SMDSAbs_Volume         : typeString = "Volume"; break;
        case SMDSAbs_0DElement      : typeString = "0DElement"; break;
        case SMDSAbs_Ball           : typeString = "Ball"; break;
        default                     : typeString = "Unknown"; break;
    }
#if PY_MAJOR_VERSION >= 3
    return PyUnicode_FromString(typeString);
#else
    return PyString_FromString(typeString);
#endif
}

PyObject* FemMeshPy::getGroupElements(PyObject *args)
{
    int id;
    if (!PyArg_ParseTuple(args, "i", &id))
         return 0;

    std::set<int> ids;
    SMDS_ElemIteratorPtr aElemIter = getFemMeshPtr()->getSMesh()->GetGroup(id)->GetGroupDS()->GetElements();
    while (aElemIter->more()) {
        const SMDS_MeshElement* aElement = aElemIter->next();
        ids.insert(aElement->GetID());
    }

    Py::Tuple tuple(ids.size());
    int index = 0;
    for (std::set<int>::iterator it = ids.begin(); it != ids.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
        tuple.setItem(index++, Py::Long(*it));
#else
        tuple.setItem(index++, Py::Int(*it));
#endif
    }

    return Py::new_reference_to(tuple);
}

// ===== Attributes ============================================================

Py::Dict FemMeshPy::getNodes(void) const
{
    //int count = getFemMeshPtr()->getSMesh()->GetMeshDS()->NbNodes();
    //Py::Tuple tup(count);
    Py::Dict dict;

    // get the actual transform of the FemMesh
    Base::Matrix4D Mtrx = getFemMeshPtr()->getTransform();

    SMDS_NodeIteratorPtr aNodeIter = getFemMeshPtr()->getSMesh()->GetMeshDS()->nodesIterator();
    for (int i=0;aNodeIter->more();i++) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        Base::Vector3d vec(aNode->X(),aNode->Y(),aNode->Z());
        // Apply the matrix to hold the BoundBox in absolute space.
        vec = Mtrx * vec;
        int id = aNode->GetID();

        dict[Py::Long(id)] = Py::asObject(new Base::VectorPy( vec ));
    }

    return dict;
}

Py::Long FemMeshPy::getNodeCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbNodes());
}

Py::Tuple FemMeshPy::getEdges(void) const
{
    std::set<int> ids;
    SMDS_EdgeIteratorPtr aEdgeIter = getFemMeshPtr()->getSMesh()->GetMeshDS()->edgesIterator();
    while (aEdgeIter->more()) {
        const SMDS_MeshEdge* aEdge = aEdgeIter->next();
        ids.insert(aEdge->GetID());
    }

    Py::Tuple tuple(ids.size());
    int index = 0;
    for (std::set<int>::iterator it = ids.begin(); it != ids.end(); ++it) {
        tuple.setItem(index++, Py::Long(*it));
    }

    return tuple;
}

Py::Tuple FemMeshPy::getEdgesOnly(void) const
{
    std::set<int> resultSet = getFemMeshPtr()->getEdgesOnly();
    Py::Tuple tuple(resultSet.size());
    int index = 0;
    for (std::set<int>::iterator it = resultSet.begin(); it != resultSet.end(); ++it) {
        tuple.setItem(index++, Py::Long(*it));
    }

    return tuple;
}

Py::Long FemMeshPy::getEdgeCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbEdges());
}

Py::Tuple FemMeshPy::getFaces(void) const
{
    std::set<int> ids;
    SMDS_FaceIteratorPtr aFaceIter = getFemMeshPtr()->getSMesh()->GetMeshDS()->facesIterator();
    while (aFaceIter->more()) {
        const SMDS_MeshFace* aFace = aFaceIter->next();
        ids.insert(aFace->GetID());
    }

    Py::Tuple tuple(ids.size());
    int index = 0;
    for (std::set<int>::iterator it = ids.begin(); it != ids.end(); ++it) {
        tuple.setItem(index++, Py::Long(*it));
    }

    return tuple;
}

Py::Tuple FemMeshPy::getFacesOnly(void) const
{
    std::set<int> resultSet = getFemMeshPtr()->getFacesOnly();
    Py::Tuple tuple(resultSet.size());
    int index = 0;
    for (std::set<int>::iterator it = resultSet.begin(); it != resultSet.end(); ++it) {
        tuple.setItem(index++, Py::Long(*it));
    }

    return tuple;
}

Py::Long FemMeshPy::getFaceCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbFaces());
}

Py::Long FemMeshPy::getTriangleCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbTriangles());
}

Py::Long FemMeshPy::getQuadrangleCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbQuadrangles());
}

Py::Long FemMeshPy::getPolygonCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbPolygons());
}

Py::Tuple FemMeshPy::getVolumes(void) const
{
    std::set<int> ids;
    SMDS_VolumeIteratorPtr aVolIter = getFemMeshPtr()->getSMesh()->GetMeshDS()->volumesIterator();
    while (aVolIter->more()) {
        const SMDS_MeshVolume* aVol = aVolIter->next();
        ids.insert(aVol->GetID());
    }

    Py::Tuple tuple(ids.size());
    int index = 0;
    for (std::set<int>::iterator it = ids.begin(); it != ids.end(); ++it) {
        tuple.setItem(index++, Py::Long(*it));
    }

    return tuple;
}

Py::Long FemMeshPy::getVolumeCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbVolumes());
}

Py::Long FemMeshPy::getTetraCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbTetras());
}

Py::Long FemMeshPy::getHexaCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbHexas());
}

Py::Long FemMeshPy::getPyramidCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbPyramids());
}

Py::Long FemMeshPy::getPrismCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbPrisms());
}

Py::Long FemMeshPy::getPolyhedronCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbPolyhedrons());
}

Py::Long FemMeshPy::getSubMeshCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbSubMesh());
}

Py::Long FemMeshPy::getGroupCount(void) const
{
    return Py::Long(getFemMeshPtr()->getSMesh()->NbGroup());
}

Py::Tuple FemMeshPy::getGroups(void) const
{
    std::list<int> groupIDs = getFemMeshPtr()->getSMesh()->GetGroupIds();

    Py::Tuple tuple(groupIDs.size());
    int index = 0;
    for (std::list<int>::iterator it = groupIDs.begin(); it != groupIDs.end(); ++it) {
#if PY_MAJOR_VERSION >= 3
        tuple.setItem(index++, Py::Long(*it));
#else
        tuple.setItem(index++, Py::Int(*it));
#endif
    }

    return tuple;
}

Py::Object FemMeshPy::getVolume(void) const
{
    return Py::Object(new Base::QuantityPy(new Base::Quantity(getFemMeshPtr()->getVolume())));

}

// ===== custom attributes ============================================================

PyObject *FemMeshPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int FemMeshPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
