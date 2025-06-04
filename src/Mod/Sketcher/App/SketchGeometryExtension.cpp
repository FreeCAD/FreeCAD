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
#include <Mod/Sketcher/App/SketchGeometryExtensionPy.h>

#include "SketchGeometryExtension.h"


using namespace Sketcher;

//---------- Geometry Extension

TYPESYSTEM_SOURCE(Sketcher::SketchGeometryExtension, Part::GeometryMigrationPersistenceExtension)

// scoped within the class, multithread ready
std::atomic<long> SketchGeometryExtension::_GeometryID;

SketchGeometryExtension::SketchGeometryExtension()
    : Id(++SketchGeometryExtension::_GeometryID)
    , InternalGeometryType(InternalType::None)
    , GeometryLayer(0)
{}

SketchGeometryExtension::SketchGeometryExtension(long cid)
    : Id(cid)
    , InternalGeometryType(InternalType::None)
    , GeometryLayer(0)
{}

void SketchGeometryExtension::copyAttributes(Part::GeometryExtension* cpy) const
{
    Part::GeometryPersistenceExtension::copyAttributes(cpy);

    static_cast<SketchGeometryExtension*>(cpy)->Id = this->Id;
    static_cast<SketchGeometryExtension*>(cpy)->InternalGeometryType = this->InternalGeometryType;
    static_cast<SketchGeometryExtension*>(cpy)->GeometryModeFlags = this->GeometryModeFlags;
    static_cast<SketchGeometryExtension*>(cpy)->GeometryLayer = this->GeometryLayer;
}

void SketchGeometryExtension::restoreAttributes(Base::XMLReader& reader)
{
    Part::GeometryPersistenceExtension::restoreAttributes(reader);

    if (reader.hasAttribute("id")) {
        Id = reader.getAttribute<long>("id");
    }

    InternalGeometryType = reader.getAttribute<InternalType::InternalType>("internalGeometryType");

    GeometryModeFlags = GeometryModeFlagType(reader.getAttribute<const char*>("geometryModeFlags"));

    if (reader.hasAttribute("geometryLayer")) {
        GeometryLayer = reader.getAttribute<long>("geometryLayer");
    }
}

void SketchGeometryExtension::saveAttributes(Base::Writer& writer) const
{
    Part::GeometryPersistenceExtension::saveAttributes(writer);

    writer.Stream() << "\" id=\"" << Id << "\" internalGeometryType=\"" << (int)InternalGeometryType
                    << "\" geometryModeFlags=\"" << GeometryModeFlags.to_string()
                    << "\" geometryLayer=\"" << GeometryLayer;
}

void SketchGeometryExtension::preSave(Base::Writer& writer) const
{
    writer.Stream() << " id=\"" << Id << "\"";
}

void SketchGeometryExtension::postSave(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<Construction value=\""
                    << (GeometryModeFlags.test(GeometryMode::Construction) ? 1 : 0) << "\"/>\n";
}

std::unique_ptr<Part::GeometryExtension> SketchGeometryExtension::copy() const
{
    auto cpy = std::make_unique<SketchGeometryExtension>();

    copyAttributes(cpy.get());
    return cpy;
}

PyObject* SketchGeometryExtension::getPyObject()
{
    return new SketchGeometryExtensionPy(new SketchGeometryExtension(*this));
}

bool SketchGeometryExtension::getInternalTypeFromName(std::string str,
                                                      InternalType::InternalType& type)
{
    auto pos = std::find_if(SketchGeometryExtension::internaltype2str.begin(),
                            SketchGeometryExtension::internaltype2str.end(),
                            [str](const char* val) {
                                return strcmp(val, str.c_str()) == 0;
                            });

    if (pos != SketchGeometryExtension::internaltype2str.end()) {
        int index = std::distance(SketchGeometryExtension::internaltype2str.begin(), pos);

        type = static_cast<InternalType::InternalType>(index);
        return true;
    }

    return false;
}

bool SketchGeometryExtension::getGeometryModeFromName(std::string str,
                                                      GeometryMode::GeometryMode& type)
{
    auto pos = std::find_if(SketchGeometryExtension::geometrymode2str.begin(),
                            SketchGeometryExtension::geometrymode2str.end(),
                            [str](const char* val) {
                                return strcmp(val, str.c_str()) == 0;
                            });

    if (pos != SketchGeometryExtension::geometrymode2str.end()) {
        int index = std::distance(SketchGeometryExtension::geometrymode2str.begin(), pos);

        type = static_cast<GeometryMode::GeometryMode>(index);
        return true;
    }

    return false;
}
