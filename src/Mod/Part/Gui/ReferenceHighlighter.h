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

#ifndef PARTGUI_REFERENCEHIGHLIGHTER_H
#define PARTGUI_REFERENCEHIGHLIGHTER_H

#include <vector>

#include <TopoDS_Shape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <App/Material.h>
#include <Mod/Part/PartGlobal.h>


namespace PartGui {

/*!
 * \brief The ReferenceHighlighter class
 * \author Werner Mayer
 */
class PartGuiExport ReferenceHighlighter
{
public:
    /*!
     * \brief ReferenceHighlighter
     * \param shape The input shape.
     * \param color The standard edge color.
     */
    ReferenceHighlighter(const TopoDS_Shape& shape, const App::Color& color);

    void setDefaultColor(const App::Color& c) {
        defaultColor = c;
    }
    void setElementColor(const App::Color& c) {
        elementColor = c;
    }
    void setObjectColor(const App::Color& c) {
        objectColor = c;
    }

    /*!
     * \brief getVertexColors
     * \param elements The sub-element names. If this list is empty \a colors will be filled with the default color.
     * \param colors The size of the \a colors array is equal to the number of vertexes of the shape
     */
    void getVertexColors(const std::vector<std::string>& elements,
                         std::vector<App::Color>& colors) const;
    /*!
     * \brief getEdgeColors
     * \param elements The sub-element names. If this list is empty \a colors will be filled with the default color.
     * \param colors The size of the \a colors array is equal to the number of edges of the shape
     */
    void getEdgeColors(const std::vector<std::string>& elements,
                       std::vector<App::Color>& colors) const;
    /*!
     * \brief getFaceColors
     * \param elements The sub-element names. If this list is empty \a colors will be filled with the default color.
     * \param colors The size of the \a colors array is equal to the number of faces of the shape
     */
    void getFaceColors(const std::vector<std::string>& elements,
                       std::vector<App::Color>& colors) const;

private:
    void getVertexColor(const std::string& element, std::vector<App::Color>& colors) const;
    void getVertexColorsOfEdge(const std::string& element, std::vector<App::Color>& colors) const;
    void getVertexColorsOfWire(const std::string& element, std::vector<App::Color>& colors) const;
    void getVertexColorsOfFace(const std::string& element, std::vector<App::Color>& colors) const;
    void getEdgeColor(const std::string& element, std::vector<App::Color>& colors) const;
    void getEdgeColorsOfWire(const std::string& element, std::vector<App::Color>& colors) const;
    void getEdgeColorsOfFace(const std::string& element, std::vector<App::Color>& colors) const;
    void getFaceColor(const std::string& element, std::vector<App::Color>& colors) const;

private:
    App::Color defaultColor;
    App::Color elementColor;
    App::Color objectColor;
    TopTools_IndexedMapOfShape vMap;
    TopTools_IndexedMapOfShape eMap;
    TopTools_IndexedMapOfShape wMap;
    TopTools_IndexedMapOfShape fMap;
};


} // namespace PartGui


#endif // PARTGUI_REFERENCEHIGHLIGHTER_H
