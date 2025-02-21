/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Base/PyObjectBase.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "PropertyBoolList.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyBoolList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyBoolList::PropertyBoolList() = default;

PropertyBoolList::~PropertyBoolList() = default;

//**************************************************************************
// Base class implementer

PyObject* PropertyBoolList::getPyObject()
{
    PyObject* tuple = PyTuple_New(getSize());
    for (int i = 0; i < getSize(); i++) {
        bool v = _lValueList[i];
        if (v) {
            PyTuple_SetItem(tuple, i, PyBool_FromLong(1));
        }
        else {
            PyTuple_SetItem(tuple, i, PyBool_FromLong(0));
        }
    }
    return tuple;
}

void PropertyBoolList::setPyObject(PyObject* value)
{
    // string is also a sequence and must be treated differently
    std::string str;
    if (PyUnicode_Check(value)) {
        str = PyUnicode_AsUTF8(value);
        boost::dynamic_bitset<> values(str);
        setValues(values);
    }
    else {
        inherited::setPyObject(value);
    }
}

bool PropertyBoolList::getPyValue(PyObject* item) const
{
    if (PyBool_Check(item)) {
        return Base::asBoolean(item);
    }
    else if (PyLong_Check(item)) {
        return (PyLong_AsLong(item) ? true : false);
    }
    else {
        std::string error = std::string("type in list must be bool or int, not ");
        error += item->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyBoolList::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<BoolList value=\"";
    std::string bitset;
    boost::to_string(_lValueList, bitset);
    writer.Stream() << bitset << "\"/>";
    writer.Stream() << std::endl;
}

void PropertyBoolList::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("BoolList");
    // get the value of my Attribute
    std::string str = reader.getAttribute("value");
    boost::dynamic_bitset<> bitset(str);
    setValues(bitset);
}

Property* PropertyBoolList::Copy() const
{
    PropertyBoolList* p = new PropertyBoolList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyBoolList::Paste(const Property& from)
{
    setValues(dynamic_cast<const PropertyBoolList&>(from)._lValueList);
}

unsigned int PropertyBoolList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size());
}

}  // namespace App
