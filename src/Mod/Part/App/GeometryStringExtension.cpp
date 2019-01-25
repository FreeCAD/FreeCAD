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

#include "GeometryStringExtension.h"

#include "GeometryStringExtensionPy.h"

using namespace Part;

//---------- Geometry Extension

TYPESYSTEM_SOURCE(Part::GeometryStringExtension,Part::GeometryExtension)

GeometryStringExtension::GeometryStringExtension()
{

}

GeometryStringExtension::GeometryStringExtension(std::string strp):str(strp)
{

}

GeometryStringExtension::~GeometryStringExtension()
{
}

// Persistence implementer
unsigned int GeometryStringExtension::getMemSize (void) const
{
    return 1;
}

void GeometryStringExtension::Save(Base::Writer &writer) const
{

    writer.Stream() << writer.ind() << "<GeoExtension type=\"" << this->getTypeId().getName()
    << "\" value=\"" << str << "\"/>" << std::endl;
}

void GeometryStringExtension::Restore(Base::XMLReader &reader)
{
    str = reader.getAttribute("value");
}

std::unique_ptr<Part::GeometryExtension> GeometryStringExtension::copy(void) const
{
    std::unique_ptr<GeometryStringExtension> cpy = std::make_unique<GeometryStringExtension>();

    cpy->str = this->str;

    return std::move(cpy);
}

PyObject * GeometryStringExtension::getPyObject(void)
{
    return new GeometryStringExtensionPy(new GeometryStringExtension(this->str));
}
