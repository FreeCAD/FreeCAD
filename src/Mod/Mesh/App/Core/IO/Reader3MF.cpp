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
#include <boost/tokenizer.hpp>
#include <memory>
#include <ostream>
#include <sstream>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#endif

#include "Core/MeshIO.h"
#include "Core/MeshKernel.h"
#include <Base/InputSource.h>
#include <Base/XMLTools.h>
#include <Base/ZipHeader.h>
#include <zipios++/zipfile.h>

#include "Reader3MF.h"


using namespace MeshCore;
using namespace XERCES_CPP_NAMESPACE;

Reader3MF::Reader3MF(std::istream& str)
{
    file = std::make_unique<zipios::ZipHeader>(str);
    if (file->isValid()) {
        zip.reset(file->getInputStream("3D/3dmodel.model"));
    }
}

Reader3MF::Reader3MF(const std::string& filename)
{
    file = std::make_unique<zipios::ZipFile>(filename);
    if (file->isValid()) {
        zip.reset(file->getInputStream("3D/3dmodel.model"));
    }
}

std::vector<int> Reader3MF::GetMeshIds() const
{
    std::vector<int> ids;
    ids.reserve(meshes.size());
    for (const auto& it : meshes) {
        ids.emplace_back(it.first);
    }

    return ids;
}

bool Reader3MF::Load()
{
    try {
        return TryLoad();
    }
    catch (const std::exception&) {
        return false;
    }
}

bool Reader3MF::TryLoad()
{
    if (!zip) {
        return false;
    }
    if (LoadModel(*zip)) {
        return true;
    }

    return LoadMeshFromComponents();
}

bool Reader3MF::LoadModel(std::istream& str)
{
    Component comp;
    comp.path = "3dmodel.model";
    return LoadModel(str, comp);
}

bool Reader3MF::LoadModel(std::istream& str, const Component& comp)
{
    try {
        return TryLoadModel(str, comp);
    }
    catch (const XMLException&) {
        return false;
    }
    catch (const DOMException&) {
        return false;
    }
}

std::unique_ptr<XercesDOMParser> Reader3MF::makeDomParser()
{
    std::unique_ptr<XercesDOMParser> parser(new XercesDOMParser);
    parser->setValidationScheme(XercesDOMParser::Val_Auto);
    parser->setDoNamespaces(false);
    parser->setDoSchema(false);
    parser->setValidationSchemaFullChecking(false);
    parser->setCreateEntityReferenceNodes(false);
    return parser;
}

bool Reader3MF::TryLoadModel(std::istream& str, const Component& comp)
{
    if (!str) {
        return false;
    }

    Base::StdInputSource inputSource(str, comp.path.c_str());
    std::unique_ptr<XercesDOMParser> parser = makeDomParser();
    parser->parse(inputSource);
    std::unique_ptr<DOMDocument> xmlDocument(parser->adoptDocument());
    return LoadModel(*xmlDocument, comp);
}

bool Reader3MF::LoadModel(DOMDocument& xmlDocument, const Component& comp)
{
    DOMNodeList* nodes = xmlDocument.getElementsByTagName(XStr("model").unicodeForm());
    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            return LoadResourcesAndBuild(static_cast<DOMElement*>(node), comp);
        }
    }

    return false;
}

bool Reader3MF::LoadResourcesAndBuild(DOMElement* node, const Component& comp)
{
    bool resource =
        LoadResources(node->getElementsByTagName(XStr("resources").unicodeForm()), comp);
    bool build = LoadBuild(node->getElementsByTagName(XStr("build").unicodeForm()));
    return (resource && build);
}

bool Reader3MF::LoadResources(DOMNodeList* nodes, const Component& comp)
{
    if (!nodes) {
        return false;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            auto elem = static_cast<DOMElement*>(node);
            DOMNodeList* objectList = elem->getElementsByTagName(XStr("object").unicodeForm());
            return LoadObject(objectList, comp);
        }
    }

    return false;
}

bool Reader3MF::LoadBuild(DOMNodeList* nodes)
{
    if (!nodes) {
        return false;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            auto elem = static_cast<DOMElement*>(node);
            DOMNodeList* objectList = elem->getElementsByTagName(XStr("item").unicodeForm());
            return LoadItems(objectList);
        }
    }

    return false;
}

bool Reader3MF::LoadItems(DOMNodeList* nodes)
{
    if (!nodes) {
        return false;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* itemNode = nodes->item(i);
        DOMNamedNodeMap* nodeMap = itemNode->getAttributes();
        LoadItem(nodeMap);
    }

    return true;
}

