/***************************************************************************
 *   Copyright (c) 2019 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com      *
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
# include <sstream>
#endif

#include "OCCError.h"
#include "TrimmedCurvePy.h"
#include "TrimmedCurvePy.cpp"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string TrimmedCurvePy::representation() const
{
    return "<Curve object>";
}

PyObject *TrimmedCurvePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    PyErr_SetString(PyExc_RuntimeError,
                    "You cannot create an instance of the abstract class 'TrimmedCurve'.");
    return nullptr;
}

// constructor method
int TrimmedCurvePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* TrimmedCurvePy::setParameterRange(PyObject * args)
{
    Handle(Geom_Geometry) g = getGeomTrimmedCurvePtr()->handle();
    Handle(Geom_TrimmedCurve) c = Handle(Geom_TrimmedCurve)::DownCast(g);
    try {
        if (!c.IsNull()) {
            double u,v;
            u=c->FirstParameter();
            v=c->LastParameter();
            if (!PyArg_ParseTuple(args, "|dd", &u,&v))
                return nullptr;
            getGeomTrimmedCurvePtr()->setRange(u,v);
            Py_Return;
        }
    }
    catch (Base::CADKernelError& e) {
        PyErr_SetString(PartExceptionOCCError, e.what());
        return nullptr;
    }

    PyErr_SetString(PartExceptionOCCError, "Geometry is not a trimmed curve");
    return nullptr;
}

PyObject *TrimmedCurvePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int TrimmedCurvePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
