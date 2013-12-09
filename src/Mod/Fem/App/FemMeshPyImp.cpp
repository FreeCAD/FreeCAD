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
#include <stdexcept>

#include <SMESH_Gen.hxx>
#include <SMESH_Mesh.hxx>
#include <SMDS_VolumeTool.hxx>

#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>

#include <Base/VectorPy.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/QuantityPy.h>

#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Part/App/TopoShapeFacePy.h>
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
        PyErr_SetString(PyExc_Exception,e.what());
        return -1;
    }
    catch (const std::exception &e) {
        PyErr_SetString(PyExc_Exception,e.what());
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
        TopoDS_Shape shape = static_cast<Part::TopoShapePy*>(pcObj)->getTopoShapePtr()->_Shape;
        getFemMeshPtr()->getSMesh()->ShapeToMesh(shape);
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
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
        shape = static_cast<Part::TopoShapePy*>(shp)->getTopoShapePtr()->_Shape;

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
        PyErr_SetString(PyExc_Exception, e.what());
        return 0;
    }
    Py_Return;
}

PyObject* FemMeshPy::setStanardHypotheses(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        getFemMeshPtr()->setStanardHypotheses();
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
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
        PyErr_SetString(PyExc_Exception, e.what());
        return 0;
    }
    Py_Return;
}

PyObject* FemMeshPy::addNode(PyObject *args)
{
    double x,y,z;
    if (!PyArg_ParseTuple(args, "ddd",&x,&y,&z))
        return 0;

    try {
        SMESH_Mesh* mesh = getFemMeshPtr()->getSMesh();
        SMESHDS_Mesh* meshDS = mesh->GetMeshDS();
        SMDS_MeshNode* node = meshDS->AddNode(x,y,z);
        if (!node)
            throw std::runtime_error("Failed to add node");
        return Py::new_reference_to(Py::Int(node->GetID()));
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return 0;
    }
}

PyObject* FemMeshPy::addEdge(PyObject *args)
{
    int n1,n2;
    if (!PyArg_ParseTuple(args, "ii",&n1,&n2))
        return 0;

    try {
        SMESH_Mesh* mesh = getFemMeshPtr()->getSMesh();
        SMESHDS_Mesh* meshDS = mesh->GetMeshDS();
        const SMDS_MeshNode* node1 = meshDS->FindNode(n1);
        const SMDS_MeshNode* node2 = meshDS->FindNode(n2);
        if (!node1 || !node2)
            throw std::runtime_error("Failed to get node of the given indices");
        SMDS_MeshEdge* edge = meshDS->AddEdge(node1, node2);
        if (!edge)
            throw std::runtime_error("Failed to add edge");
        return Py::new_reference_to(Py::Int(edge->GetID()));
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return 0;
    }
}

PyObject* FemMeshPy::addFace(PyObject *args)
{
    SMESH_Mesh* mesh = getFemMeshPtr()->getSMesh();
    SMESHDS_Mesh* meshDS = mesh->GetMeshDS();

    int n1,n2,n3;
    if (PyArg_ParseTuple(args, "iii",&n1,&n2,&n3))
    {
        // old form, debrekadet
        try {
            const SMDS_MeshNode* node1 = meshDS->FindNode(n1);
            const SMDS_MeshNode* node2 = meshDS->FindNode(n2);
            const SMDS_MeshNode* node3 = meshDS->FindNode(n3);
            if (!node1 || !node2 || !node3)
                throw std::runtime_error("Failed to get node of the given indices");
            SMDS_MeshFace* face = meshDS->AddFace(node1, node2, node3);
            if (!face)
                throw std::runtime_error("Failed to add face");
            return Py::new_reference_to(Py::Int(face->GetID()));
        }
        catch (const std::exception& e) {
            PyErr_SetString(PyExc_Exception, e.what());
            return 0;
        }
    }
    PyErr_Clear();

    PyObject *obj;
    int ElementId=-1;
    float min_eps = 1.0e-2f;
    if (PyArg_ParseTuple(args, "O!|i", &PyList_Type, &obj, &ElementId))
    {
        Py::List list(obj);
        std::vector<const SMDS_MeshNode*> Nodes;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Int NoNr(*it);
            const SMDS_MeshNode* node = meshDS->FindNode(NoNr);
            if (!node)
                throw std::runtime_error("Failed to get node of the given indices");
            Nodes.push_back(node);
        }
        
        SMDS_MeshFace* face=0;
        switch(Nodes.size()){
            case 3:
                face = meshDS->AddFace(Nodes[0],Nodes[1],Nodes[2]);
                if (!face)
                    throw std::runtime_error("Failed to add triangular face");
                break;

            default: throw std::runtime_error("Unknown node count, [3|4|6|8] are allowed"); //unknown face type
        }

        return Py::new_reference_to(Py::Int(face->GetID()));

    }

    PyErr_SetString(PyExc_TypeError, "Line constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Line\n"
        "-- Point, Point");
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
        return Py::new_reference_to(Py::Int(face->GetID()));
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
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
            return Py::new_reference_to(Py::Int(vol->GetID()));
        }
        catch (const std::exception& e) {
            PyErr_SetString(PyExc_Exception, e.what());
            return 0;
        }
    }
    PyErr_Clear();

    PyObject *obj;
    int ElementId=-1;
    float min_eps = 1.0e-2f;
    if (PyArg_ParseTuple(args, "O!|i", &PyList_Type, &obj, &ElementId))
    {
        Py::List list(obj);
        std::vector<const SMDS_MeshNode*> Nodes;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Int NoNr(*it);
            const SMDS_MeshNode* node = meshDS->FindNode(NoNr);
            if (!node)
                throw std::runtime_error("Failed to get node of the given indices");
            Nodes.push_back(node);
        }
        
        SMDS_MeshVolume* vol=0;
        switch(Nodes.size()){
            case 4:
                vol = meshDS->AddVolume(Nodes[0],Nodes[1],Nodes[2],Nodes[3]);
                if (!vol)
                    throw std::runtime_error("Failed to add Tet4 volume");
                break;
            case 8:
                vol = meshDS->AddVolume(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7]);
                if (!vol)
                    throw std::runtime_error("Failed to add Tet10 volume");
                break;
            case 10:
                vol = meshDS->AddVolume(Nodes[0],Nodes[1],Nodes[2],Nodes[3],Nodes[4],Nodes[5],Nodes[6],Nodes[7],Nodes[8],Nodes[9]);
                if (!vol)
                    throw std::runtime_error("Failed to add Tet10 volume");
                break;

            default: throw std::runtime_error("Unknown node count, [4|5|6|8|10|13|18] are allowed"); //unknown face type
        }

        return Py::new_reference_to(Py::Int(vol->GetID()));

    }

    PyErr_SetString(PyExc_TypeError, "Line constructor accepts:\n"
        "-- empty parameter list\n"
        "-- Line\n"
        "-- Point, Point");
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
    char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return 0;

    try {
        getFemMeshPtr()->read(filename);
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return 0;
    }
    Py_Return;
}

