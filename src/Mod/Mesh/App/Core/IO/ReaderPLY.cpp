// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <boost/lexical_cast.hpp>
#include <istream>
#endif

#include "Core/MeshIO.h"
#include "Core/MeshKernel.h"
#include <Base/Stream.h>
#include <Base/Tools.h>

#include "ReaderPLY.h"


using namespace MeshCore;

// http://local.wasp.uwa.edu.au/~pbourke/dataformats/ply/
ReaderPLY::ReaderPLY(MeshKernel& kernel, Material* material)
    : _kernel(kernel)
    , _material(material)
{}

bool ReaderPLY::CheckHeader(std::istream& input) const
{
    if (!input || input.bad()) {
        return false;
    }

    std::streambuf* buf = input.rdbuf();
    if (!buf) {
        return false;
    }

    // read in the first three characters
    std::array<char, 3> ply {};
    input.read(ply.data(), ply.size());
    input.ignore(1);
    if (!input) {
        return false;
    }

    return ((ply[0] == 'p') && (ply[1] == 'l') && (ply[2] == 'y'));
}

bool ReaderPLY::ReadFormat(std::istream& str)
{
    std::string format_string;
    std::string version;
    char space_format_string {};
    char space_format_version {};
    str >> space_format_string >> std::ws >> format_string >> space_format_version >> std::ws
        >> version;

    // clang-format off
    if (std::isspace(static_cast<unsigned char>(space_format_string)) == 0 ||
        std::isspace(static_cast<unsigned char>(space_format_version)) == 0) {
        return false;
    }
    // clang-format on

    if (format_string == "ascii") {
        format = ascii;
    }
    else if (format_string == "binary_big_endian") {
        format = binary_big_endian;
    }
    else if (format_string == "binary_little_endian") {
        format = binary_little_endian;
    }
    else {
        // wrong format version
        return false;
    }

    return (version == "1.0");
}

bool ReaderPLY::ReadElement(std::istream& str, std::string& element)
{
    std::string name;
    std::size_t count {};
    char space_element_name {};
    char space_name_count {};
    str >> space_element_name >> std::ws >> name >> space_name_count >> std::ws >> count;

    // clang-format off
    if (std::isspace(static_cast<unsigned char>(space_element_name)) == 0 ||
        std::isspace(static_cast<unsigned char>(space_name_count)) == 0) {
        return false;
    }
    // clang-format on

    if (name == "vertex") {
        element = name;
        v_count = count;
        meshPoints.reserve(count);
    }
    else if (name == "face") {
        element = name;
        f_count = count;
        meshFacets.reserve(count);
    }
    else {
        element.clear();
    }

    return true;
}

ReaderPLY::Property ReaderPLY::propertyOfName(const std::string& name)
{
    if (name == "x") {
        return coord_x;
    }
    if (name == "y") {
        return coord_y;
    }
    if (name == "z") {
        return coord_z;
    }
    if (name == "red" || name == "diffuse_red") {
        return color_r;
    }
    if (name == "green" || name == "diffuse_green") {
        return color_g;
    }
    if (name == "blue" || name == "diffuse_blue") {
        return color_b;
    }

    return generic;
}

bool ReaderPLY::ReadVertexProperty(std::istream& str)
{
    std::string type;
    std::string name;
    char space {};
    str >> space >> std::ws >> type >> space >> std::ws >> name >> std::ws;

    Number number {};
    if (type == "char" || type == "int8") {
        number = int8;
    }
    else if (type == "uchar" || type == "uint8") {
        number = uint8;
    }
    else if (type == "short" || type == "int16") {
        number = int16;
    }
    else if (type == "ushort" || type == "uint16") {
        number = uint16;
    }
    else if (type == "int" || type == "int32") {
        number = int32;
    }
    else if (type == "uint" || type == "uint32") {
        number = uint32;
    }
    else if (type == "float" || type == "float32") {
        number = float32;
    }
    else if (type == "double" || type == "float64") {
        number = float64;
    }
    else {
        // no valid number type
        return false;
    }

    // store the property name and type
    vertex_props.emplace_back(propertyOfName(name), number);

    return true;
}

