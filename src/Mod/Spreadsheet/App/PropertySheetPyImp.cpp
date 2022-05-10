/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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

#include "Mod/Spreadsheet/App/PropertySheet.h"

// inclusion of the generated files (generated out of PropertySheetPy.xml)
#include "PropertySheetPy.h"
#include "PropertySheetPy.cpp"

using namespace Spreadsheet;

// returns a string which represents the object e.g. when printed in python
std::string PropertySheetPy::representation(void) const
{
    return std::string("<PropertySheet object>");
}

PyObject *PropertySheetPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PropertySheetPy and the Twin object 
    return new PropertySheetPy(new PropertySheet);
}

// constructor method
int PropertySheetPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject * PropertySheetPy::mapping_subscript(PyObject * o, PyObject *key)
{
    return static_cast<PropertySheetPy*>(o)->getPropertySheetPtr()->getPyValue(key);
}

PyObject *PropertySheetPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int PropertySheetPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
