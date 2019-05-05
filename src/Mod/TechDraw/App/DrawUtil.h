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
#include <QPointF>

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#include <App/DocumentObject.h>
#include <Base/Tools2D.h>
#include <Base/Vector3D.h>
#include <Base/Matrix.h>

#include <Mod/Part/App/PartFeature.h>

#include "LineGroup.h"

#define VERTEXTOLERANCE (2.0 * Precision::Confusion())

namespace TechDraw
{

/// Convenient utility functions for TechDraw Module
class TechDrawExport DrawUtil {
    public:
        static int getIndexFromName(std::string geomName);
        static std::string getGeomTypeFromName(std::string geomName);
        static std::string makeGeomName(std::string geomType, int index);
        static bool isSamePoint(TopoDS_Vertex v1, TopoDS_Vertex v2, double tolerance = VERTEXTOLERANCE);
        static bool isZeroEdge(TopoDS_Edge e, double tolerance = VERTEXTOLERANCE);
        static double simpleMinDist(TopoDS_Shape s1, TopoDS_Shape s2);
        static double sensibleScale(double working_scale);
        static double angleWithX(TopoDS_Edge e, bool reverse);
        static double angleWithX(TopoDS_Edge e, TopoDS_Vertex v, double tolerance = VERTEXTOLERANCE);
        static bool isFirstVert(TopoDS_Edge e, TopoDS_Vertex v, double tolerance = VERTEXTOLERANCE);
        static bool isLastVert(TopoDS_Edge e, TopoDS_Vertex v, double tolerance = VERTEXTOLERANCE);
        static bool fpCompare(const double& d1, const double& d2, double tolerance = FLT_EPSILON);
        static Base::Vector3d vertex2Vector(const TopoDS_Vertex& v);
        static std::string formatVector(const Base::Vector3d& v);
        static std::string formatVector(const Base::Vector2d& v);
        static std::string formatVector(const gp_Dir& v);
        static std::string formatVector(const gp_Vec& v);
        static std::string formatVector(const gp_Pnt& v);
        static std::string formatVector(const QPointF& v);

        static bool vectorLess(const Base::Vector3d& v1, const Base::Vector3d& v2);
        static Base::Vector3d toR3(const gp_Ax2 fromSystem, const Base::Vector3d fromPoint);
        static bool checkParallel(const Base::Vector3d v1, const Base::Vector3d v2, double tolerance = FLT_EPSILON);
        //! rotate vector by angle radians around axis through org
        static Base::Vector3d vecRotate(Base::Vector3d vec,
                                        double angle,
                                        Base::Vector3d axis,
                                        Base::Vector3d org = Base::Vector3d(0.0,0.0,0.0));
        static Base::Vector3d closestBasis(Base::Vector3d v);
        static double getDefaultLineWeight(std::string s);
        static Base::Vector3d vector23(const Base::Vector2d& v2) { return Base::Vector3d(v2.x,v2.y,0.0); }
        static Base::Vector2d vector32(const Base::Vector3d& v3) { return Base::Vector2d(v3.x,v3.y); }
        //! is pt between end1 and end2?
        static bool isBetween(const Base::Vector3d pt, const Base::Vector3d end1, const Base::Vector3d end2);
        //! find intersection in 2d for 2 lines in point+direction form
        static Base::Vector3d Intersect2d(Base::Vector3d p1, Base::Vector3d d1,
                                   Base::Vector3d p2, Base::Vector3d d2);
        static Base::Vector3d gpPnt2V3(const gp_Pnt gp) { return Base::Vector3d(gp.X(),gp.Y(),gp.Z()); }
        static gp_Pnt         V32gpPnt(const Base::Vector3d v)  { return gp_Pnt(v.x,v.y,v.z); }

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
