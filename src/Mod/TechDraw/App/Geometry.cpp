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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <cmath>
# include <boost/uuid/uuid_generators.hpp>
# include <boost/uuid/uuid_io.hpp>

# include <Approx_Curve3d.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepLib.hxx>
# include <BRepLProp_CLProps.hxx>
# include <BRepTools.hxx>
# include <GC_MakeArcOfCircle.hxx>
# include <GC_MakeEllipse.hxx>
# include <gce_MakeCirc.hxx>
# include <GCPnts_AbscissaPoint.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Geom_BezierCurve.hxx>
# include <Geom_Circle.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <GeomAPI_PointsToBSpline.hxx>
# include <GeomAPI_ProjectPointOnCurve.hxx>
# include <GeomConvert_BSplineCurveToBezierCurve.hxx>
# include <GeomLProp_CLProps.hxx>
# include <gp_Ax2.hxx>
# include <gp_Circ.hxx>
# include <gp_Dir.hxx>
# include <gp_Elips.hxx>
# include <gp_Pnt.hxx>
# include <gp_Vec.hxx>
# include <Poly_Polygon3D.hxx>
# include <Precision.hxx>
# include <Standard_Version.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
#if OCC_VERSION_HEX < 0x070600
# include <BRepAdaptor_HCurve.hxx>
#endif
#endif  // #ifndef _PreComp_

#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>

#include "Geometry.h"
#include "GeometryObject.h"
#include "DrawUtil.h"


using namespace TechDraw;
using namespace Part;
using namespace std;
using DU = DrawUtil;

#if OCC_VERSION_HEX >= 0x070600
using BRepAdaptor_HCurve = BRepAdaptor_Curve;
#endif

#define GEOMETRYEDGE 0
#define COSMETICEDGE 1
#define CENTERLINE   2

// Collection of Geometric Features
Wire::Wire()
{
}

Wire::Wire(const TopoDS_Wire &w)
{
    TopExp_Explorer edges(w, TopAbs_EDGE);
    for (; edges.More(); edges.Next()) {
        const auto edge( TopoDS::Edge(edges.Current()) );
        BaseGeomPtr bg = BaseGeom::baseFactory(edge);
        if (bg) {
            geoms.push_back(bg);
        }
    }
}

Wire::~Wire()
{
    //shared_ptr to geoms should free memory when ref count goes to zero
    geoms.clear();
}

TopoDS_Wire Wire::toOccWire() const
{
    BRepBuilderAPI_MakeWire mkWire;
    for (auto& g: geoms) {
        TopoDS_Edge e = g->getOCCEdge();
        mkWire.Add(e);
    }
    if (mkWire.IsDone())  {
        return mkWire.Wire();
    }
//    BRepTools::Write(result, "toOccWire.brep");
    return TopoDS_Wire();
}

void Wire::dump(std::string s)
{
    BRepTools::Write(toOccWire(), s.c_str());            //debug
}

TopoDS_Face Face::toOccFace() const
{
    TopoDS_Face result;
    //if (!wires.empty) {
    BRepBuilderAPI_MakeFace mkFace(wires.front()->toOccWire(), true);
    int limit = wires.size();
    int iwire = 1;
    for ( ; iwire < limit; iwire++) {
//        w->dump("wireInToFace.brep");
        TopoDS_Wire wOCC = wires.at(iwire)->toOccWire();
        if(!wOCC.IsNull())  {
            mkFace.Add(wOCC);
        }
    }
    if (mkFace.IsDone())  {
        return mkFace.Face();
    }
    return TopoDS_Face();
}

Face::~Face()
{
    for(auto it : wires) {
        delete it;
    }
    wires.clear();
}

BaseGeom::BaseGeom() :
    geomType(NOTDEF),
    extractType(Plain),             //obs
    classOfEdge(ecNONE),
    hlrVisible(true),
    reversed(false),
    ref3D(-1),                      //obs?
    cosmetic(false),
    m_source(0),
    m_sourceIndex(-1)
{
    occEdge = TopoDS_Edge();
    cosmeticTag = std::string();
    tag = boost::uuids::nil_uuid();
}

BaseGeomPtr BaseGeom::copy()
{
    BaseGeomPtr result;
    if (!occEdge.IsNull()) {
        result = baseFactory(occEdge);
        if (!result) {
            result = std::make_shared<BaseGeom>();
        }
    }

    result->extractType = extractType;
    result->classOfEdge = classOfEdge;
    result->setHlrVisible( hlrVisible);
    result->reversed = reversed;
    result->ref3D = ref3D;
    result->cosmetic = cosmetic;
    result->source(m_source);
    result->sourceIndex(m_sourceIndex);
    result->cosmeticTag = cosmeticTag;

    return result;
}

std::string BaseGeom::toString() const
{
    std::stringstream ss;
    ss << geomType << ", " <<
          extractType << ", " <<
          classOfEdge << ", " <<
          hlrVisible << ", " <<
          reversed << ", " <<
          ref3D << ", " <<
          cosmetic << ", " <<
          m_source << ", " <<
          m_sourceIndex;
    return ss.str();
}

boost::uuids::uuid BaseGeom::getTag() const
{
    return tag;
}

std::string BaseGeom::getTagAsString() const
{
    return boost::uuids::to_string(getTag());
}

void BaseGeom::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<GeomType value=\"" << geomType << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<ExtractType value=\"" << extractType << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<EdgeClass value=\"" << classOfEdge << "\"/>" << endl;
    const char v = hlrVisible?'1':'0';
    writer.Stream() << writer.ind() << "<HLRVisible value=\"" <<  v << "\"/>" << endl;
    const char r = reversed?'1':'0';
    writer.Stream() << writer.ind() << "<Reversed value=\"" << r << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Ref3D value=\"" << ref3D << "\"/>" << endl;
    const char c = cosmetic?'1':'0';
    writer.Stream() << writer.ind() << "<Cosmetic value=\"" << c << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Source value=\"" << m_source << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<SourceIndex value=\"" << m_sourceIndex << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<CosmeticTag value=\"" <<  cosmeticTag << "\"/>" << endl;
//    writer.Stream() << writer.ind() << "<Tag value=\"" <<  getTagAsString() << "\"/>" << endl;
}

void BaseGeom::Restore(Base::XMLReader &reader)
{
    reader.readElement("GeomType");
    geomType = static_cast<TechDraw::GeomType>(reader.getAttributeAsInteger("value"));
    reader.readElement("ExtractType");
    extractType = static_cast<TechDraw::ExtractionType>(reader.getAttributeAsInteger("value"));
    reader.readElement("EdgeClass");
    classOfEdge = static_cast<TechDraw::edgeClass>(reader.getAttributeAsInteger("value"));
    reader.readElement("HLRVisible");
    hlrVisible = reader.getAttributeAsInteger("value") != 0;
    reader.readElement("Reversed");
    reversed = reader.getAttributeAsInteger("value") != 0;
    reader.readElement("Ref3D");
    ref3D = reader.getAttributeAsInteger("value");
    reader.readElement("Cosmetic");
    cosmetic = reader.getAttributeAsInteger("value") != 0;
    reader.readElement("Source");
    m_source = reader.getAttributeAsInteger("value");
    reader.readElement("SourceIndex");
    m_sourceIndex = reader.getAttributeAsInteger("value");
    reader.readElement("CosmeticTag");
    cosmeticTag = reader.getAttribute("value");
}

std::vector<Base::Vector3d> BaseGeom::findEndPoints()
{
    std::vector<Base::Vector3d> result;

    if (!occEdge.IsNull()) {
        gp_Pnt p = BRep_Tool::Pnt(TopExp::FirstVertex(occEdge));
        result.emplace_back(p.X(), p.Y(), p.Z());
        p = BRep_Tool::Pnt(TopExp::LastVertex(occEdge));
        result.emplace_back(p.X(), p.Y(), p.Z());
    } else {
        //TODO: this should throw something
        Base::Console().Message("Geometry::findEndPoints - OCC edge not found\n");
        throw Base::RuntimeError("no OCC edge in Geometry::findEndPoints");
    }
    return result;
}


