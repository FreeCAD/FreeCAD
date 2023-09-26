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

#include <memory>
#include <boost/uuid/uuid.hpp>

#include <Base/Reader.h>
#include <Base/Vector3D.h>
#include <Base/Writer.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>

namespace Part {
class TopoShape;
}

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

class BaseGeom;
using BaseGeomPtr = std::shared_ptr<BaseGeom>;
class Circle;
using CirclePtr = std::shared_ptr<Circle>;
class AOC;
using AOCPtr = std::shared_ptr<AOC>;
class Ellipse;
using EllipsePtr = std::shared_ptr<Ellipse>;
class AOE;
using AOEPtr = std::shared_ptr<AOE>;
class BezierSegment;
using BezierSegmentPtr = std::shared_ptr<BezierSegment>;
class BSpline;
using BSplinePtr = std::shared_ptr<BSpline>;
class Generic;
using GenericPtr = std::shared_ptr<Generic>;

class TechDrawExport BaseGeom : public std::enable_shared_from_this<BaseGeom>
{
    public:
        BaseGeom();
        //BaseGeom(BaseGeomPtr bg);   //do we need a copy constructor too?
        virtual ~BaseGeom() = default;

        virtual void Save(Base::Writer& w) const;
        virtual void Restore(Base::XMLReader& r);

        std::vector<Base::Vector3d> findEndPoints();
        Base::Vector3d getStartPoint();
        Base::Vector3d getEndPoint();
        Base::Vector3d getMidPoint();
        std::vector<Base::Vector3d> getQuads();
        double minDist(Base::Vector3d p);
        Base::Vector3d nearPoint(Base::Vector3d p);
        Base::Vector3d nearPoint(const BaseGeomPtr p);
        static BaseGeomPtr baseFactory(TopoDS_Edge edge);
        static bool validateEdge(TopoDS_Edge edge);
        bool closed();
        BaseGeomPtr copy();
        std::string dump();
        virtual std::string toString() const;
        std::vector<Base::Vector3d> intersection(TechDraw::BaseGeomPtr geom2);

        //Uniqueness
        boost::uuids::uuid getTag() const;
        virtual std::string getTagAsString() const;

        std::string geomTypeName();
        BaseGeomPtr inverted();

        // attribute setters and getters
        GeomType getGeomType() { return geomType; }
        void setGeomType(GeomType type) { geomType = type; }
        edgeClass getClassOfEdge() { return classOfEdge; }
        void setClassOfEdge(edgeClass newClass) { classOfEdge = newClass; }
        bool getHlrVisible() { return hlrVisible; }
        void setHlrVisible(bool state) { hlrVisible = state; }
        bool getReversed()  { return reversed; }
        void setReversed(bool state) { reversed = state; }
        int getRef3d()  { return ref3D; }
        void setRef3d(int ref)  { ref3D = ref; }
        TopoDS_Edge getOCCEdge()  { return occEdge; }
        void setOCCEdge(TopoDS_Edge newEdge)  { occEdge = newEdge; }
        bool getCosmetic()  { return cosmetic; }
        void setCosmetic (bool state)  { cosmetic = state; }
        int source() { return m_source; }
        void source(int s) { m_source = s; }
        int sourceIndex() { return m_sourceIndex; }
        void sourceIndex(int si) { m_sourceIndex = si; }
        std::string getCosmeticTag() { return cosmeticTag; }
        void setCosmeticTag(std::string t) { cosmeticTag = t; }
        Part::TopoShape asTopoShape(double scale);

        virtual double getStartAngle() { return 0.0; }
        virtual double getEndAngle() { return 0.0; }
        virtual bool clockwiseAngle() { return false; }
        virtual void clockwiseAngle(bool direction) { (void) direction; }

protected:
        void createNewTag();

        void intersectionLL(TechDraw::BaseGeomPtr geom1,
                            TechDraw::BaseGeomPtr geom2,
                            std::vector<Base::Vector3d>& interPoints);
        void intersectionCL(TechDraw::BaseGeomPtr geom1,
                            TechDraw::BaseGeomPtr geom2,
                            std::vector<Base::Vector3d>& interPoints);
        void intersectionCC(TechDraw::BaseGeomPtr geom1,
                            TechDraw::BaseGeomPtr geom2,
                            std::vector<Base::Vector3d>& interPoints);

