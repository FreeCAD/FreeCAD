/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAW_COSMETIC_H
#define TECHDRAW_COSMETIC_H

#include <Base/Vector3D.h>
#include <App/Material.h>

class TopoDS_Edge;

namespace TechDrawGeometry {
class BaseGeom;
}

namespace TechDraw {

class TechDrawExport CosmeticVertex
{
public:
    CosmeticVertex();
    CosmeticVertex(Base::Vector3d loc);
    virtual ~CosmeticVertex() = default;

    std::string toCSV(void) const;
    bool fromCSV(std::string& lineSpec);
    void dump(char* title);

    Base::Vector3d pageLocation;
    Base::Vector3d modelLocation;
    int            linkGeom;             //connection to corresponding "real" Vertex
    App::Color     color;
    double         size;
    int            style;
    bool           visible;

protected:
    std::vector<std::string> split(std::string csvLine);

};

class TechDrawExport CosmeticEdge
{
public:
    CosmeticEdge();
    CosmeticEdge(Base::Vector3d p1, Base::Vector3d p2);
    CosmeticEdge(TopoDS_Edge e);
    virtual ~CosmeticEdge() = default;

    std::string toCSV(void) const;
    bool fromCSV(std::string& lineSpec);
    void dump(char* title);

    TechDrawGeometry::BaseGeom* geometry; 

    Base::Vector3d start;
    Base::Vector3d end;
    int            linkGeom;             //connection to corresponding "real" Edge
    App::Color     color;
    double         width;
    int            style;
    bool           visible;

protected:
    std::vector<std::string> split(std::string csvLine);
    double getDefEdgeWidth();
    App::Color getDefEdgeColor();
    int getDefEdgeStyle();

};


} //end namespace TechDraw

#endif //TECHDRAW_COSMETIC_H
