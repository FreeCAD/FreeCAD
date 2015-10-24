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
# include <TColgp_Array1OfPnt.hxx>
# include <Handle_Geom_BSplineSurface.hxx>
#endif

#include <CXX/Objects.hxx>
#include <Base/PyObjectBase.h>
#include <Base/Console.h>

#include <Mod/Part/App/BSplineSurfacePy.h>
#include <Mod/Part/App/OCCError.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshPy.h>
#include <Mod/Points/App/PointsPy.h>

#include "ApproxSurface.h"
#include "SurfaceTriangulation.h"

using namespace Reen;


/* module functions */
static PyObject * approxSurface(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *o;
    // spline parameters
    int uDegree = 3;
    int vDegree = 3;
    int uPoles = 6;
    int vPoles = 6;
    // smoothing
    PyObject* smooth = Py_True;
    double weight = 0.1;
    double grad = 1.0;  //0.5
    double bend = 0.0; //0.2
    // other parameters
    int iteration = 5;
    PyObject* correction = Py_True;
    double factor = 1.0;

    static char* kwds_approx[] = {"Points", "UDegree", "VDegree", "NbUPoles", "NbVPoles",
                                  "Smooth", "Weight", "Grad", "Bend",
                                  "Iterations", "Correction", "PatchFactor", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|iiiiO!dddiO!d",kwds_approx,
                                    &o,&uDegree,&vDegree,&uPoles,&vPoles,
                                    &PyBool_Type,&smooth,&weight,&grad,&bend,
                                    &iteration,&PyBool_Type,&correction,&factor))
        return 0;

    double curvdiv = 1.0 - (grad + bend);
    int uOrder = uDegree + 1;
    int vOrder = vDegree + 1;

    // error checking
    if (grad < 0.0 || grad > 1.0) {
        PyErr_SetString(PyExc_ValueError, "Value of Grad out of range [0,1]");
        return 0;
    }
    if (bend < 0.0 || bend > 1.0) {
        PyErr_SetString(PyExc_ValueError, "Value of Bend out of range [0,1]");
        return 0;
    }
    if (curvdiv < 0.0 || curvdiv > 1.0) {
        PyErr_SetString(PyExc_ValueError, "Sum of Grad and Bend out of range [0,1]");
        return 0;
    }
    if (uDegree < 1 || uOrder > uPoles) {
        PyErr_SetString(PyExc_ValueError, "Value of uDegree out of range [1,NbUPoles-1]");
        return 0;
    }
    if (vDegree < 1 || vOrder > vPoles) {
        PyErr_SetString(PyExc_ValueError, "Value of vDegree out of range [1,NbVPoles-1]");
        return 0;
    }

    PY_TRY {
        Py::Sequence l(o);
        TColgp_Array1OfPnt clPoints(0, l.size()-1);

        int index=0;
        for (Py::Sequence::iterator it = l.begin(); it != l.end(); ++it) {
            Py::Tuple t(*it);
            clPoints(index++) = gp_Pnt(
                (double)Py::Float(t.getItem(0)),
                (double)Py::Float(t.getItem(1)),
                (double)Py::Float(t.getItem(2)));
        }

        Reen::BSplineParameterCorrection pc(uOrder,vOrder,uPoles,vPoles);
        Handle_Geom_BSplineSurface hSurf;

        pc.EnableSmoothing(PyObject_IsTrue(smooth) ? true : false, weight, grad, bend, curvdiv);
        hSurf = pc.CreateSurface(clPoints, iteration, PyObject_IsTrue(correction) ? true : false, factor);
        if (!hSurf.IsNull()) {
            return new Part::BSplineSurfacePy(new Part::GeomBSplineSurface(hSurf));
        }

        PyErr_SetString(Base::BaseExceptionFreeCADError, "Computation of B-Spline surface failed");
        return 0;
    } PY_CATCH_OCC;
}

#if defined(HAVE_PCL_SURFACE)
static PyObject * 
triangulate(PyObject *self, PyObject *args)
{
    PyObject *pcObj;
    double searchRadius;
    double mu=2.5;
    if (!PyArg_ParseTuple(args, "O!d|d", &(Points::PointsPy::Type), &pcObj, &searchRadius, &mu))
        return NULL;

    Points::PointsPy* pPoints = static_cast<Points::PointsPy*>(pcObj);
    Points::PointKernel* points = pPoints->getPointKernelPtr();
    
    Mesh::MeshObject* mesh = new Mesh::MeshObject();
    SurfaceTriangulation tria(*points, *mesh);
    tria.perform(searchRadius, mu);

    return new Mesh::MeshPy(mesh);
}
#endif

/* registration table  */
struct PyMethodDef ReverseEngineering_methods[] = {
    {"approxSurface", (PyCFunction)approxSurface, METH_VARARGS|METH_KEYWORDS,
     "approxSurface(Points=,UDegree=3,VDegree=3,NbUPoles=6,NbVPoles=6,Smooth=True)\n"
     "Weight=0.1,Grad=1.0,Bend=0.0,\n"
     "Iterations=5,Correction=True,PatchFactor=1.0"
    },
#if defined(HAVE_PCL_SURFACE)
    {"triangulate"     , triangulate,  METH_VARARGS},
#endif
    {NULL, NULL}        /* end of table marker */
};

