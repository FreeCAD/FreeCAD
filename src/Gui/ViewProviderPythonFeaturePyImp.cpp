/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Inventor/nodes/SoNode.h>
#endif

#include "Gui/ViewProviderPythonFeature.h"
#include <Base/Interpreter.h>

// inclusion of the generated files (generated out of ViewProviderPythonFeaturePy.xml)
#include "ViewProviderPythonFeaturePy.h"
#include "ViewProviderPythonFeaturePy.cpp"

using namespace Gui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderPythonFeaturePy::representation(void) const
{
    return "<ViewProviderPythonFeature object>";
}

PyObject* ViewProviderPythonFeaturePy::addDisplayMode(PyObject * args)
{
    char* mode;
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "Os", &obj, &mode))
        return NULL;

    void* ptr = 0;
    try {
        Base::Interpreter().convertSWIGPointerObj("pivy.coin","SoNode *", obj, &ptr, 0);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return 0;
    }

    PY_TRY {
        SoNode* node = reinterpret_cast<SoNode*>(ptr);
        getViewProviderPythonFeaturePtr()->addDisplayMaskMode(node,mode);
        Py_Return;
    } PY_CATCH;
}

PyObject*  ViewProviderPythonFeaturePy::addProperty(PyObject *args)
{
    char *sType,*sName=0,*sGroup=0,*sDoc=0;
    short attr=0;
    PyObject *ro = Py_False, *hd = Py_False;
    if (!PyArg_ParseTuple(args, "s|ssshO!O!", &sType,&sName,&sGroup,&sDoc,&attr,
        &PyBool_Type, &ro, &PyBool_Type, &hd))     // convert args: Python->C
        return NULL;                             // NULL triggers exception 

    App::Property* prop=0;
    prop = getViewProviderPythonFeaturePtr()->addDynamicProperty(sType,sName,sGroup,sDoc,attr,
        PyObject_IsTrue(ro) ? true : false, PyObject_IsTrue(hd) ? true : false);
    
    if (!prop) {
        std::stringstream str;
        str << "No property found of type '" << sType << "'" << std::ends;
        throw Py::Exception(PyExc_Exception,str.str());
    }

    return Py::new_reference_to(this);
}

PyObject*  ViewProviderPythonFeaturePy::removeProperty(PyObject *args)
{
    char *sName;
    if (!PyArg_ParseTuple(args, "s", &sName))
        return NULL;

    bool ok = getViewProviderPythonFeaturePtr()->removeDynamicProperty(sName);
    return Py_BuildValue("O", (ok ? Py_True : Py_False));
}

PyObject*  ViewProviderPythonFeaturePy::supportedProperties(PyObject *args)
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

PyObject *ViewProviderPythonFeaturePy::getCustomAttributes(const char* attr) const
{
    PY_TRY{
        if (Base::streq(attr, "__dict__")){
            PyObject* dict = ViewProviderDocumentObjectPy::getCustomAttributes(attr);
            if (dict){
                std::vector<std::string> Props = getViewProviderPythonFeaturePtr()->getDynamicPropertyNames();
                for (std::vector<std::string>::const_iterator it = Props.begin(); it != Props.end(); ++it)
                    PyDict_SetItem(dict, PyString_FromString(it->c_str()), PyString_FromString(""));
            }
            return dict;
        }

        // search for dynamic property
        App::Property* prop = getViewProviderPythonFeaturePtr()->getDynamicPropertyByName(attr);
        if (prop) return prop->getPyObject();
    } PY_CATCH;

    return 0;
}

int ViewProviderPythonFeaturePy::setCustomAttributes(const char* attr, PyObject *value)
{
    // search for dynamic property
    App::Property* prop = getViewProviderPythonFeaturePtr()->getDynamicPropertyByName(attr);

    if (!prop)
        return ViewProviderDocumentObjectPy::setCustomAttributes(attr, value);
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
