/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
# include <boost/algorithm/string.hpp>
#endif

#include <Base/Exception.h>
#include <Base/PlacementPy.h>
#include <Base/PyWrapParseTupleAndKeywords.h>

// files generated out of CommandPy.xml
#include "CommandPy.h"
#include "CommandPy.cpp"


using namespace Path;

// returns a string which represents the object e.g. when printed in python
std::string CommandPy::representation() const
{
    std::stringstream str;
    str.precision(5);
    str << "Command ";
    str << getCommandPtr()->Name;
    str << " [";
    for(std::map<std::string,double>::iterator i = getCommandPtr()->Parameters.begin(); i != getCommandPtr()->Parameters.end(); ++i) {
        std::string k = i->first;
        double v = i->second;
        str << " " << k << ":" << v;
    }
    str << " ]";
    return str.str();
}

//
// Py::Dict parameters_copy_dict is now a class member to avoid delete/create/copy on every read access from python code
// Now the pre-filled Py::Dict is returned which is more consistent with normal python behaviour.
// It should be cleared whenever the c++ Parameters object is changed eg setParameters() or other objects invalidate its content, eg setPlacement()
// https://forum.freecad.org/viewtopic.php?f=15&t=50583

PyObject *CommandPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of CommandPy and the Twin object
    return new CommandPy(new Command);
}

// constructor method
int CommandPy::PyInit(PyObject* args, PyObject* kwd)
{
    PyObject *parameters = nullptr;
    char *name = "";
    static const std::array<const char *, 3> kwlist {"name", "parameters", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwd, "|sO!", kwlist, &name, &PyDict_Type, &parameters)) {
        std::string sname(name);
        boost::to_upper(sname);
        try {
            if (!sname.empty())
                getCommandPtr()->setFromGCode(name);
        }
        catch (const Base::Exception& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
            return -1;
        }

        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (parameters && PyDict_Next(parameters, &pos, &key, &value)) {
            std::string ckey;
            if (PyUnicode_Check(key)) {
                ckey = PyUnicode_AsUTF8(key);
            }
            else {
                PyErr_SetString(PyExc_TypeError, "The dictionary can only contain string keys");
                return -1;
            }

            boost::to_upper(ckey);
            double cvalue;
            if (PyObject_TypeCheck(value,&(PyLong_Type))) {
                cvalue = (double)PyLong_AsLong(value);
            }
            else if (PyObject_TypeCheck(value, &(PyFloat_Type))) {
                cvalue = PyFloat_AsDouble(value);
            }
            else {
                PyErr_SetString(PyExc_TypeError, "The dictionary can only contain number values");
                return -1;
            }
            getCommandPtr()->Parameters[ckey]=cvalue;
        }
          parameters_copy_dict.clear();
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()

    if (Base::Wrapped_ParseTupleAndKeywords(args, kwd, "|sO!", kwlist, &name, &(Base::PlacementPy::Type),
                                            &parameters)) {
        std::string sname(name);
        boost::to_upper(sname);
        try {
            if (!sname.empty())
                getCommandPtr()->setFromGCode(name);
        }
        catch (const Base::Exception& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
            return -1;
        }
        Base::PlacementPy *p = static_cast<Base::PlacementPy*>(parameters);
        getCommandPtr()->setFromPlacement( *p->getPlacementPtr() );
        return 0;
    }
    return -1;
}

// Name attribute

Py::String CommandPy::getName() const
{
    return Py::String(getCommandPtr()->Name.c_str());
}

void CommandPy::setName(Py::String arg)
{
    std::string cmd = arg.as_std_string();
    boost::to_upper(cmd);
    getCommandPtr()->Name = cmd;
}

// Parameters attribute get/set

Py::Dict CommandPy::getParameters() const
{
    // dict now a class member , https://forum.freecad.org/viewtopic.php?f=15&t=50583
    if (parameters_copy_dict.length()==0) {
      for(std::map<std::string,double>::iterator i = getCommandPtr()->Parameters.begin(); i != getCommandPtr()->Parameters.end(); ++i) {
          parameters_copy_dict.setItem(i->first, Py::Float(i->second));
      }
    }
    return parameters_copy_dict;
}

