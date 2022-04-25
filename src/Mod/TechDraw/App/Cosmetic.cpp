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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <cmath>
# include <boost/uuid/uuid.hpp>
# include <boost/uuid/uuid_io.hpp>
# include <boost/uuid/uuid_generators.hpp>
# include <gp_Pnt.hxx>
# include <gp_Dir.hxx>
# include <gp_Ax1.hxx>
# include <gp_Circ.hxx>
# include <Geom_Circle.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Edge.hxx>
# include <BRepBndLib.hxx>
# include <Bnd_Box.hxx>
# include <Precision.hxx>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Parameter.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Vector3D.h>
#include <Base/Writer.h>

#include <App/Application.h>
#include <App/Material.h>

#include <Mod/TechDraw/App/GeomFormatPy.h>
#include <Mod/TechDraw/App/CenterLinePy.h>
#include <Mod/TechDraw/App/CosmeticEdgePy.h>
#include <Mod/TechDraw/App/CosmeticVertexPy.h>

#include "DrawUtil.h"
#include "Preferences.h"
#include "LineGroup.h"
#include "GeometryObject.h"
#include "Geometry.h"
#include "DrawViewPart.h"

#include "Cosmetic.h"

using namespace TechDraw;
using namespace std;

#define GEOMETRYEDGE 0
#define COSMETICEDGE 1
#define CENTERLINE   2

LineFormat::LineFormat()
{
    m_style = getDefEdgeStyle();
    m_weight = getDefEdgeWidth();
    m_color = getDefEdgeColor();
    m_visible = true;
}

LineFormat::LineFormat(int style,
               double weight,
               App::Color color,
               bool visible ) :
    m_style(style),
    m_weight(weight),
    m_color(color),
    m_visible(visible)
{
}

void LineFormat::dump(const char* title)
{
    Base::Console().Message("LF::dump - %s \n",title);
    Base::Console().Message("LF::dump - %s \n",toString().c_str());
}

std::string LineFormat::toString(void) const
{
    std::stringstream ss;
    ss << m_style << "," <<
          m_weight << "," <<
          m_color.asHexString() << "," <<
          m_visible;
    return ss.str();
}

//static preference getters.
double LineFormat::getDefEdgeWidth()
{
    int lgNumber = Preferences::lineGroup();
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgNumber);

    double width = lg->getWeight("Graphic");
    delete lg; 
    return width;
}

App::Color LineFormat::getDefEdgeColor()
{
    return Preferences::normalColor();
}

int LineFormat::getDefEdgeStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    int style = hGrp->GetInt("CosmoCLStyle", 2);   //dashed
    return style;
}

//****************************************************************************************

TYPESYSTEM_SOURCE(TechDraw::CosmeticVertex, Base::Persistence)

CosmeticVertex::CosmeticVertex() : TechDraw::Vertex()
{
    point(Base::Vector3d(0.0, 0.0, 0.0));
    permaPoint = Base::Vector3d(0.0, 0.0, 0.0);
    linkGeom = -1;
    color = Preferences::vertexColor();
    size  = Preferences::vertexScale() * 
            LineGroup::getDefaultWidth("Thick", Preferences::lineGroup());
    style = 1;
    visible = true;
    hlrVisible = true;
    cosmetic = true;

    createNewTag();
}

CosmeticVertex::CosmeticVertex(const TechDraw::CosmeticVertex* cv) : TechDraw::Vertex(cv)
{
    permaPoint = cv->permaPoint;
    linkGeom = cv->linkGeom;
    color = cv->color;
    size  = cv->size;
    style = cv->style;
    visible = cv->visible;
    hlrVisible = true;
    cosmetic = true;

    createNewTag();
}

CosmeticVertex::CosmeticVertex(Base::Vector3d loc) : TechDraw::Vertex(loc)
{
    permaPoint = loc;
    linkGeom = -1;
    color = Preferences::vertexColor();
    size  = Preferences::vertexScale() * 
            LineGroup::getDefaultWidth("Thick", Preferences::lineGroup());
    style = 1;        //TODO: implement styled vertexes
    visible = true;
    hlrVisible = true;
    cosmetic = true;

    createNewTag();

}

void CosmeticVertex::move(Base::Vector3d newPos)
{
    permaPoint = newPos;
}

void CosmeticVertex::moveRelative(Base::Vector3d movement)
{
    permaPoint += movement;
}

std::string CosmeticVertex::toString(void) const
{
    std::stringstream ss;
    ss << permaPoint.x << "," <<
          permaPoint.y << "," <<
          permaPoint.z << "," <<
          " / ";
    ss << point().x << "," <<
          point().y << "," <<
          point().z << "," <<
          " / " <<
          linkGeom << "," <<
          " / " <<
          color.asHexString() << ","  <<
          " / " <<
          size << "," <<
          " / " <<
          style << ","  <<
          " / " <<
          visible << " / " ;
    ss << getTagAsString();
    return ss.str();
}

// Persistence implementers
unsigned int CosmeticVertex::getMemSize (void) const
{
    return 1;
}

void CosmeticVertex::Save(Base::Writer &writer) const
{
    TechDraw::Vertex::Save(writer);
    writer.Stream() << writer.ind() << "<PermaPoint "
                << "X=\"" <<  permaPoint.x <<
                "\" Y=\"" <<  permaPoint.y <<
                "\" Z=\"" <<  permaPoint.z <<
                 "\"/>" << endl;
    writer.Stream() << writer.ind() << "<LinkGeom value=\"" <<  linkGeom << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Color value=\"" <<  color.asHexString() << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Size value=\"" <<  size << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Style value=\"" <<  style << "\"/>" << endl;
    const char v = visible?'1':'0';
    writer.Stream() << writer.ind() << "<Visible value=\"" <<  v << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Tag value=\"" <<  getTagAsString() << "\"/>" << endl;
}

void CosmeticVertex::Restore(Base::XMLReader &reader)
{
    if (!CosmeticVertex::restoreCosmetic()) {
        return;
    }
    TechDraw::Vertex::Restore(reader);
    reader.readElement("PermaPoint");
    permaPoint.x = reader.getAttributeAsFloat("X");
    permaPoint.y = reader.getAttributeAsFloat("Y");
    permaPoint.z = reader.getAttributeAsFloat("Z");
    reader.readElement("LinkGeom");
    linkGeom = reader.getAttributeAsInteger("value");
    reader.readElement("Color");
    std::string temp = reader.getAttribute("value");
    color.fromHexString(temp);
    reader.readElement("Size");
    size = reader.getAttributeAsFloat("value");
    reader.readElement("Style");
    style = reader.getAttributeAsInteger("value");
    reader.readElement("Visible");
    visible = (int)reader.getAttributeAsInteger("value")==0?false:true;
    reader.readElement("Tag");
    temp = reader.getAttribute("value");
    boost::uuids::string_generator gen;
    boost::uuids::uuid u1 = gen(temp);
    tag = u1;
}

Base::Vector3d CosmeticVertex::scaled(double factor)
{
    pnt = permaPoint * factor;
    return pnt;
}    

