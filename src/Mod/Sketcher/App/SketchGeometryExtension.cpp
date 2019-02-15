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

#include <Mod/Sketcher/App/SketchGeometryExtensionPy.h>

#include "SketchGeometryExtension.h"

using namespace Sketcher;

//---------- Geometry Extension

TYPESYSTEM_SOURCE(Sketcher::SketchGeometryExtension,Part::GeometryExtension)

// scoped within the class, multithread ready
std::atomic<long> SketchGeometryExtension::_GeometryID;

SketchGeometryExtension::SketchGeometryExtension():Id(++SketchGeometryExtension::_GeometryID)
{

}

SketchGeometryExtension::SketchGeometryExtension(long cid):Id(cid)
{

}

// Persistence implementer
unsigned int SketchGeometryExtension::getMemSize (void) const
{
    return sizeof(long int);
}

void SketchGeometryExtension::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<GeoExtension type=\"" << this->getTypeId().getName();

    const std::string name = getName();

    if(name.size() > 0)
        writer.Stream() << "\" name=\"" << name;

    writer.Stream() << "\" id=\"" << Id << "\"/>" << std::endl;
}

void SketchGeometryExtension::Restore(Base::XMLReader &reader)
{
    restoreNameAttribute(reader);

    Id = reader.getAttributeAsInteger("id");
}

std::unique_ptr<Part::GeometryExtension> SketchGeometryExtension::copy(void) const
{
    std::unique_ptr<SketchGeometryExtension> cpy = std::make_unique<SketchGeometryExtension>();

    cpy->Id = this->Id;

    cpy->setName(this->getName()); // Base Class

    return std::move(cpy);
}

PyObject * SketchGeometryExtension::getPyObject(void)
{
    return new SketchGeometryExtensionPy(new SketchGeometryExtension(*this));
}

