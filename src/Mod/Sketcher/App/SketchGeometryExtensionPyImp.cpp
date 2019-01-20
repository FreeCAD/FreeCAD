/***************************************************************************
 *   Copyright (c) 2019 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

//#ifndef _PreComp_
//# include <gp.hxx>
//#endif

#include <Mod/Part/App/Geometry.h>
#include "SketchObject.h"
#include "SketchGeometryExtensionPy.h"
#include "SketchGeometryExtensionPy.cpp"

using namespace Sketcher;

// returns a string which represents the object e.g. when printed in python
std::string SketchGeometryExtensionPy::representation(void) const
{
    std::stringstream str;
    long id = getSketchGeometryExtensionPtr()->id;
    str << "<SketchGeometryExtension (" << id << ") >";
    return str.str();
}

PyObject *SketchGeometryExtensionPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PointPy and the Twin object
    return new SketchGeometryExtensionPy(new SketchGeometryExtension);
}

// constructor method
int SketchGeometryExtensionPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{

    if (PyArg_ParseTuple(args, "")) {
        // default extension
        return 0;
    }

    PyErr_Clear();
    int Id;
    if (PyArg_ParseTuple(args, "i", &Id)) {
        this->getSketchGeometryExtensionPtr()->id=Id;
        return 0;
    }



    PyErr_SetString(PyExc_TypeError, "SketchGeometryExtension constructor accepts:\n"
        "-- empty parameter list\n"
        "-- int\n");
    return -1;
}

Py::Long SketchGeometryExtensionPy::getId(void) const
{
    return Py::Long(this->getSketchGeometryExtensionPtr()->id);
}

void SketchGeometryExtensionPy::setId(Py::Long Id)
{
    this->getSketchGeometryExtensionPtr()->id=long(Id);
}



PyObject *SketchGeometryExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int SketchGeometryExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
