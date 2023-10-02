/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAW_DIMENSIONREFERENCES_H
#define TECHDRAW_DIMENSIONREFERENCES_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <string>
#include <vector>

#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

#include <Mod/Part/App/TopoShape.h>

namespace App
{
class DocumentObject;
}

namespace Part
{
class TopoShape;
}

namespace TechDraw
{

//a convenient way of handling object+subName references
class TechDrawExport ReferenceEntry
{
public:
    ReferenceEntry( App::DocumentObject* docObject, std::string subName ) {
        setObject(docObject);
        setSubName(subName);
    }
    ReferenceEntry(const ReferenceEntry& other) {
        setObject(other.getObject());
        setSubName(other.getSubName());
    }
    ~ReferenceEntry() = default;

    App::DocumentObject* getObject() const;
    void setObject(App::DocumentObject* docObj) { m_object = docObj; }
    std::string getSubName(bool longForm = false) const;
    void setSubName(std::string subName) { m_subName = subName; }
    TopoDS_Shape getGeometry() const;
    std::string geomType() const;
    bool isWholeObject() const;

    Part::TopoShape asTopoShape() const;
    Part::TopoShape asTopoShapeVertex(TopoDS_Vertex &vert) const;
    Part::TopoShape asTopoShapeEdge(TopoDS_Edge& edge) const;

    bool is3d() const;
    bool isValid() const;

private:
    App::DocumentObject* m_object;
    std::string m_subName;
};

using ReferenceVector = std::vector<ReferenceEntry>;

} // end namespace

#endif //TECHDRAW_DIMENSIONREFERENCES_H