Base::Vector3d BaseGeom::getStartPoint()
{
    std::vector<Base::Vector3d> verts = findEndPoints();
    if (!verts.empty()) {
        return verts[0];
    } else {
        //TODO: this should throw something
        Base::Console().Message("Geometry::getStartPoint - start point not found!\n");
        Base::Vector3d badResult(0.0, 0.0, 0.0);
        return badResult;
    }
}


Base::Vector3d BaseGeom::getEndPoint()
{
    std::vector<Base::Vector3d> verts = findEndPoints();

    if (verts.size() != 2) {
        //TODO: this should throw something
        Base::Console().Message("Geometry::getEndPoint - end point not found!\n");
        Base::Vector3d badResult(0.0, 0.0, 0.0);
        return badResult;
    }
    return verts[1];
}

Base::Vector3d BaseGeom::getMidPoint()
{
    // Midpoint calculation - additional details here: https://forum.freecad.org/viewtopic.php?f=35&t=59582

    BRepAdaptor_Curve curve(occEdge);

    // As default, set the midpoint curve parameter value by simply averaging start point and end point values
    double midParam = (curve.FirstParameter() + curve.LastParameter())/2.0;

    // GCPnts_AbscissaPoint allows us to compute the parameter value depending on the distance along a curve.
    // In this case we want the curve parameter value for the half of the whole curve length,
    // thus GCPnts_AbscissaPoint::Length(curve)/2 is the distance to go from the start point.
    GCPnts_AbscissaPoint abscissa(Precision::Confusion(), curve, GCPnts_AbscissaPoint::Length(curve)/2.0,
                                  curve.FirstParameter());
    if (abscissa.IsDone()) {
        // The computation was successful - otherwise keep the average, it is better than nothing
        midParam = abscissa.Parameter();
    }

    // Now compute coordinates of the point on curve for curve parameter value equal to midParam
    BRepLProp_CLProps props(curve, midParam, 0, Precision::Confusion());
    const gp_Pnt &point = props.Value();

    return Base::Vector3d(point.X(), point.Y(), point.Z());
}

std::vector<Base::Vector3d> BaseGeom::getQuads()
{
    std::vector<Base::Vector3d> result;
    BRepAdaptor_Curve adapt(occEdge);
    double u = adapt.FirstParameter();
    double v = adapt.LastParameter();
    double range = v - u;
    double q1 = u + (range / 4.0);
    double q2 = u + (range / 2.0);
    double q3 = u + (3.0 * range / 4.0);
    BRepLProp_CLProps prop(adapt, q1, 0, Precision::Confusion());
    const gp_Pnt& p1 = prop.Value();
    result.emplace_back(p1.X(), p1.Y(), 0.0);
    prop.SetParameter(q2);
    const gp_Pnt& p2 = prop.Value();
    result.emplace_back(p2.X(), p2.Y(), 0.0);
    prop.SetParameter(q3);
    const gp_Pnt& p3 = prop.Value();
    result.emplace_back(p3.X(), p3.Y(), 0.0);
    return result;
}

double BaseGeom::minDist(Base::Vector3d p)
{
    gp_Pnt pnt(p.x, p.y,0.0);
    TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(pnt);
    return TechDraw::DrawUtil::simpleMinDist(occEdge, v);
}

//!find point on me nearest to p
Base::Vector3d BaseGeom::nearPoint(const BaseGeomPtr p)
{
    TopoDS_Edge pEdge = p->getOCCEdge();
    BRepExtrema_DistShapeShape extss(occEdge, pEdge);
    if (!extss.IsDone() || extss.NbSolution() == 0) {
        return Base::Vector3d(0.0, 0.0, 0.0);
    }
    gp_Pnt p1 = extss.PointOnShape1(1);
    return Base::Vector3d(p1.X(), p1.Y(), 0.0);
}

Base::Vector3d BaseGeom::nearPoint(Base::Vector3d p)
{
    gp_Pnt pnt(p.x, p.y,0.0);
    TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(pnt);
    BRepExtrema_DistShapeShape extss(occEdge, v);
    if (!extss.IsDone() || extss.NbSolution() == 0) {
        return Base::Vector3d(0.0, 0.0, 0.0);
    }
    gp_Pnt p1 = extss.PointOnShape1(1);
    return Base::Vector3d(p1.X(), p1.Y(), 0.0);
}

std::string BaseGeom::dump()
{
    Base::Vector3d start = getStartPoint();
    Base::Vector3d end   = getEndPoint();
    std::stringstream ss;
    ss << "BaseGeom: s:(" << start.x << ", " << start.y << ") e:(" << end.x << ", " << end.y << ") ";
    ss << "type: " << geomType << " class: " << classOfEdge << " viz: " << hlrVisible << " rev: " << reversed;
    ss << "cosmetic: " << cosmetic << " source: " << source() << " iSource: " << sourceIndex();
    return ss.str();
}

bool BaseGeom::closed()
{
    Base::Vector3d start(getStartPoint().x,
                         getStartPoint().y,
                         0.0);
    Base::Vector3d end(getEndPoint().x,
                       getEndPoint().y,
                       0.0);
    if (start.IsEqual(end, 0.00001)) {
        return true;
    }
    return false;
}

// return a BaseGeom similar to this, but inverted with respect to Y axis
BaseGeomPtr BaseGeom::inverted()
{
//    Base::Console().Message("BG::inverted()\n");
    TopoDS_Shape invertedShape = ShapeUtils::invertGeometry(occEdge);
    TopoDS_Edge invertedEdge = TopoDS::Edge(invertedShape);
    return baseFactory(invertedEdge);
}

//keep this in sync with enum GeomType
std::string BaseGeom::geomTypeName()
{
    std::vector<std::string> typeNames {
        "NotDefined",
        "Circle",
        "ArcOfCircle",
        "Ellipse",
        "ArcOfEllipse",
        "Bezier",
        "BSpline",
        "Line",         //why was this ever called "Generic"?
        "Unknown" } ;
    if (geomType >= typeNames.size()) {
        return "Unknown";
    }
    return typeNames.at(geomType);
}

//! Convert 1 OCC edge into 1 BaseGeom (static factory method)
BaseGeomPtr BaseGeom::baseFactory(TopoDS_Edge edge)
{
    if (edge.IsNull()) {
        Base::Console().Message("BG::baseFactory - input edge is NULL \n");
    }
    //weed out rubbish edges before making geometry
    if (!validateEdge(edge)) {
        return nullptr;
    }

    BaseGeomPtr result = std::make_shared<Generic> (edge);

    BRepAdaptor_Curve adapt(edge);
    switch(adapt.GetType()) {
      case GeomAbs_Circle: {
        double f = adapt.FirstParameter();
        double l = adapt.LastParameter();
        gp_Pnt s = adapt.Value(f);
        gp_Pnt e = adapt.Value(l);

        //don't understand this test.
        //if first to last is > 1 radian? are circles parameterize by rotation angle?
        //if start and end points are close?
        if (fabs(l-f) > 1.0 && s.SquareDistance(e) < 0.001) {
              result = std::make_shared<Circle>(edge);
        } else {
              result = std::make_shared<AOC>(edge);
        }
      } break;
      case GeomAbs_Ellipse: {
        double f = adapt.FirstParameter();
        double l = adapt.LastParameter();
        gp_Pnt s = adapt.Value(f);
        gp_Pnt e = adapt.Value(l);
        if (fabs(l-f) > 1.0 && s.SquareDistance(e) < 0.001) {
              result = std::make_shared<Ellipse>(edge);
        } else {
              result = std::make_shared<AOE>(edge);
        }
      } break;
      case GeomAbs_BezierCurve: {
          Handle(Geom_BezierCurve) bez = adapt.Bezier();
          //if (bez->Degree() < 4) {
          result = std::make_shared<BezierSegment>(edge);
          if (edge.Orientation() == TopAbs_REVERSED) {
              result->reversed = true;
          }

          //    OCC is quite happy with Degree > 3 but QtGui handles only 2, 3
      } break;
      case GeomAbs_BSplineCurve: {
        TopoDS_Edge circEdge;

        bool isArc = false;
        try {
            BSplinePtr bspline = std::make_shared<BSpline>(edge);
            if (bspline->isLine()) {
                result = std::make_shared<Generic>(edge);
            } else {
                circEdge = bspline->asCircle(isArc);
                if (!circEdge.IsNull()) {
                    if (isArc) {
                        result = std::make_shared<AOC>(circEdge);
                    } else {
                        result = std::make_shared<Circle>(circEdge);
                    }
                } else {
//                    Base::Console().Message("Geom::baseFactory - circEdge is Null\n");
                    result = bspline;
                }
            }
            break;
        }
        catch (const Standard_Failure& e) {
            Base::Console().Error("Geom::baseFactory - OCC error - %s - while making spline\n",
                              e.GetMessageString());
            break;
        }
        catch (...) {
            Base::Console().Error("Geom::baseFactory - unknown error occurred while making spline\n");
            break;
        } break;
      } // end bspline case
      default: {
        result = std::make_unique<Generic>(edge);
      }  break;
    }

    return result;
}