        GeomType geomType;
        ExtractionType extractType;     //obs
        edgeClass classOfEdge;
        bool hlrVisible;
        bool reversed;
        int ref3D;                      //obs?
        TopoDS_Edge occEdge;            //projected Edge
        bool cosmetic;
        //TODO: all these attributes should be private
        int m_source;         //0 - geom, 1 - cosmetic edge, 2 - centerline
        int m_sourceIndex;
        std::string cosmeticTag;
        boost::uuids::uuid tag;

};
using BaseGeomPtrVector = std::vector<BaseGeomPtr>;    //new style

class TechDrawExport Circle: public BaseGeom
{
    public:
        Circle();
        explicit Circle(const TopoDS_Edge &e);
        Circle(Base::Vector3d center, double radius);
        ~Circle() override = default;

    public:
        std::string toString() const override;
        void Save(Base::Writer& w) const override;
        void Restore(Base::XMLReader& r) override;

        Base::Vector3d center;
        double radius;
};

class TechDrawExport Ellipse: public BaseGeom
{
    public:
    explicit Ellipse(const TopoDS_Edge &e);
    Ellipse(Base::Vector3d c, double mnr,  double mjr);
    ~Ellipse() override = default;

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
        explicit AOE(const TopoDS_Edge &e);
        ~AOE() override = default;

    public:
        double getStartAngle() override { return startAngle; }
        double getEndAngle() override { return endAngle; }
        bool clockwiseAngle() override { return cw; }
        void clockwiseAngle(bool direction) override  { cw = direction; }
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
        explicit AOC(const TopoDS_Edge &e);
        AOC(Base::Vector3d c, double r, double s, double e);
        AOC();
        ~AOC() override = default;

    public:
        double getStartAngle() override { return startAngle; }
        double getEndAngle() override { return endAngle; }
        bool clockwiseAngle() override { return cw; }
        void clockwiseAngle(bool direction) override  { cw = direction; }

        std::string toString() const override;
        void Save(Base::Writer& w) const override;
        void Restore(Base::XMLReader& r) override;

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
        bool intersectsArc(Base::Vector3d p1, Base::Vector3d p2);
        double distToArc(Base::Vector3d p);
};

class TechDrawExport BezierSegment: public BaseGeom
{
public:
    explicit BezierSegment(const TopoDS_Edge &e);
    BezierSegment() { poles = degree = 0; }
    ~BezierSegment() override = default;

    int poles;
    int degree;

    std::vector<Base::Vector3d> pnts;
};

class TechDrawExport BSpline: public BaseGeom
{
    public:
        explicit BSpline(const TopoDS_Edge &e);
        ~BSpline() override = default;

    public:
        double getStartAngle() override { return startAngle; }
        double getEndAngle() override { return endAngle; }

        Base::Vector3d startPnt;
        Base::Vector3d endPnt;
        Base::Vector3d midPnt;
        double startAngle;
        double endAngle;
        /// Arc is drawn clockwise from startAngle to endAngle if true, counterclockwise if false
        bool cw;
        bool isArc;

        bool isLine();
        bool isCircle();
        TopoDS_Edge asCircle(bool& isArc);
//        void getCircleParms(bool& isCircle, double& radius, Base::Vector3d& center, bool& isArc);
        bool intersectsArc(Base::Vector3d p1, Base::Vector3d p2);
        std::vector<BezierSegment> segments;
};

class TechDrawExport Generic: public BaseGeom
{
    public:
        explicit Generic(const TopoDS_Edge &e);
        Generic();
        ~Generic() override = default;

        std::string toString() const override;
        void Save(Base::Writer& w) const override;
        void Restore(Base::XMLReader& r) override;
        Base::Vector3d asVector();
        double slope();
        Base::Vector3d apparentInter(GenericPtr g);
        std::vector<Base::Vector3d> points;
};


