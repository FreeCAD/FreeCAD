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

#include "PropertyFloatList.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyFloatList, App::PropertyLists)

PropertyFloatList::PropertyFloatList() = default;

PropertyFloatList::~PropertyFloatList() = default;

//**************************************************************************
// Base class implementer

PyObject* PropertyFloatList::getPyObject()
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++) {
        PyList_SetItem(list, i, PyFloat_FromDouble(_lValueList[i]));
    }
    return list;
}

double PropertyFloatList::getPyValue(PyObject* item) const
{
    if (PyFloat_Check(item)) {
        return PyFloat_AsDouble(item);
    }
    else if (PyLong_Check(item)) {
        return static_cast<double>(PyLong_AsLong(item));
    }
    else {
        std::string error = std::string("type in list must be float, not ");
        error += item->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyFloatList::Save(Base::Writer& writer) const
{
    if (writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<FloatList count=\"" << getSize() << "\">" << std::endl;
        writer.incInd();
        for (int i = 0; i < getSize(); i++) {
            writer.Stream() << writer.ind() << "<F v=\"" << _lValueList[i] << "\"/>" << std::endl;
        };
        writer.decInd();
        writer.Stream() << writer.ind() << "</FloatList>" << std::endl;
    }
    else {
        writer.Stream() << writer.ind() << "<FloatList file=\""
                        << (getSize() ? writer.addFile(getName(), this) : "") << "\"/>"
                        << std::endl;
    }
}

void PropertyFloatList::Restore(Base::XMLReader& reader)
{
    reader.readElement("FloatList");
    std::string file(reader.getAttribute("file"));

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), this);
    }
}

void PropertyFloatList::SaveDocFile(Base::Writer& writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    if (!isSinglePrecision()) {
        for (double it : _lValueList) {
            str << it;
        }
    }
    else {
        for (double it : _lValueList) {
            float v = static_cast<float>(it);
            str << v;
        }
    }
}

void PropertyFloatList::RestoreDocFile(Base::Reader& reader)
{
    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<double> values(uCt);
    if (!isSinglePrecision()) {
        for (double& it : values) {
            str >> it;
        }
    }
    else {
        for (double& it : values) {
            float val;
            str >> val;
            it = val;
        }
    }
    setValues(values);
}

Property* PropertyFloatList::Copy() const
{
    PropertyFloatList* p = new PropertyFloatList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyFloatList::Paste(const Property& from)
{
    setValues(dynamic_cast<const PropertyFloatList&>(from)._lValueList);
}

unsigned int PropertyFloatList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(double));
}

} // namespace App
