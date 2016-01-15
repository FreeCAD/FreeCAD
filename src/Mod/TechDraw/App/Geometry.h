/***************************************************************************
 *   Copyright (c) 2012 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#ifndef TECHDRAW_GEOMETRY_H
#define TECHDRAW_GEOMETRY_H

#include <Base/Tools2D.h>
class BRepAdaptor_Curve;

namespace TechDrawGeometry {

enum ExtractionType {
      Plain = 0,
      WithHidden = 1,
      WithSmooth = 2
};

enum GeomType {
    NOTDEF,
    CIRCLE,
    ARCOFCIRCLE,
    ELLIPSE,
    ARCOFELLIPSE,
    BSPLINE,
    GENERIC
};

class TechDrawExport BaseGeom
{
public:
   BaseGeom();
   ~BaseGeom() {}
public:
    GeomType geomType;
    ExtractionType extractType;
    bool reversed;
    std::vector<Base::Vector2D> findEndPoints();
    Base::Vector2D getStartPoint();
    Base::Vector2D getEndPoint();
};

class TechDrawExport Circle: public BaseGeom
{
public:
  Circle(const BRepAdaptor_Curve &c);
  Circle();
  ~Circle() {}
public:
  Base::Vector2D center;
  double radius;
};

class TechDrawExport Ellipse: public BaseGeom
{
public:
  Ellipse(const BRepAdaptor_Curve &c);
  Ellipse();
  ~Ellipse() {}
public:
  Base::Vector2D center;
  double minor;
  double major;
  /// Angle between the major axis of the ellipse and the X axis, in radian
  double angle;
};

class TechDrawExport AOE: public Ellipse
{
public:
  AOE(const BRepAdaptor_Curve &c);
  AOE();
  ~AOE() {}
public:
  Base::Vector2D startPnt;  //TODO: The points are used for drawing, the angles for bounding box calcs - seems redundant
  Base::Vector2D endPnt;
  Base::Vector2D midPnt;
  /// Angle in radian
  double startAngle;
  /// Angle in radian
  double endAngle;
  /// Arc is drawn clockwise from startAngle to endAngle if true, counterclockwise if false
  bool cw;
  bool largeArc;
};

class TechDrawExport AOC: public Circle
{
public:
  AOC(const BRepAdaptor_Curve &c);
  AOC();
  ~AOC() {}
public:
  Base::Vector2D startPnt;
  Base::Vector2D endPnt;
  Base::Vector2D midPnt;
  /// Angle in radian
  double startAngle;
  /// Angle in radian
  double endAngle;
  /// Arc is drawn clockwise from startAngle to endAngle if true, counterclockwise if false
  bool cw;  // TODO: Instead of this (and similar one in AOE), why not reorder startAngle and endAngle?
  bool largeArc;
};

/// Handles degree 1 to 3 Bezier segments
/*!
 * \todo extend this to higher orders if necessary
 */
struct BezierSegment
{
    /// Number of entries in pnts that are valid
    int poles;
    /// Control points for this segment
    /*!
     * Note that first and last used points define the endpoints for this
     * segment, so when we know that a sequence of BezierSegment objects are
     * going to be strung together, then we only need to know the start of
     * the first element (or the end of the last element).
     */
    Base::Vector2D pnts[4];
};

class TechDrawExport BSpline: public BaseGeom
{
public:
  BSpline(const BRepAdaptor_Curve &c);
  BSpline();
  ~BSpline(){}
public:
  bool isLine(void);
  std::vector<BezierSegment> segments;
};

class Generic: public BaseGeom
{
public:
  Generic(Base::Vector2D start, Base::Vector2D end);
  Generic(const BRepAdaptor_Curve& c);
  Generic();
  ~Generic() {}
  std::vector<Base::Vector2D> points;

};

/// Simple Collection of geometric features based on BaseGeom inherited classes in order
struct TechDrawExport Wire
{
  Wire();
  ~Wire();
  std::vector<BaseGeom *> geoms;
};

/// Simple Collection of geometric features based on BaseGeom inherited classes in order
struct TechDrawExport Face
{
  Face();
  ~Face();
  std::vector<Wire *> wires;
};

/// Simple vertex
struct TechDrawExport Vertex
{
  Vertex(double x, double y) { pnt = Base::Vector2D(x, y); }
  Vertex(Base::Vector2D v) { pnt = v; }
  ~Vertex() {}
  Base::Vector2D pnt;
  ExtractionType extractType;
};

//*** utility functions
extern "C" {

struct TechDrawExport getNextReturn {
    unsigned int index;
    bool reversed;
    explicit getNextReturn(int i = 0, bool r = false) :
        index(i),
        reversed(r)
        {}
};

std::vector<TechDrawGeometry::BaseGeom*> chainGeoms(std::vector<TechDrawGeometry::BaseGeom*> geoms);
getNextReturn nextGeom(Base::Vector2D atPoint,
                              std::vector<TechDrawGeometry::BaseGeom*> geoms,
                              std::vector<bool> used,
                              double tolerance);

} //end extern "C"
} //end namespace TechDrawGeometry

#endif //TECHDRAW_GEOMETRY_H
