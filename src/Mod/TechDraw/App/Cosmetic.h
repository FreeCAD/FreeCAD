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

#include "Geometry.h"

class TopoDS_Edge;

/*namespace TechDraw {*/
/*class BaseGeom;*/
/*}*/

namespace TechDraw {

class TechDrawExport LineFormat
{
public:
    LineFormat();
    LineFormat(int style,
               double weight,
               App::Color color,
               bool visible );
    ~LineFormat() = default;

    int m_style;
    double m_weight;
    App::Color m_color;
    bool m_visible;

    static double getDefEdgeWidth();
    static App::Color getDefEdgeColor();
    static int getDefEdgeStyle();

    void dump(char* title);
    std::string toCSV() const;
    bool fromCSV(std::string& lineSpec);
};

class TechDrawExport CosmeticVertex: public TechDraw::Vertex
{
public:
    CosmeticVertex();
    CosmeticVertex(Base::Vector3d loc);
    virtual ~CosmeticVertex() = default;

    std::string toCSV(void) const;
    bool fromCSV(std::string& lineSpec);
    void dump(char* title);

    int            linkGeom;             //connection to corresponding "real" Vertex
    App::Color     color;
    double         size;
    int            style;
    bool           visible;

protected:
/*    std::vector<std::string> split(std::string csvLine);*/

};

//class TechDrawExport CosmeticEdge: publix TechDraw::BaseGeom
class TechDrawExport CosmeticEdge
{
public:
    CosmeticEdge();
    CosmeticEdge(Base::Vector3d p1, Base::Vector3d p2);
    CosmeticEdge(TopoDS_Edge e);
    CosmeticEdge(TechDraw::BaseGeom* g);
    virtual ~CosmeticEdge() = default;

    TechDraw::BaseGeom* scaledGeometry(double scale);

    std::string toCSV(void) const;
    bool fromCSV(std::string& lineSpec);
    void dump(char* title);

    TechDraw::BaseGeom* m_geometry; 
    LineFormat m_format;
    int m_linkGeom;

protected:
/*    std::vector<std::string> split(std::string csvLine);*/

};


} //end namespace TechDraw

#endif //TECHDRAW_COSMETIC_H
