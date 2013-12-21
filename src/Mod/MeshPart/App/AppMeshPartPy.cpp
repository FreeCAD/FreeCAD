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
meshFromShape(PyObject *self, PyObject *args, PyObject* kwds)
{
    try {
        PyObject *shape;

        static char* kwds_maxLength[] = {"Shape", "MaxLength",NULL};
        PyErr_Clear();
        double maxLength=0;
        if (PyArg_ParseTupleAndKeywords(args, kwds, "O!d", kwds_maxLength,
                                        &(Part::TopoShapePy::Type), &shape, &maxLength)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->_Shape);
            mesher.setMethod(MeshPart::Mesher::Mefisto);
            mesher.setMaxLength(maxLength);
            mesher.setRegular(true);
            return new Mesh::MeshPy(mesher.createMesh());
        }

        static char* kwds_maxArea[] = {"Shape", "MaxArea",NULL};
        PyErr_Clear();
        double maxArea=0;
        if (PyArg_ParseTupleAndKeywords(args, kwds, "O!d", kwds_maxArea,
                                        &(Part::TopoShapePy::Type), &shape, &maxArea)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->_Shape);
            mesher.setMethod(MeshPart::Mesher::Mefisto);
            mesher.setMaxArea(maxArea);
            mesher.setRegular(true);
            return new Mesh::MeshPy(mesher.createMesh());
        }

        static char* kwds_localLen[] = {"Shape", "LocalLength",NULL};
        PyErr_Clear();
        double localLen=0;
        if (PyArg_ParseTupleAndKeywords(args, kwds, "O!d", kwds_localLen,
                                        &(Part::TopoShapePy::Type), &shape, &localLen)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->_Shape);
            mesher.setMethod(MeshPart::Mesher::Mefisto);
            mesher.setLocalLength(localLen);
            mesher.setRegular(true);
            return new Mesh::MeshPy(mesher.createMesh());
        }

        static char* kwds_deflection[] = {"Shape", "Deflection",NULL};
        PyErr_Clear();
        double deflection=0;
        if (PyArg_ParseTupleAndKeywords(args, kwds, "O!d", kwds_deflection,
                                        &(Part::TopoShapePy::Type), &shape, &deflection)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->_Shape);
            mesher.setMethod(MeshPart::Mesher::Mefisto);
            mesher.setDeflection(deflection);
            mesher.setRegular(true);
            return new Mesh::MeshPy(mesher.createMesh());
        }

        static char* kwds_minmaxLen[] = {"Shape", "MinLength","MaxLength",NULL};
        PyErr_Clear();
        double minLen=0, maxLen=0;
        if (PyArg_ParseTupleAndKeywords(args, kwds, "O!dd", kwds_minmaxLen,
                                        &(Part::TopoShapePy::Type), &shape, &minLen, &maxLen)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->_Shape);
            mesher.setMethod(MeshPart::Mesher::Mefisto);
            mesher.setMinMaxLengths(minLen, maxLen);
            mesher.setRegular(true);
            return new Mesh::MeshPy(mesher.createMesh());
        }

#if defined (HAVE_NETGEN)
        static char* kwds_fineness[] = {"Shape", "Fineness", "SecondOrder", "Optimize", "AllowQuad",NULL};
        PyErr_Clear();
        int fineness=0, secondOrder=0, optimize=1, allowquad=0;
        if (PyArg_ParseTupleAndKeywords(args, kwds, "O!i|iii", kwds_fineness,
                                        &(Part::TopoShapePy::Type), &shape, &fineness,
                                        &secondOrder, &optimize, &allowquad)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->_Shape);
            mesher.setMethod(MeshPart::Mesher::Netgen);
            mesher.setFineness(fineness);
            mesher.setSecondOrder(secondOrder > 0);
            mesher.setOptimize(optimize > 0);
            mesher.setQuadAllowed(allowquad > 0);
            return new Mesh::MeshPy(mesher.createMesh());
        }

        static char* kwds_user[] = {"Shape", "GrowthRate", "SegPerEdge", "SegPerRadius", "SecondOrder", "Optimize", "AllowQuad",NULL};
        PyErr_Clear();
        double growthRate=0, nbSegPerEdge=0, nbSegPerRadius=0;
        if (PyArg_ParseTupleAndKeywords(args, kwds, "O!|dddiii", kwds_user,
                                        &(Part::TopoShapePy::Type), &shape,
                                        &growthRate, &nbSegPerEdge, &nbSegPerRadius,
                                        &secondOrder, &optimize, &allowquad)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->_Shape);
            mesher.setMethod(MeshPart::Mesher::Netgen);
            mesher.setGrowthRate(growthRate);
            mesher.setNbSegPerEdge(nbSegPerEdge);
            mesher.setNbSegPerRadius(nbSegPerRadius);
            mesher.setSecondOrder(secondOrder > 0);
            mesher.setOptimize(optimize > 0);
            mesher.setQuadAllowed(allowquad > 0);
            return new Mesh::MeshPy(mesher.createMesh());
        }
#endif

        PyErr_Clear();
        if (PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &shape)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->_Shape);
#if defined (HAVE_NETGEN)
            mesher.setMethod(MeshPart::Mesher::Netgen);
#else
            mesher.setMethod(MeshPart::Mesher::Mefisto);
            mesher.setRegular(true);
#endif
            return new Mesh::MeshPy(mesher.createMesh());
        }
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return 0;
    }

    PyErr_SetString(PyExc_Exception,"Wrong arguments");
    return 0;
}

/* registration table  */
struct PyMethodDef MeshPart_methods[] = {
    {"loftOnCurve",loftOnCurve, METH_VARARGS, loft_doc},
    {"wireFromSegment",wireFromSegment, METH_VARARGS,
     "Create wire(s) from boundary of segment"},
    {"meshFromShape",(PyCFunction)meshFromShape, METH_VARARGS|METH_KEYWORDS,
     "Create mesh from shape"},
    {NULL, NULL}        /* end of table marker */
};
