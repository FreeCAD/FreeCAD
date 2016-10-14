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

#ifndef _PreComp_
# include <sstream>
#endif

#include "PropertyContainer.h"
#include "Property.h"
#include "Application.h"

// inclution of the generated files (generated out of PropertyContainerPy.xml)
#include "PropertyContainerPy.h"
#include "PropertyContainerPy.cpp"

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string PropertyContainerPy::representation(void) const
{
    return std::string("<property container>");
}

PyObject*  PropertyContainerPy::getPropertyByName(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception
    App::Property* prop = getPropertyContainerPtr()->getPropertyByName(pstr);
    if (prop) {
        return prop->getPyObject();
    }
    else {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return NULL;
    }
}

PyObject*  PropertyContainerPy::getTypeOfProperty(PyObject *args)
{
    Py::List ret;
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Property* prop =  getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return 0;
    }

    short Type =  prop->getType();
    if (Type & Prop_Hidden)
        ret.append(Py::String("Hidden"));
    if (Type & Prop_ReadOnly)
        ret.append(Py::String("ReadOnly"));
    if (Type & Prop_Output)
        ret.append(Py::String("Output"));
    if (Type & Prop_Transient)
        ret.append(Py::String("Transient"));

    return Py::new_reference_to(ret);
}

PyObject*  PropertyContainerPy::getTypeIdOfProperty(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Property* prop =  getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return 0;
    }

    Py::String str(prop->getTypeId().getName());
    return Py::new_reference_to(str);
}

PyObject*  PropertyContainerPy::setEditorMode(PyObject *args)
{
    char* name;
    short type;
    if (PyArg_ParseTuple(args, "sh", &name, &type)) {
        App::Property* prop = getPropertyContainerPtr()->getPropertyByName(name);
        if (!prop) {
            PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", name);
            return 0;
        }

        unsigned long status = prop->getStatus();
        prop->setStatus(Property::ReadOnly,(type & 1) > 0);
        prop->setStatus(Property::Hidden,(type & 2) > 0);

        if (status != prop->getStatus())
            GetApplication().signalChangePropertyEditor(*prop);

        Py_Return;
    }

    PyErr_Clear();
    PyObject *iter;
    if (PyArg_ParseTuple(args, "sO", &name, &iter)) {
        if (PyTuple_Check(iter) || PyList_Check(iter)) {
            Py::Sequence seq(iter);
            App::Property* prop = getPropertyContainerPtr()->getPropertyByName(name);
            if (!prop) {
                PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", name);
                return 0;
            }

            // reset all bits first
            unsigned long status = prop->getStatus();
            prop->setStatus(Property::ReadOnly, false);
            prop->setStatus(Property::Hidden, false);

            for (Py::Sequence::iterator it = seq.begin();it!=seq.end();++it) {
                std::string str = (std::string)Py::String(*it);
                if (str == "ReadOnly")
                    prop->setStatus(Property::ReadOnly, true);
                else if (str == "Hidden")
                    prop->setStatus(Property::Hidden, true);
            }

            if (status != prop->getStatus())
                GetApplication().signalChangePropertyEditor(*prop);

            Py_Return;
        }
    }

    PyErr_SetString(PyExc_TypeError, "First argument must be str, second can be int, list or tuple");
    return 0;
}

PyObject*  PropertyContainerPy::getEditorMode(PyObject *args)
{
    char* name;
    if (!PyArg_ParseTuple(args, "s", &name))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    App::Property* prop = getPropertyContainerPtr()->getPropertyByName(name);
    Py::List ret;
    if (prop) {
        short Type =  prop->getType();
        if ((prop->testStatus(Property::ReadOnly)) || (Type & Prop_ReadOnly))
            ret.append(Py::String("ReadOnly"));
        if ((prop->testStatus(Property::Hidden)) || (Type & Prop_Hidden))
            ret.append(Py::String("Hidden"));
    }
    return Py::new_reference_to(ret);
}

PyObject*  PropertyContainerPy::getGroupOfProperty(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Property* prop = getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return 0;
    }

    const char* Group = getPropertyContainerPtr()->getPropertyGroup(prop);
    if (Group)
        return Py::new_reference_to(Py::String(Group));
    else
        return Py::new_reference_to(Py::String(""));
}

PyObject*  PropertyContainerPy::getDocumentationOfProperty(PyObject *args)
{
    char *pstr;
    if (!PyArg_ParseTuple(args, "s", &pstr))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    Property* prop = getPropertyContainerPtr()->getPropertyByName(pstr);
    if (!prop) {
        PyErr_Format(PyExc_AttributeError, "Property container has no property '%s'", pstr);
        return 0;
    }

    const char* Group = getPropertyContainerPtr()->getPropertyDocumentation(prop);
    if (Group)
        return Py::new_reference_to(Py::String(Group));
    else
        return Py::new_reference_to(Py::String(""));
}

Py::List PropertyContainerPy::getPropertiesList(void) const
{
    Py::List ret;
    std::map<std::string,Property*> Map;

    getPropertyContainerPtr()->getPropertyMap(Map);

    for (std::map<std::string,Property*>::const_iterator It=Map.begin();It!=Map.end();++It)
        ret.append(Py::String(It->first));

    return ret;
}

PyObject *PropertyContainerPy::getCustomAttributes(const char* attr) const
{
    // search in PropertyList
    Property *prop = getPropertyContainerPtr()->getPropertyByName(attr);
    if (prop) {
        PyObject* pyobj = prop->getPyObject();
        if (!pyobj && PyErr_Occurred()) {
            // the Python exception is already set
            throw Py::Exception();
        }
        return pyobj;
    }
    else if (Base::streq(attr, "__dict__")) {
        // get the properties to the C++ PropertyContainer class
        std::map<std::string,App::Property*> Map;
        getPropertyContainerPtr()->getPropertyMap(Map);
        PyObject *dict = PyDict_New();
        if (dict) {
            for ( std::map<std::string,App::Property*>::iterator it = Map.begin(); it != Map.end(); ++it )
                PyDict_SetItem(dict, PyString_FromString(it->first.c_str()), PyString_FromString(""));
            if (PyErr_Occurred()) {
                Py_DECREF(dict);
                dict = NULL;
            }
        }
        return dict;
    }

    return 0;
}

int PropertyContainerPy::setCustomAttributes(const char* attr, PyObject *obj)
{
    // search in PropertyList
    Property *prop = getPropertyContainerPtr()->getPropertyByName(attr);
    if (prop) {
        // Read-only attributes must not be set over its Python interface
        short Type =  getPropertyContainerPtr()->getPropertyType(prop);
        if (Type & Prop_ReadOnly) {
            std::stringstream s;
            s << "Object attribute '" << attr << "' is read-only";
            throw Py::AttributeError(s.str());
        }

        prop->setPyObject(obj);
        return 1;
    }

    return 0;
}