bool BaseGeom::validateEdge(TopoDS_Edge edge)
{
    return !DrawUtil::isCrazy(edge);
}

std::vector<Base::Vector3d> BaseGeom::intersection(TechDraw::BaseGeomPtr geom2)
{
    // find intersection vertex(es) between two edges
    // permitted are: line, circle or arc of circle
    // call: interPoints = line1.intersection(line2);
    # define unknown 0
    # define isGeneric 1
    # define isArcOrCircle 2
    // we check the type of the two objects
    int edge1(unknown), edge2(unknown);
    if (this->getGeomType() == TechDraw::CIRCLE ||
        this->getGeomType() == TechDraw::ARCOFCIRCLE)
        edge1 = isArcOrCircle;
    else if (this->getGeomType() == TechDraw::GENERIC)
        edge1 = isGeneric;
    if (geom2->getGeomType() == TechDraw::CIRCLE ||
        geom2->getGeomType() == TechDraw::ARCOFCIRCLE)
        edge2 = isArcOrCircle;
    else if (geom2->getGeomType() == TechDraw::GENERIC)
        edge2 = isGeneric;
    // we calculate the intersections
    std::vector<Base::Vector3d> interPoints;
    if (edge1 == isGeneric && edge2 == isGeneric)
        intersectionLL(shared_from_this(), geom2, interPoints);
    else if (edge1 == isArcOrCircle && edge2 == isGeneric)
        intersectionCL(shared_from_this(), geom2, interPoints);
    else if (edge1 == isGeneric && edge2 == isArcOrCircle)
        intersectionCL(geom2, shared_from_this(), interPoints);
    else if (edge1 == isArcOrCircle && edge2 == isArcOrCircle)
        intersectionCC(shared_from_this(), geom2, interPoints);
    return interPoints;
}

void BaseGeom::intersectionLL(TechDraw::BaseGeomPtr geom1,
                              TechDraw::BaseGeomPtr geom2,
                              std::vector<Base::Vector3d>& interPoints)
{
    // find intersection vertex of two lines
    // Taken from: <http://de.wikipedia.org/wiki/Schnittpunkt>
    TechDraw::GenericPtr gen1 = std::static_pointer_cast<TechDraw::Generic>(geom1);
    TechDraw::GenericPtr gen2 = std::static_pointer_cast<TechDraw::Generic>(geom2);
    // we calculate vectors to start points and direction vectors
    Base::Vector3d startPnt1 = gen1->points.at(0);
    Base::Vector3d endPnt1 = gen1->points.at(1);
    Base::Vector3d startPnt2 = gen2->points.at(0);
    Base::Vector3d endPnt2 = gen2->points.at(1);
    Base::Vector3d dir1 = endPnt1 - startPnt1;
    Base::Vector3d dir2 = endPnt2 - startPnt2;
    // we create equations a*x+b*y+c=0 for both lines
    float a1 = -dir1.y;
    float b1 = dir1.x;
    float c1 = -startPnt1.x * dir1.y + startPnt1.y * dir1.x;
    float a2 = -dir2.y;
    float b2 = dir2.x;
    float c2 = -startPnt2.x * dir2.y + startPnt2.y * dir2.x;
    float denom = a1 * b2 - a2 * b1;
    if (abs(denom) >= 0.01)
    // lines not (nearly) parallel, we calculate intersections
    {
        float xIntersect = (c1 * b2 - c2 * b1) / denom;
        float yIntersect = (a1 * c2 - a2 * c1) / denom;
        yIntersect = -yIntersect;
        Base::Vector3d interPoint(xIntersect, yIntersect, 0.0);
        interPoints.push_back(interPoint);
    }
}

void BaseGeom::intersectionCL(TechDraw::BaseGeomPtr geom1,
                              TechDraw::BaseGeomPtr geom2,
                              std::vector<Base::Vector3d>& interPoints)
{
    // find intersection vertex(es) between one circle and one line
    // Taken from: <http://de.wikipedia.org/wiki/Schnittpunkt>
    TechDraw::CirclePtr gen1 = std::static_pointer_cast<TechDraw::Circle>(geom1);
    TechDraw::GenericPtr gen2 = std::static_pointer_cast<TechDraw::Generic>(geom2);
    // we calculate vectors to circle center, start point and direction vector
    Base::Vector3d cirleCenter = gen1->center;
    Base::Vector3d startPnt = gen2->points.at(0);
    Base::Vector3d endPnt = gen2->points.at(1);
    Base::Vector3d dir = endPnt - startPnt;
    // we create equations of the circle: (x-x0)^2+(y-y0)^2=r^2
    // and the line: a*x+b*y+c=0
    float r0 = gen1->radius;
    float x0 = cirleCenter.x;
    float y0 = cirleCenter.y;
    float a = -dir.y;
    float b = dir.x;
    float c = -startPnt.x * dir.y + startPnt.y * dir.x;
    // we shift line and circle so that the circle center is in the origin
    // and calculate constant d of new line equation
    float d = c - a * x0 - b * y0;
    float ab = a * a + b * b;
    float rootArg = r0 * r0 * ab - d * d;
    if (rootArg > 0)
    // line and circle have common points
    {
        if (rootArg < 0.01)
        // line is the tangent line, one intersection point
        {
            float x1 = x0 + a * d / ab;
            float y1 = -y0 + b * d / ab;
            Base::Vector3d interPoint1(x1, y1, 0.0);
            interPoints.push_back(interPoint1);
        }
        else
        // line is crossing the circle, two intersection points
        {
            float root = sqrt(rootArg);
            float x1 = x0 + (a * d + b * root) / ab;
            float y1 = -y0 - (b * d - a * root) / ab;
            float x2 = x0 + (a * d - b * root) / ab;
            float y2 = -y0 - (b * d + a * root) / ab;
            Base::Vector3d interPoint1(x1, y1, 0.0);
            interPoints.push_back(interPoint1);
            Base::Vector3d interPoint2(x2, y2, 0.0);
            interPoints.push_back(interPoint2);
        }
    }
}

void BaseGeom::intersectionCC(TechDraw::BaseGeomPtr geom1,
                            TechDraw::BaseGeomPtr geom2,
                            std::vector<Base::Vector3d>& interPoints)
{
    // find intersection vertex(es) between two circles
    // Taken from: <http://de.wikipedia.org/wiki/Schnittpunkt>
    TechDraw::CirclePtr gen1 = std::static_pointer_cast<TechDraw::Circle>(geom1);
    TechDraw::CirclePtr gen2 = std::static_pointer_cast<TechDraw::Circle>(geom2);
    Base::Vector3d Center1 = gen1->center;
    Base::Vector3d Center2 = gen2->center;
    float r1 = gen1->radius;
    float r2 = gen2->radius;
    // we calculate the distance d12 of the centers, and the
    // two orthonormal vectors m and n
    float d12 = (Center2 - Center1).Length();
    Base::Vector3d m = (Center2 - Center1).Normalize();
    Base::Vector3d n(-m.y, m.x, 0.0);
    // we calculate d0, the distance from center1 to the tie line
    // and rootArg, the square of the distance of the intersection points
    float d0 = (r1 * r1 - r2 * r2 + d12 * d12) / (2 * d12);
    float rootArg = r1 * r1 - d0 * d0;
    if (rootArg > 0)
    // the circles have intersection points
    {
        if (rootArg < 0.1)
        // the circles touch, one intersection point
        {
            Base::Vector3d interPoint1 = -Center1 + m * d0;
            interPoint1.y = -interPoint1.y;
            interPoints.push_back(interPoint1);
        }
        else
        // the circles have two intersection points
        {
            float e0 = sqrt(rootArg);
            Base::Vector3d interPoint1 = Center1 + m * d0 + n * e0;
            interPoint1.y = -interPoint1.y;
            interPoints.push_back(interPoint1);
            Base::Vector3d interPoint2 = Center1 + m * d0 - n * e0;
            interPoint2.y = -interPoint2.y;
            interPoints.push_back(interPoint2);
        }
    }
}

