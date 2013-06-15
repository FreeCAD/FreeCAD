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

#include "BaseClass.h"

// inclusion of the generated files (generated out of BaseClassPy.xml)
#include "BaseClassPy.h"
#include "BaseClassPy.cpp"

using namespace Base;

// returns a string which represent the object e.g. when printed in python
std::string BaseClassPy::representation(void) const
{
    return std::string("<binding object>");
}


PyObject*  BaseClassPy::isDerivedFrom(PyObject *args)
{
    char *name;
    if (!PyArg_ParseTuple(args, "s", &name))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception

    Base::Type type = Base::Type::fromName(name);
    if (type != Base::Type::badType() && getBaseClassPtr()->getTypeId().isDerivedFrom(type)) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

PyObject*  BaseClassPy::getAllDerivedFrom(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))     // convert args: Python->C 
        return NULL;                    // NULL triggers exception
    
    std::vector<Base::Type> ary;
    Base::Type::getAllDerivedFrom(getBaseClassPtr()->getTypeId(), ary);
    Py::List res;
    for (std::vector<Base::Type>::iterator it = ary.begin(); it != ary.end(); ++it)
        res.append(Py::String(it->getName()));
    return Py::new_reference_to(res);
}

Py::String BaseClassPy::getTypeId(void) const
{
    return Py::String(std::string(getBaseClassPtr()->getTypeId().getName()));
}

Py::Int BaseClassPy::getModule(void) const
{
    return Py::Int();
}

PyObject *BaseClassPy::getCustomAttributes(const char* attr) const
{
    // this attribute is marked 'deprecated' but to keep old code working we
    // handle it here. In a future version this will be removed.
    if (strcmp(attr, "Type") == 0) {
        PyErr_SetString(PyExc_DeprecationWarning, "Use 'TypeId' instead");
        PyErr_Print();
        return Py::new_reference_to(Py::String(std::string(getBaseClassPtr()->getTypeId().getName())));
    }
    return 0;
}

int BaseClassPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