bool ReaderPLY::ReadFaceProperty(std::istream& str)
{
    std::string type;
    std::string name;
    char space {};

    std::string list;
    std::string uchr;

    str >> space >> std::ws >> list >> std::ws;
    if (list == "list") {
        str >> uchr >> std::ws >> type >> std::ws >> name >> std::ws;
    }
    else {
        // not a 'list'
        type = list;
        str >> name;
    }
    if (name != "vertex_indices" && name != "vertex_index") {
        Number number {};
        if (type == "char" || type == "int8") {
            number = int8;
        }
        else if (type == "uchar" || type == "uint8") {
            number = uint8;
        }
        else if (type == "short" || type == "int16") {
            number = int16;
        }
        else if (type == "ushort" || type == "uint16") {
            number = uint16;
        }
        else if (type == "int" || type == "int32") {
            number = int32;
        }
        else if (type == "uint" || type == "uint32") {
            number = uint32;
        }
        else if (type == "float" || type == "float32") {
            number = float32;
        }
        else if (type == "double" || type == "float64") {
            number = float64;
        }
        else {
            // no valid number type
            return false;
        }

        // store the property name and type
        face_props.push_back(number);
    }
    return true;
}

bool ReaderPLY::ReadProperty(std::istream& str, const std::string& element)
{
    if (element == "vertex") {
        if (!ReadVertexProperty(str)) {
            return false;
        }
    }
    else if (element == "face") {
        if (!ReadFaceProperty(str)) {
            return false;
        }
    }

    return true;
}

bool ReaderPLY::ReadHeader(std::istream& input)
{
    std::string line;
    std::string element;
    while (std::getline(input, line)) {
        std::istringstream str(line);
        str.unsetf(std::ios_base::skipws);
        str >> std::ws;
        if (str.eof()) {
            continue;  // empty line
        }
        std::string kw;
        str >> kw;
        if (kw == "format") {
            if (!ReadFormat(str)) {
                return false;
            }
        }
        else if (kw == "element") {
            if (!ReadElement(str, element)) {
                return false;
            }
        }
        else if (kw == "property") {
            if (!ReadProperty(str, element)) {
                return false;
            }
        }
        else if (kw == "end_header") {
            break;  // end of the header, now read the data
        }
    }

    return true;
}

bool ReaderPLY::VerifyVertexProperty()
{
    // check if valid 3d points
    PropertyComp property;
    std::size_t num_x = std::count_if(vertex_props.begin(),
                                      vertex_props.end(),
                                      [&property](const std::pair<Property, int>& prop) {
                                          return property(prop, coord_x);
                                      });

    std::size_t num_y = std::count_if(vertex_props.begin(),
                                      vertex_props.end(),
                                      [&property](const std::pair<Property, int>& prop) {
                                          return property(prop, coord_y);
                                      });

    std::size_t num_z = std::count_if(vertex_props.begin(),
                                      vertex_props.end(),
                                      [&property](const std::pair<Property, int>& prop) {
                                          return property(prop, coord_z);
                                      });

    return ((num_x == 1) && (num_y == 1) && (num_z == 1));
}

bool ReaderPLY::VerifyColorProperty()
{
    // check if valid colors are set
    PropertyComp property;
    std::size_t num_r = std::count_if(vertex_props.begin(),
                                      vertex_props.end(),
                                      [&property](const std::pair<Property, int>& prop) {
                                          return property(prop, color_r);
                                      });

    std::size_t num_g = std::count_if(vertex_props.begin(),
                                      vertex_props.end(),
                                      [&property](const std::pair<Property, int>& prop) {
                                          return property(prop, color_g);
                                      });

    std::size_t num_b = std::count_if(vertex_props.begin(),
                                      vertex_props.end(),
                                      [&property](const std::pair<Property, int>& prop) {
                                          return property(prop, color_b);
                                      });

    std::size_t rgb_colors = num_r + num_g + num_b;
    if (rgb_colors != 0 && rgb_colors != 3) {
        return false;
    }

    // only if set per vertex
    if (rgb_colors == 3) {
        if (_material) {
            _material->binding = MeshIO::PER_VERTEX;
            _material->diffuseColor.reserve(v_count);
        }
    }

    return true;
}

