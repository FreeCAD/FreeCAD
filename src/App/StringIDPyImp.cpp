/****************************************************************************
 *   Copyright (c) 2018 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#include "StringHasher.h"

#include "StringIDPy.h"
#include "StringIDPy.cpp"

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string StringIDPy::representation(void) const
{
    return getStringIDPtr()->toString();
}

PyObject* StringIDPy::isSame(PyObject *args)
{
    PyObject *other;
    if (!PyArg_ParseTuple(args, "O!", &StringIDPy::Type, &other)) {     // convert args: Python->C 
        return Py::new_reference_to(Py::False());
    }
    auto otherID = static_cast<StringIDPy*>(other)->getStringIDPtr();
    return Py::new_reference_to(Py::Boolean(getStringIDPtr() == otherID));
}

Py::Int StringIDPy::getValue(void) const {
    return Py::Int(getStringIDPtr()->value());
}

Py::String StringIDPy::getData(void) const {
    auto sid = getStringIDPtr();
    if(sid->isBinary()) 
        return Py::String(sid->data().toBase64().constData());
    return Py::String(sid->data().constData());
}

Py::Boolean StringIDPy::getIsBinary(void) const {
    return Py::Boolean(getStringIDPtr()->isBinary());
}

Py::Boolean StringIDPy::getIsHashed(void) const {
    return Py::Boolean(getStringIDPtr()->isHashed());
}

PyObject *StringIDPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int StringIDPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


