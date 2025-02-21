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

#include "PropertyIntegerList.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyIntegerList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyIntegerList::PropertyIntegerList() = default;

PropertyIntegerList::~PropertyIntegerList() = default;

//**************************************************************************
// Base class implementer

PyObject* PropertyIntegerList::getPyObject()
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++) {
        PyList_SetItem(list, i, PyLong_FromLong(_lValueList[i]));
    }
    return list;
}

long PropertyIntegerList::getPyValue(PyObject* item) const
{
    if (PyLong_Check(item)) {
        return PyLong_AsLong(item);
    }
    std::string error = std::string("type in list must be int, not ");
    error += item->ob_type->tp_name;
    throw Base::TypeError(error);
}

void PropertyIntegerList::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<IntegerList count=\"" << getSize() << "\">" << std::endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        writer.Stream() << writer.ind() << "<I v=\"" << _lValueList[i] << "\"/>" << std::endl;
    };
    writer.decInd();
    writer.Stream() << writer.ind() << "</IntegerList>" << std::endl;
}

void PropertyIntegerList::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("IntegerList");
    // get the value of my Attribute
    int count = reader.getAttributeAsInteger("count");

    std::vector<long> values(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("I");
        values[i] = reader.getAttributeAsInteger("v");
    }

    reader.readEndElement("IntegerList");

    // assignment
    setValues(values);
}

Property* PropertyIntegerList::Copy() const
{
    PropertyIntegerList* p = new PropertyIntegerList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyIntegerList::Paste(const Property& from)
{
    setValues(dynamic_cast<const PropertyIntegerList&>(from)._lValueList);
}

unsigned int PropertyIntegerList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(long));
}

}  // namespace App
