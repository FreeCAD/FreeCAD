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

#include "ApproxSurface.h"

using namespace Reen;


/* module functions */
static PyObject * approxSurface(PyObject *self, PyObject *args)
{
    PyObject *o;
    int orderU=4,orderV=4;
    int pointsU=6,pointsV=6;
    if (!PyArg_ParseTuple(args, "O|iiii",&o,&orderU,&orderV,&pointsU,&pointsV))
        return NULL;

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

        Reen::BSplineParameterCorrection pc(orderU,orderV,pointsU,pointsV);
        Handle_Geom_BSplineSurface hSurf;

        //pc.EnableSmoothing(true, 0.1f, 0.5f, 0.2f, 0.3f);
        pc.EnableSmoothing(true, 0.1f, 1.0f, 0.0f, 0.0f);
        hSurf = pc.CreateSurface(clPoints, 5, true, 1.0);
        if (!hSurf.IsNull()) {
            return new Part::BSplineSurfacePy(new Part::GeomBSplineSurface(hSurf));
        }

        PyErr_SetString(PyExc_Exception, "Computation of B-Spline surface failed");
        return 0;
    } PY_CATCH;
}

/* registration table  */
struct PyMethodDef ReverseEngineering_methods[] = {
    {"approxSurface"   , approxSurface,  1},
    {NULL, NULL}        /* end of table marker */
};
