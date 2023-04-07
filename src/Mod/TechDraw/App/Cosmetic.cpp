/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
 *   Copyright (c) 2022 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <boost/uuid/uuid_generators.hpp>
# include <boost/uuid/uuid_io.hpp>
#endif

#include <App/Application.h>
#include <Base/Vector3D.h>
#include <Mod/TechDraw/App/CosmeticEdgePy.h>
#include <Mod/TechDraw/App/GeomFormatPy.h>

#include "Cosmetic.h"
#include "DrawUtil.h"
#include "DrawViewPart.h"
#include "GeometryObject.h"
#include "LineGroup.h"
#include "Preferences.h"


using namespace TechDraw;
using namespace std;

#define GEOMETRYEDGE 0
#define COSMETICEDGE 1
#define CENTERLINE   2

LineFormat::LineFormat()
{
    m_style = getDefEdgeStyle();
    m_weight = getDefEdgeWidth();
    m_color= getDefEdgeColor();
    m_visible = true;
}

LineFormat::LineFormat(int style,
               double weight,
               App::Color color,
               bool visible) :
    m_style(style),
    m_weight(weight),
    m_color(color),
    m_visible(visible)
{
}

void LineFormat::dump(const char* title)
{
    Base::Console().Message("LF::dump - %s \n", title);
    Base::Console().Message("LF::dump - %s \n", toString().c_str());
}

std::string LineFormat::toString() const
{
    std::stringstream ss;
    ss << m_style << ", " <<
          m_weight << ", " <<
          m_color.asHexString() << ", " <<
          m_visible;
    return ss.str();
}

