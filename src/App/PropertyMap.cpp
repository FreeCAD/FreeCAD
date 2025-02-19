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

#include "PropertyMap.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyMap, App::Property)

PropertyMap::PropertyMap() = default;

PropertyMap::~PropertyMap() = default;

//**************************************************************************
// Base class implementer


int PropertyMap::getSize() const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyMap::setValue(const std::string& key, const std::string& value)
{
    aboutToSetValue();
    _lValueList[key] = value;
    hasSetValue();
}

void PropertyMap::setValues(const std::map<std::string, std::string>& map)
{
    aboutToSetValue();
    _lValueList = map;
    hasSetValue();
}

const std::string& PropertyMap::operator[](const std::string& key) const
{
    static std::string empty;
    auto it = _lValueList.find(key);
    if (it != _lValueList.end()) {
        return it->second;
    }
    return empty;
}

PyObject* PropertyMap::getPyObject()
{
    PyObject* dict = PyDict_New();

    for (auto it = _lValueList.begin(); it != _lValueList.end(); ++it) {
        PyObject* item = PyUnicode_DecodeUTF8(it->second.c_str(), it->second.size(), nullptr);
        if (!item) {
            Py_DECREF(dict);
            throw Base::UnicodeError("UTF8 conversion failure at PropertyMap::getPyObject()");
        }
        PyDict_SetItemString(dict, it->first.c_str(), item);
        Py_DECREF(item);
    }

    return dict;
}

void PropertyMap::setPyObject(PyObject* value)
{
    if (PyMapping_Check(value)) {
        std::map<std::string, std::string> values;
        // get key and item list
        PyObject* keyList = PyMapping_Keys(value);
        PyObject* itemList = PyMapping_Values(value);
        Py_ssize_t nSize = PyList_Size(keyList);

        for (Py_ssize_t i = 0; i < nSize; ++i) {
            // check on the key:
            std::string keyStr;
            PyObject* key = PyList_GetItem(keyList, i);
            if (PyUnicode_Check(key)) {
                keyStr = PyUnicode_AsUTF8(key);
            }
            else {
                std::string error("type of the key need to be string, not ");
                error += key->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            // check on the item:
            PyObject* item = PyList_GetItem(itemList, i);
            if (PyUnicode_Check(item)) {
                values[keyStr] = PyUnicode_AsUTF8(item);
            }
            else {
                std::string error("type in values must be string, not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }
        }

        Py_XDECREF(itemList);
        Py_XDECREF(keyList);

        setValues(values);
    }
    else {
        std::string error("type must be a dict or object with mapping protocol, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

unsigned int PropertyMap::getMemSize() const
{
    size_t size = 0;
    for (const auto& it : _lValueList) {
        size += it.second.size();
        size += it.first.size();
    }
    return size;
}

void PropertyMap::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<Map count=\"" << getSize() << "\">" << std::endl;
    writer.incInd();
    for (const auto& it : _lValueList) {
        writer.Stream() << writer.ind() << "<Item key=\"" << encodeAttribute(it.first)
                        << "\" value=\"" << encodeAttribute(it.second) << "\"/>" << std::endl;
    }

    writer.decInd();
    writer.Stream() << writer.ind() << "</Map>" << std::endl;
}

void PropertyMap::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("Map");
    // get the value of my Attribute
    int count = reader.getAttributeAsInteger("count");

    std::map<std::string, std::string> values;
    for (int i = 0; i < count; i++) {
        reader.readElement("Item");
        values[reader.getAttribute("key")] = reader.getAttribute("value");
    }

    reader.readEndElement("Map");

    // assignment
    setValues(values);
}

Property* PropertyMap::Copy() const
{
    PropertyMap* p = new PropertyMap();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyMap::Paste(const Property& from)
{
    aboutToSetValue();
    _lValueList = dynamic_cast<const PropertyMap&>(from)._lValueList;
    hasSetValue();
}

}  // namespace App
