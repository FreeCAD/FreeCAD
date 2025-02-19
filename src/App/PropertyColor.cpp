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
#include <Base/Stream.h>
#include <Base/Writer.h>

#include "PropertyColor.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyColor, App::Property)

//**************************************************************************
// Construction/Destruction

PropertyColor::PropertyColor() = default;

PropertyColor::~PropertyColor() = default;

//**************************************************************************
// Base class implementer

void PropertyColor::setValue(const Color& col)
{
    aboutToSetValue();
    _cCol = col;
    hasSetValue();
}

void PropertyColor::setValue(uint32_t rgba)
{
    aboutToSetValue();
    _cCol.setPackedValue(rgba);
    hasSetValue();
}

void PropertyColor::setValue(float r, float g, float b, float a)
{
    aboutToSetValue();
    _cCol.set(r, g, b, a);
    hasSetValue();
}

const Color& PropertyColor::getValue() const
{
    return _cCol;
}

PyObject* PropertyColor::getPyObject()
{
    PyObject* rgba = PyTuple_New(4);
    PyObject* r = PyFloat_FromDouble(_cCol.r);
    PyObject* g = PyFloat_FromDouble(_cCol.g);
    PyObject* b = PyFloat_FromDouble(_cCol.b);
    PyObject* a = PyFloat_FromDouble(_cCol.a);

    PyTuple_SetItem(rgba, 0, r);
    PyTuple_SetItem(rgba, 1, g);
    PyTuple_SetItem(rgba, 2, b);
    PyTuple_SetItem(rgba, 3, a);

    return rgba;
}

void PropertyColor::setPyObject(PyObject* value)
{
    App::Color cCol;
    if (PyTuple_Check(value) && (PyTuple_Size(value) == 3 || PyTuple_Size(value) == 4)) {
        PyObject* item;
        item = PyTuple_GetItem(value, 0);
        if (PyFloat_Check(item)) {
            cCol.r = (float)PyFloat_AsDouble(item);
            item = PyTuple_GetItem(value, 1);
            if (PyFloat_Check(item)) {
                cCol.g = (float)PyFloat_AsDouble(item);
            }
            else {
                throw Base::TypeError("Type in tuple must be consistent (float)");
            }
            item = PyTuple_GetItem(value, 2);
            if (PyFloat_Check(item)) {
                cCol.b = (float)PyFloat_AsDouble(item);
            }
            else {
                throw Base::TypeError("Type in tuple must be consistent (float)");
            }
            if (PyTuple_Size(value) == 4) {
                item = PyTuple_GetItem(value, 3);
                if (PyFloat_Check(item)) {
                    cCol.a = (float)PyFloat_AsDouble(item);
                }
                else {
                    throw Base::TypeError("Type in tuple must be consistent (float)");
                }
            }
        }
        else if (PyLong_Check(item)) {
            cCol.r = PyLong_AsLong(item) / 255.0;
            item = PyTuple_GetItem(value, 1);
            if (PyLong_Check(item)) {
                cCol.g = PyLong_AsLong(item) / 255.0;
            }
            else {
                throw Base::TypeError("Type in tuple must be consistent (integer)");
            }
            item = PyTuple_GetItem(value, 2);
            if (PyLong_Check(item)) {
                cCol.b = PyLong_AsLong(item) / 255.0;
            }
            else {
                throw Base::TypeError("Type in tuple must be consistent (integer)");
            }
            if (PyTuple_Size(value) == 4) {
                item = PyTuple_GetItem(value, 3);
                if (PyLong_Check(item)) {
                    cCol.a = PyLong_AsLong(item) / 255.0;
                }
                else {
                    throw Base::TypeError("Type in tuple must be consistent (integer)");
                }
            }
        }
        else {
            throw Base::TypeError("Type in tuple must be float or integer");
        }
    }
    else if (PyLong_Check(value)) {
        cCol.setPackedValue(PyLong_AsUnsignedLong(value));
    }
    else {
        std::string error =
            std::string("type must be integer or tuple of float or tuple integer, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }

    setValue(cCol);
}

void PropertyColor::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<PropertyColor value=\"" << _cCol.getPackedValue() << "\"/>"
                    << std::endl;
}

void PropertyColor::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("PropertyColor");
    // get the value of my Attribute
    unsigned long rgba = reader.getAttributeAsUnsigned("value");
    setValue(rgba);
}

Property* PropertyColor::Copy() const
{
    PropertyColor* p = new PropertyColor();
    p->_cCol = _cCol;
    return p;
}

void PropertyColor::Paste(const Property& from)
{
    aboutToSetValue();
    _cCol = dynamic_cast<const PropertyColor&>(from)._cCol;
    hasSetValue();
}

//**************************************************************************
// PropertyColorList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(App::PropertyColorList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyColorList::PropertyColorList() = default;

PropertyColorList::~PropertyColorList() = default;

//**************************************************************************
// Base class implementer

PyObject* PropertyColorList::getPyObject()
{
    PyObject* list = PyList_New(getSize());

    for (int i = 0; i < getSize(); i++) {
        PyObject* rgba = PyTuple_New(4);
        PyObject* r = PyFloat_FromDouble(_lValueList[i].r);
        PyObject* g = PyFloat_FromDouble(_lValueList[i].g);
        PyObject* b = PyFloat_FromDouble(_lValueList[i].b);
        PyObject* a = PyFloat_FromDouble(_lValueList[i].a);

        PyTuple_SetItem(rgba, 0, r);
        PyTuple_SetItem(rgba, 1, g);
        PyTuple_SetItem(rgba, 2, b);
        PyTuple_SetItem(rgba, 3, a);

        PyList_SetItem(list, i, rgba);
    }

    return list;
}

Color PropertyColorList::getPyValue(PyObject* item) const
{
    PropertyColor col;
    col.setPyObject(item);
    return col.getValue();
}

void PropertyColorList::Save(Base::Writer& writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<ColorList file=\""
                        << (getSize() ? writer.addFile(getName(), this) : "") << "\"/>"
                        << std::endl;
    }
}

void PropertyColorList::Restore(Base::XMLReader& reader)
{
    reader.readElement("ColorList");
    if (reader.hasAttribute("file")) {
        std::string file(reader.getAttribute("file"));

        if (!file.empty()) {
            // initiate a file read
            reader.addFile(file.c_str(), this);
        }
    }
}

void PropertyColorList::SaveDocFile(Base::Writer& writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    for (auto it : _lValueList) {
        str << it.getPackedValue();
    }
}

void PropertyColorList::RestoreDocFile(Base::Reader& reader)
{
    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<Color> values(uCt);
    uint32_t value;  // must be 32 bit long
    for (auto& it : values) {
        str >> value;
        it.setPackedValue(value);
    }
    setValues(values);
}

Property* PropertyColorList::Copy() const
{
    PropertyColorList* p = new PropertyColorList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyColorList::Paste(const Property& from)
{
    setValues(dynamic_cast<const PropertyColorList&>(from)._lValueList);
}

unsigned int PropertyColorList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Color));
}

}  // namespace App
