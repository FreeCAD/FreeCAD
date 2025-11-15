// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Sayantan Deb <sayantandebin[at]gmail.com>           *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "GizmoHelper.h"

#include <utility>

#include <BOPTools_AlgoTools3D.hxx>
#include <BRep_Tool.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopoDS.hxx>
#include <Precision.hxx>
#include <IntTools_Context.hxx>

#include <Base/BoundBox.h>
#include <Base/Converter.h>
#include <Mod/Part/App/Tools.h>


EdgeMidPointProps getEdgeMidPointProps(Part::TopoShape& edge)
{
    std::unique_ptr<Part::Geometry> geom = Part::Geometry::fromShape(edge.getShape());
    auto curve = freecad_cast<Part::GeomCurve*>(geom.get());
    double u1 = curve->getFirstParameter();
    double u2 = curve->getLastParameter();
    double middle = (u1 + u2) / 2;
    Base::Vector3d position = curve->pointAtParameter(middle);

    Base::Vector3d tangent;
    bool ret = curve->tangent(middle, tangent);
    if (ret) {
        return {position, tangent, middle};
    }

    Base::Console().error(
        "Failed to calculate tangent for the draggers! Please file a bug report for this."
    );
    return {position, Base::Vector3d {0, 0, 0}, middle};
}

Base::Vector3d getCentreOfMassFromFace(TopoDS_Face& face)
{
    GProp_GProps massProps;
    BRepGProp::SurfaceProperties(face, massProps);
    return Base::convertTo<Base::Vector3d>(massProps.CentreOfMass());
}

std::optional<std::pair<Base::Vector3d, Base::Vector3d>> getFaceNormalFromPointNearEdge(
    Part::TopoShape& edge,
    double middle,
    TopoDS_Face& face
)
{
    auto _edge = TopoDS::Edge(edge.getShape());

    gp_Pnt _inwardPoint;
    gp_Dir _normal;
    Handle(IntTools_Context) context = new IntTools_Context;

    if (!BOPTools_AlgoTools3D::GetApproxNormalToFaceOnEdge(
            _edge,
            face,
            middle,
            _inwardPoint,
            _normal,
            context
        )) {
        return std::nullopt;
    }

    return {{Base::convertTo<Base::Vector3d>(_inwardPoint), Base::convertTo<Base::Vector3d>(_normal)}};
}

Base::Vector3d getFaceNormalFromPoint(Base::Vector3d& point, TopoDS_Face& face)
{
    Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
    auto pt = Base::convertTo<gp_Pnt>(point);

    Standard_Real u, v;
    GeomAPI_ProjectPointOnSurf proj(pt, surf);
    proj.LowerDistanceParameters(u, v);
    GeomLProp_SLProps props(surf, u, v, 1, 0.01);

    return Base::convertTo<Base::Vector3d>(props.Normal());
}

std::pair<TopoDS_Face, TopoDS_Face> getAdjacentFacesFromEdge(
    Part::TopoShape& edge,
    Part::TopoShape& baseShape
)
{
    TopTools_IndexedDataMapOfShapeListOfShape edgeToFaceMap;

    TopExp::MapShapesAndAncestors(baseShape.getShape(), TopAbs_EDGE, TopAbs_FACE, edgeToFaceMap);
    const TopTools_ListOfShape& faces = edgeToFaceMap.FindFromKey(edge.getShape());

    assert(faces.Extent() >= 2 && "This is probably a bug so please report it to the issue tracker");

    TopoDS_Face face1 = TopoDS::Face(faces.First());
    TopoDS_Face face2 = TopoDS::Face(*(++faces.begin()));

    return {face1, face2};
}

DraggerPlacementProps getDraggerPlacementFromEdgeAndFace(Part::TopoShape& edge, TopoDS_Face& face)
{
    auto [position, tangent, middle] = getEdgeMidPointProps(edge);

    Base::Vector3d normal;
    Base::Vector3d inwardPoint;
    if (auto ret = getFaceNormalFromPointNearEdge(edge, middle, face)) {
        inwardPoint = ret->first;
        normal = ret->second;
    }
    else {
        // Failed to compute the normal at a point on the face near the edge
        // Fallback to the COM and hope for the best
        inwardPoint = getCentreOfMassFromFace(face);
        normal = getFaceNormalFromPoint(position, face);
    }

    auto diff = inwardPoint - position;

    Base::Vector3d dir = normal.Cross(tangent);
    // Get correct direction using the com (can we do detect this with a simpler operation?)
    if (dir.Dot(diff) < 0) {
        dir = -dir;
    }

    return {position, dir, tangent};
}

DraggerPlacementProps getDraggerPlacementFromEdgeAndFace(Part::TopoShape& edge, Part::TopoShape& face)
{
    TopoDS_Face _face = TopoDS::Face(face.getShape());
    return getDraggerPlacementFromEdgeAndFace(edge, _face);
}

std::vector<Part::TopoShape> getAdjacentEdgesFromFace(Part::TopoShape& face)
{
    assert(face.getShape().ShapeType() == TopAbs_FACE);

    std::vector<Part::TopoShape> edges;
    for (TopExp_Explorer explorer(face.getShape(), TopAbs_EDGE); explorer.More(); explorer.Next()) {
        edges.push_back(explorer.Current());
    }

    return edges;
}

Base::Vector3d getMidPointFromFace(Part::TopoShape& face)
{
    TopoDS_Shape shape = face.getShape();
    assert(shape.ShapeType() == TopAbs_FACE);

    std::unique_ptr<Part::Geometry> geom = Part::Geometry::fromShape(shape);
    auto* surface = freecad_cast<Part::GeomSurface*>(geom.get());

    TopoDS_Face _face = TopoDS::Face(shape);
    BRepAdaptor_Surface adaptorSurface = BRepAdaptor_Surface(_face, true);

    double u1 = adaptorSurface.FirstUParameter();
    double u2 = adaptorSurface.LastUParameter();
    double v1 = adaptorSurface.FirstVParameter();
    double v2 = adaptorSurface.LastVParameter();

    double midU = (u1 + u2) / 2;
    double midV = (v1 + v2) / 2;

    Base::Vector3d midPoint;

    if (auto sphere = freecad_cast<Part::GeomSphere*>(geom.get())) {
        midPoint = sphere->getLocation();
    }
    else if (auto cone = freecad_cast<Part::GeomCone*>(geom.get())) {
        midPoint = cone->getApex();
    }
    else if (auto point = surface->point(midU, midV)) {
        midPoint = *point;
    }
    else if (auto com = face.centerOfGravity()) {
        midPoint = *com;
    }
    else {
        midPoint = face.getBoundBox().GetCenter();
    }

    return midPoint;
}

Base::Vector3d getMidPointFromProfile(Part::TopoShape& profile)
{
    TopoDS_Shape shape = profile.getShape();

    // Can we handle more cases?
    if (shape.ShapeType() == TopAbs_FACE) {
        return getMidPointFromFace(profile);
    }

    Base::Vector3d midPoint;
    profile.getCenterOfGravity(midPoint);

    return midPoint;
}
