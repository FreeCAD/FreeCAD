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
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/FileInfo.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/Property.h>
#include <Base/PlacementPy.h>

#include <CXX/Objects.hxx>
#include <Base/VectorPy.h>

#include "Core/MeshKernel.h"
#include "Core/MeshIO.h"
#include "Core/Evaluation.h"
#include "Core/Iterator.h"

#include "MeshPy.h"
#include "Mesh.h"
#include "FeatureMeshImport.h"

using namespace Mesh;
using namespace MeshCore;


/* module functions */
static PyObject * read(PyObject *self, PyObject *args)
{
    const char* Name;
    if (!PyArg_ParseTuple(args, "s",&Name))
        return NULL;

    PY_TRY {
        std::auto_ptr<MeshObject> mesh(new MeshObject);
        if (mesh->load(Name)) {
            return new MeshPy(mesh.release());
        }
        else {
            PyErr_SetString(PyExc_Exception, "Loading of mesh was aborted");
            return NULL;
        }
    } PY_CATCH;

    Py_Return;
}

static PyObject * open(PyObject *self, PyObject *args)
{
    const char* Name;
    if (!PyArg_ParseTuple(args, "s",&Name))
        return NULL;

    PY_TRY {
        MeshObject mesh;
        if (mesh.load(Name)) {
            Base::FileInfo file(Name);
            // create new document and add Import feature
            App::Document *pcDoc = App::GetApplication().newDocument("Unnamed");
            unsigned long segmct = mesh.countSegments();
            if (segmct > 1) {
                for (unsigned long i=0; i<segmct; i++) {
                    std::auto_ptr<MeshObject> segm(mesh.meshFromSegment(mesh.getSegment(i).getIndices()));
                    Mesh::Feature *pcFeature = static_cast<Mesh::Feature *>
                        (pcDoc->addObject("Mesh::Feature", file.fileNamePure().c_str()));
                    pcFeature->Label.setValue(file.fileNamePure().c_str());
                    pcFeature->Mesh.swapMesh(*segm);
                    pcFeature->purgeTouched();
                }
            }
            else {
                Mesh::Feature *pcFeature = static_cast<Mesh::Feature *>
                    (pcDoc->addObject("Mesh::Feature", file.fileNamePure().c_str()));
                pcFeature->Label.setValue(file.fileNamePure().c_str());
                pcFeature->Mesh.swapMesh(mesh);
                pcFeature->purgeTouched();
            }
        }
    } PY_CATCH;

    Py_Return;
}

static PyObject * importer(PyObject *self, PyObject *args)
{
    const char* Name;
    const char* DocName=0;

    if (!PyArg_ParseTuple(args, "s|s",&Name,&DocName))
        return NULL;

    PY_TRY {
        App::Document *pcDoc = 0;
        if (DocName)
            pcDoc = App::GetApplication().getDocument(DocName);
        else
            pcDoc = App::GetApplication().getActiveDocument();

        if (!pcDoc) {
            pcDoc = App::GetApplication().newDocument(DocName);
        }

        MeshObject mesh;
        if (mesh.load(Name)) {
            Base::FileInfo file(Name);
            unsigned long segmct = mesh.countSegments();
            if (segmct > 1) {
                for (unsigned long i=0; i<segmct; i++) {
                    std::auto_ptr<MeshObject> segm(mesh.meshFromSegment(mesh.getSegment(i).getIndices()));
                    Mesh::Feature *pcFeature = static_cast<Mesh::Feature *>
                        (pcDoc->addObject("Mesh::Feature", file.fileNamePure().c_str()));
                    pcFeature->Label.setValue(file.fileNamePure().c_str());
                    pcFeature->Mesh.swapMesh(*segm);
                    pcFeature->purgeTouched();
                }
            }
            else {
                Mesh::Feature *pcFeature = static_cast<Mesh::Feature *>
                    (pcDoc->addObject("Mesh::Feature", file.fileNamePure().c_str()));
                pcFeature->Label.setValue(file.fileNamePure().c_str());
                pcFeature->Mesh.swapMesh(mesh);
                pcFeature->purgeTouched();
            }
        }
    } PY_CATCH;

    Py_Return;
}