void Reader3MF::LoadItem(DOMNamedNodeMap* nodeMap)
{
    DOMNode* idAttr = nodeMap->getNamedItem(XStr("objectid").unicodeForm());
    if (idAttr) {
        std::string id = StrX(idAttr->getNodeValue()).c_str();
        int idValue = std::stoi(id);

        DOMNode* transformAttr = nodeMap->getNamedItem(XStr("transform").unicodeForm());
        if (transformAttr) {
            std::optional<Base::Matrix4D> mat = ReadTransform(transformAttr);
            if (mat) {
                auto it = meshes.find(idValue);
                if (it != meshes.end()) {
                    it->second.second = mat.value();
                }

                auto jt = std::find_if(components.begin(),
                                       components.end(),
                                       [idValue](const Component& comp) {
                                           return comp.id == idValue;
                                       });
                if (jt != components.end()) {
                    jt->transform = mat.value();
                }
            }
        }
    }
}

std::optional<Base::Matrix4D> Reader3MF::ReadTransform(DOMNode* transformAttr)
{
    constexpr const std::size_t numEntries = 12;
    using Pos2d = std::array<std::array<int, 2>, numEntries>;
    // clang-format off
    static Pos2d pos = {{
        {0, 0}, {1, 0}, {2, 0},
        {0, 1}, {1, 1}, {2, 1},
        {0, 2}, {1, 2}, {2, 2},
        {0, 3}, {1, 3}, {2, 3}
    }};
    // clang-format on

    if (transformAttr) {
        std::string transform = StrX(transformAttr->getNodeValue()).c_str();
        boost::char_separator<char> sep(" ,");
        boost::tokenizer<boost::char_separator<char>> tokens(transform, sep);
        std::vector<std::string> token_results;
        token_results.assign(tokens.begin(), tokens.end());
        if (token_results.size() == numEntries) {
            Base::Matrix4D mat;
            // NOLINTBEGIN
            int index = 0;
            for (const auto& it : pos) {
                auto [r, c] = it;
                mat[r][c] = std::stod(token_results[index++]);
            }
            // NOLINTEND
            return mat;
        }
    }
    return {};
}

bool Reader3MF::LoadObject(DOMNodeList* nodes, const Component& comp)
{
    if (!nodes) {
        return false;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* objectNode = nodes->item(i);
        if (objectNode->getNodeType() == DOMNode::ELEMENT_NODE) {
            DOMNode* idAttr = objectNode->getAttributes()->getNamedItem(XStr("id").unicodeForm());
            auto elem = static_cast<DOMElement*>(objectNode);
            if (idAttr) {
                int id = std::stoi(StrX(idAttr->getNodeValue()).c_str());
                DOMNodeList* meshNode = elem->getElementsByTagName(XStr("mesh").unicodeForm());
                if (meshNode->getLength() > 0) {
                    LoadMesh(meshNode, id, comp);
                }
                else {
                    DOMNodeList* compNode =
                        elem->getElementsByTagName(XStr("components").unicodeForm());
                    LoadComponents(compNode, id);
                }
            }
        }
    }

    return (!meshes.empty());
}

bool Reader3MF::LoadMeshFromComponents()
{
    for (const auto& it : components) {
        std::string path = it.path.substr(1);
        zip.reset(file->getInputStream(path));
        LoadModel(*zip, it);
    }

    return (!meshes.empty());
}

void Reader3MF::LoadComponents(DOMNodeList* nodes, int id)
{
    if (!nodes) {
        return;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* objectNode = nodes->item(i);
        if (objectNode->getNodeType() == DOMNode::ELEMENT_NODE) {
            auto elem = static_cast<DOMElement*>(objectNode);
            DOMNodeList* compNode = elem->getElementsByTagName(XStr("component").unicodeForm());
            if (compNode->getLength() > 0) {
                LoadComponent(compNode, id);
            }
        }
    }
}

void Reader3MF::LoadComponent(DOMNodeList* nodes, int id)
{
    if (!nodes) {
        return;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* compNode = nodes->item(i);
        if (compNode->getNodeType() == DOMNode::ELEMENT_NODE) {
            if (DOMNamedNodeMap* attr = compNode->getAttributes()) {
                LoadComponent(attr, id);
            }
        }
    }
}