TopoShape BaseGeom::asTopoShape(double scale)
{
//    Base::Console().Message("BG::asTopoShape(%.3f) - dump: %s\n", scale, dump().c_str());
    TopoDS_Shape unscaledShape = ShapeUtils::scaleShape(getOCCEdge(), 1.0 / scale);
    TopoDS_Edge unscaledEdge = TopoDS::Edge(unscaledShape);
    return unscaledEdge;
}

Ellipse::Ellipse(const TopoDS_Edge &e)
{
    geomType = ELLIPSE;
    BRepAdaptor_Curve c(e);
    occEdge = e;
    gp_Elips ellp = c.Ellipse();
    const gp_Pnt &p = ellp.Location();

    center = Base::Vector3d(p.X(), p.Y(), 0.0);

    major = ellp.MajorRadius();
    minor = ellp.MinorRadius();

    gp_Dir xaxis = ellp.XAxis().Direction();
    angle = xaxis.AngleWithRef(gp_Dir(1, 0, 0), gp_Dir(0, 0, -1));
}

Ellipse::Ellipse(Base::Vector3d c, double mnr, double mjr)
{
    geomType = ELLIPSE;
    center = c;
    major = mjr;
    minor = mnr;
    angle = 0;

    GC_MakeEllipse me(gp_Ax2(gp_Pnt(c.x, c.y, c.z), gp_Dir(0.0, 0.0, 1.0)),
                      major, minor);
    if (!me.IsDone()) {
        Base::Console().Message("G:Ellipse - failed to make Ellipse\n");
    }
    const Handle(Geom_Ellipse) gEllipse = me.Value();
    BRepBuilderAPI_MakeEdge mkEdge(gEllipse, 0.0, 2 * M_PI);
    if (mkEdge.IsDone()) {
        occEdge = mkEdge.Edge();
    }
}

AOE::AOE(const TopoDS_Edge &e) : Ellipse(e)
{
    geomType = ARCOFELLIPSE;

    BRepAdaptor_Curve c(e);
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt ePt = c.Value(l);

    double a;
    try {
        gp_Vec v1(m, s);
        gp_Vec v2(m, ePt);
        gp_Vec v3(0, 0, 1);
        a = v3.DotCross(v1, v2);
    }
    catch (const Standard_Failure& e) {
        Base::Console().Error("Geom::AOE::AOE - OCC error - %s - while making AOE in ctor\n",
                              e.GetMessageString());
    }

    startAngle = fmod(f, 2.0*M_PI);
    endAngle = fmod(l, 2.0*M_PI);
    cw = (a < 0) ? true: false;
    largeArc = (l-f > M_PI) ? true : false;

    startPnt = Base::Vector3d(s.X(), s.Y(), s.Z());
    endPnt = Base::Vector3d(ePt.X(), ePt.Y(), ePt.Z());
    midPnt = Base::Vector3d(m.X(), m.Y(), m.Z());
    if (e.Orientation() == TopAbs_REVERSED) {
        reversed = true;
    }
}


Circle::Circle()
{
    geomType = CIRCLE;
    radius = 0.0;
    center = Base::Vector3d(0.0, 0.0, 0.0);
}

Circle::Circle(Base::Vector3d c, double r)
{
    geomType = CIRCLE;
    radius = r;
    center = c;
    gp_Pnt loc(c.x, c.y, c.z);
    gp_Dir dir(0, 0, 1);
    gp_Ax1 axis(loc, dir);
    gp_Circ circle;
    circle.SetAxis(axis);
    circle.SetRadius(r);
    double angle1 = 0.0;
    double angle2 = 360.0;

    Handle(Geom_Circle) hCircle = new Geom_Circle (circle);
    BRepBuilderAPI_MakeEdge aMakeEdge(hCircle, angle1*(M_PI/180), angle2*(M_PI/180));
    TopoDS_Edge edge = aMakeEdge.Edge();
    occEdge = edge;
}


Circle::Circle(const TopoDS_Edge &e)
{
    geomType = CIRCLE;             //center, radius
    BRepAdaptor_Curve c(e);
    occEdge = e;

    gp_Circ circ = c.Circle();
    const gp_Pnt& p = circ.Location();

    radius = circ.Radius();
    center = Base::Vector3d(p.X(), p.Y(), p.Z());
}
std::string Circle::toString() const
{
    std::string baseCSV = BaseGeom::toString();
    std::stringstream ss;
    ss << center.x << ", " <<
          center.y << ", " <<
          center.z << ", " <<
          radius;
    return baseCSV + ", $$$, " + ss.str();
}

void Circle::Save(Base::Writer &writer) const
{
    BaseGeom::Save(writer);
    writer.Stream() << writer.ind() << "<Center "
                << "X=\"" <<  center.x <<
                "\" Y=\"" <<  center.y <<
                "\" Z=\"" <<  center.z <<
                 "\"/>" << endl;

    writer.Stream() << writer.ind() << "<Radius value=\"" << radius << "\"/>" << endl;
}

void Circle::Restore(Base::XMLReader &reader)
{
    BaseGeom::Restore(reader);
    // read my Element
    reader.readElement("Center");
    // get the value of my Attribute
    center.x = reader.getAttributeAsFloat("X");
    center.y = reader.getAttributeAsFloat("Y");
    center.z = reader.getAttributeAsFloat("Z");

    reader.readElement("Radius");
    radius = reader.getAttributeAsFloat("value");
}

AOC::AOC(const TopoDS_Edge &e) : Circle(e)
{
    geomType = ARCOFCIRCLE;
    BRepAdaptor_Curve c(e);

    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt ePt = c.Value(l);           //if start == end, it isn't an arc!
    gp_Vec v1(m, s);        //vector mid to start
    gp_Vec v2(m, ePt);      //vector mid to end
    gp_Vec v3(0, 0, 1);      //stdZ
    double a = v3.DotCross(v1, v2);    //error if v1 = v2?

    startAngle = fmod(f, 2.0*M_PI);
    endAngle = fmod(l, 2.0*M_PI);
    cw = (a < 0) ? true: false;
    largeArc = (fabs(l-f) > M_PI) ? true : false;

    startPnt = Base::Vector3d(s.X(), s.Y(), s.Z());
    endPnt = Base::Vector3d(ePt.X(), ePt.Y(), s.Z());
    midPnt = Base::Vector3d(m.X(), m.Y(), s.Z());
    if (e.Orientation() == TopAbs_REVERSED) {
        reversed = true;
    }
}

