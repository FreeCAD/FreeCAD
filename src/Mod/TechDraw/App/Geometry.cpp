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
# include <limits>
# include <Approx_Curve3d.hxx>
# include <BRep_Tool.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <Mod/Part/App/FCBRepAlgoAPI_Section.h>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepGProp.hxx>
# include <BRepLib.hxx>
# include <BRepLProp_CLProps.hxx>
# include <BRepTools.hxx>
# include <BRepLProp_CurveTool.hxx>
# include <GC_MakeArcOfCircle.hxx>
# include <GC_MakeEllipse.hxx>
# include <GC_MakeCircle.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <Geom_Circle.hxx>

# include <gce_MakeCirc.hxx>
# include <GCPnts_AbscissaPoint.hxx>
# include <GProp_GProps.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Geom_BezierCurve.hxx>
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
#include <Base/Converter.h>
#include <Base/Parameter.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include <Mod/Part/App/FaceMakerCheese.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>

#include "DrawViewPart.h"
#include "Geometry.h"
#include "ShapeUtils.h"
#include "DrawUtil.h"


using namespace TechDraw;
using namespace Part;
using namespace std;
using DU = DrawUtil;

#if OCC_VERSION_HEX >= 0x070600
using BRepAdaptor_HCurve = BRepAdaptor_Curve;
#endif

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

// note that the face returned is inverted in Y
TopoDS_Face Face::toOccFace() const
{
    if (wires.empty()) {
        return {};
    }

    TopoDS_Face result;
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

//**** Face
Base::Vector3d Face::getCenter() const {
    GProp_GProps faceProps;
    BRepGProp::SurfaceProperties(toOccFace(), faceProps);

    return Base::convertTo<Base::Vector3d>(faceProps.CentreOfMass());
}

double Face::getArea() const {
    GProp_GProps faceProps;
    BRepGProp::SurfaceProperties(toOccFace(), faceProps);

    return faceProps.Mass();
}

Face::~Face()
{
    for(auto it : wires) {
        delete it;
    }
    wires.clear();
}

BaseGeom::BaseGeom() :
    geomType(GeomType::NOTDEF),
    extractType(ExtractionType::Plain),             //obs
    classOfEdge(EdgeClass::NONE),
    hlrVisible(true),
    reversed(false),
    ref3D(-1),                      //obs?
    cosmetic(false),
    m_source(SourceType::GEOMETRY),
    m_sourceIndex(-1)
{
    occEdge = TopoDS_Edge();
    cosmeticTag = std::string();
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
    writer.Stream() << writer.ind() << "<Source value=\"" << m_source << "\"/>" << endl; // Should this save as text and not number?
    writer.Stream() << writer.ind() << "<SourceIndex value=\"" << m_sourceIndex << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<CosmeticTag value=\"" <<  cosmeticTag << "\"/>" << endl;
}

void BaseGeom::Restore(Base::XMLReader &reader)
{
    reader.readElement("GeomType");
    geomType = reader.getAttribute<GeomType>("value");
    reader.readElement("ExtractType");
    extractType = reader.getAttribute<ExtractionType>("value");
    reader.readElement("EdgeClass");
    classOfEdge = reader.getAttribute<EdgeClass>("value");
    reader.readElement("HLRVisible");
    hlrVisible = reader.getAttribute<bool>("value");
    reader.readElement("Reversed");
    reversed = reader.getAttribute<bool>("value");
    reader.readElement("Ref3D");
    ref3D = reader.getAttribute<long>("value");
    reader.readElement("Cosmetic");
    cosmetic = reader.getAttribute<bool>("value");
    reader.readElement("Source");
    m_source = reader.getAttribute<SourceType>("value");
    reader.readElement("SourceIndex");
    m_sourceIndex = reader.getAttribute<long>("value");
    reader.readElement("CosmeticTag");
    cosmeticTag = reader.getAttribute<const char*>("value");
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
        Base::Console().message("Geometry::findEndPoints - OCC edge not found\n");
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
        Base::Console().message("Geometry::getStartPoint - start point not found!\n");
        Base::Vector3d badResult(0.0, 0.0, 0.0);
        return badResult;
    }
}


Base::Vector3d BaseGeom::getEndPoint()
{
    std::vector<Base::Vector3d> verts = findEndPoints();

    if (verts.size() != 2) {
        //TODO: this should throw something
        Base::Console().message("Geometry::getEndPoint - end point not found!\n");
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
//    Base::Console().message("BG::inverted()\n");
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
    };
    return typeNames.at(static_cast<int>(geomType));
}

