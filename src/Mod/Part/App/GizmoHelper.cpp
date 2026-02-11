// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Sayantan Deb <sayantandebin[at]gmail.com>           *
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
#include <ElCLib.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopoDS.hxx>
#include <Precision.hxx>
#include <IntTools_Context.hxx>
#include <IntAna_IntConicQuad.hxx>
#include <IntAna_QuadQuadGeo.hxx>

#include <Base/BoundBox.h>
#include <Base/Converter.h>
#include <Mod/Part/App/Tools.h>


EdgeMidPointProps getEdgeMidPointProps(Part::TopoShape& edge)
{
    double u1, u2;
    TopoDS_Edge TDSEdge = TopoDS::Edge(edge.getShape());
    Handle(Geom_Curve) curve = BRep_Tool::Curve(TDSEdge, u1, u2);
    double middle = (u1 + u2) / 2.0;

    gp_Pnt pos;
    gp_Vec derivative;
    curve->D1(middle, pos, derivative);

    auto position = Base::convertTo<Base::Vector3d>(pos);
    auto tangent = Base::convertTo<Base::Vector3d>(derivative);
    tangent.Normalize();

    if (TDSEdge.Orientation() == TopAbs_REVERSED) {
        tangent = -tangent;
    }

    return {position, tangent, middle};
}

Base::Vector3d getCentreOfMassFromFace(TopoDS_Face& face)
{
    GProp_GProps massProps;
    BRepGProp::SurfaceProperties(face, massProps);
    return Base::convertTo<Base::Vector3d>(massProps.CentreOfMass());
}

PointOnFaceNearEdgeProps getFaceNormalFromPointNearEdge(
    Part::TopoShape& edge,
    double middle,
    TopoDS_Face& face
)
{
    auto TDSedge = TopoDS::Edge(edge.getShape());

    gp_Pnt inwardPoint;
    gp_Pnt2d inwardPoint2d;
    gp_Dir normal;
    Handle(IntTools_Context) context = new IntTools_Context;

    int res
        = BOPTools_AlgoTools3D::PointNearEdge(TDSedge, face, middle, inwardPoint2d, inwardPoint, context);

    PointOnFaceNearEdgeProps props;
    switch (res) {
        case 0:
            props.state = PointOnFaceNearEdgeProps::State::OnFace;
            break;
        case 2:
            props.state = PointOnFaceNearEdgeProps::State::OutsideFace;
            break;
        default:
            props.state = PointOnFaceNearEdgeProps::State::Undefined;
            return props;
    }

    BOPTools_AlgoTools3D::GetNormalToFaceOnEdge(TDSedge, face, middle, normal, context);

    props.position = Base::convertTo<Base::Vector3d>(inwardPoint);
    props.normal = Base::convertTo<Base::Vector3d>(normal);

    return props;
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
    auto props = getFaceNormalFromPointNearEdge(edge, middle, face);
    if (props.state != PointOnFaceNearEdgeProps::State::Undefined) {
        inwardPoint = props.position;
        normal = props.normal;
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

    // Assumption: Since the point is very close to the edge but lies outside
    // the face we can reverse the direction to point towards the face material
    if (props.state == PointOnFaceNearEdgeProps::State::OutsideFace) {
        dir = -dir;
    }

    return {position, dir};
}

DraggerPlacementProps getDraggerPlacementFromEdgeAndFace(Part::TopoShape& edge, Part::TopoShape& face)
{
    TopoDS_Face TDSFace = TopoDS::Face(face.getShape());
    return getDraggerPlacementFromEdgeAndFace(edge, TDSFace);
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

std::optional<DraggerPlacementPropsWithNormals> getDraggerPlacementFromPlaneAndFace(
    Part::TopoShape& face,
    gp_Pln& plane
)
{
    TopoDS_Face TDSFace = TopoDS::Face(face.getShape());
    if (TDSFace.IsNull()) {
        return std::nullopt;
    }

    auto cog = getCentreOfMassFromFace(TDSFace);
    auto orientation = TDSFace.Orientation();

    auto getPropsFromShapePlaneIntersection =
        [&cog](auto&& shape, const gp_Pln& plane) -> std::optional<DraggerPlacementPropsWithNormals> {
        if (plane.Axis().IsNormal(shape.Axis(), Precision::Angular())) {
            return std::nullopt;
        }

        gp_Lin line(shape.Axis());
        IntAna_IntConicQuad intersector(line, plane, Precision::Confusion());
        if (intersector.IsDone() && intersector.NbPoints() > 0) {
            auto pos = Base::convertTo<Base::Vector3d>(intersector.Point(1));
            return DraggerPlacementPropsWithNormals {
                .placementProps = {.position = pos, .dir = cog - pos},
                .normalProps = std::nullopt
            };
        }
        return std::nullopt;
    };

    auto getPropsFromPlanePlaneIntersection = [&cog, orientation](
                                                  const gp_Pln&& facePlane,
                                                  const gp_Pln& plane
                                              ) -> std::optional<DraggerPlacementPropsWithNormals> {
        if (plane.Axis().IsParallel(facePlane.Axis(), Precision::Angular())) {
            return std::nullopt;
        }

        IntAna_QuadQuadGeo intersector(facePlane, plane, Precision::Angular(), Precision::Confusion());
        if (intersector.IsDone() && intersector.NbSolutions() > 0) {
            gp_Lin line = intersector.Line(1);
            Standard_Real u = ElCLib::Parameter(line, Base::convertTo<gp_Pnt>(cog));
            auto pos = Base::convertTo<Base::Vector3d>(ElCLib::Value(u, line));
            auto lineDir = Base::convertTo<Base::Vector3d>(line.Direction());
            auto faceNormal = Base::convertTo<Base::Vector3d>(facePlane.Axis().Direction());
            if (orientation == TopAbs_REVERSED) {
                faceNormal *= -1;
            }

            return DraggerPlacementPropsWithNormals {
                .placementProps = {.position = pos, .dir = cog - pos},
                .normalProps = DraggerNormalProps {.normal = lineDir, .faceNormal = faceNormal}
            };
        }
        return std::nullopt;
    };

    BRepAdaptor_Surface adapt(TDSFace);
    switch (adapt.GetType()) {
        case GeomAbs_Cylinder:
            return getPropsFromShapePlaneIntersection(adapt.Cylinder(), plane);
        case GeomAbs_Cone:
            return getPropsFromShapePlaneIntersection(adapt.Cone(), plane);
        case GeomAbs_Plane:
            return getPropsFromPlanePlaneIntersection(adapt.Plane(), plane);
        default:
            return std::nullopt;
    }
}