boost::uuids::uuid CosmeticVertex::getTag() const
{
    return tag;
}

std::string CosmeticVertex::getTagAsString(void) const
{
    std::string tmp = boost::uuids::to_string(getTag());
    return tmp;
}

void CosmeticVertex::createNewTag()
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

void CosmeticVertex::assignTag(const TechDraw::CosmeticVertex * cv)
{
    if(cv->getTypeId() == this->getTypeId())
        this->tag = cv->tag;
    else
        throw Base::TypeError("CosmeticVertex tag can not be assigned as types do not match.");
}

CosmeticVertex* CosmeticVertex::copy(void) const
{
//    Base::Console().Message("CV::copy()\n");
    CosmeticVertex* newCV = new CosmeticVertex(this);
    return newCV;
}

CosmeticVertex* CosmeticVertex::clone(void) const
{
//    Base::Console().Message("CV::clone()\n");
    CosmeticVertex* cpy = this->copy();
    cpy->tag = this->tag;
    return cpy;
}

PyObject* CosmeticVertex::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new CosmeticVertexPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}


void CosmeticVertex::dump(const char* title)
{
    Base::Console().Message("CV::dump - %s \n",title);
    Base::Console().Message("CV::dump - %s \n",toString().c_str());
}

//******************************************

TYPESYSTEM_SOURCE(TechDraw::CosmeticEdge,Base::Persistence)

//note this ctor has no occEdge or first/last point for geometry!
CosmeticEdge::CosmeticEdge()
{
//    Base::Console().Message("CE::CE()\n");
    permaRadius = 0.0;
    m_geometry = std::make_shared<TechDraw::BaseGeom> ();
    initialize();
}

CosmeticEdge::CosmeticEdge(CosmeticEdge* ce)
{
//    Base::Console().Message("CE::CE(ce)\n");
    TechDraw::BaseGeomPtr newGeom = ce->m_geometry->copy();
    //these endpoints are already YInverted
    permaStart = ce->permaStart;
    permaEnd   = ce->permaEnd;
    permaRadius = ce->permaRadius; 
    m_geometry = newGeom;
    m_format   = ce->m_format;
    initialize();
}

CosmeticEdge::CosmeticEdge(Base::Vector3d pt1, Base::Vector3d pt2)
{
//    Base::Console().Message("CE::CE(p1,p2)\n");
    Base::Vector3d p1 = DrawUtil::invertY(pt1);
    Base::Vector3d p2 = DrawUtil::invertY(pt2);
    gp_Pnt gp1(p1.x,p1.y,p1.z);
    gp_Pnt gp2(p2.x,p2.y,p2.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
    m_geometry = TechDraw::BaseGeom::baseFactory(e);
    permaStart = p1;
    permaEnd   = p2;
    initialize();
}

CosmeticEdge::CosmeticEdge(TopoDS_Edge e)
{
//    Base::Console().Message("CE::CE(TopoDS_Edge)\n");
    m_geometry = TechDraw::BaseGeom::baseFactory(e);
    //we assume input edge is already in Yinverted coordinates
    permaStart = m_geometry->getStartPoint();
    permaEnd   = m_geometry->getEndPoint();
    if ((m_geometry->geomType == TechDraw::GeomType::CIRCLE) ||
        (m_geometry->geomType == TechDraw::GeomType::ARCOFCIRCLE) ) {
       TechDraw::CirclePtr circ = std::static_pointer_cast<TechDraw::Circle>(m_geometry);
       permaStart  = circ->center;
       permaEnd    = circ->center;
       permaRadius = circ->radius;
    } 
    initialize();
}

CosmeticEdge::CosmeticEdge(TechDraw::BaseGeomPtr g)
{
//    Base::Console().Message("CE::CE(bg)\n");
    m_geometry = g;
    permaStart = m_geometry->getStartPoint();
    permaEnd   = m_geometry->getEndPoint();
    if ((g->geomType == TechDraw::GeomType::CIRCLE) ||
       (g->geomType == TechDraw::GeomType::ARCOFCIRCLE)) {
       TechDraw::CirclePtr circ = std::static_pointer_cast<TechDraw::Circle>(g);
       permaStart  = circ->center;
       permaEnd    = circ->center;
       permaRadius = circ->radius;
    } 
    initialize();
}

CosmeticEdge::~CosmeticEdge(void)
{
    //shared pointer will delete m_geometry when ref count goes to zero.
}

void CosmeticEdge::initialize(void)
{
    m_geometry->classOfEdge = ecHARD;
    m_geometry->hlrVisible = true;
    m_geometry->cosmetic = true;
    m_geometry->source(COSMETICEDGE);

    createNewTag();
    m_geometry->setCosmeticTag(getTagAsString());
}

TechDraw::BaseGeomPtr CosmeticEdge::scaledGeometry(double scale)
{
    TechDraw::BaseGeomPtr newGeom = nullptr;
    TopoDS_Edge e = m_geometry->occEdge;
    TopoDS_Shape s = TechDraw::scaleShape(e, scale);
    TopoDS_Edge newEdge = TopoDS::Edge(s);
    newGeom = TechDraw::BaseGeom::baseFactory(newEdge);
    newGeom->classOfEdge = ecHARD;
    newGeom->hlrVisible = true;
    newGeom->cosmetic = true;
    newGeom->source(COSMETICEDGE);
    newGeom->setCosmeticTag(getTagAsString());
    return newGeom;
}

std::string CosmeticEdge::toString(void) const
{
    std::stringstream ss;
    ss << getTagAsString() << ", $$$, ";
    if (m_geometry != nullptr) {
        ss << m_geometry->geomType << 
            ",$$$," <<
            m_geometry->toString() <<
            ",$$$," <<
            m_format.toString();
    }
    return ss.str();
}

void CosmeticEdge::dump(const char* title)
{
    Base::Console().Message("CE::dump - %s \n",title);
    Base::Console().Message("CE::dump - %s \n",toString().c_str());
}

// Persistence implementers
unsigned int CosmeticEdge::getMemSize (void) const
{
    return 1;
}

void CosmeticEdge::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Style value=\"" <<  m_format.m_style << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Weight value=\"" <<  m_format.m_weight << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Color value=\"" <<  m_format.m_color.asHexString() << "\"/>" << endl;
    const char v = m_format.m_visible?'1':'0';
    writer.Stream() << writer.ind() << "<Visible value=\"" <<  v << "\"/>" << endl;

    writer.Stream() << writer.ind() << "<GeometryType value=\"" << m_geometry->geomType <<"\"/>" << endl;
    if (m_geometry->geomType == TechDraw::GeomType::GENERIC) {
        GenericPtr gen = std::static_pointer_cast<Generic>(m_geometry);
        gen->Save(writer);
    } else if (m_geometry->geomType == TechDraw::GeomType::CIRCLE) {
        TechDraw::CirclePtr circ = std::static_pointer_cast<TechDraw::Circle>(m_geometry);
        circ->Save(writer);
    } else if (m_geometry->geomType == TechDraw::GeomType::ARCOFCIRCLE) {
        TechDraw::AOCPtr aoc = std::static_pointer_cast<TechDraw::AOC>(m_geometry);
        aoc->Save(writer);
    } else {
        Base::Console().Warning("CE::Save - unimplemented geomType: %d\n", m_geometry->geomType);
    }
}