bool ReaderPLY::Load(std::istream& input)
{
    if (!CheckHeader(input)) {
        return false;
    }

    if (!ReadHeader(input)) {
        return false;
    }

    if (!VerifyVertexProperty()) {
        return false;
    }

    if (!VerifyColorProperty()) {
        return false;
    }

    // clang-format off
    return format == ascii ? LoadAscii(input)
                           : LoadBinary(input);
    // clang-format on
}

void ReaderPLY::CleanupMesh()
{
    _kernel.Clear();  // remove all data before

    MeshCleanup meshCleanup(meshPoints, meshFacets);
    if (_material) {
        meshCleanup.SetMaterial(_material);
    }
    meshCleanup.RemoveInvalids();
    MeshPointFacetAdjacency meshAdj(meshPoints.size(), meshFacets);
    meshAdj.SetFacetNeighbourhood();
    _kernel.Adopt(meshPoints, meshFacets);
}

bool ReaderPLY::ReadVertexes(std::istream& input)
{
    std::string line;
    for (std::size_t i = 0; i < v_count && std::getline(input, line); i++) {
        std::istringstream str(line);
        str.unsetf(std::ios_base::skipws);
        str >> std::ws;

        // go through the vertex properties
        PropertyArray prop_values {};
        std::size_t count_props = vertex_props.size();
        for (const auto& it : vertex_props) {
            switch (it.second) {
                case int8:
                case int16:
                case int32: {
                    int vt {};
                    str >> vt >> std::ws;
                    prop_values[it.first] = static_cast<float>(vt);
                } break;
                case uint8:
                case uint16:
                case uint32: {
                    unsigned int vt {};
                    str >> vt >> std::ws;
                    prop_values[it.first] = static_cast<float>(vt);
                } break;
                case float32: {
                    float vt {};
                    str >> vt >> std::ws;
                    prop_values[it.first] = vt;
                } break;
                case float64: {
                    double vt {};
                    str >> vt >> std::ws;
                    prop_values[it.first] = static_cast<float>(vt);
                } break;
                default:
                    return false;
            }

            // does line contain all properties
            if (--count_props > 0 && str.eof()) {
                return false;
            }
        }

        addVertexProperty(prop_values);
    }

    return true;
}

bool ReaderPLY::ReadFaces(std::istream& input)
{
    constexpr const std::size_t count_props = 4;
    std::string line;
    for (std::size_t i = 0; i < f_count && std::getline(input, line); i++) {
        std::istringstream str(line);
        str.unsetf(std::ios_base::skipws);
        str >> std::ws;

        std::array<int, count_props> v_indices {};
        std::size_t index = count_props;
        for (int& vt : v_indices) {
            str >> vt >> std::ws;
            if (--index > 0 && str.eof()) {
                return false;
            }
        }

        if (v_indices[0] != 3) {
            return false;
        }

        meshFacets.push_back(MeshFacet(v_indices[1], v_indices[2], v_indices[3]));
    }

    return true;
}

bool ReaderPLY::LoadAscii(std::istream& input)
{
    if (!ReadVertexes(input)) {
        return false;
    }

    if (!ReadFaces(input)) {
        return false;
    }

    CleanupMesh();
    return true;
}

