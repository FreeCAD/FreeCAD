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
XERCES_CPP_NAMESPACE_USE

Reader3MF::Reader3MF(std::istream& str)
{
    zipios::ZipHeader zipHeader(str);
    if (zipHeader.isValid()) {
        zip.reset(zipHeader.getInputStream("3D/3dmodel.model"));
    }
}

Reader3MF::Reader3MF(const std::string& filename)
{
    zipios::ZipFile zipFile(filename);
    if (zipFile.isValid()) {
        zip.reset(zipFile.getInputStream("3D/3dmodel.model"));
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
        if (!zip) {
            return false;
        }
        return LoadModel(*zip);
    }
    catch (const std::exception&) {
        return false;
    }
}

bool Reader3MF::LoadModel(std::istream& str)
{
    try {
        std::unique_ptr<XercesDOMParser> parser(new XercesDOMParser);
        parser->setValidationScheme(XercesDOMParser::Val_Auto);
        parser->setDoNamespaces(false);
        parser->setDoSchema(false);
        parser->setValidationSchemaFullChecking(false);
        parser->setCreateEntityReferenceNodes(false);

        Base::StdInputSource inputSource(str, "3dmodel.model");
        parser->parse(inputSource);
        std::unique_ptr<DOMDocument> xmlDocument(parser->adoptDocument());
        return LoadModel(*xmlDocument);
    }
    catch (const XMLException&) {
        return false;
    }
    catch (const DOMException&) {
        return false;
    }
}

bool Reader3MF::LoadModel(DOMDocument& xmlDocument)
{
    DOMNodeList* nodes = xmlDocument.getElementsByTagName(XStr("model").unicodeForm());
    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            bool resource = LoadResources(static_cast<DOMElement*>(node)->getElementsByTagName(
                XStr("resources").unicodeForm()));
            bool build = LoadBuild(
                static_cast<DOMElement*>(node)->getElementsByTagName(XStr("build").unicodeForm()));
            return (resource && build);
        }
    }

    return false;
}

bool Reader3MF::LoadResources(DOMNodeList* nodes)
{
    if (!nodes) {
        return false;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            DOMNodeList* objectList =
                static_cast<DOMElement*>(node)->getElementsByTagName(XStr("object").unicodeForm());
            return LoadObjects(objectList);
        }
    }

    return false;
}

bool Reader3MF::LoadBuild(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList* nodes)
{
    if (!nodes) {
        return false;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            DOMNodeList* objectList =
                static_cast<DOMElement*>(node)->getElementsByTagName(XStr("item").unicodeForm());
            return LoadItems(objectList);
        }
    }

    return false;
}

bool Reader3MF::LoadItems(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList* nodes)
{
    const std::size_t numEntries = 12;
    if (!nodes) {
        return false;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* itemNode = nodes->item(i);
        DOMNode* idAttr = itemNode->getAttributes()->getNamedItem(XStr("objectid").unicodeForm());
        if (idAttr) {
            std::string id = StrX(idAttr->getNodeValue()).c_str();
            int idValue = std::stoi(id);
            Base::Matrix4D mat;

            DOMNode* transformAttr =
                itemNode->getAttributes()->getNamedItem(XStr("transform").unicodeForm());
            if (transformAttr) {
                std::string transform = StrX(transformAttr->getNodeValue()).c_str();
                boost::char_separator<char> sep(" ,");
                boost::tokenizer<boost::char_separator<char>> tokens(transform, sep);
                std::vector<std::string> token_results;
                token_results.assign(tokens.begin(), tokens.end());
                if (token_results.size() == numEntries) {
                    mat[0][0] = std::stod(token_results[0]);
                    mat[1][0] = std::stod(token_results[1]);
                    mat[2][0] = std::stod(token_results[2]);
                    mat[0][1] = std::stod(token_results[3]);
                    mat[1][1] = std::stod(token_results[4]);
                    mat[2][1] = std::stod(token_results[5]);
                    mat[0][2] = std::stod(token_results[6]);
                    mat[1][2] = std::stod(token_results[7]);
                    mat[2][2] = std::stod(token_results[8]);
                    mat[0][3] = std::stod(token_results[9]);
                    mat[1][3] = std::stod(token_results[10]);
                    mat[2][3] = std::stod(token_results[11]);

                    try {
                        meshes.at(idValue).second = mat;
                    }
                    catch (const std::exception&) {
                    }
                }
            }
        }
    }

    return true;
}

bool Reader3MF::LoadObjects(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList* nodes)
{
    if (!nodes) {
        return false;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* objectNode = nodes->item(i);
        if (objectNode->getNodeType() == DOMNode::ELEMENT_NODE) {
            DOMNode* idAttr = objectNode->getAttributes()->getNamedItem(XStr("id").unicodeForm());
            if (idAttr) {
                int id = std::stoi(StrX(idAttr->getNodeValue()).c_str());
                DOMNodeList* meshList = static_cast<DOMElement*>(objectNode)
                                            ->getElementsByTagName(XStr("mesh").unicodeForm());
                LoadMesh(meshList, id);
            }
        }
    }

    return (!meshes.empty());
}

void Reader3MF::LoadMesh(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList* nodes, int id)
{
    if (!nodes) {
        return;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            MeshPointArray points;
            MeshFacetArray facets;
            LoadVertices(static_cast<DOMElement*>(node)->getElementsByTagName(
                             XStr("vertices").unicodeForm()),
                         points);
            LoadTriangles(static_cast<DOMElement*>(node)->getElementsByTagName(
                              XStr("triangles").unicodeForm()),
                          facets);

            MeshCleanup meshCleanup(points, facets);
            meshCleanup.RemoveInvalids();
            MeshPointFacetAdjacency meshAdj(points.size(), facets);
            meshAdj.SetFacetNeighbourhood();

            MeshKernel kernel;
            kernel.Adopt(points, facets);
            meshes.emplace(id, std::make_pair(kernel, Base::Matrix4D()));
        }
    }
}

void Reader3MF::LoadVertices(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList* nodes,
                             MeshPointArray& points)
{
    if (!nodes) {
        return;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            DOMNodeList* vertexList =
                static_cast<DOMElement*>(node)->getElementsByTagName(XStr("vertex").unicodeForm());
            if (vertexList) {
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
                            float x = std::stof(StrX(xAttr->getNodeValue()).c_str());
                            float y = std::stof(StrX(yAttr->getNodeValue()).c_str());
                            float z = std::stof(StrX(zAttr->getNodeValue()).c_str());
                            points.emplace_back(x, y, z);
                        }
                    }
                }
            }
        }
    }
}

void Reader3MF::LoadTriangles(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList* nodes,
                              MeshFacetArray& facets)
{
    if (!nodes) {
        return;
    }

    for (XMLSize_t i = 0; i < nodes->getLength(); i++) {
        DOMNode* node = nodes->item(i);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
            DOMNodeList* triangleList = static_cast<DOMElement*>(node)->getElementsByTagName(
                XStr("triangle").unicodeForm());
            if (triangleList) {
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
        }
    }
}