void CommandPy::setParameters(Py::Dict arg)
{
    PyObject* dict_copy = PyDict_Copy(arg.ptr());
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict_copy, &pos, &key, &value)) {
        std::string ckey;
        if (PyUnicode_Check(key)) {
            ckey = PyUnicode_AsUTF8(key);
        }
        else {
            throw Py::TypeError("The dictionary can only contain string keys");
        }

        boost::to_upper(ckey);
        double cvalue;
        if (PyObject_TypeCheck(value,&(PyLong_Type))) {
            cvalue = (double)PyLong_AsLong(value);
        }
        else if (PyObject_TypeCheck(value, &(PyFloat_Type))) {
            cvalue = PyFloat_AsDouble(value);
        }
        else {
            throw Py::TypeError("The dictionary can only contain number values");
        }
        getCommandPtr()->Parameters[ckey]=cvalue;
        parameters_copy_dict.clear();
    }
}

// GCode methods

PyObject* CommandPy::toGCode(PyObject *args)
{
    if (PyArg_ParseTuple(args, "")) {
        return PyUnicode_FromString(getCommandPtr()->toGCode().c_str());
    }
    throw Py::TypeError("This method accepts no argument");
}

PyObject* CommandPy::setFromGCode(PyObject *args)
{
    char *pstr=nullptr;
    if (PyArg_ParseTuple(args, "s", &pstr)) {
        std::string gcode(pstr);
        try {
            getCommandPtr()->setFromGCode(gcode);
            parameters_copy_dict.clear();
        }
        catch (const Base::Exception& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
            return nullptr;
        }

        Py_INCREF(Py_None);
        return Py_None;
    }
    throw Py::TypeError("Argument must be a string");
}

// Placement attribute get/set

Py::Object CommandPy::getPlacement() const
{
    return Py::asObject(new Base::PlacementPy(new Base::Placement(getCommandPtr()->getPlacement())));
}

void CommandPy::setPlacement(Py::Object arg)
{
    Py::Type PlacementType(Base::getTypeAsObject(&(Base::PlacementPy::Type)));
    if(arg.isType(PlacementType)) {
        getCommandPtr()->setFromPlacement( *static_cast<Base::PlacementPy*>((*arg))->getPlacementPtr() );
        parameters_copy_dict.clear();
    } else
    throw Py::TypeError("Argument must be a placement");
}

PyObject* CommandPy::transform(PyObject *args)
{
    PyObject *placement;
    if ( PyArg_ParseTuple(args, "O!", &(Base::PlacementPy::Type), &placement) ) {
        Base::PlacementPy *p = static_cast<Base::PlacementPy*>(placement);
        Path::Command trCmd = getCommandPtr()->transform( *p->getPlacementPtr() );
        parameters_copy_dict.clear();
        return new CommandPy(new Path::Command(trCmd));
    } else
    throw Py::TypeError("Argument must be a placement");
}

// custom attributes get/set

PyObject *CommandPy::getCustomAttributes(const char* attr) const
{
    std::string satt(attr);
    if (satt.length() == 1) {
        if (isalpha(satt[0])) {
            boost::to_upper(satt);
            if (getCommandPtr()->Parameters.count(satt)) {
                return PyFloat_FromDouble(getCommandPtr()->Parameters[satt]);
            }
            Py_INCREF(Py_None);
            return Py_None;
        }
    }
    return nullptr;
}

int CommandPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    std::string satt(attr);
    if (satt.length() == 1) {
        if (isalpha(satt[0])) {
            boost::to_upper(satt);
            double cvalue;
            if (PyObject_TypeCheck(obj,&(PyLong_Type))) {
                cvalue = (double)PyLong_AsLong(obj);
            } else if (PyObject_TypeCheck(obj,&(PyFloat_Type))) {
                cvalue = PyFloat_AsDouble(obj);
            } else {
                return 0;
            }
            getCommandPtr()->Parameters[satt]=cvalue;
            parameters_copy_dict.clear();
            return 1;
        }
    }
    return 0;
}




