/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#include <CXX/Objects.hxx>
#include "FeatureArea.h"

// inclusion of the generated files (generated out of FeatureAreaPy.xml)
#include "FeatureAreaPy.h"
#include "FeatureAreaPy.cpp"

#include "AreaPy.h"

using namespace Path;


// returns a string which represent the object e.g. when printed in python
std::string FeatureAreaPy::representation(void) const
{
    return std::string("<Path::FeatureArea>");
}


PyObject* FeatureAreaPy::getArea(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    return new AreaPy(new Area(getFeatureAreaPtr()->getArea()));
}

PyObject* FeatureAreaPy::setParams(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {PARAM_FIELD_STRINGS(NAME,AREA_PARAMS_CONF),NULL};

    //Declare variables defined in the NAME field of the CONF parameter list
    PARAM_PY_DECLARE(PARAM_FNAME,AREA_PARAMS_CONF);

    FeatureArea *feature = getFeatureAreaPtr();

#define AREA_SET(_param) \
    PARAM_FNAME(_param) = \
        PARAM_TYPED(PARAM_PY_CAST_,_param)(feature->PARAM_FNAME(_param).getValue());
    //populate the CONF variables with values in properties
    PARAM_FOREACH(AREA_SET,AREA_PARAMS_CONF)

    //Parse arguments to overwrite CONF variables 
    if (!PyArg_ParseTupleAndKeywords(args, keywds, 
                "|" PARAM_PY_KWDS(AREA_PARAMS_CONF), kwlist, 
                PARAM_REF(PARAM_FNAME,AREA_PARAMS_CONF)))
        Py_Error(Base::BaseExceptionFreeCADError, 
            "Wrong parameters, call getParamsDesc() to get supported params");

#define AREA_GET(_param) \
    feature->PARAM_FNAME(_param).setValue(\
        PARAM_TYPED(PARAM_CAST_PY_,_param)(PARAM_FNAME(_param)));
    //populate properties with the CONF variables
    PARAM_FOREACH(AREA_GET,AREA_PARAMS_CONF)

    return Py_None;
}

PyObject *FeatureAreaPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}


int FeatureAreaPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

