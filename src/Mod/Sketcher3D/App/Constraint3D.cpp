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


#include "Constraint3D.h"

#include <cstring>
#include <sstream>
#include <vector>

#include <Base/Exception.h>
#include <Base/Writer.h>

using namespace Sketcher3D;

TYPESYSTEM_SOURCE(Sketcher3D::Constraint3D, Base::Persistence)

namespace
{
std::vector<int> parseIntegerList(const char* text)
{
    std::vector<int> values;
    if (text == nullptr || *text == '\0') {
        return values;
    }

    std::istringstream stream(text);
    int value = 0;
    while (stream >> value) {
        values.push_back(value);
    }

    return values;
}
}  // namespace

const char* Constraint3D::typeToString(Constraint3DType t)
{
    return type2str[t];
}

unsigned int Constraint3D::getMemSize() const
{
    return static_cast<unsigned int>(sizeof(*this) + elements.capacity() * sizeof(GeoElementId3D));
}

void Constraint3D::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<Constraint3D"
                    << " Type=\"" << static_cast<int>(Type) << "\""
                    << " Value=\"" << Value << "\""
                    << " IsDriving=\"" << static_cast<int>(isDriving) << "\" ";

    if (!elements.empty()) {

        writer.Stream() << "ElementIds=\"";
        for (size_t i = 0; i < elements.size(); ++i) {
            if (i != 0) {
                writer.Stream() << " ";
            }
            writer.Stream() << elements[i].GeoId;
        }
        writer.Stream() << "\" ";

        writer.Stream() << "ElementPositions=\"";
        for (size_t i = 0; i < elements.size(); ++i) {
            if (i != 0) {
                writer.Stream() << " ";
            }
            writer.Stream() << elements[i].posIdAsInt();
        }
        writer.Stream() << "\" ";
    }

    writer.Stream() << "/>\n";
}

void Constraint3D::Restore(Base::XMLReader& xmlReader)
{
    xmlReader.readElement("Constraint3D");

    Type = xmlReader.getAttribute<Constraint3DType>("Type");
    Value = xmlReader.getAttribute<double>("Value");
    isDriving = xmlReader.getAttribute<bool>("IsDriving", true);

    elements.clear();

    const bool hasIds = xmlReader.hasAttribute("ElementIds");
    const bool hasPositions = xmlReader.hasAttribute("ElementPositions");
    if (hasIds != hasPositions) {
        throw Base::ParserError(
            "Constraint3D restore requires both ElementIds and ElementPositions attributes"
        );
    }

    if (!hasIds) {
        return;
    }

    const auto ids = parseIntegerList(xmlReader.getAttribute<const char*>("ElementIds", ""));
    const auto positions = parseIntegerList(
        xmlReader.getAttribute<const char*>("ElementPositions", "")
    );

    if (ids.size() != positions.size()) {
        throw Base::ParserError("Constraint3D restore found mismatched element id and position counts");
    }

    elements.reserve(ids.size());
    for (std::size_t i = 0; i < ids.size(); ++i) {
        elements.emplace_back(ids[i], static_cast<PointPos>(positions[i]));
    }
}
