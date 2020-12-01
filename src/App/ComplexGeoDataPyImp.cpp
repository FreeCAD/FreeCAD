/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <App/ComplexGeoDataPy.h>
#include <App/ComplexGeoDataPy.cpp>
#include <Base/BoundBoxPy.h>
#include <Base/MatrixPy.h>
#include <Base/PlacementPy.h>
#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

using namespace Data;
using namespace Base;

// returns a string which represent the object e.g. when printed in python
std::string ComplexGeoDataPy::representation(void) const
{
    return std::string("<ComplexGeoData object>");
}

PyObject*  ComplexGeoDataPy::getFacesFromSubelement(PyObject *args)
{
    char *type;
    int index;
    if (!PyArg_ParseTuple(args, "si", &type, &index))
        return 0;

    std::vector<Base::Vector3d> points;
    std::vector<Base::Vector3d> normals;
    std::vector<Data::ComplexGeoData::Facet> facets;
    try {
        Data::Segment* segm = getComplexGeoDataPtr()->getSubElement(type, index);
        getComplexGeoDataPtr()->getFacesFromSubelement(segm, points, normals, facets);
        delete segm;
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "failed to get sub-element from object");
        return 0;
    }

    Py::Tuple tuple(2);
    Py::List vertex;
    for (std::vector<Base::Vector3d>::const_iterator it = points.begin();
        it != points.end(); ++it)
        vertex.append(Py::asObject(new Base::VectorPy(*it)));
    tuple.setItem(0, vertex);
    Py::List facet;
    for (std::vector<Data::ComplexGeoData::Facet>::const_iterator
        it = facets.begin(); it != facets.end(); ++it) {
        Py::Tuple f(3);
        f.setItem(0,Py::Int((int)it->I1));
        f.setItem(1,Py::Int((int)it->I2));
        f.setItem(2,Py::Int((int)it->I3));
        facet.append(f);
    }
    tuple.setItem(1, facet);
    return Py::new_reference_to(tuple);
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

Py::Int ComplexGeoDataPy::getTag() const {
    return Py::Int(getComplexGeoDataPtr()->Tag);
}

void ComplexGeoDataPy::setTag(Py::Int tag) {
    getComplexGeoDataPtr()->Tag = tag;
}

PyObject *ComplexGeoDataPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ComplexGeoDataPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
