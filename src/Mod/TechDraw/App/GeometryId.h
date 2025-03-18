/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAW_GEOMETRYID_H
#define TECHDRAW_GEOMETRYID_H

#include <string_view>
#include <cstdlib>

#include <Base/Exception.h>

namespace TechDraw {

enum class Polytope : std::uint8_t {
    Vertex,
    Edge,
    Face
};

struct GeometryId
{
    unsigned int id;
    Polytope type;
    // Cosmetic cosmetic = false;

    GeometryId(std::string_view strId)
    {
        int length;
        if (strId.starts_with("Vertex")) {
            type = Polytope::Vertex;
            length = std::string_view("Vertex").length();
        }
        else if (strId.starts_with("Edge")) {
            type = Polytope::Edge;
            length = std::string_view("Edge").length();
        }
        else if (strId.starts_with("Face")) {
            type = Polytope::Face;
            length = std::string_view("Face").length();
        }
        else {
            throw Base::ValueError("Malformed id");
        }

        if (static_cast<int>(strId.length()) <= length) {
            // No number following Vertex/Edge/Face
            throw Base::ValueError("Malformed id");
        }
        strId.remove_prefix(length);

        if (*strId.begin() == ' ' || *strId.begin() == '-' || *strId.begin() == '+') {
            throw Base::ValueError("Malformed id");
        }
        char* endPtr;
        id = strtoul(strId.data(), &endPtr, 10);
        if (endPtr == strId.begin() || endPtr != strId.end()) {
            throw Base::ValueError("Malformed id");
        }
    }

    GeometryId(int _id, Polytope _type, bool _cosmetic=false) noexcept :
        id(_id),
        type(_type)
        // cosmetic(_cosmetic)
    {}
};

}  // namespace TechDraw

#endif
