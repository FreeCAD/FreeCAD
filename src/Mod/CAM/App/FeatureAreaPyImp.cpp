/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Mod/Part/App/TopoShapePy.h>

// inclusion of the generated files (generated out of FeatureAreaPy.xml)
#include "FeatureAreaPy.h"
#include "FeatureAreaPy.cpp"

#include "AreaPy.h"


using namespace Path;

// returns a string which represent the object e.g. when printed in python
std::string FeatureAreaPy::representation() const
{
    return std::string("<Path::FeatureArea>");
}


PyObject* FeatureAreaPy::getArea(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    return new AreaPy(new Area(getFeatureAreaPtr()->getArea()));
}

PyObject* FeatureAreaPy::setParams(PyObject *args, PyObject *keywds)
{
    static const std::array<const char *, 43> kwlist {PARAM_FIELD_STRINGS(NAME,AREA_PARAMS_CONF),nullptr};

    //Declare variables defined in the NAME field of the CONF parameter list
    PARAM_PY_DECLARE(PARAM_FNAME,AREA_PARAMS_CONF);

    FeatureArea *feature = getFeatureAreaPtr();

#define AREA_SET(_param) \
    PARAM_FNAME(_param) = \
        PARAM_TYPED(PARAM_PY_CAST_,_param)(feature->PARAM_FNAME(_param).getValue());
    //populate the CONF variables with values in properties
    PARAM_FOREACH(AREA_SET,AREA_PARAMS_CONF)

    //Parse arguments to overwrite CONF variables
    if (!Base::Wrapped_ParseTupleAndKeywords(args, keywds,
                "|" PARAM_PY_KWDS(AREA_PARAMS_CONF), kwlist,
                PARAM_REF(PARAM_FNAME,AREA_PARAMS_CONF)))
        return nullptr;

#define AREA_GET(_param) \
    feature->PARAM_FNAME(_param).setValue(\
        PARAM_TYPED(PARAM_CAST_PY_,_param)(PARAM_FNAME(_param)));
    //populate properties with the CONF variables
    PARAM_FOREACH(AREA_GET,AREA_PARAMS_CONF)

    Py_INCREF(Py_None);
    return Py_None;
}

Py::Object FeatureAreaPy::getWorkPlane() const {
    return Part::shape2pyshape(getFeatureAreaPtr()->getArea().getPlane());
}

void FeatureAreaPy::setWorkPlane(Py::Object obj) {
    PyObject* p = obj.ptr();
    if (!PyObject_TypeCheck(p, &(Part::TopoShapePy::Type))) {
        std::string error = std::string("type must be 'TopoShape', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
    getFeatureAreaPtr()->setWorkPlane(
            static_cast<Part::TopoShapePy*>(p)->getTopoShapePtr()->getShape());
}

PyObject *FeatureAreaPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}


int FeatureAreaPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

