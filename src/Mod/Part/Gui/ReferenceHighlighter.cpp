/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>

# include <boost/algorithm/string/predicate.hpp>
#endif

#include "ReferenceHighlighter.h"


using namespace PartGui;

ReferenceHighlighter::ReferenceHighlighter(const TopoDS_Shape& shape, const App::Color& color)
    : defaultColor(color)
    , elementColor(1.0f,0.0f,1.0f) // magenta
    , objectColor(0.6f,0.0f,1.0f) // purple
{
    TopExp::MapShapes(shape, TopAbs_VERTEX, vMap);
    TopExp::MapShapes(shape, TopAbs_EDGE, eMap);
    TopExp::MapShapes(shape, TopAbs_WIRE, wMap);
    TopExp::MapShapes(shape, TopAbs_FACE, fMap);
}

void ReferenceHighlighter::getVertexColor(const std::string& element, std::vector<App::Color>& colors) const
{
    int idx = std::stoi(element.substr(6)) - 1;
    assert ( idx >= 0 );
    std::size_t pos = std::size_t(idx);
    if (pos < colors.size())
        colors[pos] = elementColor;
}

void ReferenceHighlighter::getVertexColorsOfEdge(const std::string& element, std::vector<App::Color>& colors) const
{
    int idx = std::stoi(element.substr(4));
    assert ( idx > 0 );
    // get the vertexes of the edge
    TopoDS_Shape edge = eMap.FindKey(idx);
    for (TopExp_Explorer xp(edge, TopAbs_VERTEX); xp.More(); xp.Next()) {
        int vertexIndex = vMap.FindIndex(xp.Current());

        // Vertex found?
        if (vertexIndex > 0) {
            std::size_t pos = std::size_t(vertexIndex - 1);
            if (pos < colors.size())
                colors[pos] = elementColor;
        }
    }
}

void ReferenceHighlighter::getVertexColorsOfWire(const std::string& element, std::vector<App::Color>& colors) const
{
    int idx = std::stoi(element.substr(4));
    assert ( idx > 0 );
    // get the vertexes of the wire
    TopoDS_Shape wire = wMap.FindKey(idx);
    for (TopExp_Explorer xp(wire, TopAbs_VERTEX); xp.More(); xp.Next()) {
        int vertexIndex = vMap.FindIndex(xp.Current());

        // Vertex found?
        if (vertexIndex > 0) {
            std::size_t pos = std::size_t(vertexIndex - 1);
            if (pos < colors.size())
                colors[pos] = elementColor;
        }
    }
}

void ReferenceHighlighter::getVertexColorsOfFace(const std::string& element, std::vector<App::Color>& colors) const
{
    int idx = std::stoi(element.substr(4));
    assert ( idx > 0 );
    // get the vertexes of the face
    TopoDS_Shape face = fMap.FindKey(idx);
    for (TopExp_Explorer xp(face, TopAbs_VERTEX); xp.More(); xp.Next()) {
        int vertexIndex = vMap.FindIndex(xp.Current());

        // Vertex found?
        if (vertexIndex > 0) {
            std::size_t pos = std::size_t(vertexIndex - 1);
            if (pos < colors.size())
                colors[pos] = elementColor;
        }
    }
}

void ReferenceHighlighter::getVertexColors(const std::vector<std::string>& elements,
                                           std::vector<App::Color>& colors) const
{
    colors.resize(vMap.Extent(), defaultColor);

    if (!elements.empty()) {
        for (const std::string& e : elements) {
            if (boost::starts_with(e, "Vertex")) {
                getVertexColor(e, colors);
            }
            else if (boost::starts_with(e, "Edge")) {
                getVertexColorsOfEdge(e, colors);
            }
            else if (boost::starts_with(e, "Wire")) {
                getVertexColorsOfWire(e, colors);
            }
            else if (boost::starts_with(e, "Face")) {
                getVertexColorsOfFace(e, colors);
            }
        }
    }
    else {
        std::fill(colors.begin(), colors.end(), objectColor);
    }
}

void ReferenceHighlighter::getEdgeColor(const std::string& element, std::vector<App::Color>& colors) const
{
    int idx = std::stoi(element.substr(4)) - 1;
    assert ( idx >= 0 );
    std::size_t pos = std::size_t(idx);
    if (pos < colors.size())
        colors[pos] = elementColor;
}

void ReferenceHighlighter::getEdgeColorsOfWire(const std::string& element, std::vector<App::Color>& colors) const
{
    int idx = std::stoi(element.substr(4));
    assert ( idx > 0 );
    // get the edges of the wire
    TopoDS_Shape wire = wMap.FindKey(idx);
    for (TopExp_Explorer xp(wire, TopAbs_EDGE); xp.More(); xp.Next()) {
        int edgeIndex = eMap.FindIndex(xp.Current());

        // Edge found?
        if (edgeIndex > 0) {
            std::size_t pos = std::size_t(edgeIndex - 1);
            if (pos < colors.size())
                colors[pos] = elementColor;
        }
    }
}

void ReferenceHighlighter::getEdgeColorsOfFace(const std::string& element, std::vector<App::Color>& colors) const
{
    int idx = std::stoi(element.substr(4));
    assert ( idx > 0 );
    // get the edges of the faces
    TopoDS_Shape face = fMap.FindKey(idx);
    for (TopExp_Explorer xp(face, TopAbs_EDGE); xp.More(); xp.Next()) {
        int edgeIndex = eMap.FindIndex(xp.Current());

        // Edge found?
        if (edgeIndex > 0) {
            std::size_t pos = std::size_t(edgeIndex - 1);
            if (pos < colors.size())
                colors[pos] = elementColor;
        }
    }
}

void ReferenceHighlighter::getEdgeColors(const std::vector<std::string>& elements,
                                         std::vector<App::Color>& colors) const
{
    colors.resize(eMap.Extent(), defaultColor);

    if (!elements.empty()) {
        for (const std::string& e : elements) {
            if (boost::starts_with(e, "Edge")) {
                getEdgeColor(e, colors);
            }
            else if (boost::starts_with(e, "Wire")) {
                getEdgeColorsOfWire(e, colors);
            }
            else if (boost::starts_with(e, "Face")) {
                getEdgeColorsOfFace(e, colors);
            }
        }
    }
    else {
        std::fill(colors.begin(), colors.end(), objectColor);
    }
}

void ReferenceHighlighter::getFaceColor(const std::string& element, std::vector<App::Color>& colors) const
{
    int idx = std::stoi(element.substr(4)) - 1;
    assert ( idx >= 0 );
    std::size_t pos = std::size_t(idx);
    if (pos < colors.size())
        colors[pos] = elementColor;
}

void ReferenceHighlighter::getFaceColors(const std::vector<std::string>& elements,
                                         std::vector<App::Color>& colors) const
{
    colors.resize(fMap.Extent(), defaultColor);

    if (!elements.empty()) {
        for (const std::string& e : elements) {
            if (boost::starts_with(e, "Face")) {
                getFaceColor(e, colors);
            }
        }
    }
    else {
        std::fill(colors.begin(), colors.end(), objectColor);
    }
}