AOC::AOC(Base::Vector3d c, double r, double sAng, double eAng) : Circle()
{
    geomType = ARCOFCIRCLE;

    radius = r;
    center = c;
    gp_Pnt loc(c.x, c.y, c.z);
    gp_Dir dir(0, 0, 1);
    gp_Ax1 axis(loc, dir);
    gp_Circ circle;
    circle.SetAxis(axis);
    circle.SetRadius(r);

    Handle(Geom_Circle) hCircle = new Geom_Circle (circle);
    BRepBuilderAPI_MakeEdge aMakeEdge(hCircle, sAng*(M_PI/180), eAng*(M_PI/180));
    TopoDS_Edge edge = aMakeEdge.Edge();
    occEdge = edge;

    BRepAdaptor_Curve adp(edge);

    double f = adp.FirstParameter();
    double l = adp.LastParameter();
    gp_Pnt s = adp.Value(f);
    gp_Pnt m = adp.Value((l+f)/2.0);
    gp_Pnt ePt = adp.Value(l);           //if start == end, it isn't an arc!
    gp_Vec v1(m, s);        //vector mid to start
    gp_Vec v2(m, ePt);      //vector mid to end
    gp_Vec v3(0, 0, 1);      //stdZ
    double a = v3.DotCross(v1, v2);    //error if v1 = v2?

    startAngle = fmod(f, 2.0*M_PI);
    endAngle = fmod(l, 2.0*M_PI);
    cw = (a < 0) ? true: false;
    largeArc = (fabs(l-f) > M_PI) ? true : false;

    startPnt = Base::Vector3d(s.X(), s.Y(), s.Z());
    endPnt = Base::Vector3d(ePt.X(), ePt.Y(), s.Z());
    midPnt = Base::Vector3d(m.X(), m.Y(), s.Z());
    if (edge.Orientation() == TopAbs_REVERSED) {
        reversed = true;
    }
}


AOC::AOC() : Circle()
{
    geomType = ARCOFCIRCLE;

    startPnt = Base::Vector3d(0.0, 0.0, 0.0);
    endPnt = Base::Vector3d(0.0, 0.0, 0.0);
    midPnt = Base::Vector3d(0.0, 0.0, 0.0);
    startAngle = 0.0;
    endAngle = 2.0 * M_PI;
    cw = false;
    largeArc = false;

}

bool AOC::isOnArc(Base::Vector3d p)
{
    gp_Pnt pnt(p.x, p.y,p.z);
    TopoDS_Vertex v = BRepBuilderAPI_MakeVertex(pnt);
    BRepExtrema_DistShapeShape extss(occEdge, v);
    if (!extss.IsDone() || extss.NbSolution() == 0) {
        return false;
    }
    double minDist = extss.Value();
    if (minDist < Precision::Confusion()) {
        return true;
    }
    return false;
}

double AOC::distToArc(Base::Vector3d p)
{
    return minDist(p);
}


bool AOC::intersectsArc(Base::Vector3d p1, Base::Vector3d p2)
{
    gp_Pnt pnt1(p1.x, p1.y,p1.z);
    TopoDS_Vertex v1 = BRepBuilderAPI_MakeVertex(pnt1);
    gp_Pnt pnt2(p2.x, p2.y, p2.z);
    TopoDS_Vertex v2 = BRepBuilderAPI_MakeVertex(pnt2);
    BRepBuilderAPI_MakeEdge mkEdge(v1, v2);
    TopoDS_Edge line = mkEdge.Edge();
    BRepExtrema_DistShapeShape extss(occEdge, line);
    if (!extss.IsDone() || extss.NbSolution() == 0) {
        return false;
    }
    double minDist = extss.Value();
    if (minDist < Precision::Confusion()) {
        return true;
    }
    return false;
}

std::string AOC::toString() const
{
    std::string circleCSV = Circle::toString();
    std::stringstream ss;

    ss << startPnt.x << ", " <<
          startPnt.y << ", " <<
          startPnt.z << ", " <<
          endPnt.x << ", " <<
          endPnt.y << ", " <<
          endPnt.z << ", " <<
          midPnt.x << ", " <<
          midPnt.y << ", " <<
          midPnt.z << ", " <<
          startAngle << ", " <<
          endAngle << ", " <<
          cw << ", " <<
          largeArc;

    return circleCSV + ", $$$," + ss.str();
}

void AOC::Save(Base::Writer &writer) const
{
    Circle::Save(writer);
    writer.Stream() << writer.ind() << "<Start "
                << "X=\"" <<  startPnt.x <<
                "\" Y=\"" <<  startPnt.y <<
                "\" Z=\"" <<  startPnt.z <<
                 "\"/>" << endl;
    writer.Stream() << writer.ind() << "<End "
                << "X=\"" <<  endPnt.x <<
                "\" Y=\"" <<  endPnt.y <<
                "\" Z=\"" <<  endPnt.z <<
                 "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Middle "
                << "X=\"" <<  midPnt.x <<
                "\" Y=\"" <<  midPnt.y <<
                "\" Z=\"" <<  midPnt.z <<
                 "\"/>" << endl;
    writer.Stream() << writer.ind() << "<StartAngle value=\"" << startAngle << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<EndAngle value=\"" << endAngle << "\"/>" << endl;
    const char c = cw?'1':'0';
    writer.Stream() << writer.ind() << "<Clockwise value=\"" <<  c << "\"/>" << endl;
    const char la = largeArc?'1':'0';
    writer.Stream() << writer.ind() << "<Large value=\"" <<  la << "\"/>" << endl;
}

void AOC::Restore(Base::XMLReader &reader)
{
    Circle::Restore(reader);
    reader.readElement("Start");
    startPnt.x = reader.getAttributeAsFloat("X");
    startPnt.y = reader.getAttributeAsFloat("Y");
    startPnt.z = reader.getAttributeAsFloat("Z");
    reader.readElement("End");
    endPnt.x = reader.getAttributeAsFloat("X");
    endPnt.y = reader.getAttributeAsFloat("Y");
    endPnt.z = reader.getAttributeAsFloat("Z");
    reader.readElement("Middle");
    midPnt.x = reader.getAttributeAsFloat("X");
    midPnt.y = reader.getAttributeAsFloat("Y");
    midPnt.z = reader.getAttributeAsFloat("Z");

    reader.readElement("StartAngle");
    startAngle = reader.getAttributeAsFloat("value");
    reader.readElement("EndAngle");
    endAngle = reader.getAttributeAsFloat("value");
    reader.readElement("Clockwise");
    cw = (int)reader.getAttributeAsInteger("value")==0?false:true;
    reader.readElement("Large");
    largeArc = (int)reader.getAttributeAsInteger("value")==0?false:true;
}

//! Generic is a multiline
Generic::Generic(const TopoDS_Edge &e)
{
    geomType = GENERIC;
    occEdge = e;
    BRepLib::BuildCurve3d(occEdge);

    TopLoc_Location location;
    Handle(Poly_Polygon3D) polygon = BRep_Tool::Polygon3D(occEdge, location);

    if (!polygon.IsNull()) {
        const TColgp_Array1OfPnt &nodes = polygon->Nodes();
        for (int i = nodes.Lower(); i <= nodes.Upper(); i++){
            points.emplace_back(nodes(i).X(), nodes(i).Y(), nodes(i).Z());
        }
    } else {
        //no polygon representation? approximate with line
        gp_Pnt p = BRep_Tool::Pnt(TopExp::FirstVertex(occEdge));
        points.emplace_back(p.X(), p.Y(), p.Z());
        p = BRep_Tool::Pnt(TopExp::LastVertex(occEdge));
        points.emplace_back(p.X(), p.Y(), p.Z());
    }
    if (e.Orientation() == TopAbs_REVERSED) {
        reversed = true;
    }
}


Generic::Generic()
{
    geomType = GENERIC;
}

std::string Generic::toString() const
{
    std::string baseCSV = BaseGeom::toString();
    std::stringstream ss;
    ss << points.size() << ", ";
    for (auto& p: points) {
        ss << p.x << ", " <<
              p.y << ", " <<
              p.z << ", ";
    }
    std::string genericCSV = ss.str();
    genericCSV.pop_back();
    return baseCSV + ", $$$, " + genericCSV;
}


