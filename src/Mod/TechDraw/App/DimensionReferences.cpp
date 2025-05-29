/***************************************************************************
 *   Copyright (c) 2022 WandererFan <wandererfan@gmail.com>                *
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
# include <TopoDS_Shape.hxx>
#endif

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <TopExp.hxx>

#include <App/GeoFeature.h>
#include <App/DocumentObject.h>
#include <App/Document.h>
#include <App/Link.h>
#include <Base/Console.h>

#include <Mod/Measure/App/ShapeFinder.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>

#include "DimensionReferences.h"
#include "DrawUtil.h"
#include "DrawViewPart.h"
#include "ShapeUtils.h"
#include "CosmeticVertex.h"

using namespace TechDraw;
using namespace Measure;
using DU = DrawUtil;
using SU = ShapeUtils;


ReferenceEntry::ReferenceEntry( App::DocumentObject* docObject, const std::string& subName, App::Document* document)
{
    setObject(docObject);
    setSubName(subName);
    setDocument(document);
    if (docObject) {
        setObjectName(docObject->getNameInDocument());
        if (document == nullptr) {
            setDocument(docObject->getDocument());
        }
    }
}


ReferenceEntry::ReferenceEntry(const ReferenceEntry& other)
{
    setObject(other.getObject());
    setSubName(other.getSubName(true));
    setObjectName(other.getObjectName());
    setDocument(other.getDocument());
}



ReferenceEntry& ReferenceEntry::operator=(const ReferenceEntry& otherRef)
{
    if (this == &otherRef) {
        return *this;
    }
    setObject(otherRef.getObject());
    setSubName(otherRef.getSubName(true));
    setObjectName(otherRef.getObjectName());
    setDocument(otherRef.getDocument());
    return *this;
}


bool ReferenceEntry::operator==(const ReferenceEntry& otherRef) const
{
    return getObjectName() == otherRef.getObjectName() && getSubName() == otherRef.getSubName();
}


TopoDS_Shape ReferenceEntry::getGeometry() const
{
    // first, make sure the object has not been deleted!
    App::DocumentObject* obj = getDocument()->getObject(getObjectName().c_str());
    if (!obj) {
        return {};
    }

    if (getSubName().empty()) {
        return {};
    }

    if ( getObject()->isDerivedFrom<TechDraw::DrawViewPart>() ) {
        // 2d geometry from DrawViewPart will be rotated and scaled
        return getGeometry2d();
    }

    // 3d geometry
    return ShapeFinder::getLocatedShape(*getObject(), getSubName(true));
}


//! get a shape for this 2d reference
TopoDS_Shape ReferenceEntry::getGeometry2d() const
{
    std::string gType;
    try {
        auto dvp = getObject<TechDraw::DrawViewPart>();   //NOLINT cppcoreguidelines-pro-type-static-cast-downcast
        gType = geomType();
        if (gType == "Vertex") {
            // getVertex throws on not found, but we want to return null
            // shape
            auto vgeom = dvp->getVertex(getSubName());
            if (!vgeom) {
                return {};
            }
            return vgeom->getOCCVertex();
        }
        if (gType == "Edge") {
            auto egeom = dvp->getEdge(getSubName());
            if (!egeom) {
                return {};
            }
            return egeom->getOCCEdge();
        }
        if (gType == "Face") {
            auto fgeom = dvp->getFace(getSubName());
            if (!fgeom) {
                return {};
            }
            return fgeom->toOccFace();
        }
    }
    catch (...) {
        Base::Console().message("RE::getGeometry2d - no shape for dimension 2d reference - gType: **%s**\n", gType.c_str());
    }

    return {};
}


std::string ReferenceEntry::getSubName(bool longForm) const
{
    if (longForm) {
        return m_subName;
    }

    return ShapeFinder::getLastTerm(m_subName);
}


App::DocumentObject* ReferenceEntry::getObject() const
{
    if (!getDocument()) {
        return nullptr;
    }
    App::DocumentObject* obj = getDocument()->getObject(getObjectName().c_str());
    if (!obj) {
        return nullptr;
    }

    return obj;
}


//! return the reference geometry as a Part::TopoShape.
Part::TopoShape ReferenceEntry::asTopoShape() const
{
    TopoDS_Shape geom = getGeometry();
    if (geom.IsNull()) {
        return {};
    }
    if (geom.ShapeType() == TopAbs_VERTEX) {
        TopoDS_Vertex vert = TopoDS::Vertex(geom);
        return asTopoShapeVertex(vert);
    }
    if (geom.ShapeType() == TopAbs_EDGE) {
        TopoDS_Edge edge = TopoDS::Edge(geom);
        return asTopoShapeEdge(edge);
    }
    if (geom.ShapeType() == TopAbs_FACE) {
        TopoDS_Face face = TopoDS::Face(geom);
        return asTopoShapeFace(face);
    }
    throw Base::RuntimeError("Dimension Reference has unsupported geometry");
}

//! returns unscaled, unrotated version of inShape. inShape is assumed to be a 2d shape, but this is not enforced.
Part::TopoShape ReferenceEntry::asCanonicalTopoShape() const
{
    if (is3d()) {
        return asTopoShape();
    }

    // this is a 2d reference
    auto dvp = getObject<DrawViewPart>(); //NOLINT cppcoreguidelines-pro-type-static-cast-downcast
    auto rawTopoShape = asTopoShape();
    return ReferenceEntry::asCanonicalTopoShape(rawTopoShape, *dvp);
}


//! static public method returns unscaled, unrotated version of inShape. inShape is assumed to be a 2d shape,
//! but this is not enforced.  3d shapes should not be made canonical.
//! 2d shapes are inverted in Y direction and need to be inverted before and after rotation
//! operations.
Part::TopoShape ReferenceEntry::asCanonicalTopoShape(const Part::TopoShape& inShape, const DrawViewPart& dvp)
{
    gp_Ax2 OXYZ;
    auto unscaledShape = SU::scaleShape(inShape.getShape(), 1.0 / dvp.getScale());
    if (dvp.Rotation.getValue() != 0.0) {
        auto rotationDeg = dvp.Rotation.getValue();
        unscaledShape = SU::invertGeometry(unscaledShape);
        unscaledShape = SU::rotateShape(unscaledShape, OXYZ, -rotationDeg);
        unscaledShape = SU::invertGeometry(unscaledShape);
    }
        return {unscaledShape};
}


Part::TopoShape ReferenceEntry::asTopoShapeVertex(const TopoDS_Vertex& vert)
{
    return { vert };
}

Part::TopoShape ReferenceEntry::asTopoShapeEdge(const TopoDS_Edge &edge)
{
    return { edge };
}

Part::TopoShape ReferenceEntry::asTopoShapeFace(const TopoDS_Face &face)
{
    return { face };
}

std::string ReferenceEntry::geomType() const
{
    return DrawUtil::getGeomTypeFromName(getSubName());
}

GeomType ReferenceEntry::geomEdgeType() const
{
    int geoId = TechDraw::DrawUtil::getIndexFromName(getSubName());
    auto dvp = getObject<TechDraw::DrawViewPart>();
    BaseGeomPtr geom = dvp->getGeomByIndex(geoId);

    if (geomType() == "Edge" && geom) {
        return geom->getGeomType();
    }

    return GeomType::NOTDEF;
}

bool ReferenceEntry::isWholeObject() const
{
    return getSubName().empty();
}

//! true if this reference point to 3d model geometry
bool ReferenceEntry::is3d() const
{
    if (getObject() &&
        getObject()->isDerivedFrom<TechDraw::DrawViewPart>() &&
        !getSubName().empty()) {
        // this is a well formed 2d reference
        return false;
    }

    if (getObject() &&
        getObject()->isDerivedFrom<TechDraw::DrawViewPart>() &&
        getSubName().empty()) {
        // this is a broken 3d reference, so it should be treated as 3d
        return true;
    }

    // either we have no object or we have an object and it is a 3d object
    return true;
}


//! true if the target of this reference has a shape
bool ReferenceEntry::hasGeometry() const
{
    if (!getObject()) {
        return false;
    }

    if ( getObject()->isDerivedFrom<TechDraw::DrawViewPart>() ) {
        // 2d reference
        return hasGeometry2d();
    }

    // 3d reference
    // TODO: shouldn't this be ShapeFinder.getLocatedShape?
    auto shape = Part::Feature::getTopoShape(getObject(), Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    auto subShape = shape.getSubShape(getSubName().c_str());

    return !subShape.IsNull();
}


//! check if this 2d reference has valid geometry in the model
bool ReferenceEntry::hasGeometry2d() const
{
    auto dvp = getObject<TechDraw::DrawViewPart>();   //NOLINT cppcoreguidelines-pro-type-static-cast-downcast
    if (getSubName().empty()) {
        return false;
    }
    int geomNumber = DU::getIndexFromName(getSubName());
    std::string gType = geomType();
    if (gType == "Vertex") {
        auto vert = dvp->getProjVertexByIndex(geomNumber);
        if (vert) {
            return true;
        }
    }
    else if (gType == "Edge") {
        auto edge = dvp->getGeomByIndex(geomNumber);
        if (edge) {
            return true;
        }
    }
    else if (gType == "Face") {
        auto face = dvp->getFace(getSubName());
        if (face) {
            return true;
        }
    }
    return false;
}


