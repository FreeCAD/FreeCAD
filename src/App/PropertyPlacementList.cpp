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

#include <Base/PlacementPy.h>
#include <Base/PyObjectBase.h>
#include <Base/Reader.h>
#include <Base/Stream.h>
#include <Base/Writer.h>

#include "PropertyPlacement.h"
#include "PropertyPlacementList.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyPlacementList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyPlacementList::PropertyPlacementList() = default;

PropertyPlacementList::~PropertyPlacementList() = default;

//**************************************************************************
// Base class implementer

PyObject* PropertyPlacementList::getPyObject()
{
    PyObject* list = PyList_New(getSize());

    for (int i = 0; i < getSize(); i++) {
        PyList_SetItem(list, i, new Base::PlacementPy(new Base::Placement(_lValueList[i])));
    }

    return list;
}

Base::Placement PropertyPlacementList::getPyValue(PyObject* item) const
{
    PropertyPlacement val;
    val.setPyObject(item);
    return val.getValue();
}

void PropertyPlacementList::Save(Base::Writer& writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<PlacementList file=\""
                        << writer.addFile(getName(), this) << "\"/>" << std::endl;
    }
}

void PropertyPlacementList::Restore(Base::XMLReader& reader)
{
    reader.readElement("PlacementList");
    std::string file(reader.getAttribute("file"));

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), this);
    }
}

void PropertyPlacementList::SaveDocFile(Base::Writer& writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)getSize();
    str << uCt;
    if (!isSinglePrecision()) {
        for (const auto& it : _lValueList) {
            str << it.getPosition().x << it.getPosition().y << it.getPosition().z
                << it.getRotation()[0] << it.getRotation()[1] << it.getRotation()[2]
                << it.getRotation()[3];
        }
    }
    else {
        for (const auto& it : _lValueList) {
            float x = (float)it.getPosition().x;
            float y = (float)it.getPosition().y;
            float z = (float)it.getPosition().z;
            float q0 = (float)it.getRotation()[0];
            float q1 = (float)it.getRotation()[1];
            float q2 = (float)it.getRotation()[2];
            float q3 = (float)it.getRotation()[3];
            str << x << y << z << q0 << q1 << q2 << q3;
        }
    }
}

void PropertyPlacementList::RestoreDocFile(Base::Reader& reader)
{
    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    std::vector<Base::Placement> values(uCt);
    if (!isSinglePrecision()) {
        for (auto& it : values) {
            Base::Vector3d pos;
            double q0, q1, q2, q3;
            str >> pos.x >> pos.y >> pos.z >> q0 >> q1 >> q2 >> q3;
            Base::Rotation rot(q0, q1, q2, q3);
            it.setPosition(pos);
            it.setRotation(rot);
        }
    }
    else {
        float x {}, y {}, z {};
        float q0 {}, q1 {}, q2 {}, q3 {};
        for (auto& it : values) {
            str >> x >> y >> z >> q0 >> q1 >> q2 >> q3;
            Base::Vector3d pos(x, y, z);
            Base::Rotation rot(q0, q1, q2, q3);
            it.setPosition(pos);
            it.setRotation(rot);
        }
    }
    setValues(values);
}

Property* PropertyPlacementList::Copy() const
{
    PropertyPlacementList* p = new PropertyPlacementList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyPlacementList::Paste(const Property& from)
{
    setValues(dynamic_cast<const PropertyPlacementList&>(from)._lValueList);
}

unsigned int PropertyPlacementList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(Base::Vector3d));
}

}  // namespace App