void CosmeticEdge::Restore(Base::XMLReader &reader)
{
    if (!CosmeticVertex::restoreCosmetic()) {
        return;
    }
//    Base::Console().Message("CE::Restore - reading elements\n");
    reader.readElement("Style");
    m_format.m_style = reader.getAttributeAsInteger("value");
    reader.readElement("Weight");
    m_format.m_weight = reader.getAttributeAsFloat("value");
    reader.readElement("Color");
    std::string temp = reader.getAttribute("value");
    m_format.m_color.fromHexString(temp);
    reader.readElement("Visible");
    m_format.m_visible = (int)reader.getAttributeAsInteger("value")==0?false:true;
    reader.readElement("GeometryType");
    TechDraw::GeomType gType = (TechDraw::GeomType)reader.getAttributeAsInteger("value");

    if (gType == TechDraw::GeomType::GENERIC) {
        TechDraw::GenericPtr gen = std::make_shared<TechDraw::Generic> ();
        gen->Restore(reader);
        gen->occEdge = GeometryUtils::edgeFromGeneric(gen);
        m_geometry = (TechDraw::BaseGeomPtr) gen;
        permaStart = gen->getStartPoint();
        permaEnd   = gen->getEndPoint();
    } else if (gType == TechDraw::GeomType::CIRCLE) {
        TechDraw::CirclePtr circ = std::make_shared<TechDraw::Circle> ();
        circ->Restore(reader);
        circ->occEdge = GeometryUtils::edgeFromCircle(circ);
        m_geometry = (TechDraw::BaseGeomPtr) circ;
        permaRadius = circ->radius;
        permaStart  = circ->center;
        permaEnd    = circ->center;
    } else if (gType == TechDraw::GeomType::ARCOFCIRCLE) {
        TechDraw::AOCPtr aoc = std::make_shared<TechDraw::AOC> ();
        aoc->Restore(reader);
        aoc->occEdge = GeometryUtils::edgeFromCircleArc(aoc);
        m_geometry = (TechDraw::BaseGeomPtr) aoc;
        permaStart = aoc->startPnt;
        permaEnd   = aoc->endPnt;
        permaRadius = aoc->radius;
    } else {
        Base::Console().Warning("CE::Restore - unimplemented geomType: %d\n", gType);
    }
}

boost::uuids::uuid CosmeticEdge::getTag() const
{
    return tag;
}

std::string CosmeticEdge::getTagAsString(void) const
{
    std::string tmp = boost::uuids::to_string(getTag());
    return tmp;
}

void CosmeticEdge::createNewTag()
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

void CosmeticEdge::assignTag(const TechDraw::CosmeticEdge * ce)
{
    if(ce->getTypeId() == this->getTypeId())
        this->tag = ce->tag;
    else
        throw Base::TypeError("CosmeticEdge tag can not be assigned as types do not match.");
}

CosmeticEdge* CosmeticEdge::copy(void) const
{
//    Base::Console().Message("CE::copy()\n");
    CosmeticEdge* newCE = new CosmeticEdge();
    TechDraw::BaseGeomPtr newGeom = m_geometry->copy();
    newCE->m_geometry = newGeom;
    newCE->m_format = m_format;
    return newCE;
}

CosmeticEdge* CosmeticEdge::clone(void) const
{
//    Base::Console().Message("CE::clone()\n");
    CosmeticEdge* cpy = this->copy();
    cpy->tag = this->tag;
    return cpy;
}

PyObject* CosmeticEdge::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new CosmeticEdgePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}


//*********************************************************

TYPESYSTEM_SOURCE(TechDraw::CenterLine,Base::Persistence)

CenterLine::CenterLine(void)
{
    m_start = Base::Vector3d(0.0, 0.0, 0.0);
    m_end = Base::Vector3d(0.0, 0.0, 0.0);
    m_mode = CLMODE::VERTICAL;
    m_hShift = 0.0;
    m_vShift = 0.0;
    m_rotate = 0.0;
    m_extendBy = 0.0;
    m_type = CLTYPE::FACE;
    m_flip2Line = false;

    m_geometry = std::make_shared<TechDraw::BaseGeom> ();

    initialize();
}

CenterLine::CenterLine(TechDraw::CenterLine* cl)
{
    m_start = cl->m_start;
    m_end = cl->m_end;
    m_mode = cl->m_mode;
    m_hShift = cl->m_hShift;
    m_vShift = cl->m_vShift;
    m_rotate = cl->m_rotate;
    m_extendBy = cl->m_extendBy;
    m_type = cl->m_type;
    m_flip2Line = cl->m_flip2Line;

    m_geometry = cl->m_geometry;    //new BaseGeom(cl->m_geometry);??

    initialize();
}

CenterLine::CenterLine(TechDraw::BaseGeomPtr bg)
{
    m_start = bg->getStartPoint();
    m_end = bg->getEndPoint();
    m_mode = CLMODE::VERTICAL;
    m_hShift = 0.0;
    m_vShift = 0.0;
    m_rotate = 0.0;
    m_extendBy = 0.0;
    m_type = CLTYPE::FACE;
    m_flip2Line = false;

    m_geometry = bg;

    initialize();
}

