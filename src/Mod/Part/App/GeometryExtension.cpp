/***************************************************************************
 *   Copyright (c) 2019 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include <Base/Reader.h>
#include <Base/Writer.h>

#include "GeometryExtension.h"
#include "GeometryExtensionPy.h"


using namespace Part;

TYPESYSTEM_SOURCE_ABSTRACT(Part::GeometryExtension,Base::BaseClass)

GeometryExtension::GeometryExtension() = default;

PyObject* GeometryExtension::copyPyObject() const
{
    Py::Tuple tuple;
    Py::Object obj = Py::asObject(const_cast<GeometryExtension*>(this)->getPyObject());
    return static_cast<GeometryExtensionPy *>(obj.ptr())->copy(tuple.ptr());
}

void GeometryExtension::copyAttributes(Part::GeometryExtension * cpy) const
{
    cpy->setName(this->getName()); // Base Class
}

TYPESYSTEM_SOURCE_ABSTRACT(Part::GeometryPersistenceExtension,Part::GeometryExtension)

void GeometryPersistenceExtension::restoreAttributes(Base::XMLReader &reader)
{
    if(reader.hasAttribute("name")) {
        std::string name = reader.getAttribute("name");
        setName(name);
    }
}
void GeometryPersistenceExtension::saveAttributes(Base::Writer &writer) const
{
    const std::string name = getName();

    if(!name.empty())
        writer.Stream() << "\" name=\"" << name;

}

void GeometryPersistenceExtension::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<GeoExtension type=\"" << this->getTypeId().getName();

    saveAttributes(writer);

    writer.Stream() << "\"/>" << std::endl;
}

void GeometryPersistenceExtension::Restore(Base::XMLReader &reader)
{
    restoreAttributes(reader);
}
