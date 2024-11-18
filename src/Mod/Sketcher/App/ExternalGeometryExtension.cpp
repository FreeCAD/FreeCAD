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

#include "ExternalGeometryExtension.h"
#include "ExternalGeometryExtensionPy.h"


using namespace Sketcher;

//---------- Geometry Extension

constexpr std::array<const char*, ExternalGeometryExtension::NumFlags>
    ExternalGeometryExtension::flag2str;

TYPESYSTEM_SOURCE(Sketcher::ExternalGeometryExtension, Part::GeometryMigrationPersistenceExtension)

void ExternalGeometryExtension::copyAttributes(Part::GeometryExtension* cpy) const
{
    Part::GeometryPersistenceExtension::copyAttributes(cpy);

    static_cast<ExternalGeometryExtension*>(cpy)->Ref = this->Ref;
    static_cast<ExternalGeometryExtension*>(cpy)->RefIndex = this->RefIndex;
    static_cast<ExternalGeometryExtension*>(cpy)->Flags = this->Flags;
}

void ExternalGeometryExtension::restoreAttributes(Base::XMLReader& reader)
{
    Part::GeometryPersistenceExtension::restoreAttributes(reader);

    Ref = reader.getAttribute("Ref", "");
    RefIndex = reader.getAttributeAsInteger("RefIndex", "-1");
    Flags = FlagType(reader.getAttributeAsUnsigned("Flags", "0"));
}

void ExternalGeometryExtension::saveAttributes(Base::Writer& writer) const
{
    Part::GeometryPersistenceExtension::saveAttributes(writer);
    writer.Stream() << "\" Ref=\"" << Base::Persistence::encodeAttribute(Ref);
    writer.Stream() << "\" Flags=\"" << Flags.to_ulong();
    if (RefIndex >= 0) {
        writer.Stream() << "\" RefIndex=\"" << RefIndex;
    }
}

void ExternalGeometryExtension::preSave(Base::Writer& writer) const
{
    if (Ref.size()) {
        writer.Stream() << " ref=\"" << Base::Persistence::encodeAttribute(Ref) << "\"";
    }
    if (RefIndex >= 0) {
        writer.Stream() << " refIndex=\"" << RefIndex << "\"";
    }
    if (Flags.any()) {
        writer.Stream() << " flags=\"" << Flags.to_ulong() << "\"";
    }
}

std::unique_ptr<Part::GeometryExtension> ExternalGeometryExtension::copy() const
{
    auto cpy = std::make_unique<ExternalGeometryExtension>();

    copyAttributes(cpy.get());

#if defined(__GNUC__) && (__GNUC__ <= 4)
    return std::move(cpy);
#else
    return cpy;
#endif
}

PyObject* ExternalGeometryExtension::getPyObject()
{
    return new ExternalGeometryExtensionPy(new ExternalGeometryExtension(*this));
}

bool ExternalGeometryExtension::getFlagsFromName(std::string str,
                                                 ExternalGeometryExtension::Flag& flag)
{
    auto pos = std::find_if(ExternalGeometryExtension::flag2str.begin(),
                            ExternalGeometryExtension::flag2str.end(),
                            [str](const char* val) {
                                return strcmp(val, str.c_str()) == 0;
                            });

    if (pos != ExternalGeometryExtension::flag2str.end()) {
        int index = std::distance(ExternalGeometryExtension::flag2str.begin(), pos);

        flag = static_cast<ExternalGeometryExtension::Flag>(index);
        return true;
    }

    return false;
}