CenterLine::CenterLine(Base::Vector3d pt1, Base::Vector3d pt2)
{
    m_start = pt1;
    m_end = pt2;
    m_mode = CLMODE::VERTICAL;
    m_hShift = 0.0;
    m_vShift = 0.0;
    m_rotate = 0.0;
    m_extendBy = 0.0;
    m_type = CLTYPE::FACE;
    m_flip2Line = false;

    Base::Vector3d p1 = DrawUtil::invertY(pt1);
    Base::Vector3d p2 = DrawUtil::invertY(pt2);
    gp_Pnt gp1(p1.x,p1.y,p1.z);
    gp_Pnt gp2(p2.x,p2.y,p2.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
    m_geometry = TechDraw::BaseGeom::baseFactory(e);

    initialize();
}

CenterLine::CenterLine(Base::Vector3d pt1, Base::Vector3d pt2,
                       int m, 
                       double h,
                       double v,
                       double r,
                       double x)
{
    m_start = pt1;
    m_end = pt2;
    m_mode = m;
    m_hShift = h;
    m_vShift = v;
    m_rotate = r;
    m_extendBy = x;
    m_type = CLTYPE::FACE;
    m_flip2Line = false;

    //not sure this is right?
    Base::Vector3d p1 = DrawUtil::invertY(pt1);
    Base::Vector3d p2 = DrawUtil::invertY(pt2);
    gp_Pnt gp1(p1.x,p1.y,p1.z);
    gp_Pnt gp2(p2.x,p2.y,p2.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
    m_geometry = TechDraw::BaseGeom::baseFactory(e);

    initialize();
}

CenterLine::~CenterLine()
{
}

void CenterLine::initialize()
{
    m_geometry->classOfEdge = ecHARD;
    m_geometry->hlrVisible = true;
    m_geometry->cosmetic = true;
    m_geometry->source(CENTERLINE);

    createNewTag();
    m_geometry->setCosmeticTag(getTagAsString());
}

CenterLine* CenterLine::CenterLineBuilder(DrawViewPart* partFeat, 
                                          std::vector<std::string> subNames, 
                                          int mode,
                                          bool flip)
{
//    Base::Console().Message("CL::CLBuilder()\n - subNames: %d", subNames.size());
    std::pair<Base::Vector3d, Base::Vector3d> ends;
    std::vector<std::string> faces;
    std::vector<std::string> edges;
    std::vector<std::string> verts;
    
    std::string geomType = TechDraw::DrawUtil::getGeomTypeFromName(subNames.front());
    int type = CLTYPE::FACE;
    if (geomType == "Face") {
        type = CLTYPE::FACE;
        ends = TechDraw::CenterLine::calcEndPoints(partFeat,
                             subNames,
                             mode,
                             0.0,
                             0.0, 0.0, 0.0);
        faces = subNames;
    } else if (geomType == "Edge") {
        type = CLTYPE::EDGE;
        ends = TechDraw::CenterLine::calcEndPoints2Lines(partFeat,
                         subNames,
                         mode,
                         0.0,
                         0.0, 0.0, 0.0, flip);
        edges = subNames;
    } else if (geomType == "Vertex") {
        type = CLTYPE::VERTEX;
        ends = TechDraw::CenterLine::calcEndPoints2Points(partFeat,
                         subNames,
                         mode,
                         0.0,
                         0.0, 0.0, 0.0, flip);
        verts = subNames;
    }
    if ((ends.first).IsEqual(ends.second, Precision::Confusion())) {
        Base::Console().Warning("CenterLineBuilder - endpoints are equal: %s\n",
                              DrawUtil::formatVector(ends.first).c_str());
        Base::Console().Warning("CenterLineBuilder - check V/H/A and/or Flip parameters\n");
        return nullptr;
    }
    TechDraw::CenterLine* cl = new TechDraw::CenterLine(ends.first, ends.second);
    if (cl != nullptr) {
        cl->m_type = type;
        cl->m_mode = mode;
        cl->m_faces = faces;
        cl->m_edges = edges;
        cl->m_verts = verts;
        cl->m_flip2Line = flip;
    }
    return cl;
}

TechDraw::BaseGeomPtr CenterLine::scaledGeometry(TechDraw::DrawViewPart* partFeat)
{
//    Base::Console().Message("CL::scaledGeometry() - m_type: %d\n", m_type);
    double scale = partFeat->getScale();
    std::pair<Base::Vector3d, Base::Vector3d> ends;
    try {
        if (m_faces.empty() &&
            m_edges.empty() &&
            m_verts.empty() ) {
//            Base::Console().Message("CL::scaledGeometry - no geometry to scale!\n");
            //CenterLine was created by points without a geometry reference,
            ends = calcEndPointsNoRef(m_start, m_end, scale, m_extendBy,
                                      m_hShift,m_vShift, m_rotate);
        } else if (m_type == CLTYPE::FACE) {
            ends = calcEndPoints(partFeat,
                                 m_faces,
                                 m_mode, m_extendBy,
                                 m_hShift,m_vShift, m_rotate);
        } else if (m_type == CLTYPE::EDGE) {
            ends = calcEndPoints2Lines(partFeat,
                                       m_edges,
                                       m_mode,
                                       m_extendBy,
                                       m_hShift, m_vShift, m_rotate, m_flip2Line);
        } else if (m_type == CLTYPE::VERTEX) {
            ends = calcEndPoints2Points(partFeat,
                                        m_verts,
                                        m_mode,
                                        m_extendBy,
                                        m_hShift, m_vShift, m_rotate, m_flip2Line);
        }
    }

    catch (...) {
        Base::Console().Error("CL::scaledGeometry - failed to calculate endpoints!\n");
        return nullptr;
    }

    TechDraw::BaseGeomPtr newGeom = nullptr;
    Base::Vector3d p1 = ends.first;
    Base::Vector3d p2 = ends.second;
    if (!p1.IsEqual(p2, 0.00001)) {
        gp_Pnt gp1(p1.x,p1.y,p1.z);
        gp_Pnt gp2(p2.x,p2.y,p2.z);
        TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
        TopoDS_Shape s = TechDraw::scaleShape(e, scale);
        TopoDS_Edge newEdge = TopoDS::Edge(s);
        newGeom = TechDraw::BaseGeom::baseFactory(newEdge);
        newGeom->classOfEdge = ecHARD;
        newGeom->hlrVisible = true;
        newGeom->cosmetic = true;
        newGeom->source(CENTERLINE);
        newGeom->setCosmeticTag(getTagAsString());
    } else { 
        Base::Console().Warning("Centerline endpoints are equal. Could not draw.\n");
        //what to do here?  //return current geom?
        return m_geometry;  
    }
    
    return newGeom;
}

std::string CenterLine::toString(void) const
{
    std::stringstream ss;
    ss << m_start.x << "," <<
          m_start.y << "," <<
          m_start.z << "," <<
          m_end.x << "," <<
          m_end.y << "," <<
          m_end.z << "," <<
          m_mode << "," <<
          m_type << "," <<
          m_hShift << "," <<
          m_vShift << "," <<
          m_rotate << "," <<
          m_flip2Line << "," <<
          m_extendBy << "," <<
          m_faces.size();
    if (!m_faces.empty()) {
        for (auto& f: m_faces) {
            if (!f.empty()) {
                ss << "," << f ;
            }
        }
    }

    std::string clCSV = ss.str();
    std::string fmtCSV = m_format.toString();
    return clCSV + ",$$$," + fmtCSV;
}

void CenterLine::dump(const char* title)
{
    Base::Console().Message("CL::dump - %s \n",title);
    Base::Console().Message("CL::dump - %s \n",toString().c_str());
}

//end points for centerline with no geometry reference
std::pair<Base::Vector3d, Base::Vector3d> CenterLine::calcEndPointsNoRef(
                                                      Base::Vector3d start,
                                                      Base::Vector3d end,
                                                      double scale,
                                                      double ext,
                                                      double hShift, double vShift,
                                                      double rotate)
{
//    Base::Console().Message("CL::calcEndPointsNoRef()\n");
    std::pair<Base::Vector3d, Base::Vector3d> result;
    Base::Vector3d p1 = start;
    Base::Vector3d p2 = end;
    Base::Vector3d mid = (p1 + p2) / 2.0;

    //extend
    Base::Vector3d clDir = p2 - p1;
    clDir.Normalize();
    p1 = p1 - (clDir * ext);
    p2 = p2 + (clDir * ext);

    //rotate
    if (!DrawUtil::fpCompare(rotate, 0.0)) {
        //rotate p1, p2 about mid point
        double revRotate = -rotate;
        double cosTheta = cos(revRotate * M_PI / 180.0);
        double sinTheta = sin(revRotate * M_PI / 180.0);
        Base::Vector3d toOrg = p1 - mid;
        double xRot = toOrg.x * cosTheta - toOrg.y * sinTheta;
        double yRot = toOrg.y * cosTheta + toOrg.x * sinTheta;
        p1 = Base::Vector3d(xRot, yRot, 0.0) + mid;
        toOrg = p2 - mid;
        xRot = toOrg.x * cosTheta - toOrg.y * sinTheta;
        yRot = toOrg.y * cosTheta + toOrg.x * sinTheta;
        p2 = Base::Vector3d(xRot, yRot, 0.0) + mid;
    }

    //shift
    if (!DrawUtil::fpCompare(hShift, 0.0)) {
        double hss = hShift * scale;
        p1.x = p1.x + hss;
        p2.x = p2.x + hss;
    }
    if (!DrawUtil::fpCompare(vShift, 0.0)) {
        double vss = vShift * scale;
        p1.y = p1.y + vss;
        p2.y = p2.y + vss;
    }

    result.first = p1 / scale;
    result.second = p2 / scale;
    return result;
}

//end points for face centerline
std::pair<Base::Vector3d, Base::Vector3d> CenterLine::calcEndPoints(DrawViewPart* partFeat,
                                                      std::vector<std::string> faceNames, 
                                                      int mode, double ext,
                                                      double hShift, double vShift,
                                                      double rotate)
{
//    Base::Console().Message("CL::calcEndPoints()\n");
    std::pair<Base::Vector3d, Base::Vector3d> result;
    if (faceNames.empty()) {
        Base::Console().Warning("CL::calcEndPoints - no faces!\n");
        return result;
    }

    Bnd_Box faceBox;
    faceBox.SetGap(0.0);

    double scale = partFeat->getScale();

    for (auto& fn: faceNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(fn) != "Face") {
            continue;
        }
        int idx = TechDraw::DrawUtil::getIndexFromName(fn);
        std::vector<TechDraw::BaseGeomPtr> faceEdges = 
                                                partFeat->getFaceEdgesByIndex(idx);
        if (!faceEdges.empty()) {
            for (auto& fe: faceEdges) {
                if (!fe->cosmetic) {
                    BRepBndLib::AddOptimal(fe->occEdge, faceBox);
                }
            }
        }
    }

    if (faceBox.IsVoid()) {
        Base::Console().Error("CL::calcEndPoints - faceBox is void!\n");
//        return result;
        throw Base::IndexError("CenterLine wrong number of faces.");
    }

    double Xmin,Ymin,Zmin,Xmax,Ymax,Zmax;
    faceBox.Get(Xmin,Ymin,Zmin,Xmax,Ymax,Zmax);

    double Xspan = fabs(Xmax - Xmin);
    Xspan = (Xspan / 2.0);
    double Xmid = Xmin + Xspan;

    double Yspan = fabs(Ymax - Ymin);
    Yspan = (Yspan / 2.0);
    double Ymid = Ymin + Yspan;

    Base::Vector3d p1, p2;
    if (mode == 0) {                    //vertical
        p1 = Base::Vector3d(Xmid, Ymax, 0.0);
        p2 = Base::Vector3d(Xmid, Ymin, 0.0);
    } else if (mode == 1) {            //horizontal
        p1 = Base::Vector3d(Xmin, Ymid, 0.0);
        p2 = Base::Vector3d(Xmax,Ymid, 0.0);
    } else {      //vert == 2 //aligned, but aligned doesn't make sense for face(s) bbox
        Base::Console().Message("CL::calcEndPoints - aligned is not applicable to Face center lines\n");
        p1 = Base::Vector3d(Xmid, Ymax, 0.0);
        p2 = Base::Vector3d(Xmid, Ymin, 0.0);
    }
    
    Base::Vector3d mid = (p1 + p2) / 2.0;

    //extend
    Base::Vector3d clDir = p2 - p1;
    clDir.Normalize();
    p1 = p1 - (clDir * ext);
    p2 = p2 + (clDir * ext);

    //rotate
    if (!DrawUtil::fpCompare(rotate, 0.0)) {
        //rotate p1, p2 about mid point
        double revRotate = -rotate;
        double cosTheta = cos(revRotate * M_PI / 180.0);
        double sinTheta = sin(revRotate * M_PI / 180.0);
        Base::Vector3d toOrg = p1 - mid;
        double xRot = toOrg.x * cosTheta - toOrg.y * sinTheta;
        double yRot = toOrg.y * cosTheta + toOrg.x * sinTheta;
        p1 = Base::Vector3d(xRot, yRot, 0.0) + mid;
        toOrg = p2 - mid;
        xRot = toOrg.x * cosTheta - toOrg.y * sinTheta;
        yRot = toOrg.y * cosTheta + toOrg.x * sinTheta;
        p2 = Base::Vector3d(xRot, yRot, 0.0) + mid;
    }

    //shift
    if (!DrawUtil::fpCompare(hShift, 0.0)) {
        double hss = hShift * scale;
        p1.x = p1.x + hss;
        p2.x = p2.x + hss;
    }
    if (!DrawUtil::fpCompare(vShift, 0.0)) {
        double vss = vShift * scale;
        p1.y = p1.y + vss;
        p2.y = p2.y + vss;
    }

    result.first = p1 / scale;
    result.second = p2 / scale;
    return result;
}

std::pair<Base::Vector3d, Base::Vector3d> CenterLine::calcEndPoints2Lines(DrawViewPart* partFeat,
                                                      std::vector<std::string> edgeNames, 
                                                      int mode, double ext,
                                                      double hShift, double vShift,
                                                      double rotate, bool flip)
                                                      
{
    Q_UNUSED(flip)

//    Base::Console().Message("CL::calc2Lines() - mode: %d flip: %d edgeNames: %d\n", mode, flip, edgeNames.size());
    std::pair<Base::Vector3d, Base::Vector3d> result;
    if (edgeNames.empty()) {
        Base::Console().Warning("CL::calcEndPoints2Lines - no edges!\n");
        return result;
    }

    double scale = partFeat->getScale();
    const std::vector<TechDraw::BaseGeomPtr> dbEdges = partFeat->getEdgeGeometry();

    std::vector<TechDraw::BaseGeomPtr> edges;
    for (auto& en: edgeNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(en) != "Edge") {
            continue;
        }
        int idx = TechDraw::DrawUtil::getIndexFromName(en);
        TechDraw::BaseGeomPtr bg = partFeat->getGeomByIndex(idx);
        if (bg != nullptr) {
            edges.push_back(bg);
        } else {
            Base::Console().Message("CL::calcEndPoints2Lines - no geom for index: %d\n", idx);
        }
    }
    if (edges.size() != 2) {
        Base::Console().Message("CL::calcEndPoints2Lines - wrong number of edges: %d!\n", edges.size());
//        return result;
        throw Base::IndexError("CenterLine wrong number of edges.");
    }

    Base::Vector3d l1p1 = edges.front()->getStartPoint();
    Base::Vector3d l1p2 = edges.front()->getEndPoint();
    Base::Vector3d l2p1 = edges.back()->getStartPoint();
    Base::Vector3d l2p2 = edges.back()->getEndPoint();

    // The centerline is drawn using the midpoints of the two lines that connect l1p1-l2p1 and l1p2-l2p2.
    // However, we don't know which point should be l1p1 to get a geometrically correct result, see
    // https://wiki.freecadweb.org/File:TD-CenterLineFlip.png for an illustration of the problem.
    // Thus we test this by a circulation test, see this post for a brief explanation:
    // https://forum.freecadweb.org/viewtopic.php?p=505733#p505615
    if (DrawUtil::circulation(l1p1, l1p2, l2p1) != DrawUtil::circulation(l1p2, l2p2, l2p1)) {
        Base::Vector3d temp; // reverse line 1
        temp = l1p1;
        l1p1 = l1p2;
        l1p2 = temp;
    }

    Base::Vector3d p1 = (l1p1 + l2p1) / 2.0;
    Base::Vector3d p2   = (l1p2 + l2p2) / 2.0;
    Base::Vector3d mid = (p1 + p2) / 2.0;

    //orientation
    if (mode == 0) {           //Vertical
            p1.x = mid.x;
            p2.x = mid.x;
    } else if (mode == 1) {    //Horizontal
            p1.y = mid.y;
            p2.y = mid.y;
    } else if (mode == 2) {    //Aligned
        // no op
    }

    //extend
    Base::Vector3d clDir = p2 - p1;
    clDir.Normalize();
    p1 = p1 - (clDir * ext);
    p2 = p2 + (clDir * ext);

    //rotate
    if (!DrawUtil::fpCompare(rotate, 0.0)) {
        //rotate p1, p2 about mid 
        double revRotate = -rotate;
        double cosTheta = cos(revRotate * M_PI / 180.0);
        double sinTheta = sin(revRotate * M_PI / 180.0);
        Base::Vector3d toOrg = p1 - mid;
        double xRot = toOrg.x * cosTheta - toOrg.y * sinTheta;
        double yRot = toOrg.y * cosTheta + toOrg.x * sinTheta;
        p1 = Base::Vector3d(xRot, yRot, 0.0) + mid;
        toOrg = p2 - mid;
        xRot = toOrg.x * cosTheta - toOrg.y * sinTheta;
        yRot = toOrg.y * cosTheta + toOrg.x * sinTheta;
        p2 = Base::Vector3d(xRot, yRot, 0.0) + mid;
    }

    //shift
    if (!DrawUtil::fpCompare(hShift, 0.0)) {
        double hss = hShift * scale;
        p1.x = p1.x + hss;
        p2.x = p2.x + hss;
    }
    if (!DrawUtil::fpCompare(vShift, 0.0)) {
        double vss = vShift * scale;
        p1.y = p1.y + vss;
        p2.y = p2.y + vss;
    }

    result.first = p1 / scale;
    result.second = p2 / scale;
    return result;
}