//! Convert 1 OCC edge into 1 BaseGeom (static factory method)
// this should not return nullptr as things will break later on.
// regular geometry is stored scaled, but cosmetic geometry is stored in 1:1 scale, so the crazy edge
// check is not appropriate.
BaseGeomPtr BaseGeom::baseFactory(TopoDS_Edge edge, bool isCosmetic)
{
    if (edge.IsNull()) {
        Base::Console().message("BG::baseFactory - input edge is NULL \n");
    }
    //weed out rubbish edges before making geometry
    if (!isCosmetic && !validateEdge(edge)) {
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
            } else if (bspline->isCircle())  {
                circEdge = bspline->asCircle(isArc);
                if (circEdge.IsNull()) {
                    result = bspline;
                } else {
                    if (isArc) {
                        result = std::make_shared<AOC>(circEdge);
                    } else {
                        result = std::make_shared<Circle>(circEdge);
                    }
                 }
            } else {
//              Base::Console().message("Geom::baseFactory - circEdge is Null\n");
                result = bspline;
            }
            break;
        }
        catch (const Standard_Failure& e) {
            Base::Console().log("Geom::baseFactory - OCC error - %s - while making spline\n",
                              e.GetMessageString());
            break;
        }
        catch (...) {
            Base::Console().log("Geom::baseFactory - unknown error occurred while making spline\n");
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

TopoDS_Edge BaseGeom::completeEdge(const TopoDS_Edge &edge) {
    // Extend given edge so we can get intersections even outside its boundaries
    try {
        BRepAdaptor_Curve curve(edge);
        switch (curve.GetType()) {
            case GeomAbs_Line:
                // Edge longer than 10m is considered "crazy", thus limit intersection(s) to this perimeter
                return BRepBuilderAPI_MakeEdge(curve.Line(), -10000.0, +10000.0);
            case GeomAbs_Circle:
                // If an arc of circle was provided, return full circle
                return BRepBuilderAPI_MakeEdge(curve.Circle());
            case GeomAbs_Ellipse:
                // If an arc of ellipse was provided, return full ellipse
                return BRepBuilderAPI_MakeEdge(curve.Ellipse());
            default:
                // Currently we are not extrapolating B-splines, though it is technically possible
                return BRepBuilderAPI_MakeEdge(curve.Curve().Curve());
        }
    }
    catch (Standard_Failure &e) {
        Base::Console().error("BaseGeom::completeEdge OCC error: %s\n", e.GetMessageString());
    }

    return TopoDS_Edge();
}

std::vector<Base::Vector3d> BaseGeom::intersection(TechDraw::BaseGeomPtr geom2)
{
    // find intersection vertex(es) between two edges
    // call: interPoints = line1.intersection(line2);
    std::vector<Base::Vector3d> interPoints;

    TopoDS_Edge edge1 = completeEdge(this->getOCCEdge());
    if (edge1.IsNull()) {
        return interPoints;
    }

    TopoDS_Edge edge2 = completeEdge(geom2->getOCCEdge());
    if (edge2.IsNull()) {
        return interPoints;
    }

    FCBRepAlgoAPI_Section sectionOp(edge1, edge2);
    sectionOp.SetFuzzyValue(FUZZYADJUST*EWTOLERANCE);
    sectionOp.SetNonDestructive(true);

    sectionOp.Build();
    if (!sectionOp.HasErrors()) {
        TopoDS_Shape sectionShape = sectionOp.Shape();
        if (!sectionShape.IsNull()) {
            TopExp_Explorer explorer(sectionShape, TopAbs_VERTEX);
            while (explorer.More()) {
                Base::Vector3d pt(Base::convertTo<Base::Vector3d>(BRep_Tool::Pnt(TopoDS::Vertex(explorer.Current()))));
                interPoints.push_back(pt);
                explorer.Next();
            }
        }
    }

    return interPoints;
}

TopoShape BaseGeom::asTopoShape(double scale)
{
//    Base::Console().message("BG::asTopoShape(%.3f) - dump: %s\n", scale, dump().c_str());
    TopoDS_Shape unscaledShape = ShapeUtils::scaleShape(getOCCEdge(), 1.0 / scale);
    TopoDS_Edge unscaledEdge = TopoDS::Edge(unscaledShape);
    return unscaledEdge;
}

Ellipse::Ellipse(const TopoDS_Edge &e)
{
    geomType = GeomType::ELLIPSE;
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
    geomType = GeomType::ELLIPSE;
    center = c;
    major = mjr;
    minor = mnr;
    angle = 0;

    GC_MakeEllipse me(gp_Ax2(gp_Pnt(c.x, c.y, c.z), gp_Dir(0.0, 0.0, 1.0)),
                      major, minor);
    if (!me.IsDone()) {
        Base::Console().message("G:Ellipse - failed to make Ellipse\n");
    }
    const Handle(Geom_Ellipse) gEllipse = me.Value();
    BRepBuilderAPI_MakeEdge mkEdge(gEllipse, 0.0, 2 * std::numbers::pi);
    if (mkEdge.IsDone()) {
        occEdge = mkEdge.Edge();
    }
}

AOE::AOE(const TopoDS_Edge &e) : Ellipse(e)
{
    geomType = GeomType::ARCOFELLIPSE;

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
        Base::Console().error("Geom::AOE::AOE - OCC error - %s - while making AOE in ctor\n",
                              e.GetMessageString());
    }

    startAngle = fmod(f, 2.0*std::numbers::pi);
    endAngle = fmod(l, 2.0*std::numbers::pi);
    cw = (a < 0) ? true: false;
    largeArc = (l-f > std::numbers::pi) ? true : false;

    startPnt = Base::Vector3d(s.X(), s.Y(), s.Z());
    endPnt = Base::Vector3d(ePt.X(), ePt.Y(), ePt.Z());
    midPnt = Base::Vector3d(m.X(), m.Y(), m.Z());
    if (e.Orientation() == TopAbs_REVERSED) {
        reversed = true;
    }
}