static PyObject * exporter(PyObject *self, PyObject *args)
{
    PyObject* object;
    const char* filename;
    if (!PyArg_ParseTuple(args, "Os",&object,&filename))
        return NULL;

    float fTolerance = 0.1f;
    MeshObject global_mesh;

    PY_TRY {
        Py::Sequence list(object);
        Base::Type meshId = Base::Type::fromName("Mesh::Feature");
        Base::Type partId = Base::Type::fromName("Part::Feature");
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* item = (*it).ptr();
            if (PyObject_TypeCheck(item, &(App::DocumentObjectPy::Type))) {
                App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(item)->getDocumentObjectPtr();
                if (obj->getTypeId().isDerivedFrom(meshId)) {
                    const MeshObject& mesh = static_cast<Mesh::Feature*>(obj)->Mesh.getValue();
                    MeshCore::MeshKernel kernel = mesh.getKernel();
                    kernel.Transform(mesh.getTransform());
                    if (global_mesh.countFacets() == 0)
                        global_mesh.setKernel(kernel);
                    else
                        global_mesh.addMesh(kernel);
                }
                else if (obj->getTypeId().isDerivedFrom(partId)) {
                    App::Property* shape = obj->getPropertyByName("Shape");
                    Base::Reference<MeshObject> mesh(new MeshObject());
                    if (shape && shape->getTypeId().isDerivedFrom(App::PropertyComplexGeoData::getClassTypeId())) {
                        std::vector<Base::Vector3d> aPoints;
                        std::vector<Data::ComplexGeoData::Facet> aTopo;
                        static_cast<App::PropertyComplexGeoData*>(shape)->getFaces(aPoints, aTopo,fTolerance);
                        mesh->addFacets(aTopo, aPoints);
                        if (global_mesh.countFacets() == 0)
                            global_mesh = *mesh;
                        else
                            global_mesh.addMesh(*mesh);
                    }
                }
                else {
                    Base::Console().Message("'%s' is not a mesh or shape, export will be ignored.\n", obj->Label.getValue());
                }
            }
        }

        // export mesh compound
        global_mesh.save(filename);
    } PY_CATCH;

    Py_Return;
}

static PyObject * 
show(PyObject *self, PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(MeshPy::Type), &pcObj))
        return NULL;

    PY_TRY {
        App::Document *pcDoc = App::GetApplication().getActiveDocument();
        if (!pcDoc)
            pcDoc = App::GetApplication().newDocument();
        MeshPy* pMesh = static_cast<MeshPy*>(pcObj);
        Mesh::Feature *pcFeature = (Mesh::Feature *)pcDoc->addObject("Mesh::Feature", "Mesh");
        Mesh::MeshObject* mo = pMesh->getMeshObjectPtr();
        if (!mo) {
            PyErr_SetString(PyExc_ReferenceError,
                "object doesn't reference a valid mesh");
            return 0;
        }
        // copy the data
        pcFeature->Mesh.setValue(*mo);
    } PY_CATCH;

    Py_Return;
}

static PyObject *
createPlane(PyObject *self, PyObject *args)
{
    float x=1,y=0,z=0;
    if (!PyArg_ParseTuple(args, "|fff",&x,&y,&z))     // convert args: Python->C 
        return NULL;                                   // NULL triggers exception 

    if(y==0) 
        y=x;

    float hx = x/2.0f;
    float hy = y/2.0f;

    PY_TRY {
        std::vector<MeshCore::MeshGeomFacet> TriaList;
        TriaList.push_back(MeshCore::MeshGeomFacet(Base::Vector3f(-hx, -hy, 0.0),Base::Vector3f(hx, hy, 0.0),Base::Vector3f(-hx, hy, 0.0)));
        TriaList.push_back(MeshCore::MeshGeomFacet(Base::Vector3f(-hx, -hy, 0.0),Base::Vector3f(hx, -hy, 0.0),Base::Vector3f(hx, hy, 0.0)));

        std::auto_ptr<MeshObject> mesh(new MeshObject);
        mesh->addFacets(TriaList);
        return new MeshPy(mesh.release());
    } PY_CATCH;
}

