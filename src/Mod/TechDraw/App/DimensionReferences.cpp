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

TopoDS_Shape ReferenceEntry::getGeometry() const
{
//    Base::Console().Message("RE::getGeometry() - obj: %s  sub: %s\n",
//                            getObject()->getNameInDocument(), getSubName());
    if ( getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) ) {
        std::string gType;
        try {
            auto dvp = static_cast<TechDraw::DrawViewPart*>(getObject());
            gType = geomType();
            if (gType == "Vertex") {
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
//            Base::Console().Message("RE::getGeometry - no shape for dimension reference - gType: %s\n", gType.c_str());
            return {};
        }
    }

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
    // For PartDesign objects, when the reference is created from a selection,
    // the SelectionObject is a Feature within the Body.
    PartDesign::Body* pdBody = PartDesign::Body::findBodyOf(m_object);
    if (pdBody && pdBody->Tip.getValue()) {
        return pdBody->Tip.getValue();
    }
    return m_object;
}

Part::TopoShape ReferenceEntry::asTopoShape() const
{
//    Base::Console().Message("RE::asTopoShape()\n");
    TopoDS_Shape geom = getGeometry();
    if (geom.IsNull()) {
        throw Base::RuntimeError("Dimension Reference has null geometry");
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
    return DrawUtil::getGeomTypeFromName(getSubName());
}

bool ReferenceEntry::isWholeObject() const
{
    return getSubName().empty();
}

bool ReferenceEntry::is3d() const
{
    return !getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId());
}

//! check if this reference has valid geometry
bool ReferenceEntry::isValid() const
{
    TopoDS_Shape geom = getGeometry();
    if (geom.IsNull()) {
        return false;
    }
    return true;
}
