/***************************************************************************
 *   Copyright (c) 2016 Jürgen Riegel <juergen.riegel@web.de>              *
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

// inclusion of the generated files (generated out of GeoFeaturePy.xml)
#include "GeoFeaturePy.h"
#include "GeoFeaturePy.cpp"
#include <Base/PlacementPy.h>
#include <CXX/Objects.hxx>

using namespace App;

// returns a string which represents the object e.g. when printed in python
std::string GeoFeaturePy::representation() const
{
    return {"<GeoFeature object>"};
}

PyObject* GeoFeaturePy::getPaths(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return nullptr;
}

PyObject* GeoFeaturePy::getGlobalPlacement(PyObject * args) {

    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Base::Placement p = static_cast<GeoFeature*>(getDocumentObjectPtr())->globalPlacement();
        return new Base::PlacementPy(new Base::Placement(p));
    }
    catch (const Base::Exception& e) {
        throw Py::RuntimeError(e.what());
    }
}

PyObject* GeoFeaturePy::getPropertyNameOfGeometry(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    GeoFeature* object = this->getGeoFeaturePtr();
    const PropertyComplexGeoData* prop = object->getPropertyOfGeometry();
    const char* name = prop ? prop->getName() : nullptr;
    if (Property::isValidName(name)) {
        return Py::new_reference_to(Py::String(std::string(name)));
    }
    return Py::new_reference_to(Py::None());
}

PyObject* GeoFeaturePy::getPropertyOfGeometry(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    GeoFeature* object = this->getGeoFeaturePtr();
    const PropertyComplexGeoData* prop = object->getPropertyOfGeometry();
    if (prop) {
        return const_cast<PropertyComplexGeoData*>(prop)->getPyObject();
    }
    return Py::new_reference_to(Py::None());
}

PyObject *GeoFeaturePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int GeoFeaturePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