Circle::Circle()
{
    geomType = GeomType::CIRCLE;
    radius = 0.0;
    center = Base::Vector3d(0.0, 0.0, 0.0);
}

Circle::Circle(Base::Vector3d c, double r)
{
    geomType = GeomType::CIRCLE;
    radius = r;
    center = c;
    gp_Pnt loc(c.x, c.y, c.z);
    gp_Dir dir(0, 0, 1);
    gp_Ax1 axis(loc, dir);
    gp_Circ circle;
    circle.SetAxis(axis);
    circle.SetRadius(r);

    Handle(Geom_Circle) hCircle = new Geom_Circle (circle);
    BRepBuilderAPI_MakeEdge aMakeEdge(hCircle, 0.0, 2.0 * std::numbers::pi);
    TopoDS_Edge edge = aMakeEdge.Edge();
    occEdge = edge;
}


Circle::Circle(const TopoDS_Edge &e)
{
    geomType = GeomType::CIRCLE;             //center, radius
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
    center.x = reader.getAttribute<double>("X");
    center.y = reader.getAttribute<double>("Y");
    center.z = reader.getAttribute<double>("Z");

    reader.readElement("Radius");
    radius = reader.getAttribute<double>("value");
}

AOC::AOC(const TopoDS_Edge &e) : Circle(e)
{
    geomType = GeomType::ARCOFCIRCLE;
    BRepAdaptor_Curve c(e);

    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l+f)/2.0);
    gp_Pnt ePt = c.Value(l);           //if start == end, it isn't an arc!
    gp_Vec v1(m, s);        //vector mid to start
    gp_Vec v2(m, ePt);      //vector mid to end
    gp_Vec v3(0, 0, 1);      //stdZ

    // this is the wrong determination of cw/ccw.  needs to be determined by edge.
    double a = v3.DotCross(v1, v2);    //error if v1 = v2?

    startAngle = fmod(f, 2.0*std::numbers::pi);
    endAngle = fmod(l, 2.0*std::numbers::pi);


    cw = (a < 0) ? true: false;
    largeArc = (fabs(l-f) > std::numbers::pi) ? true : false;

    startPnt = Base::convertTo<Base::Vector3d>(s);
    endPnt = Base::convertTo<Base::Vector3d>(ePt);
    midPnt = Base::convertTo<Base::Vector3d>(m);
    if (e.Orientation() == TopAbs_REVERSED) {
        reversed = true;
    }
}

