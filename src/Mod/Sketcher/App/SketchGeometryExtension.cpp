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
constexpr std::array<const char *, InternalType::NumInternalGeometryType> SketchGeometryExtension::internaltype2str;

TYPESYSTEM_SOURCE(Sketcher::SketchGeometryExtension,Part::GeometryExtension)

// scoped within the class, multithread ready
std::atomic<long> SketchGeometryExtension::_GeometryID;

SketchGeometryExtension::SketchGeometryExtension():Id(++SketchGeometryExtension::_GeometryID),InternalGeometryType(InternalType::None)
{

}

SketchGeometryExtension::SketchGeometryExtension(long cid):Id(cid),InternalGeometryType(InternalType::None)
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

    writer.Stream() << "\" id=\"" << Id
                    << "\" internalGeometryType=\"" << (int) InternalGeometryType << "\"/>" << std::endl;
}

void SketchGeometryExtension::Restore(Base::XMLReader &reader)
{
    restoreNameAttribute(reader);

    Id = reader.getAttributeAsInteger("id");
    InternalGeometryType = (InternalType::InternalType) reader.getAttributeAsInteger("internalGeometryType");
}

std::unique_ptr<Part::GeometryExtension> SketchGeometryExtension::copy(void) const
{
    auto cpy = std::make_unique<SketchGeometryExtension>();

    cpy->Id = this->Id;
    cpy->InternalGeometryType = this->InternalGeometryType;

    cpy->setName(this->getName()); // Base Class

#if defined (__GNUC__) && (__GNUC__ <=4)
    return std::move(cpy);
#else
    return cpy;
#endif
}

PyObject * SketchGeometryExtension::getPyObject(void)
{
    return new SketchGeometryExtensionPy(new SketchGeometryExtension(*this));
}

bool SketchGeometryExtension::getInternalTypeFromName(std::string str, InternalType::InternalType &type)
{
    auto pos = std::find_if(    SketchGeometryExtension::internaltype2str.begin(),
                                SketchGeometryExtension::internaltype2str.end(),
                                [str](const char * val) {
                                    return strcmp(val,str.c_str())==0;}
                                );

    if( pos != SketchGeometryExtension::internaltype2str.end()) {
            int index = std::distance( SketchGeometryExtension::internaltype2str.begin(), pos );

            type = static_cast<InternalType::InternalType>(index);
            return true;
    }

    return false;
}