std::pair<Base::Vector3d, Base::Vector3d> CenterLine::calcEndPoints2Points(DrawViewPart* partFeat,
                                                      std::vector<std::string> vertNames, 
                                                      int mode, double ext,
                                                      double hShift, double vShift,
                                                      double rotate, bool flip)
                                                      
{
//    Base::Console().Message("CL::calc2Points()\n");
    std::pair<Base::Vector3d, Base::Vector3d> result;
    if (vertNames.empty()) {
        Base::Console().Warning("CL::calcEndPoints2Points - no points!\n");
        return result;
    }

    double scale = partFeat->getScale();

    std::vector<TechDraw::VertexPtr> points;
    for (auto& vn: vertNames) {
        if (TechDraw::DrawUtil::getGeomTypeFromName(vn) != "Vertex") {
            continue;
        }
        int idx = TechDraw::DrawUtil::getIndexFromName(vn);
        TechDraw::VertexPtr v = partFeat->getProjVertexByIndex(idx);
        if (v != nullptr) {
            points.push_back(v);
        }
    }
    if (points.size() != 2) {
        //this should fail harder.  maybe be in a try/catch.
//        Base::Console().Message("CL::calcEndPoints2Points - wrong number of points!\n");
//        return result;
        throw Base::IndexError("CenterLine wrong number of points.");
    }

    Base::Vector3d v1 = points.front()->point();
    Base::Vector3d v2 = points.back()->point();

    Base::Vector3d mid = (v1 + v2) / 2.0;
    Base::Vector3d dir = v2 - v1;
    double length = dir.Length();
    dir.Normalize();
    Base::Vector3d clDir(dir.y, -dir.x, dir.z);

    Base::Vector3d p1 = mid + clDir * (length / 2.0);
    Base::Vector3d p2 = mid - clDir * (length / 2.0);

    if (flip) {                   //is flip relevant to 2 point???
        Base::Vector3d temp;
        temp = p1;
        p1 = p2;
        p2 = temp;
    }
    
    if (mode == 0) {           //Vertical
            p1.x = mid.x;
            p2.x = mid.x;
    } else if (mode == 1) {    //Horizontal
            p1.y = mid.y;
            p2.y = mid.y;
    } else if (mode == 2) {    //Aligned
        // no op
    }


    //extend
    p1 = p1 + (clDir * ext);
    p2 = p2 - (clDir * ext);

    //rotate
    if (!DrawUtil::fpCompare(rotate, 0.0)) {
        //rotate p1, p2 about mid
        double revRotate = -rotate;
        double cosTheta = cos(revRotate * M_PI / 180.0);
        double sinTheta = sin(revRotate * M_PI / 180.0);
        Base::Vector3d toOrg = p1 - mid;
        double xRot = toOrg.x * cosTheta - toOrg.y * sinTheta;
        double yRot = toOrg.y * cosTheta + toOrg.x * sinTheta;
        p1 = Base::Vector3d(xRot, yRot, 0.0) + mid;
        toOrg = p2 - mid;
        xRot = toOrg.x * cosTheta - toOrg.y * sinTheta;
        yRot = toOrg.y * cosTheta + toOrg.x * sinTheta;
        p2 = Base::Vector3d(xRot, yRot, 0.0) + mid;
    }

    //shift
    if (!DrawUtil::fpCompare(hShift, 0.0)) {
        double hss = hShift * scale;
        p1.x = p1.x + hss;
        p2.x = p2.x + hss;
    }
    if (!DrawUtil::fpCompare(vShift, 0.0)) {
        double vss = vShift * scale;
        p1.y = p1.y + vss;
        p2.y = p2.y + vss;
    }

    result.first = p1 / scale;
    result.second = p2 / scale;
    return result;
}

