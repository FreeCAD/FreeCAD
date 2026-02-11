// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <istream>
#include <map>

#include "Core/MeshIO.h"
#include "Core/MeshKernel.h"
#include <Base/Color.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/Tools.h>

#include "ReaderOBJ.h"


using namespace MeshCore;

namespace
{

class ReaderOBJImp
{
public:
    using string_list = std::vector<std::string>;

    explicit ReaderOBJImp(Material* material)
        : _material {material}
    {}

    void Load(const std::string& line)
    {
        if (Ignore(line)) {
            return;
        }

        boost::char_separator<char> sep(" /\t");
        boost::tokenizer<boost::char_separator<char>> tokens(line, sep);
        token_results.assign(tokens.begin(), tokens.end());
        if (token_results.size() < 2) {
            return;
        }

        if (MatchVertex(token_results)) {
            LoadVertex(token_results);
        }
        else if (MatchVertexWithColor(token_results)) {
            LoadVertexWithColor(token_results);
        }
        else if (MatchGroup(token_results)) {
            LoadGroup(token_results);
        }
        else if (MatchLibrary(token_results)) {
            LoadLibrary(token_results);
        }
        else if (MatchUseMaterial(token_results)) {
            LoadUseMaterial(token_results);
        }
        else if (MatchFace(token_results)) {
            LoadFace(token_results);
        }
        else if (MatchQuad(token_results)) {
            LoadQuad(token_results);
        }
    }

    void SetupMaterial()
    {
        // Add the last added material name
        if (!materialName.empty()) {
            _materialNames.emplace_back(materialName, countMaterialFacets);
        }

        // now get back the colors from the vertex property
        if (rgb_value == MeshIO::PER_VERTEX) {
            if (_material) {
                _material->binding = MeshIO::PER_VERTEX;
                _material->diffuseColor.reserve(meshPoints.size());

                for (const auto& it : meshPoints) {
                    unsigned long prop = it._ulProp;
                    Base::Color c;
                    c.setPackedValue(static_cast<uint32_t>(prop));
                    _material->diffuseColor.push_back(c);
                }
            }
        }
        else if (!materialName.empty()) {
            // At this point the materials from the .mtl file are not known and will be read-in by
            // the calling instance but the color list is pre-filled with a default value
            if (_material) {
                _material->binding = MeshIO::PER_FACE;
                const float rgb = 0.8F;
                _material->diffuseColor.resize(meshFacets.size(), Base::Color(rgb, rgb, rgb));
            }
        }
    }

private:
    bool Ignore(const std::string& line) const
    {
        // clang-format off
        return boost::starts_with(line, "vn ") ||
               boost::starts_with(line, "vt ") ||
               boost::starts_with(line, "s ") ||
               boost::starts_with(line, "o ") ||
               boost::starts_with(line, "#");
        // clang-format on
    }

    bool MatchVertex(const string_list& tokens) const
    {
        return tokens[0] == "v" && tokens.size() == 4;
    }

    void LoadVertex(const string_list& tokens)
    {
        float x = std::stof(tokens[1]);
        float y = std::stof(tokens[2]);
        float z = std::stof(tokens[3]);
        meshPoints.push_back(MeshPoint(Base::Vector3f(x, y, z)));
    }

    bool MatchVertexWithColor(const string_list& tokens) const
    {
        return tokens[0] == "v" && tokens.size() == 7;  // NOLINT
    }

    void LoadVertexWithColor(const string_list& tokens)
    {
        LoadVertex(tokens);

        // NOLINTBEGIN
        float r = std::stof(tokens[4]);
        float g = std::stof(tokens[5]);
        float b = std::stof(tokens[6]);
        if (r > 1.0F || g > 1.0F || b > 1.0F) {
            r /= 255.0F;
            g /= 255.0F;
            b /= 255.0F;
        }
        // NOLINTEND

        SetVertexColor(Base::Color(r, g, b));
    }

    void SetVertexColor(const Base::Color& c)
    {
        unsigned long prop = static_cast<uint32_t>(c.getPackedValue());
        meshPoints.back().SetProperty(prop);
        rgb_value = MeshIO::PER_VERTEX;
    }

    bool MatchGroup(const string_list& tokens) const
    {
        return tokens[0] == "g" && tokens.size() == 2;
    }

    void LoadGroup(const string_list& tokens)
    {
        new_segment = true;
        groupName = Base::Tools::escapedUnicodeToUtf8(tokens[1]);
    }

    bool MatchLibrary(const string_list& tokens) const
    {
        return tokens[0] == "mtllib" && tokens.size() == 2;
    }

    void LoadLibrary(const string_list& tokens)
    {
        if (_material) {
            _material->library = Base::Tools::escapedUnicodeToUtf8(tokens[1]);
        }
    }

