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

#ifndef DrawUtil_h_
#define DrawUtil_h_

#include <string>

#include <QByteArray>
#include <QPointF>
#include <QString>

#include <Geom_Curve.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Vec.hxx>

#include <Base/Vector3D.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/TechDraw/TechDrawGlobal.h>


#ifndef M_2PI
#define M_2PI ((M_PI)*2.0)
#endif

#define VERTEXTOLERANCE (2.0 * Precision::Confusion())
#define VECTORTOLERANCE (Precision::Confusion())

#define SVG_NS_URI "http://www.w3.org/2000/svg"
#define FREECAD_SVG_NS_URI "http://www.freecad.org/wiki/index.php?title=Svg_Namespace"

//some shapes are being passed in where edges that should be connected are in fact
//separated by more than 2*Precision::Confusion (expected tolerance for 2 TopoDS_Vertex)
//this value is used in EdgeWalker, DrawProjectSplit and DrawUtil and needs to be in sync in
//all 3 files.
#define EWTOLERANCE 0.0001//arbitrary number that seems to give good results for drawing

//a multiplier for EWTOLERANCE used in fuzzy fuse and common operations.
#define FUZZYADJUST 4.0


namespace TechDraw
{

//used by sort_Edges
struct EdgePoints
{
    gp_Pnt v1, v2;
    std::list<TopoDS_Edge>::iterator it;
    TopoDS_Edge edge;
};

/// Convenient utility functions for TechDraw Module
class TechDrawExport DrawUtil
{
public:
    static int getIndexFromName(const std::string& geomName);
    static std::string getGeomTypeFromName(const std::string& geomName);
    static std::string makeGeomName(const std::string& geomType, int index);
    static bool isSamePoint(TopoDS_Vertex v1, TopoDS_Vertex v2, double tolerance = VERTEXTOLERANCE);
    static bool isZeroEdge(TopoDS_Edge e, double tolerance = VERTEXTOLERANCE);
    static double simpleMinDist(TopoDS_Shape s1, TopoDS_Shape s2);
    static double sensibleScale(double working_scale);
    static double angleWithX(TopoDS_Edge e, bool reverse);
    static double angleWithX(TopoDS_Edge e, TopoDS_Vertex v, double tolerance = VERTEXTOLERANCE);
    static double incidenceAngleAtVertex(TopoDS_Edge e, TopoDS_Vertex v, double tolerance);

    static bool isFirstVert(TopoDS_Edge e, TopoDS_Vertex v, double tolerance = VERTEXTOLERANCE);
    static bool isLastVert(TopoDS_Edge e, TopoDS_Vertex v, double tolerance = VERTEXTOLERANCE);
    static bool fpCompare(const double& d1, const double& d2, double tolerance = FLT_EPSILON);
    static std::pair<Base::Vector3d, Base::Vector3d>
    boxIntersect2d(Base::Vector3d point, Base::Vector3d dir, double xRange, double yRange);
    static bool apparentIntersection(const Handle(Geom_Curve) curve1,
                                     const Handle(Geom_Curve) curve2, Base::Vector3d& result);
    static bool apparentIntersection(TopoDS_Edge& edge0, TopoDS_Edge& edge1, gp_Pnt& intersect);

    static bool intersect2Lines3d(Base::Vector3d p0, Base::Vector3d d0, Base::Vector3d p1,
                                  Base::Vector3d d1, Base::Vector3d& intersect);

    static Base::Vector3d vertex2Vector(const TopoDS_Vertex& v);

    static std::string formatVector(const Base::Vector3d& v);
    static std::string formatVector(const gp_Dir& v);
    static std::string formatVector(const gp_Dir2d& v);
    static std::string formatVector(const gp_Vec& v);
    static std::string formatVector(const gp_Pnt& v);
    static std::string formatVector(const gp_Pnt2d& v);
    static std::string formatVector(const QPointF& v);