/// Simple Collection of geometric features based on BaseGeom inherited classes in order
class TechDrawExport Wire
{
    public:
        Wire();
        explicit Wire(const TopoDS_Wire &w);
        ~Wire();

        TopoDS_Wire toOccWire() const;
        void dump(std::string s);
        BaseGeomPtrVector geoms;
};

/// Simple Collection of geometric features based on BaseGeom inherited classes in order
class TechDrawExport Face
{
    public:
        Face() = default;
        ~Face();
        TopoDS_Face toOccFace() const;
        std::vector<Wire *> wires;
};
using FacePtr = std::shared_ptr<Face>;

class TechDrawExport Vertex
{
    public:
        Vertex();
        explicit Vertex(const Vertex* v);
        Vertex(double x, double y);
        explicit Vertex(Base::Vector3d v);
        virtual ~Vertex()  = default;

        virtual void Save(Base::Writer &/*writer*/) const;
        virtual void Restore(Base::XMLReader &/*reader*/);
        virtual void dump(const char* title = "");

        bool isEqual(const Vertex& v, double tol);
        Base::Vector3d point() const { return Base::Vector3d(pnt.x, pnt.y, 0.0); }
        void point(Base::Vector3d v){ pnt = Base::Vector3d(v.x, v.y); }

        double x() {return pnt.x;}
        double y() {return pnt.y;}

        boost::uuids::uuid getTag() const;
        virtual std::string getTagAsString() const;

        // attribute setters and getters
        bool getHlrVisible() { return hlrVisible; }
        void setHlrVisible(bool state) { hlrVisible = state; }
        int getRef3d()  { return ref3D; }
        void setRef3d(int ref)  { ref3D = ref; }
        TopoDS_Vertex getOCCVertex()  { return occVertex; }
        void setOCCVertex(TopoDS_Vertex newVertex)  { occVertex = newVertex; }
        bool getCosmetic()  { return cosmetic; }
        void setCosmetic (bool state)  { cosmetic = state; }
        std::string getCosmeticTag() { return cosmeticTag; }
        void setCosmeticTag(std::string t) { cosmeticTag = t; }
        bool isCenter() {return m_center;}
        void isCenter(bool state) { m_center = state; }
        bool isReference() { return m_reference; }
        void isReference(bool state) { m_reference = state; }

        Part::TopoShape asTopoShape(double scale);

    protected:
        //Uniqueness
        void createNewTag();
        void assignTag(const TechDraw::Vertex* v);

        Base::Vector3d pnt;
        ExtractionType extractType;       //obs?
        bool hlrVisible;                 //visible according to HLR
        int ref3D;                        //obs. never used.
        bool m_center;
        TopoDS_Vertex occVertex;
        bool cosmetic;
        int cosmeticLink;                 //deprec. use cosmeticTag
        std::string cosmeticTag;
        bool m_reference;                   //reference vertex (ex robust dimension)

        boost::uuids::uuid tag;
};
using VertexPtr = std::shared_ptr<Vertex>;

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
         * returns index[1:geoms.size()), reversed [true, false]
         */
        static ReturnType nextGeom( Base::Vector3d atPoint,
                                    std::vector<TechDraw::BaseGeomPtr> geoms,
                                    std::vector<bool> used,
                                    double tolerance );

        //! return a vector of BaseGeomPtr's in tail to nose order
        static BaseGeomPtrVector chainGeoms(BaseGeomPtrVector geoms);
        static TopoDS_Edge edgeFromGeneric(TechDraw::GenericPtr g);
        static TopoDS_Edge edgeFromCircle(TechDraw::CirclePtr c);
        static TopoDS_Edge edgeFromCircleArc(TechDraw::AOCPtr c);

        static bool isCircle(TopoDS_Edge occEdge);
        static bool getCircleParms(TopoDS_Edge occEdge, double& radius, Base::Vector3d& center, bool& isArc);
        static TopoDS_Edge asCircle(TopoDS_Edge occEdge, bool& arc);
        static bool isLine(TopoDS_Edge occEdge);
};

} //end namespace TechDraw

#endif //TECHDRAW_GEOMETRY_H
