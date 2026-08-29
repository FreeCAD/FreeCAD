// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MeasuredGeometryHelper.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <string_view>
#include <utility>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

#include <App/DocumentObject.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PropertyTopoShape.h>

namespace MeasureGui::MeasuredGeometryHelper
{
namespace
{

std::string stableSketchGeometryKey(const std::string& subname)
{
    const auto geometryMarker = subname.rfind(";g");
    const auto externalMarker = subname.rfind(";e");
    auto marker = geometryMarker;
    if (marker == std::string::npos
        || (externalMarker != std::string::npos && externalMarker > marker)) {
        marker = externalMarker;
    }
    if (marker == std::string::npos) {
        return {};
    }

    std::size_t end = marker + 2;
    const std::size_t idStart = end;
    while (end < subname.size() && subname[end] >= '0' && subname[end] <= '9') {
        ++end;
    }
    if (end == idStart) {
        return {};
    }

    // A mapped Sketcher vertex appends v<point-position> to the geometry id.
    if (end < subname.size() && subname[end] == 'v') {
        const std::size_t positionStart = ++end;
        while (end < subname.size() && subname[end] >= '0' && subname[end] <= '9') {
            ++end;
        }
        if (end == positionStart) {
            return {};
        }
    }

    // Element maps use a dot (";g7.3"), while mapped link subnames use a semicolon
    // (";g7;SKT.Edge1"). Both forms identify the same persistent Sketcher geometry.
    if (end < subname.size() && subname[end] != '.' && subname[end] != ';') {
        return {};
    }

    return subname.substr(marker, end - marker);
}

std::string stableSketchGeometryKeyForElement(App::DocumentObject* owner, const std::string& element)
{
    if (!owner || element.empty()) {
        return {};
    }

    Part::TopoShape ownerShape;
    std::string localElement = element;
    constexpr std::string_view internalPrefix("Internal");
    if (localElement.starts_with(internalPrefix)) {
        localElement.erase(0, internalPrefix.size());
        auto* internalShape = owner->getPropertyByName<Part::PropertyPartShape>("InternalShape");
        if (internalShape) {
            ownerShape = internalShape->getShape();
        }
    }
    else {
        ownerShape = Part::Feature::getTopoShape(owner, Part::ShapeOption::ResolveLink);
    }

    if (ownerShape.isNull()) {
        return {};
    }

    return stableSketchGeometryKey(ownerShape.getElementName(localElement.c_str()).name.toString());
}

struct ResolvedGeometry
{
    App::DocumentObject* owner {nullptr};
    Part::TopoShape ownerShape;
    Part::TopoShape selectedShape;
    std::string objectPathPrefix;
    std::string edgeNamePrefix;
    bool internalShape {false};
};

bool resolveGeometry(App::DocumentObject* object, const std::string& subname, ResolvedGeometry& resolved)
{
    if (!object || subname.empty()) {
        return false;
    }

    App::SubObjectT subject(object, subname.c_str());
    const auto objectPath = subject.getSubObjectList();
    const char* elementName = subject.getElementName();
    if (objectPath.empty() || !elementName || !elementName[0]) {
        return false;
    }

    resolved.owner = objectPath.back();

    // Keep the mapped element name and resolve it against the shape that actually owns the
    // subelement. Resolving getOldElementName() from the root Body can remap an older Pad face to
    // a numerically matching but different edge of the current Body tip.
    std::string localElementName = elementName;
    if (localElementName.empty()) {
        localElementName = subject.getOldElementName();
    }

    constexpr std::string_view internalPrefix("Internal");
    if (localElementName.starts_with(internalPrefix)) {
        localElementName.erase(0, internalPrefix.size());
        resolved.edgeNamePrefix.assign(internalPrefix);
        resolved.internalShape = true;

        // Sketcher generated faces/edges live in InternalShape, not in the visible Shape.
        auto* internalShape = resolved.owner->getPropertyByName<Part::PropertyPartShape>(
            "InternalShape"
        );
        if (internalShape) {
            resolved.ownerShape = internalShape->getShape();
        }
    }
    else {
        resolved.ownerShape
            = Part::Feature::getTopoShape(resolved.owner, Part::ShapeOption::ResolveLink);
    }

    if (resolved.ownerShape.isNull()) {
        return false;
    }

    resolved.selectedShape = resolved.ownerShape.getSubTopoShape(localElementName.c_str(), true);
    if (resolved.selectedShape.isNull()) {
        return false;
    }

    resolved.objectPathPrefix = subject.getSubNameNoElement();
    return true;
}

std::string visibleSketchCircleSubname(const ResolvedGeometry& resolved, const TopoDS_Edge& internalEdge)
{
    if (!resolved.internalShape || !resolved.owner) {
        return {};
    }

    Part::TopoShape visibleShape
        = Part::Feature::getTopoShape(resolved.owner, Part::ShapeOption::ResolveLink);
    if (visibleShape.isNull()) {
        return {};
    }

    try {
        BRepAdaptor_Curve internalCurve(internalEdge);
        if (internalCurve.GetType() != GeomAbs_Circle) {
            return {};
        }

        const auto internalCircle = internalCurve.Circle();
        const double tolerance = std::max(Precision::Confusion(), internalCircle.Radius() * 1.0e-7);

        for (TopExp_Explorer edges(visibleShape.getShape(), TopAbs_EDGE); edges.More(); edges.Next()) {
            const auto candidate = TopoDS::Edge(edges.Current());
            BRepAdaptor_Curve candidateCurve(candidate);
            if (candidateCurve.GetType() != GeomAbs_Circle) {
                continue;
            }

            const auto candidateCircle = candidateCurve.Circle();
            if (std::abs(candidateCircle.Radius() - internalCircle.Radius()) > tolerance
                || candidateCircle.Location().Distance(internalCircle.Location()) > tolerance) {
                continue;
            }

            const int edgeIndex = visibleShape.findShape(candidate);
            if (edgeIndex > 0) {
                return resolved.objectPathPrefix + "Edge" + std::to_string(edgeIndex);
            }
        }
    }
    catch (const Standard_Failure&) {
        return {};
    }

    return {};
}

std::string edgeSubname(const ResolvedGeometry& resolved, const TopoDS_Edge& edge)
{
    const int edgeIndex = resolved.ownerShape.findShape(edge);
    if (edgeIndex <= 0) {
        return {};
    }

    return resolved.objectPathPrefix + resolved.edgeNamePrefix + "Edge" + std::to_string(edgeIndex);
}

bool isCircularEdge(const TopoDS_Edge& edge)
{
    try {
        return BRepAdaptor_Curve(edge).GetType() == GeomAbs_Circle;
    }
    catch (const Standard_Failure&) {
        return false;
    }
}

}  // namespace

CanonicalGeometryReference canonicalGeometryReference(
    App::DocumentObject* object,
    const std::string& subname
)
{
    if (!object) {
        return {};
    }

    if (subname.empty()) {
        return {object, {}, {}};
    }

    const App::SubObjectT reference(object, subname.c_str());
    const auto path = reference.getSubObjectList();
    App::DocumentObject* owner = path.empty() ? object : path.back();

    std::string element = reference.getOldElementName();
    if (element.empty()) {
        const char* rawElement = reference.getElementName();
        if (rawElement) {
            element = rawElement;
        }
    }

    std::string stableSketchGeometry = stableSketchGeometryKey(reference.getSubName());
    if (stableSketchGeometry.empty()) {
        // Preserve the mapped id supplied by a PropertyLinkSub even if SubObjectT normalizes its
        // stored subname while resolving the path.
        stableSketchGeometry = stableSketchGeometryKey(subname);
    }
    if (stableSketchGeometry.empty()) {
        // Old-style EdgeN references may not contain the id text. Recover it from the current
        // Sketcher element map so old and new link representations still compare consistently.
        stableSketchGeometry = stableSketchGeometryKeyForElement(owner, element);
    }

    return {owner, std::move(stableSketchGeometry), std::move(element)};
}

bool referencesSameGeometry(
    App::DocumentObject* firstObject,
    const std::string& firstSubname,
    App::DocumentObject* secondObject,
    const std::string& secondSubname,
    bool allowCurrentElementFallback
)
{
    if (!firstObject || !secondObject) {
        return false;
    }

    // Empty subnames represent whole objects, not every sub-element owned by those objects.
    if (firstSubname.empty() || secondSubname.empty()) {
        return firstSubname.empty() && secondSubname.empty() && firstObject == secondObject;
    }

    const auto first = canonicalGeometryReference(firstObject, firstSubname);
    const auto second = canonicalGeometryReference(secondObject, secondSubname);
    if (!first.owner || first.owner != second.owner) {
        return false;
    }

    // When both sides carry persistent Sketcher ids, those ids are authoritative. If only one
    // side is mapped, permit an EdgeN fallback solely for temporary boundary subnames resolved
    // from the current shape, never for duplicate-measurement detection.
    if (!first.stableSketchGeometry.empty() || !second.stableSketchGeometry.empty()) {
        if (!first.stableSketchGeometry.empty() && !second.stableSketchGeometry.empty()) {
            return first.stableSketchGeometry == second.stableSketchGeometry;
        }
        if (!allowCurrentElementFallback) {
            return false;
        }
    }

    return !first.element.empty() && first.element == second.element;
}

std::vector<std::string> getBoundarySubnames(App::DocumentObject* object, const std::string& subname)
{
    ResolvedGeometry resolved;
    if (!resolveGeometry(object, subname, resolved)) {
        return {};
    }

    std::vector<std::string> result;
    std::set<int> edgeIndices;
    const TopoDS_Shape& selected = resolved.selectedShape.getShape();

    const auto append = [&](const TopoDS_Edge& edge) {
        const int edgeIndex = resolved.ownerShape.findShape(edge);
        if (edgeIndex <= 0 || !edgeIndices.insert(edgeIndex).second) {
            return;
        }
        result.push_back(
            resolved.objectPathPrefix + resolved.edgeNamePrefix + "Edge" + std::to_string(edgeIndex)
        );
    };

    if (selected.ShapeType() == TopAbs_EDGE) {
        append(TopoDS::Edge(selected));
        return result;
    }

    for (TopExp_Explorer edges(selected, TopAbs_EDGE); edges.More(); edges.Next()) {
        append(TopoDS::Edge(edges.Current()));
    }

    return result;
}

std::vector<std::string> getCircularBoundarySubnames(
    App::DocumentObject* object,
    const std::string& subname
)
{
    ResolvedGeometry resolved;
    if (!resolveGeometry(object, subname, resolved)) {
        return {};
    }

    const TopoDS_Shape& selected = resolved.selectedShape.getShape();

    // A directly measured circular edge is unambiguous.
    if (selected.ShapeType() == TopAbs_EDGE) {
        const auto edge = TopoDS::Edge(selected);
        if (!isCircularEdge(edge)) {
            return {};
        }

        const std::string visible = visibleSketchCircleSubname(resolved, edge);
        const std::string local = edgeSubname(resolved, edge);
        return local.empty() ? std::vector<std::string> {}
                             : std::vector<std::string> {visible.empty() ? local : visible};
    }

    if (selected.ShapeType() != TopAbs_FACE) {
        return {};
    }

    // Match MeasureRadiusHandler's planar-face case: one circular boundary. Cylinders, spheres and
    // tori have no single unambiguous outline here, so they deliberately do not get an edge overlay.
    try {
        if (BRepAdaptor_Surface(TopoDS::Face(selected)).GetType() != GeomAbs_Plane) {
            return {};
        }
    }
    catch (const Standard_Failure&) {
        return {};
    }

    TopExp_Explorer edges(selected, TopAbs_EDGE);
    if (!edges.More()) {
        return {};
    }
    const auto edge = TopoDS::Edge(edges.Current());
    edges.Next();
    if (edges.More() || !isCircularEdge(edge)) {
        return {};
    }

    const std::string visible = visibleSketchCircleSubname(resolved, edge);
    const std::string local = edgeSubname(resolved, edge);
    return local.empty() ? std::vector<std::string> {}
                         : std::vector<std::string> {visible.empty() ? local : visible};
}

}  // namespace MeasureGui::MeasuredGeometryHelper