void Generic::Save(Base::Writer &writer) const
{
    BaseGeom::Save(writer);
    writer.Stream() << writer.ind()
                        << "<Points PointsCount =\"" << points.size() << "\">" << endl;
    writer.incInd();
    for (auto& p: points) {
        writer.Stream() << writer.ind() << "<Point "
                    << "X=\"" <<  p.x <<
                    "\" Y=\"" <<  p.y <<
                    "\" Z=\"" <<  p.z <<
                     "\"/>" << endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Points>" << endl ;

}

void Generic::Restore(Base::XMLReader &reader)
{
    BaseGeom::Restore(reader);
    reader.readElement("Points");
    int stop = reader.getAttributeAsInteger("PointsCount");
    int i = 0;
    for ( ; i < stop; i++) {
        reader.readElement("Point");
        Base::Vector3d p;
        p.x = reader.getAttributeAsFloat("X");
        p.y = reader.getAttributeAsFloat("Y");
        p.z = reader.getAttributeAsFloat("Z");
        points.push_back(p);
    }
    reader.readEndElement("Points");
}

Base::Vector3d Generic::asVector()
{
    return getEndPoint() - getStartPoint();
}

double Generic::slope()
{
    Base::Vector3d v = asVector();
    if (v.x == 0.0) {
        return DOUBLE_MAX;
    } else {
        return v.y/v.x;
    }
}

Base::Vector3d Generic::apparentInter(GenericPtr g)
{
    Base::Vector3d dir0 = asVector();
    Base::Vector3d dir1 = g->asVector();

    // Line Intersetion (taken from ViewProviderSketch.cpp)
    double det = dir0.x*dir1.y - dir0.y*dir1.x;
    if ((det > 0 ? det : -det) < 1e-10)
        throw Base::ValueError("Invalid selection - Det = 0");

    double c0 = dir0.y*points.at(0).x - dir0.x*points.at(0).y;
    double c1 = dir1.y*g->points.at(1).x - dir1.x*g->points.at(1).y;
    double x = (dir0.x*c1 - dir1.x*c0)/det;
    double y = (dir0.y*c1 - dir1.y*c0)/det;

    // Apparent Intersection point
    return Base::Vector3d(x, y, 0.0);
}

BSpline::BSpline(const TopoDS_Edge &e)
{
    geomType = BSPLINE;
    BRepAdaptor_Curve c(e);
    isArc = !c.IsClosed();
    Handle(Geom_BSplineCurve) cSpline = c.BSpline();
    occEdge = e;
    Handle(Geom_BSplineCurve) spline;

    double f, l;
    f = c.FirstParameter();
    l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt ePt = c.Value(l);
    startPnt = Base::Vector3d(s.X(), s.Y(), s.Z());
    endPnt = Base::Vector3d(ePt.X(), ePt.Y(), ePt.Z());
    midPnt = Base::Vector3d(m.X(), m.Y(), m.Z());
    gp_Vec v1(m, s);
    gp_Vec v2(m, ePt);
    gp_Vec v3(0, 0, 1);
    double a = v3.DotCross(v1, v2);
    cw = (a < 0) ? true: false;

    startAngle = atan2(startPnt.y, startPnt.x);
    if (startAngle < 0) {
         startAngle += 2.0 * M_PI;
    }
    endAngle = atan2(endPnt.y, endPnt.x);
    if (endAngle < 0) {
         endAngle += 2.0 * M_PI;
    }

    Standard_Real tol3D = 0.001;                                   //1/1000 of a mm? screen can't resolve this
    Standard_Integer maxDegree = 3, maxSegment = 200;
    Handle(BRepAdaptor_HCurve) hCurve = new BRepAdaptor_HCurve(c);
    // approximate the curve using a tolerance
    //Approx_Curve3d approx(hCurve, tol3D, GeomAbs_C2, maxSegment, maxDegree);   //gives degree == 5  ==> too many poles ==> buffer overrun
    Approx_Curve3d approx(hCurve, tol3D, GeomAbs_C0, maxSegment, maxDegree);
    if (approx.IsDone() && approx.HasResult()) {
        spline = approx.Curve();
    }
    else if (approx.HasResult()) { //result, but not within tolerance
        spline = approx.Curve();
    }
    else {
        f = c.FirstParameter();
        l = c.LastParameter();
        s = c.Value(f);
        ePt = c.Value(l);
        TColgp_Array1OfPnt controlPoints(0, 1);
        controlPoints.SetValue(0, s);
        controlPoints.SetValue(1, ePt);
        spline = GeomAPI_PointsToBSpline(controlPoints, 1).Curve();
    }

    GeomConvert_BSplineCurveToBezierCurve crt(spline);

    gp_Pnt controlPoint;
    for (Standard_Integer i = 1; i <= crt.NbArcs(); ++i) {
        BezierSegment tempSegment;
        Handle(Geom_BezierCurve) bezier = crt.Arc(i);
        tempSegment.poles = bezier->NbPoles();
        tempSegment.degree = bezier->Degree();
        for (int pole = 1; pole <= tempSegment.poles; ++pole) {
            controlPoint = bezier->Pole(pole);
            tempSegment.pnts.emplace_back(controlPoint.X(), controlPoint.Y(), controlPoint.Z());
        }
        segments.push_back(tempSegment);
    }
    if (e.Orientation() == TopAbs_REVERSED) {
        reversed = true;
    }
}


//! Can this BSpline be represented by a straight line?
// if len(first-last) == sum(len(pi - pi+1)) then it is a line
bool BSpline::isLine()
{
    return GeometryUtils::isLine(occEdge);
}

//used by DVDim for approximate dims
bool BSpline::isCircle()
{
    return GeometryUtils::isCircle(occEdge);
}

// make a circular edge from BSpline
TopoDS_Edge BSpline::asCircle(bool& arc)
{
    return GeometryUtils::asCircle(occEdge, arc);
}

bool BSpline::intersectsArc(Base::Vector3d p1, Base::Vector3d p2)
{
    gp_Pnt pnt1(p1.x, p1.y,p1.z);
    TopoDS_Vertex v1 = BRepBuilderAPI_MakeVertex(pnt1);
    gp_Pnt pnt2(p2.x, p2.y, p2.z);
    TopoDS_Vertex v2 = BRepBuilderAPI_MakeVertex(pnt2);
    BRepBuilderAPI_MakeEdge mkEdge(v1, v2);
    TopoDS_Edge line = mkEdge.Edge();
    BRepExtrema_DistShapeShape extss(occEdge, line);
    if (!extss.IsDone() || extss.NbSolution() == 0) {
        return false;
    }
    double minDist = extss.Value();
    if (minDist < Precision::Confusion()) {
        return true;
    }
    return false;
}


BezierSegment::BezierSegment(const TopoDS_Edge &e)
{
    geomType = BEZIER;
    occEdge = e;
    BRepAdaptor_Curve c(e);
    Handle(Geom_BezierCurve) bez = c.Bezier();
    poles = bez->NbPoles();
    degree = bez->Degree();
    for (int i = 1; i <= poles; ++i) {
        gp_Pnt controlPoint = bez->Pole(i);
        pnts.emplace_back(controlPoint.X(), controlPoint.Y(), controlPoint.Z());
    }
    if (e.Orientation() == TopAbs_REVERSED) {
        reversed = true;
    }
}

//**** Vertex
Vertex::Vertex()
{
    pnt = Base::Vector3d(0.0, 0.0, 0.0);
    extractType = ExtractionType::Plain;       //obs?
    hlrVisible = false;
    ref3D = -1;                        //obs. never used.
    m_center = false;
    BRepBuilderAPI_MakeVertex mkVert(gp_Pnt(0.0, 0.0, 0.0));
    occVertex = mkVert.Vertex();
    cosmetic = false;
    cosmeticLink = -1;
    cosmeticTag = std::string();
    m_reference = false;
    createNewTag();
}

Vertex::Vertex(const Vertex* v)
{
    pnt = v->point();
    extractType = v->extractType;       //obs?
    hlrVisible = v->hlrVisible;
    ref3D = v->ref3D;                  //obs. never used.
    m_center = v->m_center;
    occVertex = v->occVertex;
    cosmetic = v->cosmetic;
    cosmeticLink = v->cosmeticLink;
    cosmeticTag = v->cosmeticTag;
    m_reference = false;
    createNewTag();
}

Vertex::Vertex(double x, double y)
{
    pnt = Base::Vector3d(x, y, 0.0);
    extractType = ExtractionType::Plain;       //obs?
    hlrVisible = false;
    ref3D = -1;                        //obs. never used.
    m_center = false;
    BRepBuilderAPI_MakeVertex mkVert(gp_Pnt(x, y, 0.0));
    occVertex = mkVert.Vertex();
    cosmetic = false;
    cosmeticLink = -1;
    cosmeticTag = std::string();
    m_reference = false;
    createNewTag();
}

Vertex::Vertex(Base::Vector3d v) : Vertex(v.x, v.y)
{
//    Base::Console().Message("V::V(%s)\n",
//                            DrawUtil::formatVector(v).c_str());
}


bool Vertex::isEqual(const Vertex& v, double tol)
{
    double dist = (pnt - (v.pnt)).Length();
    if (dist <= tol) {
        return true;
    }
    return false;
}

void Vertex::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Point "
                << "X=\"" <<  pnt.x <<
                "\" Y=\"" <<  pnt.y <<
                "\" Z=\"" <<  pnt.z <<
                 "\"/>" << endl;

    writer.Stream() << writer.ind() << "<Extract value=\"" <<  extractType << "\"/>" << endl;
    const char v = hlrVisible ? '1':'0';
    writer.Stream() << writer.ind() << "<HLRVisible value=\"" <<  v << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Ref3D value=\"" <<  ref3D << "\"/>" << endl;
    const char c = m_center ?'1':'0';
    writer.Stream() << writer.ind() << "<IsCenter value=\"" <<  c << "\"/>" << endl;
    const char c2 = cosmetic?'1':'0';
    writer.Stream() << writer.ind() << "<Cosmetic value=\"" <<  c2 << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<CosmeticLink value=\"" <<  cosmeticLink << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<CosmeticTag value=\"" <<  cosmeticTag << "\"/>" << endl;

    //do we need to save this?  always recreated by program.
//    const char r = reference?'1':'0';
//    writer.Stream() << writer.ind() << "<Reference value=\"" <<  r << "\"/>" << endl;

    writer.Stream() << writer.ind() << "<VertexTag value=\"" <<  getTagAsString() << "\"/>" << endl;
}

