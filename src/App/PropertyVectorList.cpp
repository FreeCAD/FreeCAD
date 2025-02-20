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
#include <Base/VectorPy.h>
#include <Base/Writer.h>

#include "PropertyVector.h"
#include "PropertyVectorList.h"

 
namespace App {

TYPESYSTEM_SOURCE(App::PropertyVectorList, App::PropertyLists)

PropertyVectorList::PropertyVectorList() = default;

PropertyVectorList::~PropertyVectorList() = default;

//**************************************************************************
// Base class implementer

void PropertyVectorList::setValue(double x, double y, double z)
{
    setValue(Base::Vector3d(x, y, z));
}

PyObject* PropertyVectorList::getPyObject()
{
    PyObject* list = PyList_New(getSize());

    for (int i = 0; i < getSize(); i++) {
        PyList_SetItem(list, i, new Base::VectorPy(_lValueList[i]));
    }

    return list;
}

Base::Vector3d PropertyVectorList::getPyValue(PyObject* item) const
{
    PropertyVector val;
    val.setPyObject(item);
    return val.getValue();
}

void PropertyVectorList::Save(Base::Writer& writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<VectorList file=\"" << writer.addFile(getName(), this)
                        << "\"/>" << std::endl;
    }
}

void PropertyVectorList::Restore(Base::XMLReader& reader)
{
    reader.readElement("VectorList");
    std::string file(reader.getAttribute("file"));

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), this);
    }
}

void PropertyVectorList::SaveDocFile(Base::Writer& writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    if (!isSinglePrecision()) {
        for (const auto& it : _lValueList) {
            str << it.x << it.y << it.z;
        }
    }
    else {
        for (const auto& it : _lValueList) {
            float x = (float)it.x;
            float y = (float)it.y;
            float z = (float)it.z;
            str << x << y << z;
        }
    }
}

void PropertyVectorList::RestoreDocFile(Base::Reader& reader)
{
    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<Base::Vector3d> values(uCt);
    if (!isSinglePrecision()) {
        for (auto& it : values) {
            str >> it.x >> it.y >> it.z;
        }
    }
    else {
        Base::Vector3f vec;
        for (auto& it : values) {
            str >> vec.x >> vec.y >> vec.z;
            it.Set(vec.x, vec.y, vec.z);
        }
    }
    setValues(values);
}

Property* PropertyVectorList::Copy() const
{
    PropertyVectorList* p = new PropertyVectorList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyVectorList::Paste(const Property& from)
{
    setValues(dynamic_cast<const PropertyVectorList&>(from)._lValueList);
}

unsigned int PropertyVectorList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Base::Vector3d));
}

}  // namespace App