static PyObject *
createSphere(PyObject *self, PyObject *args)
{
    float radius = 5.0f;
    int sampling = 50;
    if (!PyArg_ParseTuple(args, "|fi",&radius,&sampling))     // convert args: Python->C 
        return NULL;                                   // NULL triggers exception 

    PY_TRY {
        MeshObject* mesh = MeshObject::createSphere(radius, sampling);
        if (!mesh) {
            PyErr_SetString(PyExc_Exception, "Creation of sphere failed");
            return NULL;
        }
        return new MeshPy(mesh);
    } PY_CATCH;
}

static PyObject *
createEllipsoid(PyObject *self, PyObject *args)
{
    float radius1 = 2.0f;
    float radius2 = 4.0f;
    int sampling = 50;
    if (!PyArg_ParseTuple(args, "|ffi",&radius1,&radius2,&sampling))     // convert args: Python->C 
        return NULL;                                   // NULL triggers exception 

    PY_TRY {
        MeshObject* mesh = MeshObject::createEllipsoid(radius1, radius2, sampling);
        if (!mesh) {
            PyErr_SetString(PyExc_Exception, "Creation of ellipsoid failed");
            return NULL;
        }
        return new MeshPy(mesh);
    } PY_CATCH;
}

static PyObject *
createCylinder(PyObject *self, PyObject *args)
{
    float radius = 2.0f;
    float length = 10.0f;
    int closed = 1;
    float edgelen = 1.0f;
    int sampling = 50;
    if (!PyArg_ParseTuple(args, "|ffifi",&radius,&length,&closed,&edgelen,&sampling))     // convert args: Python->C 
        return NULL;                                   // NULL triggers exception 

    PY_TRY {
        MeshObject* mesh = MeshObject::createCylinder(radius, length, closed, edgelen, sampling);
        if (!mesh) {
            PyErr_SetString(PyExc_Exception, "Creation of cylinder failed");
            return NULL;
        }
        return new MeshPy(mesh);
    } PY_CATCH;
}

static PyObject *
createCone(PyObject *self, PyObject *args)
{
    float radius1 = 2.0f;
    float radius2 = 4.0f;
    float len = 10.0f;
    int closed = 1;
    float edgelen = 1.0f;
    int sampling = 50;
    if (!PyArg_ParseTuple(args, "|fffifi",&radius1,&radius2,&len,&closed,&edgelen,&sampling))     // convert args: Python->C 
        return NULL;                                   // NULL triggers exception 

    PY_TRY {
        MeshObject* mesh = MeshObject::createCone(radius1, radius2, len, closed, edgelen, sampling);
        if (!mesh) {
            PyErr_SetString(PyExc_Exception, "Creation of cone failed");
            return NULL;
        }
        return new MeshPy(mesh);
    } PY_CATCH;
}

static PyObject *
createTorus(PyObject *self, PyObject *args)
{
    float radius1 = 10.0f;
    float radius2 = 2.0f;
    int sampling = 50;
    if (!PyArg_ParseTuple(args, "|ffi",&radius1,&radius2,&sampling))     // convert args: Python->C 
        return NULL;                                   // NULL triggers exception 

    PY_TRY {
        MeshObject* mesh = MeshObject::createTorus(radius1, radius2, sampling);
        if (!mesh) {
            PyErr_SetString(PyExc_Exception, "Creation of torus failed");
            return NULL;
        }
        return new MeshPy(mesh);
    } PY_CATCH;
}

static PyObject * 
createBox(PyObject *self, PyObject *args)
{
    float length = 10.0f;
    float width = 10.0f;
    float height = 10.0f;
    float edgelen = -1.0f;
    if (!PyArg_ParseTuple(args, "|ffff",&length,&width,&height,&edgelen))     // convert args: Python->C 
        return NULL;                                   // NULL triggers exception 

    PY_TRY {
        MeshObject* mesh;
        if (edgelen < 0.0f)
            mesh = MeshObject::createCube(length, width, height);
        else
            mesh = MeshObject::createCube(length, width, height, edgelen);

        if (!mesh) {
            PyErr_SetString(PyExc_Exception, "Creation of box failed");
            return NULL;
        }
        return new MeshPy(mesh);
    } PY_CATCH;
}

