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
#include <Base/Vector3D.h>

#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>

namespace TechDrawGeometry {

enum ExtractionType {               //obs sb vis/hid + hard/smooth/seam/out(edgeClass?)
    Plain,
    WithHidden,
    WithSmooth
};

enum edgeClass {
    ecNONE,  // Not used, OCC index starts at 1
    ecUVISO,
    ecOUTLINE,
    ecSMOOTH,
    ecSEAM,
    ecHARD
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
        virtual ~BaseGeom() = default;

    public:
        GeomType geomType;
        ExtractionType extractType;     //obs
        edgeClass classOfEdge;
        bool visible;
        bool reversed;
        int ref3D;
        TopoDS_Edge occEdge;            //projected Edge

        std::vector<Base::Vector2D> findEndPoints();
        Base::Vector2D getStartPoint();
        Base::Vector2D getEndPoint();
        static BaseGeom* baseFactory(TopoDS_Edge edge);
};

typedef std::vector<BaseGeom *> BaseGeomPtrVector;

class TechDrawExport Circle: public BaseGeom
{
    public:
        Circle(const TopoDS_Edge &e);
        ~Circle() = default;

    public:
        Base::Vector2D center;
        double radius;
};

class TechDrawExport Ellipse: public BaseGeom
{
    public:
        Ellipse(const TopoDS_Edge &e);
        ~Ellipse() = default;

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
        AOE(const TopoDS_Edge &e);
        ~AOE() = default;

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
        AOC(const TopoDS_Edge &e);
        ~AOC() = default;

    public:
        Base::Vector2D startPnt;
        Base::Vector2D endPnt;
        Base::Vector2D midPnt;

        /// Angle in radian  ??angle with horizontal?
        double startAngle;

        /// Angle in radian
        double endAngle;

        /// Arc is drawn clockwise from startAngle to endAngle if true, counterclockwise if false
        bool cw;  // TODO: Instead of this (and similar one in AOE), why not reorder startAngle and endAngle?
        bool largeArc;

        bool isOnArc(Base::Vector3d v);
        bool intersectsArc(Base::Vector3d p1,Base::Vector3d p2);
        double distToArc(Base::Vector3d p);
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
        BSpline(const TopoDS_Edge &e);
        ~BSpline() = default;

    public:
        bool isLine(void);
        std::vector<BezierSegment> segments;
};

class TechDrawExport Generic: public BaseGeom
{
    public:
        Generic(const TopoDS_Edge &e);
        Generic();
        ~Generic() = default;

       std::vector<Base::Vector2D> points;
};


/// Simple Collection of geometric features based on BaseGeom inherited classes in order
class TechDrawExport Wire
{
    public:
        Wire();
        Wire(const TopoDS_Wire &w);
        ~Wire();

        std::vector<BaseGeom *> geoms;
};

/// Simple Collection of geometric features based on BaseGeom inherited classes in order
class TechDrawExport Face
{
    public:
        Face() = default;
        ~Face();

        std::vector<Wire *> wires;
};

//! 2D Vertex
class TechDrawExport Vertex
{
    public:
        Vertex(double x, double y);
        Vertex(Base::Vector2D v) : Vertex(v.fX,v.fY) {}
        ~Vertex() = default;

        Base::Vector2D pnt;
        ExtractionType extractType;       //obs?
        bool visible;
        int ref3D;                        //obs. never used.
        bool isCenter;
        TopoDS_Vertex occVertex;
        bool isEqual(Vertex* v, double tol);
        Base::Vector3d getAs3D(void) {return Base::Vector3d(pnt.fX,pnt.fY,0.0);}
        double x() {return pnt.fX;}
        double y() {return pnt.fY;}
};

/// Encapsulates some useful static methods
class TechDrawExport GeometryUtils
{
    public:
        /// Used by nextGeom()
        struct TechDrawExport ReturnType {
            unsigned int index;
            bool reversed;
            explicit ReturnType(int i = 0, bool r = false) :
                index(i),
                reversed(r)
                {}
        };

        /// Find an unused geom starts or ends at atPoint.
        /*!
         * returns index[1:geoms.size()),reversed [true,false]
         */
        static ReturnType nextGeom( Base::Vector2D atPoint,
                                    std::vector<TechDrawGeometry::BaseGeom*> geoms,
                                    std::vector<bool> used,
                                    double tolerance );

        //! return a vector of BaseGeom*'s in tail to nose order
        static std::vector<BaseGeom*> chainGeoms(std::vector<BaseGeom*> geoms);
};

} //end namespace TechDrawGeometry

#endif //TECHDRAW_GEOMETRY_H
