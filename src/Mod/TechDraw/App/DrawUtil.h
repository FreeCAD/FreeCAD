/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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

#ifndef _DrawUtil_h_
#define _DrawUtil_h_

#include <string>

#include <QString>
#include <QByteArray>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

namespace TechDraw
{

/// Convenient utility functions for TechDraw Module
class TechDrawExport DrawUtil {
    public:
        static int getIndexFromName(std::string geomName);
        static std::string getGeomTypeFromName(std::string geomName);
        static std::string makeGeomName(std::string geomType, int index);
        static bool isSamePoint(TopoDS_Vertex v1, TopoDS_Vertex v2);
        static bool isZeroEdge(TopoDS_Edge e);
        static double sensibleScale(double working_scale);
        //debugging routines
        static void dumpVertexes(const char* text, const TopoDS_Shape& s);
        static void dumpEdge(char* label, int i, TopoDS_Edge e);
        static void dump1Vertex(const char* label, const TopoDS_Vertex& v);
        static void countFaces(const char* label, const TopoDS_Shape& s);
        static void countWires(const char* label, const TopoDS_Shape& s);
        static void countEdges(const char* label, const TopoDS_Shape& s);
        static const char* printBool(bool b);
        static QString qbaToDebug(const QByteArray& line);
};

} //end namespace TechDraw
#endif