void Vertex::Restore(Base::XMLReader &reader)
{
    reader.readElement("Point");
    pnt.x = reader.getAttributeAsFloat("X");
    pnt.y = reader.getAttributeAsFloat("Y");
    pnt.z = reader.getAttributeAsFloat("Z");

    reader.readElement("Extract");
    extractType = static_cast<ExtractionType>(reader.getAttributeAsInteger("value"));
//    reader.readElement("Visible");
//    hlrVisible = (bool)reader.getAttributeAsInteger("value")==0?false:true;
    reader.readElement("Ref3D");
    ref3D = reader.getAttributeAsInteger("value");
    reader.readElement("IsCenter");
    hlrVisible = reader.getAttributeAsInteger("value") != 0;
    reader.readElement("Cosmetic");
    cosmetic = reader.getAttributeAsInteger("value") != 0;
    reader.readElement("CosmeticLink");
    cosmeticLink = reader.getAttributeAsInteger("value");
    reader.readElement("CosmeticTag");
    cosmeticTag = reader.getAttribute("value");

    //will restore read to eof looking for "Reference" in old docs??  YES!!
//    reader.readElement("Reference");
//    m_reference = (bool)reader.getAttributeAsInteger("value")==0?false:true;

    reader.readElement("VertexTag");
    std::string temp = reader.getAttribute("value");
    boost::uuids::string_generator gen;
    boost::uuids::uuid u1 = gen(temp);
    tag = u1;

    BRepBuilderAPI_MakeVertex mkVert(gp_Pnt(pnt.x, pnt.y, pnt.z));
    occVertex = mkVert.Vertex();
}

