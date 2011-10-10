/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <BRepBuilderAPI_MakePolygon.hxx>
#endif

#include <Base/PyObjectBase.h>
#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Part/App/TopoShapeWirePy.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshPy.h>
#include "MeshAlgos.h"
#include "Mesher.h"

static PyObject *                        
loftOnCurve(PyObject *self, PyObject *args)
{
    Part::TopoShapePy   *pcObject;
    PyObject *pcTopoObj,*pcListObj;
    float x=0.0f,y=0.0f,z=1.0f,size = 0.1f;

    if (!PyArg_ParseTuple(args, "O!O(fff)f", &(Part::TopoShapePy::Type), &pcTopoObj,&pcListObj,&x,&y,&z,&size))     // convert args: Python->C 
//  if (!PyArg_ParseTuple(args, "O!O!", &(App::TopoShapePy::Type), &pcTopoObj,&PyList_Type,&pcListObj,x,y,z,size))     // convert args: Python->C 
        return NULL;                             // NULL triggers exception 

    pcObject = (Part::TopoShapePy*)pcTopoObj;
    MeshCore::MeshKernel M;

    std::vector<Base::Vector3f> poly;

    if (!PyList_Check(pcListObj))
        Py_Error(PyExc_Exception,"List of Tuble of three or two floats needed as second parameter!");
  
    int nSize = PyList_Size(pcListObj);
    for (int i=0; i<nSize;++i)
    {
        PyObject* item = PyList_GetItem(pcListObj, i);
        if (!PyTuple_Check(item))
            Py_Error(PyExc_Exception,"List of Tuble of three or two floats needed as second parameter!");
        int nTSize = PyTuple_Size(item);
        if(nTSize != 2 && nTSize != 3)
            Py_Error(PyExc_Exception,"List of Tuble of three or two floats needed as second parameter!");

        Base::Vector3f vec(0,0,0);

        for(int l = 0; l < nTSize;l++)
        {
            PyObject* item2 = PyTuple_GetItem(item, l);
            if (!PyFloat_Check(item2))
                Py_Error(PyExc_Exception,"List of Tuble of three or two floats needed as second parameter!");
            vec[l] = (float)PyFloat_AS_DOUBLE(item2);
        }
        poly.push_back(vec);
    }
    
    PY_TRY {
        TopoDS_Shape aShape = pcObject->getTopoShapePtr()->_Shape;
        // use the MeshAlgos 
        MeshPart::MeshAlgos::LoftOnCurve(M,aShape,poly,Base::Vector3f(x,y,z),size);

    } PY_CATCH;

    return new Mesh::MeshPy(new Mesh::MeshObject(M));
}

PyDoc_STRVAR(loft_doc,
"Loft on curve.");

static PyObject *
wireFromSegment(PyObject *self, PyObject *args)
{
    PyObject *o, *m;
    if (!PyArg_ParseTuple(args, "O!O!", &(Mesh::MeshPy::Type), &m,&PyList_Type,&o))
        return 0;
    Py::List list(o);
    Mesh::MeshObject* mesh = static_cast<Mesh::MeshPy*>(m)->getMeshObjectPtr();
    std::vector<unsigned long> segm;
    segm.reserve(list.size());
    for (unsigned int i=0; i<list.size(); i++) {
        segm.push_back((int)Py::Int(list[i]));
    }

    std::list<std::vector<Base::Vector3f> > bounds;
    MeshCore::MeshAlgorithm algo(mesh->getKernel());
    algo.GetFacetBorders(segm, bounds);

    Py::List wires;
    std::list<std::vector<Base::Vector3f> >::iterator bt;

    try {
        for (bt = bounds.begin(); bt != bounds.end(); ++bt) {
            BRepBuilderAPI_MakePolygon mkPoly;
            for (std::vector<Base::Vector3f>::reverse_iterator it = bt->rbegin(); it != bt->rend(); ++it) {
                mkPoly.Add(gp_Pnt(it->x,it->y,it->z));
            }
            if (mkPoly.IsDone()) {
                PyObject* wire = new Part::TopoShapeWirePy(new Part::TopoShape(mkPoly.Wire()));
                wires.append(Py::Object(wire, true));
            }
        }
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }

    return Py::new_reference_to(wires);
}

static PyObject *
meshFromShape(PyObject *self, PyObject *args)
{
    PyObject *shape;
    float maxLength=1.0f/*0.5f*/;
    float maxArea=0/*1.0f*/;
    float localLen=0/*0.1f*/;
    float deflection=0/*0.01f*/;
    if (!PyArg_ParseTuple(args, "O!|ffff", &(Part::TopoShapePy::Type), &shape,
                                           &maxLength,&maxArea,&localLen,&deflection))
        return 0;

    try {
        MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->_Shape);
        mesher.setMaxLength(maxLength);
        mesher.setMaxArea(maxArea);
        mesher.setLocalLength(localLen);
        mesher.setDeflection(deflection);
        mesher.setRegular(true);
        return new Mesh::MeshPy(mesher.createMesh());
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return 0;
    }
}

/* registration table  */
struct PyMethodDef MeshPart_methods[] = {
    {"loftOnCurve",loftOnCurve, METH_VARARGS, loft_doc},
    {"wireFromSegment",wireFromSegment, METH_VARARGS,
     "Create wire(s) from boundary of segment"},
    {"meshFromShape",meshFromShape, METH_VARARGS,
     "Create mesh from shape"},
    {NULL, NULL}        /* end of table marker */
};
