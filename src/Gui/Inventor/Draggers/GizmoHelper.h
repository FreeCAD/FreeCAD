#ifndef GIZMO_HELPER_H
#define GIZMO_HELPER_H

#include <utility>

#include <BRep_Tool.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopoDS.hxx>

#include <Base/Converter.h>
#include <Base/Vector3D.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/Geometry.h>

#include "Gizmo.h"
#include "SoLinearDragger.h"

struct EdgeMidPointProps {
    Base::Vector3d position;
    Base::Vector3d tangent;
};
inline EdgeMidPointProps getEdgeMidPointProps(Part::TopoShape& edge)
{
    std::unique_ptr<Part::Geometry> geom = Part::Geometry::fromShape(edge.getShape());
    auto curve = freecad_cast<Part::GeomCurve*>(geom.get());
    double u1 = curve->getFirstParameter();
    double u2 = curve->getLastParameter();
    double middle = (u1 + u2) / 2;
    Base::Vector3d position = curve->pointAtParameter(middle);

    Base::Vector3d tangent;
    assert(curve->tangent(middle, tangent) && "This is probably a bug so please report it to the issue tracker");

    return {position, tangent};
}

inline Base::Vector3d getCentreOfMassFromFace(TopoDS_Face& face)
{
    GProp_GProps massProps;
    BRepGProp::SurfaceProperties(face, massProps);
    return Base::convertTo<Base::Vector3d>(massProps.CentreOfMass());
}

inline Base::Vector3d getFaceNormalFromPoint(Base::Vector3d& point, TopoDS_Face& face)
{
    Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
    auto pt = Base::convertTo<gp_Pnt>(point);

    Standard_Real u, v;
    GeomAPI_ProjectPointOnSurf proj(pt, surf);
    proj.LowerDistanceParameters(u, v);
    GeomLProp_SLProps props(surf, u, v, 1, 0.01);

    return Base::convertTo<Base::Vector3d>(props.Normal());
}

inline std::pair<TopoDS_Face, TopoDS_Face> getAdjacentFacesFromEdge(Part::TopoShape& edge, Part::TopoShape& baseShape)
{
    TopTools_IndexedDataMapOfShapeListOfShape edgeToFaceMap;

    TopExp::MapShapesAndAncestors(baseShape.getShape(), TopAbs_EDGE, TopAbs_FACE, edgeToFaceMap);
    const TopTools_ListOfShape& faces = edgeToFaceMap.FindFromKey(edge.getShape());

    assert(faces.Extent() >= 2 && "This is probably a bug so please report it to the issue tracker");

    TopoDS_Face face1 = TopoDS::Face(faces.First());
    TopoDS_Face face2 = TopoDS::Face(*(++faces.begin()));

    return {face1, face2};
}

struct DraggerPlacementProps {
    Base::Vector3d position;
    Base::Vector3d dir;
    Base::Vector3d tangent;
};
inline DraggerPlacementProps getDraggerPlacementFromEdgeAndFace(Part::TopoShape& edge, TopoDS_Face& face)
{
    auto [position, tangent] = getEdgeMidPointProps(edge);
    Base::Vector3d normal = getFaceNormalFromPoint(position, face);

    Base::Vector3d com = getCentreOfMassFromFace(face);
    auto diff = com - position;

    // Get correct direction using the com (can we do detect this with a simpler operation?)
    Base::Vector3d dir = normal.Cross(tangent);
    if (dir.Dot(diff) < 0) {
        dir = -dir;
    }

    return {position, dir, tangent};
}

inline DraggerPlacementProps getDraggerPlacementFromEdgeAndFace(Part::TopoShape& edge, Part::TopoShape& face)
{
    TopoDS_Face _face = TopoDS::Face(face.getShape());
    return getDraggerPlacementFromEdgeAndFace(edge, _face);
}

inline std::vector<Part::TopoShape> getAdjacentEdgesFromFace(Part::TopoShape& face)
{
    assert(face.getShape().ShapeType() == TopAbs_FACE);

    std::vector<Part::TopoShape> edges;
    for (TopExp_Explorer explorer(face.getShape(), TopAbs_EDGE); explorer.More(); explorer.Next()) {
        edges.push_back(explorer.Current());
    }

    return edges;
}

inline void reverseGizmoDir(Gui::LinearGizmo* gizmo) {
    auto dir = gizmo->getDraggerContainer()->getPointerDirection();
    gizmo->getDraggerContainer()->setPointerDirection(dir * -1);
}

#endif /* GIZMO_HELPER_H */
