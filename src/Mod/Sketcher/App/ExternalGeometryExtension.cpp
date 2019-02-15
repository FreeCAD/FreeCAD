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

#include <Mod/Sketcher/App/ExternalGeometryExtensionPy.h>

#include "ExternalGeometryExtension.h"

using namespace Sketcher;

//---------- Geometry Extension

constexpr std::array<const char *,ExternalGeometryExtension::NumFlags> ExternalGeometryExtension::flag2str;

TYPESYSTEM_SOURCE(Sketcher::ExternalGeometryExtension,Part::GeometryExtension)

// Persistence implementer
unsigned int ExternalGeometryExtension::getMemSize (void) const
{
    return sizeof(long int);
}

void ExternalGeometryExtension::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<GeoExtension type=\"" << this->getTypeId().getName();

    const std::string name = getName();

    if(name.size() > 0)
        writer.Stream() << "\" name=\"" << name;

    writer.Stream() << "\" Ref=\"" << Ref << "\" Flags=\"" << Flags.to_string() << "\"/>" << std::endl;
}

void ExternalGeometryExtension::Restore(Base::XMLReader &reader)
{
    restoreNameAttribute(reader);

    Ref = reader.getAttribute("Ref");
    Flags = FlagType(reader.getAttribute("Flags"));

}

std::unique_ptr<Part::GeometryExtension> ExternalGeometryExtension::copy(void) const
{
    std::unique_ptr<ExternalGeometryExtension> cpy = std::make_unique<ExternalGeometryExtension>();

    cpy->Ref = this->Ref;
    cpy->Flags = this->Flags;

    cpy->setName(this->getName()); // Base Class

    return std::move(cpy);
}

PyObject * ExternalGeometryExtension::getPyObject(void)
{
    return new ExternalGeometryExtensionPy(new ExternalGeometryExtension(*this));
}