    static bool vectorLess(const Base::Vector3d& v1, const Base::Vector3d& v2);
    //!std::map require comparator to be a type not a function
    struct vectorLessType
    {
        bool operator()(const Base::Vector3d& a, const Base::Vector3d& b) const
        {
            return DrawUtil::vectorLess(a, b);
        }
    };
    static bool vertexEqual(TopoDS_Vertex& v1, TopoDS_Vertex& v2);
    static bool vectorEqual(Base::Vector3d& v1, Base::Vector3d& v2);

    static TopoDS_Shape vectorToCompound(std::vector<TopoDS_Edge> vecIn, bool invert = true);
    static TopoDS_Shape vectorToCompound(std::vector<TopoDS_Wire> vecIn, bool invert = true);
    static TopoDS_Shape shapeVectorToCompound(std::vector<TopoDS_Shape> vecIn, bool invert = true);
    static std::vector<TopoDS_Edge> shapeToVector(TopoDS_Shape shapeIn);

    static Base::Vector3d toR3(const gp_Ax2& fromSystem, const Base::Vector3d& fromPoint);
    static bool checkParallel(const Base::Vector3d v1, const Base::Vector3d v2,
                              double tolerance = FLT_EPSILON);
    //! rotate vector by angle radians around axis through org
    static Base::Vector3d vecRotate(Base::Vector3d vec, double angle, Base::Vector3d axis,
                                    Base::Vector3d org = Base::Vector3d(0.0, 0.0, 0.0));

    static Base::Vector3d closestBasis(Base::Vector3d v);
    static gp_Vec closestBasis(gp_Vec inVec);
    static Base::Vector3d closestBasis(Base::Vector3d vDir, gp_Ax2 coordSys);
    static Base::Vector3d closestBasis(gp_Dir gDir, gp_Ax2 coordSys);

    static double getWidthInDirection(gp_Dir direction, TopoDS_Shape& shape);

    static double getDefaultLineWeight(std::string s);
    //! is pt between end1 and end2?
    static bool isBetween(const Base::Vector3d pt, const Base::Vector3d end1,
                          const Base::Vector3d end2);
    //! find intersection in 2d for 2 lines in point+direction form
    static Base::Vector3d Intersect2d(Base::Vector3d p1, Base::Vector3d d1, Base::Vector3d p2,
                                      Base::Vector3d d2);
    static Base::Vector2d Intersect2d(Base::Vector2d p1, Base::Vector2d d1, Base::Vector2d p2,
                                      Base::Vector2d d2);

    static Base::Vector3d toVector3d(const gp_Pnt gp)
    {
        return Base::Vector3d(gp.X(), gp.Y(), gp.Z());
    }
    static Base::Vector3d toVector3d(const gp_Dir gp)
    {
        return Base::Vector3d(gp.X(), gp.Y(), gp.Z());
    }
    static Base::Vector3d toVector3d(const gp_Vec gp)
    {
        return Base::Vector3d(gp.X(), gp.Y(), gp.Z());
    }
    static Base::Vector3d toVector3d(const QPointF gp)
    {
        return Base::Vector3d(gp.x(), gp.y(), 0.0);
    }

    static gp_Pnt togp_Pnt(const Base::Vector3d v)
    {
        return gp_Pnt(v.x, v.y, v.z);
    }
    static gp_Dir togp_Dir(const Base::Vector3d v)
    {
        return gp_Dir(v.x, v.y, v.z);
    }
    static gp_Vec togp_Vec(const Base::Vector3d v)
    {
        return gp_Vec(v.x, v.y, v.z);
    }
    static QPointF toQPointF(const Base::Vector3d v)
    {
        return QPointF(v.x, v.y);
    }

