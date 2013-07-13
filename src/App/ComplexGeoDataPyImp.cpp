/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
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

#include "ComplexGeoData.h"

// inclusion of the generated files (generated out of ComplexGeoDataPy.xml)
#include "ComplexGeoDataPy.h"
#include "ComplexGeoDataPy.cpp"
#include <Base/BoundBoxPy.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/GeometryPyCXX.h>

using namespace Data;
using namespace Base;

// returns a string which represent the object e.g. when printed in python
std::string ComplexGeoDataPy::representation(void) const
{
    return std::string("<ComplexGeoData object>");
}

Py::Object ComplexGeoDataPy::getBoundBox(void) const
{
    return Py::BoundingBox(getComplexGeoDataPtr()->getBoundBox());
}

Py::Object ComplexGeoDataPy::getPlacement(void) const
{
    return Py::Placement(getComplexGeoDataPtr()->getPlacement());
}

void  ComplexGeoDataPy::setPlacement(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::PlacementPy::Type))) {
        Base::Placement* trf = static_cast<Base::PlacementPy*>(p)->getPlacementPtr();
        getComplexGeoDataPtr()->setPlacement(*trf);
    }
    else {
        std::string error = std::string("type must be 'Placement', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

Py::Object ComplexGeoDataPy::getMatrix(void) const
{
    return Py::Matrix(getComplexGeoDataPtr()->getTransform());
}

// FIXME would be better to call it setTransform() as in all other interfaces...
void  ComplexGeoDataPy::setMatrix(Py::Object arg)
{
    PyObject* p = arg.ptr();
    if (PyObject_TypeCheck(p, &(Base::MatrixPy::Type))) {
        Base::Matrix4D mat = static_cast<Base::MatrixPy*>(p)->value();
        getComplexGeoDataPtr()->setTransform(mat);
    }
    else {
        std::string error = std::string("type must be 'Matrix', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

PyObject *ComplexGeoDataPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ComplexGeoDataPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
