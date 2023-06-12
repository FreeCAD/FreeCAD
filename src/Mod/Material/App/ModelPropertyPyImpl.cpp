/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <boost/uuid/uuid_io.hpp>
#endif

#include "ModelPropertyPy.h"
#include "ModelPropertyPy.cpp"
#include "Model.h"


using namespace Materials;

// returns a string which represents the object e.g. when printed in python
std::string ModelPropertyPy::representation() const
{
    ModelPropertyPy::PointerType ptr = getModelPropertyPtr();
    std::stringstream str;
    str << "Property [Name=(";
    str << ptr->getName();
    str << "), Type=(";
    str << ptr->getPropertyType();
    str << "), Units=(";
    str << ptr->getUnits();
    str << "), URL=(";
    str << ptr->getURL();
    str << "), Description=(";
    str << ptr->getDescription();
    str << ")]";

    return str.str();
}

PyObject *ModelPropertyPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // never create such objects with the constructor
    return new ModelPropertyPy(new ModelProperty());
}

// constructor method
int ModelPropertyPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::String ModelPropertyPy::getName() const
{
    return Py::String(getModelPropertyPtr()->getName());
}

Py::String ModelPropertyPy::getType() const
{
    return Py::String(getModelPropertyPtr()->getPropertyType());
}

Py::String ModelPropertyPy::getUnits() const
{
    return Py::String(getModelPropertyPtr()->getUnits());
}

Py::String ModelPropertyPy::getURL() const
{
    return Py::String(getModelPropertyPtr()->getURL());
}

Py::String ModelPropertyPy::getDescription() const
{
    return Py::String(getModelPropertyPtr()->getDescription());
}

PyObject *ModelPropertyPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ModelPropertyPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