    bool MatchUseMaterial(const string_list& tokens) const
    {
        return tokens[0] == "usemtl" && tokens.size() == 2;
    }

    void LoadUseMaterial(const string_list& tokens)
    {
        if (!materialName.empty()) {
            _materialNames.emplace_back(materialName, countMaterialFacets);
        }
        materialName = Base::Tools::escapedUnicodeToUtf8(tokens[1]);
        countMaterialFacets = 0;
    }

    bool MatchFace(const string_list& tokens) const
    {
        // NOLINTBEGIN
        const auto num = tokens.size();
        return tokens[0] == "f" && (num == 4 || num == 7 || num == 10);
        // NOLINTEND
    }

    void LoadFace(const string_list& tokens)
    {
        StartNewSegment();

        // NOLINTBEGIN
        int index1 = 1;
        int index2 = 2;
        int index3 = 3;
        if (tokens.size() == 7) {
            index2 = 3;
            index3 = 5;
        }
        else if (tokens.size() == 10) {
            index2 = 4;
            index3 = 7;
        }
        // NOLINTEND

        // 3-vertex face
        int i1 = std::stoi(tokens[index1]);
        i1 = i1 > 0 ? i1 - 1 : i1 + static_cast<int>(meshPoints.size());
        int i2 = std::stoi(tokens[index2]);
        i2 = i2 > 0 ? i2 - 1 : i2 + static_cast<int>(meshPoints.size());
        int i3 = std::stoi(tokens[index3]);
        i3 = i3 > 0 ? i3 - 1 : i3 + static_cast<int>(meshPoints.size());

        AddFace(i1, i2, i3);
    }

    bool MatchQuad(const string_list& tokens) const
    {
        // NOLINTBEGIN
        const auto num = tokens.size();
        return tokens[0] == "f" && (num == 5 || num == 9 || num == 13);
        // NOLINTEND
    }

    void LoadQuad(const string_list& tokens)
    {
        StartNewSegment();

        // NOLINTBEGIN
        int index1 = 1;
        int index2 = 2;
        int index3 = 3;
        int index4 = 4;
        if (tokens.size() == 9) {
            index2 = 3;
            index3 = 5;
            index4 = 7;
        }
        else if (tokens.size() == 13) {
            index2 = 4;
            index3 = 7;
            index4 = 10;
        }
        // NOLINTEND

        // 4-vertex face
        int i1 = std::stoi(tokens[index1]);
        i1 = i1 > 0 ? i1 - 1 : i1 + static_cast<int>(meshPoints.size());
        int i2 = std::stoi(tokens[index2]);
        i2 = i2 > 0 ? i2 - 1 : i2 + static_cast<int>(meshPoints.size());
        int i3 = std::stoi(tokens[index3]);
        i3 = i3 > 0 ? i3 - 1 : i3 + static_cast<int>(meshPoints.size());
        int i4 = std::stoi(tokens[index4]);
        i4 = i4 > 0 ? i4 - 1 : i4 + static_cast<int>(meshPoints.size());

        AddFace(i1, i2, i3);
        AddFace(i3, i4, i1);
    }

    void StartNewSegment()
    {
        // starts a new segment
        if (new_segment) {
            if (!groupName.empty()) {
                _groupNames.push_back(groupName);
                groupName.clear();
            }
            new_segment = false;
            segment++;
        }
    }

    void AddFace(int i, int j, int k)
    {
        MeshFacet item;
        item.SetVertices(i, j, k);
        item.SetProperty(segment);
        meshFacets.push_back(item);
        countMaterialFacets++;
    }

public:
    // NOLINTBEGIN
    MeshPointArray meshPoints;
    MeshFacetArray meshFacets;
    // NOLINTEND

    using MaterialPerSegment = std::pair<std::string, unsigned long>;
    std::vector<MaterialPerSegment> GetMaterialNames() const
    {
        return _materialNames;
    }

private:
    string_list token_results;
    MeshIO::Binding rgb_value = MeshIO::OVERALL;
    unsigned long countMaterialFacets = 0;
    unsigned long segment = 0;
    bool new_segment = true;

    std::string groupName;
    std::string materialName;
    Material* _material;
    std::vector<std::string> _groupNames;
    std::vector<MaterialPerSegment> _materialNames;
};

}  // namespace

ReaderOBJ::ReaderOBJ(MeshKernel& kernel, Material* material)
    : _kernel(kernel)
    , _material(material)
{}

bool ReaderOBJ::Load(const std::string& file)
{
    Base::FileInfo fi(file);
    Base::ifstream str(fi, std::ios::in);
    return Load(str);
}