AOC::AOC(Base::Vector3d c, double r, double sAng, double eAng) : Circle()
{
    geomType = GeomType::ARCOFCIRCLE;

    radius = r;
    center = c;
    gp_Pnt loc(c.x, c.y, c.z);
    gp_Dir dir(0, 0, 1);
    gp_Ax1 axis(loc, dir);
    gp_Circ circle;
    circle.SetAxis(axis);
    circle.SetRadius(r);

    Handle(Geom_Circle) hCircle = new Geom_Circle (circle);
    BRepBuilderAPI_MakeEdge aMakeEdge(hCircle, Base::toRadians(sAng), Base::toRadians(eAng));
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

    // this is a bit of an arcane method of determining if v2 is clockwise from v1 or counter clockwise from v1.
    // The v1 x v2 points up if v2 is ccw from v1 and points down if v2 is cw from v1.  Taking (v1 x v2) * stdZ
    // gives 1 for parallel with stdZ (v2 is ccw from v1) or -1 for antiparallel with stdZ (v2 is clockwise from v1).
    // this cw flag is a problem.  we should just declare that arcs are always ccw and flip the start and end angles.
    double a = v3.DotCross(v1, v2);    //error if v1 = v2?

    startAngle = fmod(f, 2.0*std::numbers::pi);
    endAngle = fmod(l, 2.0*std::numbers::pi);
    cw = (a < 0) ? true: false;
    largeArc = (fabs(l-f) > std::numbers::pi) ? true : false;

    startPnt = Base::convertTo<Base::Vector3d>(s);
    endPnt = Base::convertTo<Base::Vector3d>(ePt);
    midPnt = Base::convertTo<Base::Vector3d>(m);
    if (edge.Orientation() == TopAbs_REVERSED) {
        reversed = true;
    }
}