PyObject* FemMeshPy::write(PyObject *args)
{
    char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return 0;

    try {
        getFemMeshPtr()->write(filename);
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return 0;
    }
    Py_Return;
}

PyObject* FemMeshPy::writeABAQUS(PyObject *args)
{
    char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return 0;

    try {
        getFemMeshPtr()->writeABAQUS(filename);
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
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
        PyErr_SetString(PyExc_Exception, e.what());
        return 0;
    }
    Py_Return;
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
        PyErr_SetString(PyExc_Exception, "No valid ID");
        return 0;
    }
}

PyObject* FemMeshPy::getNodesByFace(PyObject *args)
{
    PyObject *pW;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapeFacePy::Type), &pW))
         return 0;

    try {
        const TopoDS_Shape& sh = static_cast<Part::TopoShapeFacePy*>(pW)->getTopoShapePtr()->_Shape;
        if (sh.IsNull()) {
            PyErr_SetString(PyExc_Exception, "Face is empty");
            return 0;
        }
        Py::List ret;
        throw Py::Exception("Not yet implemented");


    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
  
}



// ===== Atributes ============================================================

Py::Dict FemMeshPy::getNodes(void) const
{
    //int count = getFemMeshPtr()->getSMesh()->GetMeshDS()->NbNodes();
    //Py::Tuple tup(count);
    Py::Dict dict;

    // get the actuall transform of the FemMesh
    Base::Matrix4D Mtrx = getFemMeshPtr()->getTransform();

    SMDS_NodeIteratorPtr aNodeIter = getFemMeshPtr()->getSMesh()->GetMeshDS()->nodesIterator();
	for (int i=0;aNodeIter->more();i++) {
		const SMDS_MeshNode* aNode = aNodeIter->next();
        Base::Vector3d vec(aNode->X(),aNode->Y(),aNode->Z());
        // Apply the matrix to hold the BoundBox in absolute space. 
        vec = Mtrx * vec;
        int id = aNode->GetID();

        dict[Py::Int(id)] = Py::asObject(new Base::VectorPy( vec ));
	}

    return dict;
}

Py::Int FemMeshPy::getNodeCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbNodes());
}

Py::Int FemMeshPy::getEdgeCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbEdges());
}

Py::Int FemMeshPy::getFacesCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbFaces());
}

Py::Int FemMeshPy::getTriangleCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbTriangles());
}

Py::Int FemMeshPy::getQuadrangleCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbQuadrangles());
}

Py::Int FemMeshPy::getPolygonCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbPolygons());
}

Py::Int FemMeshPy::getVolumeCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbVolumes());
}

Py::Int FemMeshPy::getTetraCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbTetras());
}

Py::Int FemMeshPy::getHexaCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbHexas());
}

Py::Int FemMeshPy::getPyramidCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbPyramids());
}

Py::Int FemMeshPy::getPrismCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbPrisms());
}

Py::Int FemMeshPy::getPolyhedronCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbPolyhedrons());
}

Py::Int FemMeshPy::getSubMeshCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbSubMesh());
}

Py::Int FemMeshPy::getGroupCount(void) const
{
    return Py::Int(getFemMeshPtr()->getSMesh()->NbGroup());
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