bool ReaderOBJ::Load(std::istream& str)
{
    if (!str || str.bad()) {
        return false;
    }

    std::streambuf* buf = str.rdbuf();
    if (!buf) {
        return false;
    }

    ReaderOBJImp reader(_material);
    std::string line;
    while (std::getline(str, line)) {
        boost::trim(line);
        reader.Load(line);
    }
    reader.SetupMaterial();
    _materialNames = reader.GetMaterialNames();

    _kernel.Clear();  // remove all data before

    MeshCleanup meshCleanup(reader.meshPoints, reader.meshFacets);
    if (_material) {
        meshCleanup.SetMaterial(_material);
    }
    meshCleanup.RemoveInvalids();
    MeshPointFacetAdjacency meshAdj(reader.meshPoints.size(), reader.meshFacets);
    meshAdj.SetFacetNeighbourhood();
    _kernel.Adopt(reader.meshPoints, reader.meshFacets);

    return true;
}

// NOLINTNEXTLINE
bool ReaderOBJ::LoadMaterial(std::istream& str)
{
    std::string line;

    if (!_material) {
        return false;
    }

    if (!str || str.bad()) {
        return false;
    }

    std::streambuf* buf = str.rdbuf();
    if (!buf) {
        return false;
    }

    std::map<std::string, Base::Color> materialAmbientColor;
    std::map<std::string, Base::Color> materialDiffuseColor;
    std::map<std::string, Base::Color> materialSpecularColor;
    std::map<std::string, float> materialTransparency;
    std::string materialName;
    std::vector<Base::Color> ambientColor;
    std::vector<Base::Color> diffuseColor;
    std::vector<Base::Color> specularColor;
    std::vector<float> transparency;

    auto readColor = [](const std::vector<std::string>& tokens) -> Base::Color {
        if (tokens.size() == 2) {
            // If only R is given then G and B will be equal
            float r = std::stof(tokens[1]);
            return Base::Color(r, r, r);
        }
        if (tokens.size() == 4) {
            float r = std::stof(tokens[1]);
            float g = std::stof(tokens[2]);
            float b = std::stof(tokens[3]);
            return Base::Color(r, g, b);
        }

        throw std::length_error("Unexpected number of tokens");
    };

    while (std::getline(str, line)) {
        boost::char_separator<char> sep(" ");
        boost::tokenizer<boost::char_separator<char>> tokens(line, sep);
        std::vector<std::string> token_results;
        token_results.assign(tokens.begin(), tokens.end());

        try {
            if (token_results.size() >= 2) {
                if (token_results[0] == "newmtl") {
                    materialName = Base::Tools::escapedUnicodeToUtf8(token_results[1]);
                }
                else if (token_results[0] == "d") {
                    float a = std::stof(token_results[1]);
                    materialTransparency[materialName] = 1.0F - a;
                }
                // If only R is given then G and B will be equal
                else if (token_results[0] == "Ka") {
                    materialAmbientColor[materialName] = readColor(token_results);
                }
                else if (token_results[0] == "Kd") {
                    materialDiffuseColor[materialName] = readColor(token_results);
                }
                else if (token_results[0] == "Ks") {
                    materialSpecularColor[materialName] = readColor(token_results);
                }
            }
        }
        catch (const std::exception&) {
        }
    }

    for (const auto& it : _materialNames) {
        {
            auto jt = materialAmbientColor.find(it.first);
            if (jt != materialAmbientColor.end()) {
                std::vector<Base::Color> mat(it.second, jt->second);
                ambientColor.insert(ambientColor.end(), mat.begin(), mat.end());
            }
        }
        {
            auto jt = materialDiffuseColor.find(it.first);
            if (jt != materialDiffuseColor.end()) {
                std::vector<Base::Color> mat(it.second, jt->second);
                diffuseColor.insert(diffuseColor.end(), mat.begin(), mat.end());
            }
        }
        {
            auto jt = materialSpecularColor.find(it.first);
            if (jt != materialSpecularColor.end()) {
                std::vector<Base::Color> mat(it.second, jt->second);
                specularColor.insert(specularColor.end(), mat.begin(), mat.end());
            }
        }
        {
            auto jt = materialTransparency.find(it.first);
            if (jt != materialTransparency.end()) {
                std::vector<float> transp(it.second, jt->second);
                transparency.insert(transparency.end(), transp.begin(), transp.end());
            }
        }
    }

    if (diffuseColor.size() == _material->diffuseColor.size()) {
        _material->binding = MeshIO::PER_FACE;
        _material->ambientColor.swap(ambientColor);
        _material->diffuseColor.swap(diffuseColor);
        _material->specularColor.swap(specularColor);
        _material->transparency.swap(transparency);
        return true;
    }

    _material->binding = MeshIO::OVERALL;
    _material->ambientColor.clear();
    _material->diffuseColor.clear();
    _material->specularColor.clear();
    _material->transparency.clear();
    return false;
}
