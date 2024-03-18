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
#include <Base/Console.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>

#include "DimensionReferences.h"
#include "DrawUtil.h"
#include "DrawViewPart.h"
#include "ShapeUtils.h"

using namespace TechDraw;
using DU = DrawUtil;


ReferenceEntry::ReferenceEntry( App::DocumentObject* docObject, std::string subName, App::Document* document)
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
    setSubName(other.getSubName());
    setObjectName(other.getObjectName());
    setDocument(other.getDocument());
}



ReferenceEntry& ReferenceEntry::operator=(const ReferenceEntry& otherRef)
{
    setObject(otherRef.getObject());
    setSubName(otherRef.getSubName());
    setObjectName(otherRef.getObjectName());
    setDocument(otherRef.getDocument());
    return *this;
}


TopoDS_Shape ReferenceEntry::getGeometry() const
{
    // Base::Console().Message("RE::getGeometry() - objectName: %s  sub: **%s**\n",
    //                        getObjectName(), getSubName());
    // first, make sure the object has not been deleted!
    App::DocumentObject* obj = getDocument()->getObject(getObjectName().c_str());
    if (!obj) {
        Base::Console().Message("RE::getGeometry - %s no longer exists!\n", getObjectName().c_str());
        return {};
    }

    if (getSubName().empty()) {
        Base::Console().Message("RE::getGeometry - Reference has no subelement!\n");
        return {};
    }

    if ( getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) ) {
        // Base::Console().Message("RE::getGeometry - getting 2d geometry\n");
        std::string gType;
        try {
            auto dvp = static_cast<TechDraw::DrawViewPart*>(getObject());
            gType = geomType();
            if (gType == "Vertex") {
                // getVertex throws on not found, but we want to return null
                // shape
                auto vgeom = dvp->getVertex(getSubName());
                return vgeom->getOCCVertex();
            }
            if (gType == "Edge") {
                auto egeom = dvp->getEdge(getSubName());
                return egeom->getOCCEdge();
            }
            if (gType == "Face") {
                auto fgeom = dvp->getFace(getSubName());
                return fgeom->toOccFace();
            }
        }
        catch (...) {
            Base::Console().Message("RE::getGeometry - no shape for dimension 2d reference - gType: **%s**\n", gType.c_str());
            return {};
        }
    }

    // Base::Console().Message("RE::getGeometry - getting 2d geometry\n");
    Part::TopoShape shape = Part::Feature::getTopoShape(getObject());
    auto geoFeat = dynamic_cast<App::GeoFeature*>(getObject());
    if (geoFeat) {
        shape.setPlacement(geoFeat->globalPlacement());
    }

    if (getSubName().empty()) {
        return shape.getShape();
    }
    // TODO: what happens if the subelement is no longer present?
    return shape.getSubShape(getSubName().c_str());
}

std::string ReferenceEntry::getSubName(bool longForm) const
{
    if (longForm) {
        return m_subName;
    }
    std::string workingSubName(m_subName);
    size_t lastDot = workingSubName.rfind('.');
    if (lastDot != std::string::npos) {
        workingSubName = workingSubName.substr(lastDot + 1);
    }
    return workingSubName;
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

Part::TopoShape ReferenceEntry::asTopoShape() const
{
//    Base::Console().Message("RE::asTopoShape()\n");
    TopoDS_Shape geom = getGeometry();
    if (geom.IsNull()) {
        // throw Base::RuntimeError("Dimension Reference has null geometry");
        Base::Console().Message("RE::asTopoShape - reference geometry is null\n");
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
    throw Base::RuntimeError("Dimension Reference has unsupported geometry");
}

Part::TopoShape ReferenceEntry::asTopoShapeVertex(TopoDS_Vertex& vert) const
{
    Base::Vector3d point = DU::toVector3d(BRep_Tool::Pnt(vert));
    if (!is3d()) {
        auto dvp = static_cast<TechDraw::DrawViewPart*>(getObject());
        point = point / dvp->getScale();
    }
    BRepBuilderAPI_MakeVertex mkVert(DU::togp_Pnt(point));
    return { mkVert.Vertex() };
}

Part::TopoShape ReferenceEntry::asTopoShapeEdge(TopoDS_Edge &edge) const
{
//    Base::Console().Message("RE::asTopoShapeEdge()\n");
    TopoDS_Edge unscaledEdge = edge;
    if (!is3d()) {
        // 2d reference - projected and scaled. scale might have changed, so we need to unscale
        auto dvp = static_cast<TechDraw::DrawViewPart*>(getObject());
        TopoDS_Shape unscaledShape = ShapeUtils::scaleShape(edge, 1.0 / dvp->getScale());
        unscaledEdge = TopoDS::Edge(unscaledShape);
    }
    return { unscaledEdge };
}

std::string ReferenceEntry::geomType() const
{
    // Base::Console().Message("RE::geomType() - subName: **%s**\n", getSubName().c_str());
    return DrawUtil::getGeomTypeFromName(getSubName());
}

bool ReferenceEntry::isWholeObject() const
{
    return getSubName().empty();
}

bool ReferenceEntry::is3d() const
{
    if (getObject() &&
        getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) &&
        !getSubName().empty()) {
        // this is a well formed 2d reference
        return false;
    }

    if (getObject() &&
        getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) &&
        getSubName().empty()) {
        // this is a broken 3d reference, so it should be treated as 3d
        return true;
    }

    // either we have no object or we have an object and it is a 3d object
    return true;
}

//! check if this reference has valid geometry in the model
bool ReferenceEntry::hasGeometry() const
{
    // Base::Console().Message("RE::hasGeometry()\n");
    if (!getObject()) {
        return false;
    }

    if ( getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) ) {
        // 2d reference
        auto dvp = static_cast<TechDraw::DrawViewPart*>(getObject());
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
        } else if (gType == "Edge") {
            auto edge = dvp->getGeomByIndex(geomNumber);
            if (edge) {
                return true;
            }
        }
        // if we ever have dimensions for faces, add something here.
        return false;
    }

    // 3d reference
    auto shape = Part::Feature::getTopoShape(getObject());
    auto subShape = shape.getSubShape(getSubName().c_str());
    if (!subShape.IsNull()) {
        return true;
    }

    return false;
}
