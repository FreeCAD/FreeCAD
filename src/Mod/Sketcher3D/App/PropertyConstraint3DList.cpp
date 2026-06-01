// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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


#include "PropertyConstraint3DList.h"

#include <Base/Reader.h>
#include <Base/Writer.h>

using namespace Sketcher3D;
using namespace App;

TYPESYSTEM_SOURCE(Sketcher3D::PropertyConstraint3DList, App::PropertyLists)

PropertyConstraint3DList::PropertyConstraint3DList() = default;
PropertyConstraint3DList::~PropertyConstraint3DList() = default;

void PropertyConstraint3DList::setSize(int newSize)
{
    aboutToSetValue();
    _constraints.resize(newSize);
    hasSetValue();
}

int PropertyConstraint3DList::getSize() const
{
    return static_cast<int>(_constraints.size());
}

void PropertyConstraint3DList::setValue(const Constraint3D* lValue)
{
    aboutToSetValue();
    _constraints.clear();
    if (lValue) {
        _constraints.push_back(*lValue);
    }
    hasSetValue();
}

void PropertyConstraint3DList::setConstraints(const std::vector<Constraint3D>& constraints)
{
    aboutToSetValue();
    _constraints = constraints;
    hasSetValue();
}

void PropertyConstraint3DList::setConstraintAt(int index, const Constraint3D& constraint)
{
    if (index < 0) {
        aboutToSetValue();
        _constraints.push_back(constraint);
        hasSetValue();
    }
    else if (index < static_cast<int>(_constraints.size())) {
        aboutToSetValue();
        _constraints[index] = constraint;
        hasSetValue();
    }
    else {
        throw Base::IndexError("Index out of bound");
    }
}

void PropertyConstraint3DList::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<Constraints3D count=\"" << getSize() << "\">" << std::endl;
    writer.incInd();
    for (const auto& c : _constraints) {
        c.Save(writer);
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Constraints3D>" << std::endl;
}

void PropertyConstraint3DList::Restore(Base::XMLReader& reader)
{
    std::vector<Constraint3D> list;

    reader.readElement("Constraints3D");
    int count = reader.getAttribute("count", 0);
    list.reserve(count);

    for (int i = 0; i < count; ++i) {
        Constraint3D c;
        c.Restore(reader);
        list.push_back(std::move(c));
    }

    reader.readEndElement("Constraints3D");

    aboutToSetValue();
    _constraints = std::move(list);
    hasSetValue();
}

Property* PropertyConstraint3DList::Copy() const
{
    auto* p = new PropertyConstraint3DList();
    p->_constraints = _constraints;
    return p;
}

void PropertyConstraint3DList::Paste(const Property& from)
{
    if (from.getTypeId() != getTypeId()) {
        throw Base::TypeError("Incompatible property type");
    }
    const auto& other = static_cast<const PropertyConstraint3DList&>(from);
    aboutToSetValue();
    _constraints = other._constraints;
    hasSetValue();
}

unsigned int PropertyConstraint3DList::getMemSize() const
{
    return static_cast<unsigned int>(_constraints.size() * sizeof(Constraint3D));
}