void Vertex::createNewTag()
{
    // Initialize a random number generator, to avoid Valgrind false positives.
    static boost::mt19937 ran;
    static bool seeded = false;

    if (!seeded) {
        ran.seed(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }
    static boost::uuids::basic_random_generator<boost::mt19937> gen(&ran);

    tag = gen();
}


boost::uuids::uuid Vertex::getTag() const
{
    return tag;
}

std::string Vertex::getTagAsString() const
{
    return boost::uuids::to_string(getTag());
}

void Vertex::dump(const char* title)
{
    Base::Console().Message("TD::Vertex - %s - point: %s vis: %d cosmetic: %d  cosLink: %d cosTag: %s\n",
                            title, DrawUtil::formatVector(pnt).c_str(), hlrVisible, cosmetic, cosmeticLink,
                            cosmeticTag.c_str());
}

TopoShape Vertex::asTopoShape(double scale)
{
    Base::Vector3d point = DU::toVector3d(BRep_Tool::Pnt(getOCCVertex()));
    point = point / scale;
    BRepBuilderAPI_MakeVertex mkVert(DU::togp_Pnt(point));
    return TopoShape(mkVert.Vertex());
}


/*static*/
BaseGeomPtrVector GeometryUtils::chainGeoms(BaseGeomPtrVector geoms)
{
    BaseGeomPtrVector result;
    std::vector<bool> used(geoms.size(), false);

    if (geoms.empty()) {
        return result;
    }

    if (geoms.size() == 1) {
        //don't bother for single geom (circles, ellipses, etc)
        result.push_back(geoms[0]);
    } else {
        //start with first edge
        result.push_back(geoms[0]);
        Base::Vector3d atPoint = (geoms[0])->getEndPoint();
        used[0] = true;
        for (unsigned int i = 1; i < geoms.size(); i++) { //do size-1 more edges
            auto next( nextGeom(atPoint, geoms, used, Precision::Confusion()) );
            if (next.index) { //found an unused edge with vertex == atPoint
                BaseGeomPtr nextEdge = geoms.at(next.index);
                used[next.index] = true;
                nextEdge->setReversed(next.reversed);
                result.push_back(nextEdge);
                if (next.reversed) {
                    atPoint = nextEdge->getStartPoint();
                } else {
                    atPoint = nextEdge->getEndPoint();
                }
            }
        }
    }
    return result;
}


/*static*/ GeometryUtils::ReturnType GeometryUtils::nextGeom(
        Base::Vector3d atPoint,
        BaseGeomPtrVector geoms,
        std::vector<bool> used,
        double tolerance )
{
    ReturnType result(0, false);
    auto index(0);
    for (auto itGeom : geoms) {
        if (used[index]) {
            ++index;
            continue;
        }
        if ((atPoint - itGeom->getStartPoint()).Length() < tolerance) {
            result.index = index;
            result.reversed = false;
            break;
        } else if ((atPoint - itGeom->getEndPoint()).Length() < tolerance) {
            result.index = index;
            result.reversed = true;
            break;
        }
        ++index;
    }
    return result;
}

TopoDS_Edge GeometryUtils::edgeFromGeneric(TechDraw::GenericPtr g)
{
//    Base::Console().Message("GU::edgeFromGeneric()\n");
    //TODO: note that this isn't quite right as g can be a polyline!
    //sb points.first, points.last
    //and intermediates should be added to Point
    Base::Vector3d first = g->points.front();
    Base::Vector3d last  = g->points.back();
    gp_Pnt gp1(first.x, first.y, first.z);
    gp_Pnt gp2(last.x, last.y, last.z);
    return BRepBuilderAPI_MakeEdge(gp1, gp2);
}

TopoDS_Edge GeometryUtils::edgeFromCircle(TechDraw::CirclePtr c)
{
    gp_Pnt loc(c->center.x, c->center.y, c->center.z);
    gp_Dir dir(0, 0, 1);
    gp_Ax1 axis(loc, dir);
    gp_Circ circle;
    circle.SetAxis(axis);
    circle.SetRadius(c->radius);
    Handle(Geom_Circle) hCircle = new Geom_Circle (circle);
    BRepBuilderAPI_MakeEdge aMakeEdge(hCircle, 0.0, 2.0 * M_PI);
    return aMakeEdge.Edge();
}

TopoDS_Edge GeometryUtils::edgeFromCircleArc(TechDraw::AOCPtr c)
{
    gp_Pnt loc(c->center.x, c->center.y, c->center.z);
    gp_Dir dir(0, 0, 1);
    gp_Ax1 axis(loc, dir);
    gp_Circ circle;
    circle.SetAxis(axis);
    circle.SetRadius(c->radius);
    Handle(Geom_Circle) hCircle = new Geom_Circle (circle);
    double startAngle = c->startAngle;
    double endAngle = c->endAngle;
    BRepBuilderAPI_MakeEdge aMakeEdge(hCircle, startAngle, endAngle);
    return aMakeEdge.Edge();
}

//used by DVDim for approximate dims
bool GeometryUtils::isCircle(TopoDS_Edge occEdge)
{
    double radius;
    Base::Vector3d center;
    bool isArc = false;
    return GeometryUtils::getCircleParms(occEdge, radius, center, isArc);
}

//tries to interpret a BSpline edge as a circle. Used by DVDim for approximate dimensions.
bool GeometryUtils::getCircleParms(TopoDS_Edge occEdge, double& radius, Base::Vector3d& center, bool& isArc)
{
    double curveLimit = EWTOLERANCE;
    BRepAdaptor_Curve c(occEdge);
    Handle(Geom_BSplineCurve) spline = c.BSpline();
    double f, l;
    f = c.FirstParameter();
    l = c.LastParameter();
    double parmRange = fabs(l - f);
    int testCount = 6;
    double parmStep = parmRange/testCount;
    std::vector<double> curvatures;
    std::vector<gp_Pnt> centers;
    gp_Pnt curveCenter;
    double sumCurvature = 0;
    Base::Vector3d sumCenter, valueAt;
    try {
        GeomLProp_CLProps prop(spline, f, 3, Precision::Confusion());
        curvatures.push_back(prop.Curvature());
        sumCurvature += prop.Curvature();
        prop.CentreOfCurvature(curveCenter);
        centers.push_back(curveCenter);
        sumCenter += DrawUtil::toVector3d(curveCenter);

        for (int i = 1; i < (testCount - 1); i++) {
            prop.SetParameter(parmStep * i);
            curvatures.push_back(prop.Curvature());
            sumCurvature += prop.Curvature();
            prop.CentreOfCurvature(curveCenter);
            centers.push_back(curveCenter);
            sumCenter += DrawUtil::toVector3d(curveCenter);
        }
        prop.SetParameter(l);
        curvatures.push_back(prop.Curvature());
        sumCurvature += prop.Curvature();
        prop.CentreOfCurvature(curveCenter);
        centers.push_back(curveCenter);
        sumCenter += DrawUtil::toVector3d(curveCenter);
    }
    catch (Standard_Failure&) {
        Base::Console().Error("OCC error.  Could not interpret BSpline as Circle\n");
        return false;
    }
    Base::Vector3d avgCenter = sumCenter/testCount;

    double avgCurve = sumCurvature/testCount;
    double errorCurve  = 0;
    for (auto& cv: curvatures) {
        errorCurve += fabs(avgCurve - cv);    //fabs???
    }
    errorCurve  = errorCurve/testCount;

    isArc = !c.IsClosed();
    bool isCircle(false);
    if ( errorCurve < curveLimit ) {
        isCircle = true;
        radius = 1.0/avgCurve;
        center = avgCenter;
    }
    return isCircle;
}

// make a circle or arc of circle Edge from BSpline Edge
TopoDS_Edge GeometryUtils::asCircle(TopoDS_Edge occEdge, bool& arc)
{
    TopoDS_Edge result;
    BRepAdaptor_Curve c(occEdge);

    // find the two ends
    Handle(Geom_Curve) curve = c.Curve().Curve();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt e = c.Value(l);

    if (s.IsEqual(e, 0.001)) {    //more reliable
        arc = false;
    } else {
        arc = true;
    }
    //    arc  = !c.IsClosed();    //reliable?

    Handle(Geom_BSplineCurve) spline = c.BSpline();

    if (spline->NbPoles() < 5) {    //need 5 poles (s-p1-pm-p2-e) for algo
        return result;              //how to do with fewer poles?
    }

    try {
        // get three points on curve (non extreme poles)
        int nb_poles = spline->NbPoles();
        gp_Pnt p1 = spline->Pole(2);          //OCC numbering starts at 1!!
        gp_Pnt p2 = spline->Pole(nb_poles-1);
        gp_Pnt pm;
        if (nb_poles == 5) {
            pm = spline->Pole(3);   //5 poles => 2.5 => 2
        } else {
            pm = spline->Pole(nb_poles / 2);
        }

        // project three poles onto the curve
        GeomAPI_ProjectPointOnCurve proj1;
        GeomAPI_ProjectPointOnCurve proj2;
        GeomAPI_ProjectPointOnCurve projm;
        proj1.Init(p1, curve, f, l);
        proj1.Perform(p1);
        proj2.Init(p2, curve, f, l);
        proj2.Perform(p2);
        projm.Init(pm, curve, f, l);
        projm.Perform(pm);
        if ( (proj1.NbPoints() == 0) ||
            (proj2.NbPoints() == 0) ||
            (projm.NbPoints() == 0) ) {
            return result;
        }
        gp_Pnt pc1, pc2, pcm;

        // get projected points
        pc1 = proj1.NearestPoint();
        pc2 = proj2.NearestPoint();
        pcm = projm.NearestPoint();

        // make 2 circles and find their radii
        gce_MakeCirc gce_circ1 = gce_MakeCirc(s, pc1, pcm);   //3 point circle
        if (gce_circ1.Status() != gce_Done) {
            return result;
        }
        gp_Circ circle1 = gce_circ1.Value();
        double radius1 = circle1.Radius();
        gp_Pnt center1 = circle1.Location();
        Base::Vector3d vc1 = DrawUtil::toVector3d(center1);

        gce_MakeCirc gce_circ2 = gce_MakeCirc(pcm, pc2, e);
        if (gce_circ2.Status() != gce_Done) {
            return result;
        }
        gp_Circ circle2 = gce_circ2.Value();
        double radius2 = circle2.Radius();
        gp_Pnt center2 = circle2.Location();
        Base::Vector3d vc2 = DrawUtil::toVector3d(center2);

        // compare radii & centers
        double allowError = 0.001;           //mm^-3 good enough for printing
        double radius;
        Base::Vector3d center;
        if ( (DrawUtil::fpCompare(radius2, radius1, allowError)) &&
            (vc1.IsEqual(vc2, allowError)) ) {
            if (arc) {
                GC_MakeArcOfCircle makeArc(s, pcm, e);
                Handle(Geom_TrimmedCurve) tCurve = makeArc.Value();
                BRepBuilderAPI_MakeEdge mkEdge(tCurve);
                result = mkEdge.Edge();
            } else {
                radius = (radius1 + radius2) / 2.0;
                center = (vc1 + vc2) / 2.0;
                gp_Pnt gCenter(center.x, center.y, center.z);
                gp_Ax2 stdZ(gCenter, gp_Dir(0, 0, 1));
                gp_Circ newCirc(stdZ, radius);
                BRepBuilderAPI_MakeEdge mkEdge(newCirc);
                result = mkEdge.Edge();
            }
        }
    }
    catch (...) {
        // return null shape to indicate that we could not make a circle from this bspline
        return TopoDS_Edge();
    }
    return result;
}

bool GeometryUtils::isLine(TopoDS_Edge occEdge)
{
    BRepAdaptor_Curve c(occEdge);

    Handle(Geom_BSplineCurve) spline = c.BSpline();
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt e = c.Value(l);

    bool samePnt = s.IsEqual(e, FLT_EPSILON);
    if (samePnt) {
        return false;
    }

    Base::Vector3d vs = DrawUtil::toVector3d(s);
    Base::Vector3d ve = DrawUtil::toVector3d(e);
    double endLength = (vs - ve).Length();
    int low = 0;
    int high = spline->NbPoles() - 1;
    TColgp_Array1OfPnt poles(low, high);
    spline->Poles(poles);
    double lenTotal = 0.0;
    for (int i = 0; i < high; i++) {
        gp_Pnt p1 = poles(i);
        Base::Vector3d v1 = DrawUtil::toVector3d(p1);
        gp_Pnt p2 = poles(i+1);
        Base::Vector3d v2 = DrawUtil::toVector3d(p2);
        lenTotal += (v2-v1).Length();
    }

    if (DrawUtil::fpCompare(lenTotal, endLength)) {
        return true;
    }
    return false;
}