    static std::string shapeToString(TopoDS_Shape s);
    static TopoDS_Shape shapeFromString(std::string s);
    static Base::Vector3d invertY(Base::Vector3d v);
    static QPointF invertY(QPointF p);
    static std::vector<std::string> split(std::string csvLine);
    static std::vector<std::string> tokenize(std::string csvLine,
                                             std::string delimiter = ", $$$, ");
    static App::Color pyTupleToColor(PyObject* pColor);
    static PyObject* colorToPyTuple(App::Color color);
    static bool isCrazy(TopoDS_Edge e);
    static Base::Vector3d getFaceCenter(TopoDS_Face f);
    static bool circulation(Base::Vector3d A, Base::Vector3d B, Base::Vector3d C);
    static Base::Vector3d getTrianglePoint(Base::Vector3d p1, Base::Vector3d d, Base::Vector3d p2);
    static int countSubShapes(TopoDS_Shape shape, TopAbs_ShapeEnum subShape);
    static void encodeXmlSpecialChars(std::string& inoutText);
    static std::list<TopoDS_Edge> sort_Edges(double tol3d, std::list<TopoDS_Edge>& edges);

    // Supplementary mathematical functions
    static int sgn(double x);
    static double sqr(double x);
    static void angleNormalize(double& fi);
    static double angleComposition(double fi, double delta);
    static double angleDifference(double fi1, double fi2, bool reflex = false);

    // Interval marking functions
    static unsigned int intervalMerge(std::vector<std::pair<double, bool>>& marking,
                                      double boundary, bool wraps);
    static void intervalMarkLinear(std::vector<std::pair<double, bool>>& marking, double start,
                                   double length, bool value);
    static void intervalMarkCircular(std::vector<std::pair<double, bool>>& marking, double start,
                                     double length, bool value);

    // Supplementary 2D analytic geometry functions
    static int findRootForValue(double Ax2, double Bxy, double Cy2, double Dx, double Ey, double F,
                                double value, bool findX, double roots[]);
    static bool mergeBoundedPoint(const Base::Vector2d& point, const Base::BoundBox2d& boundary,
                                  std::vector<Base::Vector2d>& storage);

    static void findConicRectangleIntersections(double conicAx2, double conicBxy, double conicCy2,
                                                double conicDx, double conicEy, double conicF,
                                                const Base::BoundBox2d& rectangle,
                                                std::vector<Base::Vector2d>& intersections);
    static void findLineRectangleIntersections(const Base::Vector2d& linePoint, double lineAngle,
                                               const Base::BoundBox2d& rectangle,
                                               std::vector<Base::Vector2d>& intersections);
    static void findCircleRectangleIntersections(const Base::Vector2d& circleCenter,
                                                 double circleRadius,
                                                 const Base::BoundBox2d& rectangle,
                                                 std::vector<Base::Vector2d>& intersections);

    static void findLineSegmentRectangleIntersections(const Base::Vector2d& linePoint,
                                                      double lineAngle, double segmentBasePosition,
                                                      double segmentLength,
                                                      const Base::BoundBox2d& rectangle,
                                                      std::vector<Base::Vector2d>& intersections);
    static void findCircularArcRectangleIntersections(const Base::Vector2d& circleCenter,
                                                      double circleRadius, double arcBaseAngle,
                                                      double arcRotation,
                                                      const Base::BoundBox2d& rectangle,
                                                      std::vector<Base::Vector2d>& intersections);
    static void copyFile(std::string inSpec, std::string outSpec);

    static std::string translateArbitrary(std::string context, std::string baseName, std::string uniqueName);

    static bool isCosmeticVertex(App::DocumentObject* owner, std::string element);
    static bool isCosmeticEdge(App::DocumentObject* owner, std::string element);
    static bool isCenterLine(App::DocumentObject* owner, std::string element);

    //debugging routines
    static void dumpVertexes(const char* text, const TopoDS_Shape& s);
    static void dumpEdge(const char* label, int i, TopoDS_Edge e);
    static void dump1Vertex(const char* label, const TopoDS_Vertex& v);
    static void countFaces(const char* label, const TopoDS_Shape& s);
    static void countWires(const char* label, const TopoDS_Shape& s);
    static void countEdges(const char* label, const TopoDS_Shape& s);
    static const char* printBool(bool b);
    static QString qbaToDebug(const QByteArray& line);
    static void dumpCS(const char* text, const gp_Ax2& CS);
    static void dumpCS3(const char* text, const gp_Ax3& CS);
    static void dumpEdges(const char* text, const TopoDS_Shape& s);
};

}//end namespace TechDraw
#endif