static PyObject * 
calculateEigenTransform(PyObject *self, PyObject *args)
{
    PyObject *input;

    if (!PyArg_ParseTuple(args, "O",&input))
        return NULL;

    if(! PySequence_Check(input) ){
        PyErr_SetString(PyExc_Exception, "Input have to be a sequence of Base.Vector()");
        return NULL;
    }

    PY_TRY {
        MeshCore::MeshKernel aMesh;
        MeshCore::MeshPointArray vertices;
        vertices.clear();
        MeshCore::MeshFacetArray faces;
        faces.clear();
        MeshCore::MeshPoint current_node;

        Py::Sequence list(input);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            PyObject* value = (*it).ptr();
            if (PyObject_TypeCheck(value, &(Base::VectorPy::Type))) {
                Base::VectorPy  *pcObject = static_cast<Base::VectorPy*>(value);
                Base::Vector3d* val = pcObject->getVectorPtr();


			    current_node.Set(float(val->x),float(val->y),float(val->z));
			    vertices.push_back(current_node);
            }
		}

		MeshCore::MeshFacet aFacet;
		aFacet._aulPoints[0] = 0;aFacet._aulPoints[1] = 1;aFacet._aulPoints[2] = 2;
		faces.push_back(aFacet);
		//Fill the Kernel with the temp smesh structure and delete the current containers
		aMesh.Adopt(vertices,faces);
		MeshCore::MeshEigensystem pca(aMesh);
		pca.Evaluate();
		Base::Matrix4D Trafo = pca.Transform();

        return new Base::PlacementPy(new Base::Placement(Trafo) );

	} PY_CATCH;

	Py_Return;
}


PyDoc_STRVAR(open_doc,
"open(string) -- Create a new document and a Mesh::Import feature to load the file into the document.");

PyDoc_STRVAR(inst_doc,
"insert(string|mesh,[string]) -- Load or insert a mesh into the given or active document.");

PyDoc_STRVAR(export_doc,
"export(list,string) -- Export a list of objects into a single file.");

PyDoc_STRVAR(calculateEigenTransform_doc,
"calculateEigenTransform(seq(Base.Vector)) -- Calculates the eigen Transformation from a list of points.\n"
"calculate the point's local coordinate system with the center\n"
"of gravity as origin. The local coordinate system is computed\n"
"this way that u has minimum and w has maximum expansion.\n"
"The local coordinate system is right-handed.\n"
);

/* List of functions defined in the module */

struct PyMethodDef Mesh_Import_methods[] = { 
    {"open"       ,open ,       METH_VARARGS, open_doc},
    {"insert"     ,importer,    METH_VARARGS, inst_doc},
    {"export"     ,exporter,    METH_VARARGS, export_doc},
    {"read"       ,read,        Py_NEWARGS,   "Read a mesh from a file and returns a Mesh object."},
    {"show"       ,show,        Py_NEWARGS,   "Put a mesh object in the active document or creates one if needed"},
    {"createBox"  ,createBox,   Py_NEWARGS,   "Create a solid mesh box"},
    {"createPlane",createPlane, Py_NEWARGS,   "Create a mesh XY plane normal +Z"},
    {"createSphere",createSphere, Py_NEWARGS,   "Create a tessellated sphere"},
    {"createEllipsoid",createEllipsoid, Py_NEWARGS,   "Create a tessellated ellipsoid"},
    {"createCylinder",createCylinder, Py_NEWARGS,   "Create a tessellated cylinder"},
    {"createCone",createCone, Py_NEWARGS,   "Create a tessellated cone"},
    {"createTorus",createTorus, Py_NEWARGS,   "Create a tessellated torus"},
    {"calculateEigenTransform",calculateEigenTransform, METH_VARARGS,   calculateEigenTransform_doc},
    {NULL, NULL}  /* sentinel */
};
