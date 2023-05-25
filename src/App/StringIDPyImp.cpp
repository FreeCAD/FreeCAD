/***************************************************************************
*   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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
std::string StringIDPy::representation() const
{
   return getStringIDPtr()->toString(_index);
}

PyObject* StringIDPy::isSame(PyObject *args)
{
   PyObject *other = nullptr;
   if (PyArg_ParseTuple(args, "O!", &StringIDPy::Type, &other) == 0) { // convert args: Python->C
       return Py::new_reference_to(Py::False());
   }
   auto *otherPy = static_cast<StringIDPy*>(other);
   return Py::new_reference_to(Py::Boolean(
       otherPy->getStringIDPtr() == this->getStringIDPtr()
       && otherPy->_index == this->_index));
}

Py::Int StringIDPy::getValue() const {
   return Py::Int(getStringIDPtr()->value());
}

Py::List StringIDPy::getRelated() const {
   Py::List list;
   for (const auto &id : getStringIDPtr()->relatedIDs()) {
       list.append(Py::Long(id.value()));
}
   return list;
}

Py::String StringIDPy::getData() const {
   return {Py::String(getStringIDPtr()->dataToText(this->_index))};
}

Py::Boolean StringIDPy::getIsBinary() const {
   return {getStringIDPtr()->isBinary()};
}

Py::Boolean StringIDPy::getIsHashed() const {
   return {getStringIDPtr()->isHashed()};
}

Py::Int StringIDPy::getIndex() const {
   return Py::Int(this->_index);
}

void StringIDPy::setIndex(Py::Int index) {
   this->_index = index;
}

PyObject *StringIDPy::getCustomAttributes(const char* /*attr*/) const
{
   return nullptr;
}

int StringIDPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
   return 0;
}
