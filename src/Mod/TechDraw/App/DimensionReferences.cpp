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
#include "GeometryObject.h"


using namespace TechDraw;
using DU = DrawUtil;

TopoDS_Shape ReferenceEntry::getGeometry() const
{
//    Base::Console().Message("RE::getGeometry()\n");
    if ( getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) ) {
        TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(getObject());
        std::string gType = geomType();
        if (gType == "Vertex") {
            auto vgeom = dvp->getVertex(getSubName());
            return vgeom->getOCCVertex();
        } else if (gType == "Edge") {
            auto egeom = dvp->getEdge(getSubName());
            return egeom->getOCCEdge();
        } else if (gType == "Face") {
            auto fgeom = dvp->getFace(getSubName());
            return fgeom->toOccFace();
        }
        //Base::Console().Message("RE::getGeometry - returns null shape! - gType: %s\n", gType.c_str());
        return TopoDS_Shape();
    }

    Part::TopoShape shape = Part::Feature::getTopoShape(getObject());
    App::GeoFeature* geoFeat = dynamic_cast<App::GeoFeature*>(getObject());
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

Part::TopoShape ReferenceEntry::asTopoShape()
{
//    Base::Console().Message("RE::asTopoShape()\n");
    TopoDS_Shape geom = getGeometry();

    if (geom.ShapeType() == TopAbs_VERTEX) {
        TopoDS_Vertex vert = TopoDS::Vertex(geom);
        return asTopoShapeVertex(vert);
    } else if (geom.ShapeType() == TopAbs_EDGE) {
        TopoDS_Edge edge = TopoDS::Edge(geom);
        return asTopoShapeEdge(edge);
    } else {
        throw Base::RuntimeError("Dimension Reference has unsupported geometry");
    }
    return Part::TopoShape();
}

Part::TopoShape ReferenceEntry::asTopoShapeVertex(TopoDS_Vertex vert)
{
    Base::Vector3d point = DU::toVector3d(BRep_Tool::Pnt(vert));
    if (!is3d()) {
        TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(getObject());
        point = point / dvp->getScale();
    }
    BRepBuilderAPI_MakeVertex mkVert(DU::togp_Pnt(point));
    return Part::TopoShape(mkVert.Vertex());
}

Part::TopoShape ReferenceEntry::asTopoShapeEdge(TopoDS_Edge edge)
{
//    Base::Console().Message("RE::asTopoShapeEdge()\n");
    TopoDS_Edge unscaledEdge = edge;
    if (!is3d()) {
        // 2d reference - projected and scaled. scale might have changed, so we need to unscale
        TechDraw::DrawViewPart* dvp = static_cast<TechDraw::DrawViewPart*>(getObject());
        TopoDS_Shape unscaledShape = TechDraw::scaleShape(edge, 1.0 / dvp->getScale());
        unscaledEdge = TopoDS::Edge(unscaledShape);
    }
    return Part::TopoShape(unscaledEdge);
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
    if ( getObject()->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) ) {
        return false;
    }
    return true;
}