// Persistence implementers
unsigned int CenterLine::getMemSize (void) const
{
    return 1;
}

void CenterLine::Save(Base::Writer &writer) const
{
    writer.Stream() << writer.ind() << "<Start "
                << "X=\"" <<  m_start.x <<
                "\" Y=\"" <<  m_start.y <<
                "\" Z=\"" <<  m_start.z <<
                 "\"/>" << endl;
    writer.Stream() << writer.ind() << "<End "
                << "X=\"" <<  m_end.x <<
                "\" Y=\"" <<  m_end.y <<
                "\" Z=\"" <<  m_end.z <<
                 "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Mode value=\"" << m_mode <<"\"/>" << endl;
    writer.Stream() << writer.ind() << "<HShift value=\"" << m_hShift <<"\"/>" << endl;
    writer.Stream() << writer.ind() << "<VShift value=\"" << m_vShift <<"\"/>" << endl;
    writer.Stream() << writer.ind() << "<Rotate value=\"" << m_rotate <<"\"/>" << endl;
    writer.Stream() << writer.ind() << "<Extend value=\"" << m_extendBy <<"\"/>" << endl;
    writer.Stream() << writer.ind() << "<Type value=\"" << m_type <<"\"/>" << endl;
    writer.Stream() << writer.ind() << "<Flip value=\"" << m_flip2Line <<"\"/>" << endl;
    writer.Stream()
         << writer.ind()
             << "<Faces "
                << "FaceCount=\"" <<  m_faces.size() <<
             "\">" << endl;

    writer.incInd();
    for (auto& f: m_faces) {
        writer.Stream()
            << writer.ind()
            << "<Face value=\"" << f <<"\"/>" << endl;
    }
    writer.decInd();

    writer.Stream() << writer.ind() << "</Faces>" << endl ;

    writer.Stream()
         << writer.ind()
             << "<Edges "
                << "EdgeCount=\"" <<  m_edges.size() <<
             "\">" << endl;

    writer.incInd();
    for (auto& e: m_edges) {
        writer.Stream()
            << writer.ind()
            << "<Edge value=\"" << e <<"\"/>" << endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Edges>" << endl ;

    writer.Stream()
         << writer.ind()
             << "<CLPoints "
                << "CLPointCount=\"" <<  m_verts.size() <<
             "\">" << endl;

    writer.incInd();
    for (auto& p: m_verts) {
        writer.Stream()
            << writer.ind()
            << "<CLPoint value=\"" << p <<"\"/>" << endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</CLPoints>" << endl ;

    writer.Stream() << writer.ind() << "<Style value=\"" <<  m_format.m_style << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Weight value=\"" <<  m_format.m_weight << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Color value=\"" <<  m_format.m_color.asHexString() << "\"/>" << endl;
    const char v = m_format.m_visible?'1':'0';
    writer.Stream() << writer.ind() << "<Visible value=\"" <<  v << "\"/>" << endl;

//stored geometry
    if (m_geometry != nullptr) {
        writer.Stream() << writer.ind() << "<GeometryType value=\"" << m_geometry->geomType <<"\"/>" << endl;
        if (m_geometry->geomType == TechDraw::GeomType::GENERIC) {
            GenericPtr gen = std::static_pointer_cast<Generic>(m_geometry);
            gen->Save(writer);
        } else if (m_geometry->geomType == TechDraw::GeomType::CIRCLE) {
            TechDraw::CirclePtr circ = std::static_pointer_cast<TechDraw::Circle>(m_geometry);
            circ->Save(writer);
        } else if (m_geometry->geomType == TechDraw::GeomType::ARCOFCIRCLE) {
            TechDraw::AOCPtr aoc = std::static_pointer_cast<TechDraw::AOC>(m_geometry);
            aoc->Save(writer);
        } else {
            Base::Console().Message("CL::Save - unimplemented geomType: %d\n", m_geometry->geomType);
        }
    } else {
        Base::Console().Error("CL::Save - m_geometry is null\n");
        //TODO: create a placeholder for missing geom???
    }
}

void CenterLine::Restore(Base::XMLReader &reader)
{
    if (!CosmeticVertex::restoreCosmetic()) {
        return;
    }
//    Base::Console().Message("CL::Restore - reading elements\n");
    // read my Element
    reader.readElement("Start");
    // get the value of my Attribute
    m_start.x = reader.getAttributeAsFloat("X");
    m_start.y = reader.getAttributeAsFloat("Y");
    m_start.z = reader.getAttributeAsFloat("Z");

    reader.readElement("End");
    m_end.x = reader.getAttributeAsFloat("X");
    m_end.y = reader.getAttributeAsFloat("Y");
    m_end.z = reader.getAttributeAsFloat("Z");

    reader.readElement("Mode");
    m_mode = reader.getAttributeAsInteger("value");

    reader.readElement("HShift");
    m_hShift = reader.getAttributeAsFloat("value");
    reader.readElement("VShift");
    m_vShift = reader.getAttributeAsFloat("value");
    reader.readElement("Rotate");
    m_rotate = reader.getAttributeAsFloat("value");
    reader.readElement("Extend");
    m_extendBy = reader.getAttributeAsFloat("value");
    reader.readElement("Type");
    m_type = reader.getAttributeAsInteger("value");
    reader.readElement("Flip");
    m_flip2Line = (bool)reader.getAttributeAsInteger("value")==0?false:true;

    reader.readElement("Faces");
    int count = reader.getAttributeAsInteger("FaceCount");

    int i = 0;
    for ( ; i < count; i++) {
        reader.readElement("Face");
        std::string f = reader.getAttribute("value");
        m_faces.push_back(f);
    }
    reader.readEndElement("Faces");

    reader.readElement("Edges");
    count = reader.getAttributeAsInteger("EdgeCount");

    i = 0;
    for ( ; i < count; i++) {
        reader.readElement("Edge");
        std::string e = reader.getAttribute("value");
        m_edges.push_back(e);
    }
    reader.readEndElement("Edges");

    reader.readElement("CLPoints");
    count = reader.getAttributeAsInteger("CLPointCount");

    i = 0;
    for ( ; i < count; i++) {
        reader.readElement("CLPoint");
        std::string p = reader.getAttribute("value");
        m_verts.push_back(p);
    }
    reader.readEndElement("CLPoints");

    reader.readElement("Style");
    m_format.m_style = reader.getAttributeAsInteger("value");
    reader.readElement("Weight");
    m_format.m_weight = reader.getAttributeAsFloat("value");
    reader.readElement("Color");
    std::string temp = reader.getAttribute("value");
    m_format.m_color.fromHexString(temp);
    reader.readElement("Visible");
    m_format.m_visible = (int)reader.getAttributeAsInteger("value")==0?false:true;

//stored geometry
    reader.readElement("GeometryType");
    TechDraw::GeomType gType = (TechDraw::GeomType)reader.getAttributeAsInteger("value");
    if (gType == TechDraw::GeomType::GENERIC) {
        TechDraw::GenericPtr gen = std::make_shared<TechDraw::Generic> ();
        gen->Restore(reader);
        gen->occEdge = GeometryUtils::edgeFromGeneric(gen);
        m_geometry = gen;
    } else if (gType == TechDraw::GeomType::CIRCLE) {
        TechDraw::CirclePtr circ = std::make_shared<TechDraw::Circle> ();
        circ->Restore(reader);
        circ->occEdge = GeometryUtils::edgeFromCircle(circ);
        m_geometry = circ;
    } else if (gType == TechDraw::GeomType::ARCOFCIRCLE) {
        TechDraw::AOCPtr aoc = std::make_shared<TechDraw::AOC> ();
        aoc->Restore(reader);
        aoc->occEdge = GeometryUtils::edgeFromCircleArc(aoc);
        m_geometry = aoc;
    } else {
        Base::Console().Warning("CL::Restore - unimplemented geomType: %d\n", gType);
    } 
}

CenterLine* CenterLine::copy(void) const
{
    CenterLine* newCL = new CenterLine();
    newCL->m_start = m_start;
    newCL->m_end = m_end; 
    newCL->m_mode = m_mode;
    newCL->m_hShift = m_hShift;
    newCL->m_vShift = m_vShift;
    newCL->m_rotate = m_rotate;
    newCL->m_extendBy = m_extendBy;
    newCL->m_type = m_type;
    newCL->m_flip2Line = m_flip2Line;

    newCL->m_faces = m_faces;
    newCL->m_edges = m_edges;
    newCL->m_verts = m_verts;
    
    TechDraw::BaseGeomPtr newGeom = m_geometry->copy();
    newCL->m_geometry = newGeom;

    newCL->m_format = m_format;

    return newCL;
}

boost::uuids::uuid CenterLine::getTag() const
{
    return tag;
}

std::string CenterLine::getTagAsString(void) const
{
    std::string tmp = boost::uuids::to_string(getTag());
    return tmp;
}

void CenterLine::createNewTag()
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

void CenterLine::assignTag(const TechDraw::CenterLine * ce)
{
    if(ce->getTypeId() == this->getTypeId())
        this->tag = ce->tag;
    else
        throw Base::TypeError("CenterLine tag can not be assigned as types do not match.");
}

CenterLine *CenterLine::clone(void) const
{
    CenterLine* cpy = this->copy();
    cpy->tag = this->tag;

    return cpy;
}

PyObject* CenterLine::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new CenterLinePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}


void CenterLine::setShifts(double h, double v)
{
    m_hShift = h;
    m_vShift = v;
}

double CenterLine::getHShift(void)
{
    return m_hShift;
}

double CenterLine::getVShift(void)
{
    return m_vShift;
}

void CenterLine::setRotate(double r)
{
    m_rotate = r;
}

double CenterLine::getRotate(void)
{
    return m_rotate;
}

void CenterLine::setExtend(double e)
{
    m_extendBy = e;
}

double CenterLine::getExtend(void)
{
    return m_extendBy;
}

void CenterLine::setFlip(bool f)
{
    m_flip2Line = f;
}

bool CenterLine::getFlip(void)
{
    return m_flip2Line;
}
//------------------------------------------------------------------------------

TYPESYSTEM_SOURCE(TechDraw::GeomFormat,Base::Persistence)

GeomFormat::GeomFormat() :
    m_geomIndex(-1)
{
    m_format.m_style = LineFormat::getDefEdgeStyle();
    m_format.m_weight = LineFormat::getDefEdgeWidth();
    m_format.m_color = LineFormat::getDefEdgeColor();
    m_format.m_visible = true;

    createNewTag();
}

GeomFormat::GeomFormat(GeomFormat* gf) 
{
    m_geomIndex  = gf->m_geomIndex;
    m_format.m_style = gf->m_format.m_style;
    m_format.m_weight = gf->m_format.m_weight;
    m_format.m_color = gf->m_format.m_color;
    m_format.m_visible = gf->m_format.m_visible;

    createNewTag();
}

GeomFormat::GeomFormat(int idx,
                       TechDraw::LineFormat fmt) :
    m_geomIndex(idx)
{
    m_format.m_style = fmt.m_style;
    m_format.m_weight = fmt.m_weight;
    m_format.m_color = fmt.m_color;
    m_format.m_visible = fmt.m_visible;

    createNewTag();
}

GeomFormat::~GeomFormat()
{
}

void GeomFormat::dump(const char* title) const
{
    Base::Console().Message("GF::dump - %s \n",title);
    Base::Console().Message("GF::dump - %s \n",toString().c_str());
}

std::string GeomFormat::toString(void) const
{
    std::stringstream ss;
    ss << m_geomIndex << ",$$$," <<
          m_format.toString();
    return ss.str();
}

// Persistence implementer
unsigned int GeomFormat::getMemSize (void) const
{
    return 1;
}

void GeomFormat::Save(Base::Writer &writer) const
{
    const char v = m_format.m_visible?'1':'0';
    writer.Stream() << writer.ind() << "<GeomIndex value=\"" <<  m_geomIndex << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Style value=\"" <<  m_format.m_style << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Weight value=\"" <<  m_format.m_weight << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Color value=\"" <<  m_format.m_color.asHexString() << "\"/>" << endl;
    writer.Stream() << writer.ind() << "<Visible value=\"" <<  v << "\"/>" << endl;
}

void GeomFormat::Restore(Base::XMLReader &reader)
{
    if (!CosmeticVertex::restoreCosmetic()) {
        return;
    }
    // read my Element
    reader.readElement("GeomIndex");
    // get the value of my Attribute
    m_geomIndex = reader.getAttributeAsInteger("value");

    reader.readElement("Style");
    m_format.m_style = reader.getAttributeAsInteger("value");
    reader.readElement("Weight");
    m_format.m_weight = reader.getAttributeAsFloat("value");
    reader.readElement("Color");
    std::string temp = reader.getAttribute("value");
    m_format.m_color.fromHexString(temp);
    reader.readElement("Visible");
    m_format.m_visible = (int)reader.getAttributeAsInteger("value")==0?false:true;
}

boost::uuids::uuid GeomFormat::getTag() const
{
    return tag;
}

std::string GeomFormat::getTagAsString(void) const
{
    std::string tmp = boost::uuids::to_string(getTag());
    return tmp;
}

void GeomFormat::createNewTag()
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

void GeomFormat::assignTag(const TechDraw::GeomFormat * ce)
{
    if(ce->getTypeId() == this->getTypeId())
        this->tag = ce->tag;
    else
        throw Base::TypeError("GeomFormat tag can not be assigned as types do not match.");
}

GeomFormat *GeomFormat::clone(void) const
{
    GeomFormat* cpy = this->copy();
    cpy->tag = this->tag;

    return cpy;
}

GeomFormat* GeomFormat::copy(void) const
{
    GeomFormat* newFmt = new GeomFormat();
    newFmt->m_geomIndex = m_geomIndex;
    newFmt->m_format.m_style = m_format.m_style;
    newFmt->m_format.m_weight = m_format.m_weight;
    newFmt->m_format.m_color = m_format.m_color;
    newFmt->m_format.m_visible = m_format.m_visible;
    return newFmt;
}

PyObject* GeomFormat::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new GeomFormatPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

bool CosmeticVertex::restoreCosmetic(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    bool result = hGrp->GetBool("restoreCosmetic", true);
    return result;
}

