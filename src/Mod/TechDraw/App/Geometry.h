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
#include <Base/Reader.h>
#include <Base/Writer.h>

#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>

namespace TechDraw {

enum ExtractionType {               //obs
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
    BEZIER,
    BSPLINE,
    GENERIC
};

enum SourceType {
    GEOM,
    COSEDGE,
    CENTERLINE
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
        int ref3D;                      //obs?
        TopoDS_Edge occEdge;            //projected Edge
        bool cosmetic;
        int source(void) { return m_source; }
        void source(int s) { m_source = s; }
        int sourceIndex(void) { return m_sourceIndex; }
        void sourceIndex(int si) { m_sourceIndex = si; }

        virtual std::string toString(void) const;
/*        virtual bool fromCSV(std::string s);*/
        virtual void Save(Base::Writer& w) const;
        virtual void Restore(Base::XMLReader& r);
        std::vector<Base::Vector3d> findEndPoints();
        Base::Vector3d getStartPoint();
        Base::Vector3d getEndPoint();
        Base::Vector3d getMidPoint();
        std::vector<Base::Vector3d> getQuads();
        double minDist(Base::Vector3d p);
        Base::Vector3d nearPoint(Base::Vector3d p);
        Base::Vector3d nearPoint(const BaseGeom* p);
        static BaseGeom* baseFactory(TopoDS_Edge edge);
        bool closed(void);
        BaseGeom* copy();
        std::string dump();

    protected:
        int m_source;         //0 - geom, 1 - cosmetic edge, 2 - centerline
        int m_sourceIndex;

};

typedef std::vector<BaseGeom *> BaseGeomPtrVector;        //obs?

class TechDrawExport Circle: public BaseGeom
{
    public:
        Circle(const TopoDS_Edge &e);
        Circle(void);
        ~Circle() = default;

    public:
        virtual std::string toString(void) const override;
/*        virtual bool fromCSV(std::string s) override;*/
        virtual void Save(Base::Writer& w) const override;
        virtual void Restore(Base::XMLReader& r) override;

        Base::Vector3d center;
        double radius;
};

class TechDrawExport Ellipse: public BaseGeom
{
    public:
        Ellipse(const TopoDS_Edge &e);
        ~Ellipse() = default;

    public:
        Base::Vector3d center;
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
        Base::Vector3d startPnt;  //TODO: The points are used for drawing, the angles for bounding box calcs - seems redundant
        Base::Vector3d endPnt;
        Base::Vector3d midPnt;

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
        AOC(void);
        ~AOC() = default;

    public:
        virtual std::string toString(void) const override;
/*        virtual bool fromCSV(std::string s) override;*/
        virtual void Save(Base::Writer& w) const override;
        virtual void Restore(Base::XMLReader& r) override;

        Base::Vector3d startPnt;
        Base::Vector3d endPnt;
        Base::Vector3d midPnt;

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

class TechDrawExport BezierSegment: public BaseGeom
{
public:
    BezierSegment(const TopoDS_Edge &e);
    BezierSegment() { poles = degree = 0; }
    ~BezierSegment() = default;

    int poles;
    int degree;

    //Base::Vector3d pnts[4];
    std::vector<Base::Vector3d> pnts;
};

class TechDrawExport BSpline: public BaseGeom
{
    public:
        BSpline(const TopoDS_Edge &e);
        ~BSpline() = default;

    public:
        Base::Vector3d startPnt;
        Base::Vector3d endPnt;
        Base::Vector3d midPnt;
        double startAngle;
        double endAngle;
        /// Arc is drawn clockwise from startAngle to endAngle if true, counterclockwise if false
        bool cw;
        bool isArc;

        bool isLine(void);
        bool isCircle(void);
        TopoDS_Edge asCircle(bool& isArc);
        void getCircleParms(bool& isCircle, double& radius, Base::Vector3d& center, bool& isArc);
        bool intersectsArc(Base::Vector3d p1,Base::Vector3d p2);
        std::vector<BezierSegment> segments;
};

class TechDrawExport Generic: public BaseGeom
{
    public:
        Generic(const TopoDS_Edge &e);
        Generic();
        ~Generic() = default;

        virtual std::string toString(void) const override;
/*        virtual bool fromCSV(std::string s) override;*/
        virtual void Save(Base::Writer& w) const override;
        virtual void Restore(Base::XMLReader& r) override;
        Base::Vector3d asVector(void);
        double slope(void);
        Base::Vector3d apparentInter(Generic* g);
        std::vector<Base::Vector3d> points;
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

class TechDrawExport Vertex
{
    public:
        Vertex();
        Vertex(const Vertex* v);
        Vertex(double x, double y);
        Vertex(Base::Vector3d v) : Vertex(v.x,v.y) {}
        virtual ~Vertex() {}

        virtual void Save(Base::Writer &/*writer*/) const;
        virtual void Restore(Base::XMLReader &/*reader*/);

        Base::Vector3d pnt;
        ExtractionType extractType;       //obs?
        bool visible;
        int ref3D;                        //obs. never used.
        bool isCenter;
        TopoDS_Vertex occVertex;
        bool isEqual(Vertex* v, double tol);
        Base::Vector3d point(void) const { return Base::Vector3d(pnt.x,pnt.y,0.0); }
        void point(Base::Vector3d v){ pnt = Base::Vector3d(v.x, v.y); }
        bool cosmetic;
        int cosmeticLink;

        double x() {return pnt.x;}
        double y() {return pnt.y;}
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
        static ReturnType nextGeom( Base::Vector3d atPoint,
                                    std::vector<TechDraw::BaseGeom*> geoms,
                                    std::vector<bool> used,
                                    double tolerance );

        //! return a vector of BaseGeom*'s in tail to nose order
        static std::vector<BaseGeom*> chainGeoms(std::vector<BaseGeom*> geoms);
        static TopoDS_Edge edgeFromGeneric(TechDraw::Generic* g);
        static TopoDS_Edge edgeFromCircle(TechDraw::Circle* c);
        static TopoDS_Edge edgeFromCircleArc(TechDraw::AOC* c);
};

} //end namespace TechDraw

#endif //TECHDRAW_GEOMETRY_H
