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


#pragma once

#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/MeshGlobal.h>
#include <iosfwd>

namespace Base
{
class InputStream;
}

namespace MeshCore
{

class MeshKernel;
struct Material;

/** Loads the mesh object from data in PLY format. */
class MeshExport ReaderPLY
{
public:
    /*!
     * \brief ReaderPLY
     */
    explicit ReaderPLY(MeshKernel& kernel, Material* = nullptr);
    /*!
     * \brief Load the mesh from the input stream
     * \return true on success and false otherwise
     */
    bool Load(std::istream& input);

private:
    bool CheckHeader(std::istream& input) const;
    bool ReadHeader(std::istream& input);
    bool VerifyVertexProperty();
    bool VerifyColorProperty();
    bool ReadFormat(std::istream& str);
    bool ReadElement(std::istream& str, std::string& element);
    bool ReadProperty(std::istream& str, const std::string& element);
    bool ReadVertexProperty(std::istream& str);
    bool ReadFaceProperty(std::istream& str);
    bool ReadVertexes(std::istream& input);
    bool ReadFaces(std::istream& input);
    bool ReadVertexes(Base::InputStream& is);
    bool ReadFaces(Base::InputStream& is);
    bool LoadAscii(std::istream& input);
    bool LoadBinary(std::istream& input);
    void CleanupMesh();

private:
    enum Property
    {
        coord_x,
        coord_y,
        coord_z,
        color_r,
        color_g,
        color_b,
        generic,
        num_props
    };

    static Property propertyOfName(const std::string& name);
    using PropertyArray = std::array<float, num_props>;
    void addVertexProperty(const PropertyArray& prop);

    enum Number
    {
        int8,
        uint8,
        int16,
        uint16,
        int32,
        uint32,
        float32,
        float64
    };

    struct PropertyComp
    {
        using argument_type_1st = std::pair<Property, int>;
        using argument_type_2nd = Property;
        using result_type = bool;

        // clang-format off
        bool operator()(const argument_type_1st& x,
                        const argument_type_2nd& y) const
        {
            return x.first == y;
        }
        // clang-format on
    };

    enum Format
    {
        unknown,
        ascii,
        binary_little_endian,
        binary_big_endian
    } format = unknown;

    std::vector<std::pair<Property, Number>> vertex_props;
    std::vector<Number> face_props;

    std::size_t v_count = 0;
    std::size_t f_count = 0;
    MeshPointArray meshPoints;
    MeshFacetArray meshFacets;
    MeshKernel& _kernel;
    Material* _material;
};

}  // namespace MeshCore