//static preference getters.
double LineFormat::getDefEdgeWidth()
{
    return TechDraw::LineGroup::getDefaultWidth("Graphic");
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

//******************************************

TYPESYSTEM_SOURCE(TechDraw::CosmeticEdge, Base::Persistence)

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

CosmeticEdge::CosmeticEdge(Base::Vector3d pt1, Base::Vector3d pt2) :
//                             🠓 returns TopoDS_Edge
    CosmeticEdge::CosmeticEdge(TopoDS_EdgeFromVectors(pt1, pt2))
{
}

//                                                       🠓 returns TechDraw::BaseGeomPtr
CosmeticEdge::CosmeticEdge(TopoDS_Edge e) : CosmeticEdge(TechDraw::BaseGeom::baseFactory(e))
{
}

CosmeticEdge::CosmeticEdge(TechDraw::BaseGeomPtr g)
{
//    Base::Console().Message("CE::CE(bg)\n");
    m_geometry = g;
    //we assume input edge is already in Yinverted coordinates
    permaStart = m_geometry->getStartPoint();
    permaEnd   = m_geometry->getEndPoint();
    if ((g->getGeomType() == TechDraw::GeomType::CIRCLE) ||
       (g->getGeomType() == TechDraw::GeomType::ARCOFCIRCLE)) {
       TechDraw::CirclePtr circ = std::static_pointer_cast<TechDraw::Circle>(g);
       permaStart  = circ->center;
       permaEnd    = circ->center;
       permaRadius = circ->radius;
    }
    initialize();
}

CosmeticEdge::~CosmeticEdge()
{
    //shared pointer will delete m_geometry when ref count goes to zero.
}

void CosmeticEdge::initialize()
{
    m_geometry->setClassOfEdge(ecHARD);
    m_geometry->setHlrVisible( true);
    m_geometry->setCosmetic(true);
    m_geometry->source(COSMETICEDGE);

    createNewTag();
    m_geometry->setCosmeticTag(getTagAsString());
}

TopoDS_Edge CosmeticEdge::TopoDS_EdgeFromVectors(Base::Vector3d pt1, Base::Vector3d pt2)
{
    // Base::Console().Message("CE::CE(p1, p2)\n");
    Base::Vector3d p1 = DrawUtil::invertY(pt1);
    Base::Vector3d p2 = DrawUtil::invertY(pt2);
    gp_Pnt gp1(p1.x, p1.y, p1.z);
    gp_Pnt gp2(p2.x, p2.y, p2.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
    return e;
}

TechDraw::BaseGeomPtr CosmeticEdge::scaledGeometry(double scale)
{
    TopoDS_Edge e = m_geometry->getOCCEdge();
    TopoDS_Shape s = TechDraw::scaleShape(e, scale);
    TopoDS_Edge newEdge = TopoDS::Edge(s);
    TechDraw::BaseGeomPtr newGeom = TechDraw::BaseGeom::baseFactory(newEdge);
    newGeom->setClassOfEdge(ecHARD);
    newGeom->setHlrVisible( true);
    newGeom->setCosmetic(true);
    newGeom->source(COSMETICEDGE);
    newGeom->setCosmeticTag(getTagAsString());
    return newGeom;
}

std::string CosmeticEdge::toString() const
{
    std::stringstream ss;
    ss << getTagAsString() << ", $$$, ";
    if (m_geometry) {
        ss << m_geometry->getGeomType() <<
            ", $$$, " <<
            m_geometry->toString() <<
            ", $$$, " <<
            m_format.toString();
    }
    return ss.str();
}

void CosmeticEdge::dump(const char* title)
{
    Base::Console().Message("CE::dump - %s \n", title);
    Base::Console().Message("CE::dump - %s \n", toString().c_str());
}

// Persistence implementers
unsigned int CosmeticEdge::getMemSize () const
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

    writer.Stream() << writer.ind() << "<GeometryType value=\"" << m_geometry->getGeomType() <<"\"/>" << endl;
    if (m_geometry->getGeomType() == TechDraw::GeomType::GENERIC) {
        GenericPtr gen = std::static_pointer_cast<Generic>(m_geometry);
        gen->Save(writer);
    } else if (m_geometry->getGeomType() == TechDraw::GeomType::CIRCLE) {
        TechDraw::CirclePtr circ = std::static_pointer_cast<TechDraw::Circle>(m_geometry);
        circ->Save(writer);
    } else if (m_geometry->getGeomType() == TechDraw::GeomType::ARCOFCIRCLE) {
        TechDraw::AOCPtr aoc = std::static_pointer_cast<TechDraw::AOC>(m_geometry);
        aoc->Save(writer);
    } else {
        Base::Console().Warning("CE::Save - unimplemented geomType: %d\n", static_cast<int>(m_geometry->getGeomType()));
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
    m_format.m_visible = reader.getAttributeAsInteger("value") != 0;
    reader.readElement("GeometryType");
    TechDraw::GeomType gType = static_cast<TechDraw::GeomType>(reader.getAttributeAsInteger("value"));

    if (gType == TechDraw::GeomType::GENERIC) {
        TechDraw::GenericPtr gen = std::make_shared<TechDraw::Generic> ();
        gen->Restore(reader);
        gen->setOCCEdge(GeometryUtils::edgeFromGeneric(gen));
        m_geometry = gen;
        permaStart = gen->getStartPoint();
        permaEnd   = gen->getEndPoint();
    } else if (gType == TechDraw::GeomType::CIRCLE) {
        TechDraw::CirclePtr circ = std::make_shared<TechDraw::Circle> ();
        circ->Restore(reader);
        circ->setOCCEdge(GeometryUtils::edgeFromCircle(circ));
        m_geometry = circ;
        permaRadius = circ->radius;
        permaStart  = circ->center;
        permaEnd    = circ->center;
    } else if (gType == TechDraw::GeomType::ARCOFCIRCLE) {
        TechDraw::AOCPtr aoc = std::make_shared<TechDraw::AOC> ();
        aoc->Restore(reader);
        aoc->setOCCEdge(GeometryUtils::edgeFromCircleArc(aoc));
        m_geometry = aoc;
        permaStart = aoc->startPnt;
        permaEnd   = aoc->endPnt;
        permaRadius = aoc->radius;
    } else {
        Base::Console().Warning("CE::Restore - unimplemented geomType: %d\n", static_cast<int>(gType));
    }
}

boost::uuids::uuid CosmeticEdge::getTag() const
{
    return tag;
}

std::string CosmeticEdge::getTagAsString() const
{
    return boost::uuids::to_string(getTag());
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

CosmeticEdge* CosmeticEdge::copy() const
{
//    Base::Console().Message("CE::copy()\n");
    CosmeticEdge* newCE = new CosmeticEdge();
    TechDraw::BaseGeomPtr newGeom = m_geometry->copy();
    newCE->m_geometry = newGeom;
    newCE->m_format = m_format;
    return newCE;
}

CosmeticEdge* CosmeticEdge::clone() const
{
//    Base::Console().Message("CE::clone()\n");
    CosmeticEdge* cpy = this->copy();
    cpy->tag = this->tag;
    return cpy;
}

PyObject* CosmeticEdge::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new CosmeticEdgePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

//------------------------------------------------------------------------------

TYPESYSTEM_SOURCE(TechDraw::GeomFormat, Base::Persistence)

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
    Base::Console().Message("GF::dump - %s \n", title);
    Base::Console().Message("GF::dump - %s \n", toString().c_str());
}

std::string GeomFormat::toString() const
{
    std::stringstream ss;
    ss << m_geomIndex << ", $$$, " <<
          m_format.toString();
    return ss.str();
}

// Persistence implementer
unsigned int GeomFormat::getMemSize () const
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

std::string GeomFormat::getTagAsString() const
{
    return boost::uuids::to_string(getTag());
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

GeomFormat *GeomFormat::clone() const
{
    GeomFormat* cpy = this->copy();
    cpy->tag = this->tag;
    return cpy;
}

GeomFormat* GeomFormat::copy() const
{
    GeomFormat* newFmt = new GeomFormat();
    newFmt->m_geomIndex = m_geomIndex;
    newFmt->m_format.m_style = m_format.m_style;
    newFmt->m_format.m_weight = m_format.m_weight;
    newFmt->m_format.m_color = m_format.m_color;
    newFmt->m_format.m_visible = m_format.m_visible;
    return newFmt;
}

PyObject* GeomFormat::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new GeomFormatPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

bool CosmeticVertex::restoreCosmetic()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    return hGrp->GetBool("restoreCosmetic", true);
}

