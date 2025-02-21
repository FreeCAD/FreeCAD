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

#include "PropertyStringList.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyStringList, App::PropertyLists)

PropertyStringList::PropertyStringList() = default;

PropertyStringList::~PropertyStringList() = default;

//**************************************************************************
// Base class implementer

void PropertyStringList::setValues(const std::list<std::string>& lValue)
{
    std::vector<std::string> vals;
    vals.reserve(lValue.size());
    for (const auto& v : lValue) {
        vals.push_back(v);
    }
    setValues(vals);
}

PyObject* PropertyStringList::getPyObject()
{
    PyObject* list = PyList_New(getSize());

    for (int i = 0; i < getSize(); i++) {
        PyObject* item =
            PyUnicode_DecodeUTF8(_lValueList[i].c_str(), _lValueList[i].size(), nullptr);
        if (!item) {
            Py_DECREF(list);
            throw Base::UnicodeError(
                "UTF8 conversion failure at PropertyStringList::getPyObject()");
        }
        PyList_SetItem(list, i, item);
    }

    return list;
}

std::string PropertyStringList::getPyValue(PyObject* item) const
{
    std::string ret;
    if (PyUnicode_Check(item)) {
        ret = PyUnicode_AsUTF8(item);
    }
    else if (PyBytes_Check(item)) {
        ret = PyBytes_AsString(item);
    }
    else {
        std::string error = std::string("type in list must be str or unicode, not ");
        error += item->ob_type->tp_name;
        throw Base::TypeError(error);
    }
    return ret;
}

unsigned int PropertyStringList::getMemSize() const
{
    size_t size = 0;
    for (int i = 0; i < getSize(); i++) {
        size += _lValueList[i].size();
    }
    return static_cast<unsigned int>(size);
}

void PropertyStringList::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<StringList count=\"" << getSize() << "\">" << std::endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        std::string val = encodeAttribute(_lValueList[i]);
        writer.Stream() << writer.ind() << "<String value=\"" << val << "\"/>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</StringList>" << std::endl;
}

void PropertyStringList::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("StringList");
    // get the value of my Attribute
    int count = reader.getAttributeAsInteger("count");

    std::vector<std::string> values(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("String");
        values[i] = reader.getAttribute("value");
    }

    reader.readEndElement("StringList");

    // assignment
    setValues(values);
}

Property* PropertyStringList::Copy() const
{
    PropertyStringList* p = new PropertyStringList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyStringList::Paste(const Property& from)
{
    setValues(dynamic_cast<const PropertyStringList&>(from)._lValueList);
}

}  // namespace App
