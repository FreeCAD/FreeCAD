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

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Tools.h>

#include "GeometryIntExtension.h"

#include <Mod/Part/App/GeometryIntExtensionPy.h>

using namespace Part;

//---------- Geometry Extension

TYPESYSTEM_SOURCE(Part::GeometryIntExtension,Part::GeometryExtension)

GeometryIntExtension::GeometryIntExtension():id(0)
{

}

GeometryIntExtension::GeometryIntExtension(long cid):id(cid)
{

}

GeometryIntExtension::~GeometryIntExtension()
{
}

// Persistence implementer
unsigned int GeometryIntExtension::getMemSize (void) const
{
    return sizeof(long int);
}

void GeometryIntExtension::Save(Base::Writer &writer) const
{

    writer.Stream() << writer.ind() << "<GeoExtension type=\"" << this->getTypeId().getName()
    << "\" value=\"" << id << "\"/>" << std::endl;
}

void GeometryIntExtension::Restore(Base::XMLReader &reader)
{
    id = reader.getAttributeAsInteger("value");
}

std::unique_ptr<Part::GeometryExtension> GeometryIntExtension::copy(void) const
{
    std::unique_ptr<GeometryIntExtension> cpy = std::make_unique<GeometryIntExtension>();

    cpy->id = this->id;

    return std::move(cpy);
}

PyObject * GeometryIntExtension::getPyObject(void)
{
    return new GeometryIntExtensionPy(new GeometryIntExtension(this->id));
}