AOC::AOC() : Circle()
{
    geomType = GeomType::ARCOFCIRCLE;

    startPnt = Base::Vector3d(0.0, 0.0, 0.0);
    endPnt = Base::Vector3d(0.0, 0.0, 0.0);
    midPnt = Base::Vector3d(0.0, 0.0, 0.0);
    startAngle = 0.0;
    endAngle = 2.0 * std::numbers::pi;
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

BaseGeomPtr AOC::copy()
{
    auto base = BaseGeom::copy();
    TechDraw::CirclePtr circle =  std::static_pointer_cast<TechDraw::Circle>(base);
    TechDraw::AOCPtr aoc = std::static_pointer_cast<TechDraw::AOC>(circle);
    if (aoc) {
        aoc->clockwiseAngle(clockwiseAngle());
        aoc->startPnt = startPnt;
        aoc->startAngle = startAngle;
        aoc->endPnt = endPnt;
        aoc->endAngle = endAngle;
        aoc->largeArc = largeArc;
    }
    return base;
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
    startPnt.x = reader.getAttribute<double>("X");
    startPnt.y = reader.getAttribute<double>("Y");
    startPnt.z = reader.getAttribute<double>("Z");
    reader.readElement("End");
    endPnt.x = reader.getAttribute<double>("X");
    endPnt.y = reader.getAttribute<double>("Y");
    endPnt.z = reader.getAttribute<double>("Z");
    reader.readElement("Middle");
    midPnt.x = reader.getAttribute<double>("X");
    midPnt.y = reader.getAttribute<double>("Y");
    midPnt.z = reader.getAttribute<double>("Z");

    reader.readElement("StartAngle");
    startAngle = reader.getAttribute<double>("value");
    reader.readElement("EndAngle");
    endAngle = reader.getAttribute<double>("value");
    reader.readElement("Clockwise");
    cw = reader.getAttribute<bool>("value");
    reader.readElement("Large");
    largeArc = reader.getAttribute<bool>("value");
}

//! Generic is a multiline
Generic::Generic(const TopoDS_Edge &e)
{
    geomType = GeomType::GENERIC;
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
    geomType = GeomType::GENERIC;
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
    int stop = reader.getAttribute<long>("PointsCount");
    int i = 0;
    for ( ; i < stop; i++) {
        reader.readElement("Point");
        Base::Vector3d p;
        p.x = reader.getAttribute<double>("X");
        p.y = reader.getAttribute<double>("Y");
        p.z = reader.getAttribute<double>("Z");
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
        return std::numeric_limits<double>::max();
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
    geomType = GeomType::BSPLINE;
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
         startAngle += 2.0 * std::numbers::pi;
    }
    endAngle = atan2(endPnt.y, endPnt.x);
    if (endAngle < 0) {
         endAngle += 2.0 * std::numbers::pi;
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


//! Can this B-spline be represented by a straight line?
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

// make a circular edge from B-spline
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
    geomType = GeomType::BEZIER;
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
Vertex::Vertex() :
    extractType(ExtractionType::Plain),    // obsolete?
    hlrVisible(false),
    ref3D(-1),                              // obsolete
    m_center(false),
    cosmetic(false),
    cosmeticLink(-1),
    m_reference(false)

{
    pnt = Base::Vector3d(0.0, 0.0, 0.0);
    BRepBuilderAPI_MakeVertex mkVert(gp_Pnt(0.0, 0.0, 0.0));
    occVertex = mkVert.Vertex();
    cosmeticTag = std::string();
}

Vertex::Vertex(const Vertex* v) :
    extractType(v->extractType),    // obsolete?
    hlrVisible(v->hlrVisible),
    ref3D(v->ref3D),                              // obsolete
    m_center(v->m_center),
    occVertex(v->occVertex),
    cosmetic(v->cosmetic),
    cosmeticLink(v->cosmeticLink),
    cosmeticTag(v->cosmeticTag),
    m_reference(v->m_reference)
{
    pnt = v->point();
}

Vertex::Vertex(double x, double y) :
    extractType(ExtractionType::Plain),    // obsolete?
    hlrVisible(false),
    ref3D(-1),                              // obsolete
    m_center(false),
    cosmetic(false),
    cosmeticLink(-1),
    m_reference(false)
{
    pnt = Base::Vector3d(x, y, 0.0);
    BRepBuilderAPI_MakeVertex mkVert(gp_Pnt(x, y, 0.0));
    occVertex = mkVert.Vertex();
    cosmeticTag = std::string();
}

Vertex::Vertex(Base::Vector3d v) : Vertex(v.x, v.y)
{

}


bool Vertex::isEqual(const Vertex& v, double tol)
{
    double dist = (pnt - (v.pnt)).Length();
    return (dist <= tol);
}

void Vertex::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Point "
                << "X=\"" <<  pnt.x <<
                "\" Y=\"" <<  pnt.y <<
                "\" Z=\"" <<  pnt.z <<
                 "\"/>" << '\n';

    writer.Stream() << writer.ind() << "<Extract value=\"" <<  extractType << "\"/>" << '\n';
    const char v = hlrVisible ? '1':'0';
    writer.Stream() << writer.ind() << "<HLRVisible value=\"" <<  v << "\"/>" << '\n';
    writer.Stream() << writer.ind() << "<Ref3D value=\"" <<  ref3D << "\"/>" << '\n';
    const char c = m_center ?'1':'0';
    writer.Stream() << writer.ind() << "<IsCenter value=\"" <<  c << "\"/>" << '\n';
    const char c2 = cosmetic?'1':'0';
    writer.Stream() << writer.ind() << "<Cosmetic value=\"" <<  c2 << "\"/>" << '\n';
    writer.Stream() << writer.ind() << "<CosmeticLink value=\"" <<  cosmeticLink << "\"/>" << '\n';
    writer.Stream() << writer.ind() << "<CosmeticTag value=\"" <<  cosmeticTag << "\"/>" << '\n';
}

void Vertex::Restore(Base::XMLReader &reader)
{
    reader.readElement("Point");
    pnt.x = reader.getAttribute<double>("X");
    pnt.y = reader.getAttribute<double>("Y");
    pnt.z = reader.getAttribute<double>("Z");

    reader.readElement("Extract");
    extractType = reader.getAttribute<ExtractionType>("value");
    reader.readElement("HLRVisible");
    hlrVisible = reader.getAttribute<bool>("value");
    reader.readElement("Ref3D");
    ref3D = reader.getAttribute<int>("value");
    reader.readElement("IsCenter");
    m_center = reader.getAttribute<bool>("value");
    reader.readElement("Cosmetic");
    cosmetic = reader.getAttribute<bool>("value");
    reader.readElement("CosmeticLink");
    cosmeticLink = reader.getAttribute<int>("value");
    reader.readElement("CosmeticTag");
    cosmeticTag = reader.getAttribute<const char*>("value");

    // restore tag from VertexTag if it exists
    restoreVertexTag(reader);

    BRepBuilderAPI_MakeVertex mkVert(gp_Pnt(pnt.x, pnt.y, pnt.z));
    occVertex = mkVert.Vertex();
}

//! look at the next element in the file.  If it is a VertexTag, set the tag.
//! readNextElement will stop searching when it encounters an end element (ex: </CosmeticVertex>) or
//! end of document.
void Vertex::restoreVertexTag(Base::XMLReader& reader)
{
    if (!reader.readNextElement()) {
        return;
    }

    if(strcmp(reader.localName(),"VertexTag") == 0) {
        std::string temp = reader.getAttribute<const char*>("value");
        setTag(Tag::fromString(temp));
    }
    // else we can not set the tag here.  if this is a CosmeticVertex, the tag will be set later.
    // the tag is not used for geometry vertices.
}

void Vertex::dump(const char* title)
{
    Base::Console().message("TD::Vertex - %s - point: %s vis: %d cosmetic: %d  cosLink: %d cosTag: %s\n",
                            title, DrawUtil::formatVector(pnt).c_str(), hlrVisible, cosmetic, cosmeticLink,
                            cosmeticTag.c_str());
}

TopoShape Vertex::asTopoShape(double scale)
{
    Base::Vector3d point = Base::convertTo<Base::Vector3d>(BRep_Tool::Pnt(getOCCVertex()));
    point = point / scale;
    BRepBuilderAPI_MakeVertex mkVert(Base::convertTo<gp_Pnt>(point));
    return {mkVert.Vertex()};
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
//    Base::Console().message("GU::edgeFromGeneric()\n");
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
    BRepBuilderAPI_MakeEdge aMakeEdge(hCircle, 0.0, 2.0 * std::numbers::pi);
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
bool GeometryUtils::isCircle(const TopoDS_Edge& occEdge)
{
    double radius{0};
    Base::Vector3d center;
    bool isArc = false;
    return GeometryUtils::getCircleParms(occEdge, radius, center, isArc);
}

//! tries to interpret a B-spline edge as a circle. Used by DVDim for approximate dimensions.
//! calculates the radius and center of circles using groups of 4 points on the b-spline.  if the
//! groups of 4 points all lie on a circle, we use that circle to get the radius and center.
bool GeometryUtils::getCircleParms(const TopoDS_Edge& occEdge, double& radius, Base::Vector3d& center, bool& isArc)
{
    constexpr int PointCount{8};    // number of points on the edge to examine (>= 8)
    constexpr int TestCount{3};     // number of candidate circles to test

    BRepAdaptor_Curve curveAdapt(occEdge);
    double firstParam = curveAdapt.FirstParameter();
    auto firstPoint = Base::convertTo<Base::Vector3d>(curveAdapt.Value(firstParam));
    double lastParam = curveAdapt.LastParameter();
    auto lastPoint = Base::convertTo<Base::Vector3d>(curveAdapt.Value(lastParam));

    double parmRange = fabs(lastParam - firstParam);
    double parmStep = parmRange / PointCount;

    std::vector<Base::Vector3d> pointsOnCurve;
    for (size_t iPoint = 0; iPoint < PointCount; iPoint++) {
        auto iPointMath = static_cast<double>(iPoint);
        auto newpoint = curveAdapt.Value(firstParam + iPointMath * parmStep);
        pointsOnCurve.push_back(Base::convertTo<Base::Vector3d>(newpoint));
    }

    double tolerance = EWTOLERANCE;     // not as demanding as Precision::Confusion() but more
                                        // demanding than using the edge length

    isArc = false;
    if (!firstPoint.IsEqual(lastPoint, tolerance)) {
        // we were dropping information by not including lastPoint
        pointsOnCurve.push_back(lastPoint);
        isArc = true;
    }

    int passCount{0};
    int firstIndex{0};
    for (int iTest = 0; iTest < TestCount; iTest++) {
        firstIndex++;
        auto A = pointsOnCurve.at(firstIndex);
        auto B = pointsOnCurve.at(firstIndex + 1);
        auto C = pointsOnCurve.at(firstIndex + 2);
        auto D = pointsOnCurve.at(firstIndex + 3);
        if (pointsAreOnCircle(A, B, C, D, tolerance)) {
            passCount++;
        }
    }

    if (passCount != TestCount) {
        // at least 1 test failed.
        return false;
    }

    // each group of 4 points lies on a circle.  Since the groups of 4 overlap, all the points lie
    // on the same circle. https://en.wikipedia.org/wiki/Ptolemy%27s_theorem  and
    // https://math.stackexchange.com/questions/3130053/how-to-check-if-a-set-of-points-in-cartesian-space-could-lie-on-the-circumferenc
    // so we can use any three points to make our circle.

    auto gPoint0 = Base::convertTo<gp_Pnt>(pointsOnCurve.at(1));
    auto gPoint1 = Base::convertTo<gp_Pnt>(pointsOnCurve.at(3));
    auto gPoint2 = Base::convertTo<gp_Pnt>(pointsOnCurve.at(5));    //NOLINT readability-magic-numbers
    try {
        GC_MakeCircle mkCircle(gPoint0, gPoint1, gPoint2);
        if (!mkCircle.IsDone()) {
            return false;
        }

        const Handle(Geom_Circle) circleFromParms = mkCircle.Value();
        radius = circleFromParms->Circ().Radius();
        center = Base::convertTo<Base::Vector3d>(circleFromParms->Circ().Location());
        return true;
    }
    catch (Standard_Failure&) {
        // we think this is a circle, but occt disagrees
        Base::Console().message("Geo::getCircleParms - failed to make a circle\n");
    }

    return false;
}


//! returns true if the A, B, C and D all lie on the same circle according to Ptolemy's theorem
//! we can skip the test for same plane, since the points are all on the XY plane(?not true
//! for 3d dims?).
bool GeometryUtils::pointsAreOnCircle(Base::Vector3d A,
                                      Base::Vector3d B,
                                      Base::Vector3d C,
                                      Base::Vector3d D,
                                      double tolerance)
{
    auto AB = (B-A).Length();
    auto AC = (C-A).Length();
    auto AD = (D-A).Length();
    auto BC = (C-B).Length();
    auto BD = (D-B).Length();
    auto CD = (D-C).Length();

    auto pieceLength = AB + BC + CD;
    auto wholeLength = AD;
    if (DU::fpCompare(pieceLength, wholeLength, tolerance)) {
        // these points are colinear
        return false;
    }

    bool eq1 = DU::fpCompare(AB*CD + AC*BD, AD*BC, tolerance);
    bool eq2 = DU::fpCompare(AB*CD + AD*BC, AC*BD, tolerance);
    bool eq3 = DU::fpCompare(AC*BD + AD*BC, AB*CD, tolerance);
    return eq1 || eq2  || eq3;
}

//! make a circle or arc of circle Edge from BSpline Edge
// Note that the input edge has been inverted by GeometryObject, so +Y points down.
TopoDS_Edge GeometryUtils::asCircle(const TopoDS_Edge& splineEdge, bool& arc)
{
    double radius{0};
    Base::Vector3d center;
    bool isArc = false;
    bool canMakeCircle = GeometryUtils::getCircleParms(splineEdge, radius, center, isArc);
    if (!canMakeCircle) {
        throw Base::RuntimeError("GU::asCircle received non-circular edge!");
    }
    arc = isArc;

    gp_Pnt gCenter = Base::convertTo<gp_Pnt>(center);
    gp_Dir gNormal{0, 0, 1};
    Handle(Geom_Circle) circleFromParms = GC_MakeCircle(gCenter, gNormal, radius);

    if (!isArc) {
        return BRepBuilderAPI_MakeEdge(circleFromParms);
    }

    // find the ends of the edge from the underlying curve
    BRepAdaptor_Curve curveAdapt(splineEdge);
    double firstParam = curveAdapt.FirstParameter();
    double lastParam = curveAdapt.LastParameter();
    gp_Pnt startPoint = curveAdapt.Value(firstParam);
    gp_Pnt endPoint = curveAdapt.Value(lastParam);

    double midRange = (lastParam + firstParam) / 2;
    gp_Pnt midPoint = curveAdapt.Value(midRange);

    // this should be using circleFromParms as a base instead of points on the original spline.
    // could be problems with very small errors??  other versions of GC_MakeArcOfCircle have
    // poorly explained parameters.
    GC_MakeArcOfCircle mkArc(startPoint, midPoint, endPoint);
    auto circleArc = mkArc.Value();
    if (!mkArc.IsDone()) {
        throw Base::RuntimeError("GU::asCircle failed to create arc");
    }
    return BRepBuilderAPI_MakeEdge(circleArc);
}

bool GeometryUtils::isLine(const TopoDS_Edge& occEdge)
{
    BRepAdaptor_Curve adapt(occEdge);

    Handle(Geom_BSplineCurve) spline = adapt.BSpline();
    double firstParm = adapt.FirstParameter();
    double lastParm = adapt.LastParameter();
    auto startPoint = Base::convertTo<Base::Vector3d>(adapt.Value(firstParm));
    auto endPoint = Base::convertTo<Base::Vector3d>(adapt.Value(lastParm));
    auto edgeLong = edgeLength(occEdge);

    constexpr double LimitFactor{0.001};     // 0.1%  not sure about this value
    double tolerance = edgeLong * LimitFactor;
    if (startPoint.IsEqual(endPoint, tolerance)) {
        // either not a line or a zero length line?
        return false;
    }

    // in a line the sum of the lengths of the segments should equal the distance
    // from start to end
    double endPointLength = (endPoint - startPoint).Length();

    int low = 0;
    int high = spline->NbPoles() - 1;
    TColgp_Array1OfPnt poles(low, high);
    spline->Poles(poles);
    double lenTotal = 0.0;
    for (int i = 0; i < high; i++) {
        gp_Pnt p1 = poles(i);
        Base::Vector3d v1 = Base::convertTo<Base::Vector3d>(p1);
        gp_Pnt p2 = poles(i+1);
        Base::Vector3d v2 = Base::convertTo<Base::Vector3d>(p2);
        lenTotal += (v2-v1).Length();
    }

    return DrawUtil::fpCompare(lenTotal, endPointLength, tolerance);
}


//! make a line Edge from B-spline Edge
TopoDS_Edge GeometryUtils::asLine(const TopoDS_Edge& occEdge)
{
    BRepAdaptor_Curve c(occEdge);

    // find the two ends
    Handle(Geom_Curve) curve = c.Curve().Curve();
    double first = c.FirstParameter();
    double last = c.LastParameter();
    gp_Pnt start = c.Value(first);
    gp_Pnt end = c.Value(last);

    TopoDS_Edge result = BRepBuilderAPI_MakeEdge(start, end);
    return result;
}


double GeometryUtils::edgeLength(TopoDS_Edge occEdge)
{
    BRepAdaptor_Curve adapt(occEdge);
    const Handle(Geom_Curve) curve = adapt.Curve().Curve();
    double first = BRepLProp_CurveTool::FirstParameter(adapt);
    double last = BRepLProp_CurveTool::LastParameter(adapt);
    try {
        GeomAdaptor_Curve adaptor(curve);
        return GCPnts_AbscissaPoint::Length(adaptor,first,last,Precision::Confusion());
    }
    catch (Standard_Failure& exc) {
        THROWM(Base::CADKernelError, exc.GetMessageString())
    }
}

//! return a perforated shape/face (using Part::FaceMakerCheese) formed by creating holes in the input face.
TopoDS_Face GeometryUtils::makePerforatedFace(FacePtr bigCheese,  const std::vector<FacePtr> &holesAll)
{
    std::vector<TopoDS_Wire> cheeseIngredients;

    // v0.0 brute force

    // Note: TD Faces are not perforated and should only ever have 1 wire.  They are capable of
    // having voids, but for now we will just take the first contour wire in all cases.

    if (bigCheese->wires.empty())  {
        // run in circles.  scream and shout.
        return {};
    }

    auto flippedFace = ShapeUtils::fromQtAsFace(bigCheese->toOccFace());

    if (holesAll.empty()) {
        return flippedFace;
    }

    auto outer = ShapeUtils::fromQtAsWire(bigCheese->wires.front()->toOccWire());
    cheeseIngredients.push_back(outer);
    for (auto& hole : holesAll) {
        if (hole->wires.empty()) {
            continue;
        }
        auto holeR3 = ShapeUtils::fromQtAsWire(hole->wires.front()->toOccWire());
        cheeseIngredients.push_back(holeR3);
    }

    TopoDS_Shape faceShape;
    try {
        faceShape = Part::FaceMakerCheese::makeFace(cheeseIngredients);
    }
    catch (const Standard_Failure&) {
        Base::Console().warning("Area - could not make holes in face\n");
        return flippedFace;
    }


    // v0.0 just grab the first face
    TopoDS_Face foundFace;
    TopExp_Explorer expFaces(faceShape, TopAbs_FACE);
    if (expFaces.More()) {
        foundFace = TopoDS::Face(expFaces.Current());
    }
    // TODO: sort out the compound => shape but !compound => face business in FaceMakerCheese here.
    //       first guess is it does not affect us?

    return foundFace;
}


//! find faces within the bounds of the input face
std::vector<FacePtr> GeometryUtils::findHolesInFace(const DrawViewPart* dvp, const std::string& bigCheeseSubRef)
{
    if (!dvp || bigCheeseSubRef.empty()) {
        return {};
    }

    std::vector<FacePtr> holes;
    auto bigCheeseIndex = DU::getIndexFromName(bigCheeseSubRef);

    // v0.0 brute force
    auto facesAll = dvp->getFaceGeometry();
    if (facesAll.empty()) {
        // tarfu
        throw Base::RuntimeError("GU::findHolesInFace - no holes to find!!");
    }

    auto bigCheeseFace = facesAll.at(bigCheeseIndex);
    auto bigCheeseOCCFace = bigCheeseFace->toOccFace();
    auto bigCheeseArea = bigCheeseFace->getArea();

    int iFace{0};
    for (auto& face : facesAll) {
        if (iFace == bigCheeseIndex) {
            iFace++;
            continue;
        }
        if (face->getArea() > bigCheeseArea) {
            iFace++;
            continue;
        }
        auto faceCenter = Base::convertTo<gp_Pnt>(face->getCenter());
        auto faceCenterVertex = BRepBuilderAPI_MakeVertex(faceCenter);
        auto distance = DU::simpleMinDist(faceCenterVertex, bigCheeseOCCFace);
        if (distance > EWTOLERANCE) {
            // hole center not within outer contour.  not the best test but cheese maker handles it
            // for us?
            // FaceMakerCheese does not support partial overlaps and just ignores them?
            iFace++;
            continue;
        }
        holes.push_back(face);
        iFace++;
    }

    return holes;
}