void Reader3MF::LoadComponent(DOMNamedNodeMap* attr, int id)
{
    auto validComponent = [](const Component& comp) {
        return (comp.id > 0 && comp.objectId >= 0 && !comp.path.empty());
    };

    Component component;
    component.id = id;
    if (DOMNode* pathAttr = attr->getNamedItem(XStr("p:path").unicodeForm())) {
        component.path = StrX(pathAttr->getNodeValue()).c_str();
    }
    if (DOMNode* idAttr = attr->getNamedItem(XStr("objectid").unicodeForm())) {
        component.objectId = std::stoi(StrX(idAttr->getNodeValue()).c_str());
    }
    if (DOMNode* transformAttr = attr->getNamedItem(XStr("transform").unicodeForm())) {
        std::optional<Base::Matrix4D> mat = ReadTransform(transformAttr);
        if (mat) {
            component.transform = mat.value();
        }
    }
    if (validComponent(component)) {
        components.push_back(component);
    }
}

void Reader3MF::LoadMesh(DOMNodeList* nodes, int id, const Component& comp)
{
    if (!nodes) {
        return;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            auto elem = static_cast<DOMElement*>(node);
            MeshPointArray points;
            MeshFacetArray facets;
            LoadVertices(elem->getElementsByTagName(XStr("vertices").unicodeForm()), points);
            LoadTriangles(elem->getElementsByTagName(XStr("triangles").unicodeForm()), facets);

            MeshCleanup meshCleanup(points, facets);
            meshCleanup.RemoveInvalids();
            MeshPointFacetAdjacency meshAdj(points.size(), facets);
            meshAdj.SetFacetNeighbourhood();

            Base::Matrix4D mat = comp.transform;
            MeshKernel kernel;
            kernel.Adopt(points, facets);
            meshes.emplace(id, std::make_pair(kernel, mat));
        }
    }
}

void Reader3MF::LoadVertices(DOMNodeList* nodes, MeshPointArray& points)
{
    if (!nodes) {
        return;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            auto elem = static_cast<DOMElement*>(node);
            DOMNodeList* vertexList = elem->getElementsByTagName(XStr("vertex").unicodeForm());
            if (vertexList) {
                ReadVertices(vertexList, points);
            }
        }
    }
}

void Reader3MF::ReadVertices(DOMNodeList* vertexList, MeshPointArray& points)
{
    XMLSize_t numVertices = vertexList->getLength();
    points.reserve(numVertices);
    for (XMLSize_t j = 0; j < numVertices; j++) {
        DOMNode* vertexNode = vertexList->item(j);
        DOMNamedNodeMap* attr = vertexNode->getAttributes();
        if (attr) {
            DOMNode* xAttr = attr->getNamedItem(XStr("x").unicodeForm());
            DOMNode* yAttr = attr->getNamedItem(XStr("y").unicodeForm());
            DOMNode* zAttr = attr->getNamedItem(XStr("z").unicodeForm());
            if (xAttr && yAttr && zAttr) {
                // NOLINTBEGIN
                float x = std::stof(StrX(xAttr->getNodeValue()).c_str());
                float y = std::stof(StrX(yAttr->getNodeValue()).c_str());
                float z = std::stof(StrX(zAttr->getNodeValue()).c_str());
                points.emplace_back(x, y, z);
                // NOLINTEND
            }
        }
    }
}

void Reader3MF::LoadTriangles(DOMNodeList* nodes, MeshFacetArray& facets)
{
    if (!nodes) {
        return;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            auto elem = static_cast<DOMElement*>(node);
            DOMNodeList* triangleList = elem->getElementsByTagName(XStr("triangle").unicodeForm());
            if (triangleList) {
                ReadTriangles(triangleList, facets);
            }
        }
    }
}

void Reader3MF::ReadTriangles(DOMNodeList* triangleList, MeshFacetArray& facets)
{
    XMLSize_t numTriangles = triangleList->getLength();
    facets.reserve(numTriangles);
    for (XMLSize_t j = 0; j < numTriangles; j++) {
        DOMNode* triangleNode = triangleList->item(j);
        DOMNamedNodeMap* attr = triangleNode->getAttributes();
        if (attr) {
            DOMNode* v1Attr = attr->getNamedItem(XStr("v1").unicodeForm());
            DOMNode* v2Attr = attr->getNamedItem(XStr("v2").unicodeForm());
            DOMNode* v3Attr = attr->getNamedItem(XStr("v3").unicodeForm());
            if (v1Attr && v2Attr && v3Attr) {
                PointIndex v1 = std::stoul(StrX(v1Attr->getNodeValue()).c_str());
                PointIndex v2 = std::stoul(StrX(v2Attr->getNodeValue()).c_str());
                PointIndex v3 = std::stoul(StrX(v3Attr->getNodeValue()).c_str());
                facets.emplace_back(v1, v2, v3);
            }
        }
    }
}