void ReaderPLY::addVertexProperty(const PropertyArray& prop)
{
    Base::Vector3f pt;
    pt.x = (prop[coord_x]);
    pt.y = (prop[coord_y]);
    pt.z = (prop[coord_z]);
    meshPoints.push_back(pt);

    if (_material && _material->binding == MeshIO::PER_VERTEX) {
        // NOLINTBEGIN
        float r = (prop[color_r]) / 255.0F;
        float g = (prop[color_g]) / 255.0F;
        float b = (prop[color_b]) / 255.0F;
        // NOLINTEND
        _material->diffuseColor.emplace_back(r, g, b);
    }
}

bool ReaderPLY::ReadVertexes(Base::InputStream& is)
{
    for (std::size_t i = 0; i < v_count; i++) {
        // go through the vertex properties
        PropertyArray prop_values {};
        for (const auto& it : vertex_props) {
            switch (it.second) {
                case int8: {
                    int8_t vt {};
                    is >> vt;
                    prop_values[it.first] = static_cast<float>(vt);
                } break;
                case uint8: {
                    uint8_t vt {};
                    is >> vt;
                    prop_values[it.first] = static_cast<float>(vt);
                } break;
                case int16: {
                    int16_t vt {};
                    is >> vt;
                    prop_values[it.first] = static_cast<float>(vt);
                } break;
                case uint16: {
                    uint16_t vt {};
                    is >> vt;
                    prop_values[it.first] = static_cast<float>(vt);
                } break;
                case int32: {
                    int32_t vt {};
                    is >> vt;
                    prop_values[it.first] = static_cast<float>(vt);
                } break;
                case uint32: {
                    uint32_t vt {};
                    is >> vt;
                    prop_values[it.first] = static_cast<float>(vt);
                } break;
                case float32: {
                    float vt {};
                    is >> vt;
                    prop_values[it.first] = vt;
                } break;
                case float64: {
                    double vt {};
                    is >> vt;
                    prop_values[it.first] = static_cast<float>(vt);
                } break;
                default:
                    return false;
            }
        }

        addVertexProperty(prop_values);
    }

    return true;
}

bool ReaderPLY::ReadFaces(Base::InputStream& is)
{
    unsigned char num {};
    uint32_t f1 {};
    uint32_t f2 {};
    uint32_t f3 {};
    for (std::size_t i = 0; i < f_count; i++) {
        is >> num;
        if (num == 3) {
            is >> f1 >> f2 >> f3;
            if (f1 < v_count && f2 < v_count && f3 < v_count) {
                meshFacets.push_back(MeshFacet(f1, f2, f3));
            }
            for (auto it : face_props) {
                switch (it) {
                    case int8: {
                        int8_t vt {};
                        is >> vt;
                    } break;
                    case uint8: {
                        uint8_t vt {};
                        is >> vt;
                    } break;
                    case int16: {
                        int16_t vt {};
                        is >> vt;
                    } break;
                    case uint16: {
                        uint16_t vt {};
                        is >> vt;
                    } break;
                    case int32: {
                        int32_t vt {};
                        is >> vt;
                    } break;
                    case uint32: {
                        uint32_t vt {};
                        is >> vt;
                    } break;
                    case float32: {
                        unsigned char cnt {};
                        is >> cnt;
                        float vt {};
                        for (unsigned char j = 0; j < cnt; j++) {
                            is >> vt;
                        }
                    } break;
                    case float64: {
                        unsigned char cnt {};
                        is >> cnt;
                        double vt {};
                        for (unsigned char j = 0; j < cnt; j++) {
                            is >> vt;
                        }
                    } break;
                    default:
                        return false;
                }
            }
        }
    }

    return true;
}

bool ReaderPLY::LoadBinary(std::istream& input)
{
    Base::InputStream is(input);
    if (format == binary_little_endian) {
        is.setByteOrder(Base::Stream::LittleEndian);
    }
    else {
        is.setByteOrder(Base::Stream::BigEndian);
    }

    if (!ReadVertexes(is)) {
        return false;
    }

    if (!ReadFaces(is)) {
        return false;
    }

    CleanupMesh();
    return true;
}
