/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

// inclusion of the generated files (generated out of FeaturePythonPy.xml)
#include "FeaturePythonPy.h"
#include "FeaturePythonPy.cpp"

using namespace Mesh;

// returns a string which represent the object e.g. when printed in python
std::string FeaturePythonPy::representation(void) const
{
    return std::string("<Python feature object>");
}

PyObject*  FeaturePythonPy::addProperty(PyObject *args)
{
    char *sType,*sName=0,*sGroup=0,*sDoc=0;
    short attr=0;
    PyObject *ro = Py_False, *hd = Py_False;
    if (!PyArg_ParseTuple(args, "s|ssshO!O!", &sType,&sName,&sGroup,&sDoc,&attr,
        &PyBool_Type, &ro, &PyBool_Type, &hd))     // convert args: Python->C
        return NULL;                             // NULL triggers exception 

    App::Property* prop=0;
    prop = getFeaturePtr()->addDynamicProperty(sType,sName,sGroup,sDoc,attr,ro==Py_True,hd==Py_True);
    
    if (!prop) {
        std::stringstream str;
        str << "No property found of type '" << sType << "'" << std::ends;
        throw Py::Exception(PyExc_Exception,str.str());
    }

    return Py::new_reference_to(this);
}

PyObject*  FeaturePythonPy::supportedProperties(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception
    
    std::vector<Base::Type> ary;
    Base::Type::getAllDerivedFrom(App::Property::getClassTypeId(), ary);
    Py::List res;
    for (std::vector<Base::Type>::iterator it = ary.begin(); it != ary.end(); ++it) {
        Base::BaseClass *data = static_cast<Base::BaseClass*>(it->createInstance());
        if (data) {
            delete data;
            res.append(Py::String(it->getName()));
        }
    }
    return Py::new_reference_to(res);
}

PyObject *FeaturePythonPy::getCustomAttributes(const char* attr) const
{
    PY_TRY{
        if (Base::streq(attr, "__dict__")){
            PyObject* dict = DocumentObjectPy::getCustomAttributes(attr);
            if (dict){
                std::vector<std::string> Props = getFeaturePtr()->getDynamicPropertyNames();
                for (std::vector<std::string>::const_iterator it = Props.begin(); it != Props.end(); ++it)
                    PyDict_SetItem(dict, PyString_FromString(it->c_str()), PyString_FromString(""));
            }
            return dict;
        }

        // search for dynamic property
        App::Property* prop = getFeaturePtr()->getDynamicPropertyByName(attr);
        if (prop) return prop->getPyObject();
    } PY_CATCH;

    return 0;
}

int FeaturePythonPy::setCustomAttributes(const char* attr, PyObject *value)
{
    // search for dynamic property
    App::Property* prop = getFeaturePtr()->getDynamicPropertyByName(attr);

    if (!prop)
        return DocumentObjectPy::setCustomAttributes(attr, value);
    else {
        try {
            prop->setPyObject(value);
        } catch (Base::Exception &exc) {
            PyErr_Format(PyExc_AttributeError, "Attribute (Name: %s) error: '%s' ", attr, exc.what());
            return -1;
        } catch (...) {
            PyErr_Format(PyExc_AttributeError, "Unknown error in attribute %s", attr);
            return -1;
        }

        return 1;
    }
}
