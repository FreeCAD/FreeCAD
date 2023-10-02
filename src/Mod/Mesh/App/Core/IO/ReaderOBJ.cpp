/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#ifndef _PreComp_
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>
#include <istream>
#endif

#include "Core/MeshIO.h"
#include "Core/MeshKernel.h"
#include <Base/Tools.h>

#include "ReaderOBJ.h"


using namespace MeshCore;

ReaderOBJ::ReaderOBJ(MeshKernel& kernel, Material* material)
    : _kernel(kernel)
    , _material(material)
{}

bool ReaderOBJ::Load(std::istream& str)
{
    boost::regex rx_m("^mtllib\\s+(.+)\\s*$");
    boost::regex rx_u(R"(^usemtl\s+([\x21-\x7E]+)\s*$)");
    boost::regex rx_g(R"(^g\s+([\x21-\x7E]+)\s*$)");
    boost::regex rx_p("^v\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$");
    boost::regex rx_c("^v\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+(\\d{1,3})\\s+(\\d{1,3})\\s+(\\d{1,3})\\s*$");
    boost::regex rx_t("^v\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)"
                      "\\s+([-+]?[0-9]*)\\.?([0-9]+([eE][-+]?[0-9]+)?)\\s*$");
    boost::regex rx_f3("^f\\s+([-+]?[0-9]+)/?[-+]?[0-9]*/?[-+]?[0-9]*"
                       "\\s+([-+]?[0-9]+)/?[-+]?[0-9]*/?[-+]?[0-9]*"
                       "\\s+([-+]?[0-9]+)/?[-+]?[0-9]*/?[-+]?[0-9]*\\s*$");
    boost::regex rx_f4("^f\\s+([-+]?[0-9]+)/?[-+]?[0-9]*/?[-+]?[0-9]*"
                       "\\s+([-+]?[0-9]+)/?[-+]?[0-9]*/?[-+]?[0-9]*"
                       "\\s+([-+]?[0-9]+)/?[-+]?[0-9]*/?[-+]?[0-9]*"
                       "\\s+([-+]?[0-9]+)/?[-+]?[0-9]*/?[-+]?[0-9]*\\s*$");
    boost::cmatch what;

    unsigned long segment = 0;
    MeshPointArray meshPoints;
    MeshFacetArray meshFacets;

    std::string line;
    float fX {}, fY {}, fZ {};
    int i1 = 1, i2 = 1, i3 = 1, i4 = 1;
    MeshFacet item;

    if (!str || str.bad()) {
        return false;
    }

    std::streambuf* buf = str.rdbuf();
    if (!buf) {
        return false;
    }

    MeshIO::Binding rgb_value = MeshIO::OVERALL;
    bool new_segment = true;
    std::string groupName;
    std::string materialName;
    unsigned long countMaterialFacets = 0;

    while (std::getline(str, line)) {
        if (boost::regex_match(line.c_str(), what, rx_p)) {
            fX = (float)std::atof(what[1].first);
            fY = (float)std::atof(what[4].first);
            fZ = (float)std::atof(what[7].first);
            meshPoints.push_back(MeshPoint(Base::Vector3f(fX, fY, fZ)));
        }
        else if (boost::regex_match(line.c_str(), what, rx_c)) {
            fX = (float)std::atof(what[1].first);
            fY = (float)std::atof(what[4].first);
            fZ = (float)std::atof(what[7].first);
            float r = std::min<int>(std::atof(what[10].first), 255) / 255.0f;
            float g = std::min<int>(std::atof(what[11].first), 255) / 255.0f;
            float b = std::min<int>(std::atof(what[12].first), 255) / 255.0f;
            meshPoints.push_back(MeshPoint(Base::Vector3f(fX, fY, fZ)));

            App::Color c(r, g, b);
            unsigned long prop = static_cast<uint32_t>(c.getPackedValue());
            meshPoints.back().SetProperty(prop);
            rgb_value = MeshIO::PER_VERTEX;
        }
        else if (boost::regex_match(line.c_str(), what, rx_t)) {
            fX = (float)std::atof(what[1].first);
            fY = (float)std::atof(what[4].first);
            fZ = (float)std::atof(what[7].first);
            float r = static_cast<float>(std::atof(what[10].first));
            float g = static_cast<float>(std::atof(what[13].first));
            float b = static_cast<float>(std::atof(what[16].first));
            meshPoints.push_back(MeshPoint(Base::Vector3f(fX, fY, fZ)));

            App::Color c(r, g, b);
            unsigned long prop = static_cast<uint32_t>(c.getPackedValue());
            meshPoints.back().SetProperty(prop);
            rgb_value = MeshIO::PER_VERTEX;
        }
        else if (boost::regex_match(line.c_str(), what, rx_g)) {
            new_segment = true;
            groupName = Base::Tools::escapedUnicodeToUtf8(what[1].first);
        }
        else if (boost::regex_match(line.c_str(), what, rx_m)) {
            if (_material) {
                _material->library = Base::Tools::escapedUnicodeToUtf8(what[1].first);
            }
        }
        else if (boost::regex_match(line.c_str(), what, rx_u)) {
            if (!materialName.empty()) {
                _materialNames.emplace_back(materialName, countMaterialFacets);
            }
            materialName = Base::Tools::escapedUnicodeToUtf8(what[1].first);
            countMaterialFacets = 0;
        }
        else if (boost::regex_match(line.c_str(), what, rx_f3)) {
            // starts a new segment
            if (new_segment) {
                if (!groupName.empty()) {
                    _groupNames.push_back(groupName);
                    groupName.clear();
                }
                new_segment = false;
                segment++;
            }

            // 3-vertex face
            i1 = std::atoi(what[1].first);
            i1 = i1 > 0 ? i1 - 1 : i1 + static_cast<int>(meshPoints.size());
            i2 = std::atoi(what[2].first);
            i2 = i2 > 0 ? i2 - 1 : i2 + static_cast<int>(meshPoints.size());
            i3 = std::atoi(what[3].first);
            i3 = i3 > 0 ? i3 - 1 : i3 + static_cast<int>(meshPoints.size());
            item.SetVertices(i1, i2, i3);
            item.SetProperty(segment);
            meshFacets.push_back(item);
            countMaterialFacets++;
        }
        else if (boost::regex_match(line.c_str(), what, rx_f4)) {
            // starts a new segment
            if (new_segment) {
                if (!groupName.empty()) {
                    _groupNames.push_back(groupName);
                    groupName.clear();
                }
                new_segment = false;
                segment++;
            }

            // 4-vertex face
            i1 = std::atoi(what[1].first);
            i1 = i1 > 0 ? i1 - 1 : i1 + static_cast<int>(meshPoints.size());
            i2 = std::atoi(what[2].first);
            i2 = i2 > 0 ? i2 - 1 : i2 + static_cast<int>(meshPoints.size());
            i3 = std::atoi(what[3].first);
            i3 = i3 > 0 ? i3 - 1 : i3 + static_cast<int>(meshPoints.size());
            i4 = std::atoi(what[4].first);
            i4 = i4 > 0 ? i4 - 1 : i4 + static_cast<int>(meshPoints.size());

            item.SetVertices(i1, i2, i3);
            item.SetProperty(segment);
            meshFacets.push_back(item);
            countMaterialFacets++;

            item.SetVertices(i3, i4, i1);
            item.SetProperty(segment);
            meshFacets.push_back(item);
            countMaterialFacets++;
        }
    }

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
                App::Color c;
                c.setPackedValue(static_cast<uint32_t>(prop));
                _material->diffuseColor.push_back(c);
            }
        }
    }
    else if (!materialName.empty()) {
        // At this point the materials from the .mtl file are not known and will be read-in by the
        // calling instance but the color list is pre-filled with a default value
        if (_material) {
            _material->binding = MeshIO::PER_FACE;
            _material->diffuseColor.resize(meshFacets.size(), App::Color(0.8f, 0.8f, 0.8f));
        }
    }

    _kernel.Clear();  // remove all data before

    MeshCleanup meshCleanup(meshPoints, meshFacets);
    if (_material) {
        meshCleanup.SetMaterial(_material);
    }
    meshCleanup.RemoveInvalids();
    MeshPointFacetAdjacency meshAdj(meshPoints.size(), meshFacets);
    meshAdj.SetFacetNeighbourhood();
    _kernel.Adopt(meshPoints, meshFacets);

    return true;
}

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

    std::map<std::string, App::Color> materialAmbientColor;
    std::map<std::string, App::Color> materialDiffuseColor;
    std::map<std::string, App::Color> materialSpecularColor;
    std::map<std::string, float> materialTransparency;
    std::string materialName;
    std::vector<App::Color> ambientColor;
    std::vector<App::Color> diffuseColor;
    std::vector<App::Color> specularColor;
    std::vector<float> transparency;

    auto readColor = [](const std::vector<std::string>& tokens) -> App::Color {
        if (tokens.size() == 2) {
            // If only R is given then G and B will be equal
            float r = boost::lexical_cast<float>(tokens[1]);
            return App::Color(r, r, r);
        }
        else if (tokens.size() == 4) {
            float r = boost::lexical_cast<float>(tokens[1]);
            float g = boost::lexical_cast<float>(tokens[2]);
            float b = boost::lexical_cast<float>(tokens[3]);
            return App::Color(r, g, b);
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
                    float a = boost::lexical_cast<float>(token_results[1]);
                    materialTransparency[materialName] = 1.0f - a;
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
        catch (const boost::bad_lexical_cast&) {
        }
        catch (const std::exception&) {
        }
    }

    for (const auto& it : _materialNames) {
        {
            auto jt = materialAmbientColor.find(it.first);
            if (jt != materialAmbientColor.end()) {
                std::vector<App::Color> mat(it.second, jt->second);
                ambientColor.insert(ambientColor.end(), mat.begin(), mat.end());
            }
        }
        {
            auto jt = materialDiffuseColor.find(it.first);
            if (jt != materialDiffuseColor.end()) {
                std::vector<App::Color> mat(it.second, jt->second);
                diffuseColor.insert(diffuseColor.end(), mat.begin(), mat.end());
            }
        }
        {
            auto jt = materialSpecularColor.find(it.first);
            if (jt != materialSpecularColor.end()) {
                std::vector<App::Color> mat(it.second, jt->second);
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
    else {
        _material->binding = MeshIO::OVERALL;
        _material->ambientColor.clear();
        _material->diffuseColor.clear();
        _material->specularColor.clear();
        _material->transparency.clear();
        return false;
    }
}
