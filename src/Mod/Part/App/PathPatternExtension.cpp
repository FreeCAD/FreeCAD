// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PathPatternExtension.h"

#include <Mod/Part/App/Tools.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Mat.hxx>
#include <gp_Quaternion.hxx>

#include <App/DocumentObject.h>
#include <Base/Exception.h>

#include "PartFeature.h"
#include "PatternConstants.h"
#include "TopoShape.h"
#include "CircularPatternExtension.h"
#include "TopExp_Explorer.hxx"
#include "BaseGeometry.h"

using namespace Part;

EXTENSION_PROPERTY_SOURCE(Part::PathPatternExtension, App::DocumentObjectExtension)

const App::PropertyIntegerConstraint::Constraints PathPatternExtension::intCount
    = {1, std::numeric_limits<int>::max(), 1};

const char* PathPatternExtension::SpacingModeEnums[]
    = {"Fixed count", "Fixed spacing", "Fixed count and spacing", nullptr};

namespace
{

bool verticesCoincide(const TopoDS_Vertex& first, const TopoDS_Vertex& second)
{
    return BRep_Tool::Pnt(first).Distance(BRep_Tool::Pnt(second)) <= Precision::Confusion();
}

std::vector<TopoDS_Edge> getPathEdges(const App::PropertyLinkSub& pathProperty)
{
    auto* feature = freecad_cast<Part::Feature*>(pathProperty.getValue());
    if (!feature) {
        throw Base::TypeError("Path must reference an object with a shape");
    }

    const Part::TopoShape shape = feature->Shape.getShape();
    if (shape.isNull()) {
        throw Base::ValueError("Path shape is empty");
    }

    std::list<Part::TopoShape> edges;
    const auto subnames = pathProperty.getSubValues();

    for (const auto& subname : subnames) {
        if (subname.empty()) {
            continue;
        }

        const TopoDS_Shape subshape = shape.getSubShape(subname.c_str());

        if (subshape.ShapeType() == TopAbs_EDGE) {
            edges.emplace_back(subshape);
        }
        else if (subshape.ShapeType() == TopAbs_FACE) {
            for (TopExp_Explorer explorer(subshape, TopAbs_EDGE); explorer.More(); explorer.Next()) {
                edges.emplace_back(explorer.Current());
            }
        }
        else {
            throw Base::TypeError("Path subelements must be edges or faces");
        }
    }

    if (edges.empty()) {
        const auto wires = shape.getSubShapes(TopAbs_WIRE);
        if (!wires.empty()) {
            for (BRepTools_WireExplorer explorer(TopoDS::Wire(wires.front())); explorer.More();
                 explorer.Next()) {
                edges.emplace_back(explorer.Current());
            }
        }
        else {
            for (const auto& edge : shape.getSubShapes(TopAbs_EDGE)) {
                edges.emplace_back(edge);
            }
        }
    }

    if (edges.empty()) {
        throw Base::ValueError("Path does not contain edges");
    }

    std::vector<TopoDS_Edge> result;
    result.reserve(edges.size());
    result.push_back(TopoDS::Edge(edges.front().getShape()));
    edges.pop_front();

    while (!edges.empty()) {
        TopoDS_Vertex currentFirst;
        TopoDS_Vertex currentLast;
        TopoDS_Vertex ignored;
        TopExp::Vertices(result.front(), currentFirst, ignored, true);
        TopExp::Vertices(result.back(), ignored, currentLast, true);

        bool connected = false;
        for (auto it = edges.begin(); it != edges.end(); ++it) {
            TopoDS_Edge candidate = TopoDS::Edge(it->getShape());

            TopoDS_Vertex candidateFirst;
            TopoDS_Vertex candidateLast;
            TopExp::Vertices(candidate, candidateFirst, candidateLast, true);

            if (verticesCoincide(currentLast, candidateFirst)) {
                result.push_back(candidate);
            }
            else if (verticesCoincide(currentLast, candidateLast)) {
                result.push_back(TopoDS::Edge(candidate.Reversed()));
            }
            else if (verticesCoincide(currentFirst, candidateLast)) {
                result.insert(result.begin(), candidate);
            }
            else if (verticesCoincide(currentFirst, candidateFirst)) {
                result.insert(result.begin(), TopoDS::Edge(candidate.Reversed()));
            }
            else {
                continue;
            }

            edges.erase(it);
            connected = true;
            break;
        }

        if (!connected) {
            throw Base::ValueError("Path edges must form one connected path");
        }
    }

    return result;
}

double edgeLength(const TopoDS_Edge& edge)
{
    BRepAdaptor_Curve curve(edge);
    return GCPnts_AbscissaPoint::Length(
        curve,
        curve.FirstParameter(),
        curve.LastParameter(),
        Precision::Confusion()
    );
}

bool isClosed(const std::vector<TopoDS_Edge>& edges)
{
    TopoDS_Vertex first;
    TopoDS_Vertex ignored;
    TopoDS_Vertex last;
    TopExp::Vertices(edges.front(), first, ignored, true);
    TopExp::Vertices(edges.back(), ignored, last, true);
    if (first.IsNull() || last.IsNull()) {
        return false;
    }
    return verticesCoincide(first, last);
}

std::vector<double> calculateDistances(
    double pathLength,
    bool closed,
    int count,
    PathPatternSpacingMode mode,
    double spacing,
    double startOffset,
    double endOffset
)
{
    const double tolerance = Precision::Confusion();

    if (!std::isfinite(pathLength) || pathLength < 0.0 || !std::isfinite(startOffset)
        || !std::isfinite(endOffset)) {
        throw Base::ValueError("Path length and offsets must be finite");
    }
    if (startOffset < 0.0 || endOffset < 0.0) {
        throw Base::ValueError("Path offsets cannot be negative");
    }
    if (startOffset + endOffset > pathLength + tolerance) {
        throw Base::ValueError("Path offsets exceed the path length");
    }
    if (count < 1) {
        throw Base::ValueError("Occurrence count must be at least one");
    }

    const double available = std::max(0.0, pathLength - startOffset - endOffset);

    std::vector<double> distances;

    if (mode == PathPatternSpacingMode::FixedCount) {
        const bool fullClosedPath = closed && startOffset <= tolerance && endOffset <= tolerance;

        const int segments = fullClosedPath ? count : std::max(1, count - 1);
        const double step = available / segments;

        distances.reserve(static_cast<std::size_t>(count));
        for (int index = 0; index < count; ++index) {
            distances.push_back(startOffset + index * step);
        }
        return distances;
    }

    if (!std::isfinite(spacing) || spacing <= tolerance) {
        throw Base::ValueError("Path spacing must be greater than zero");
    }

    const int maximum = mode == PathPatternSpacingMode::FixedCountAndSpacing
        ? count
        : PatternConstants::MaximumOccurrences + 1;

    distances.reserve(
        static_cast<std::size_t>(std::min(maximum, PatternConstants::MaximumOccurrences))
    );

    const double endDistance = pathLength - endOffset;
    for (int index = 0; index < maximum; ++index) {
        const double distance = startOffset + index * spacing;

        if (distance > endDistance + tolerance) {
            break;
        }

        distances.push_back(distance);
        if (distances.size() > PatternConstants::MaximumOccurrences) {
            throw Base::ValueError(
                "Path pattern would create more than "
                + std::to_string(PatternConstants::MaximumOccurrences) + " occurrences"
            );
        }
    }

    return distances;
}

/** Build an occurrence frame at an arc-length distance along the ordered path.
 * `ends` contains cumulative edge lengths. Evaluate from the edge's oriented start.
 * With alignment disabled the frame only translates. Otherwise the Tangent vector is aligned with
 * the path's tangent on the first transform; the preferred vertical vector is projected
 * perpendicular to X to define Z. If they are parallel, a world axis supplies a stable fallback. Y
 * completes a right-handed frame.
 */
std::tuple<gp_Trsf, gp_Pnt, gp_Vec> frameAt(
    const std::vector<TopoDS_Edge>& edges,
    const std::vector<double>& ends,
    double distance,
    bool align,
    const gp_Vec& vertical,
    const bool reverseTangent = false,
    const gp_Pnt* originPosition = nullptr
)
{
    auto end = std::upper_bound(ends.begin(), ends.end(), distance);
    const std::size_t index = end == ends.end() ? edges.size() - 1
                                                : static_cast<std::size_t>(end - ends.begin());

    const double edgeStart = index == 0 ? 0.0 : ends[index - 1];
    const double edgeEnd = ends[index] - edgeStart;
    const double localDistance = std::clamp(distance - edgeStart, 0.0, edgeEnd);

    BRepAdaptor_Curve curve(edges[index]);
    const bool reversed = edges[index].Orientation() == TopAbs_REVERSED;
    const double startParameter = reversed ? curve.LastParameter() : curve.FirstParameter();

    double parameter = startParameter;

    if (localDistance > Precision::Confusion()) {
        GCPnts_AbscissaPoint point(curve, reversed ? -localDistance : localDistance, startParameter);
        if (!point.IsDone()) {
            throw Base::CADKernelError("Failed to evaluate point on path");
        }
        parameter = point.Parameter();
    }

    gp_Pnt position;
    gp_Vec tangent;
    curve.D1(parameter, position, tangent);

    if (tangent.Magnitude() <= Precision::Confusion()) {
        throw Base::ValueError("Path tangent is null");
    }
    tangent.Normalize();
    if (reversed ^ reverseTangent) {
        tangent.Reverse();
    }

    gp_Trsf toOrigin;
    if (originPosition) {
        toOrigin.SetTranslation(
            gp_Vec(-originPosition->X(), -originPosition->Y(), -originPosition->Z())
        );
    }

    gp_Trsf rotationTransform;
    if (align) {
        const gp_Dir xDirection(tangent);

        gp_Vec zCandidate = vertical;
        zCandidate -= gp_Vec(xDirection) * zCandidate.Dot(gp_Vec(xDirection));

        if (zCandidate.Magnitude() <= Precision::Confusion()) {
            zCandidate = std::abs(xDirection.Z()) < 0.9 ? gp_Vec(0.0, 0.0, 1.0)
                                                        : gp_Vec(0.0, 1.0, 0.0);

            zCandidate -= gp_Vec(xDirection) * zCandidate.Dot(gp_Vec(xDirection));
        }

        const gp_Dir zDirection(zCandidate);
        const gp_Dir yDirection(gp_Vec(zDirection).Crossed(gp_Vec(xDirection)));

        const gp_Mat rotationMatrix(
            xDirection.X(),
            yDirection.X(),
            zDirection.X(),
            xDirection.Y(),
            yDirection.Y(),
            zDirection.Y(),
            xDirection.Z(),
            yDirection.Z(),
            zDirection.Z()
        );

        rotationTransform.SetRotation(gp_Quaternion(rotationMatrix));
    }

    gp_Trsf toTarget;
    toTarget.SetTranslation(gp_Vec(position.X(), position.Y(), position.Z()));

    // toOrigin -> rotationTransform -> toTarget
    const gp_Trsf frame = toTarget * rotationTransform * toOrigin;

    return {frame, position, tangent};
}

}  // namespace

PathPatternExtension::PathPatternExtension()
{
    initExtensionType(PathPatternExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY_TYPE(
        Path,
        (nullptr),
        "PathPattern",
        App::Prop_None,
        "Path edges used by the pattern."
    );
    EXTENSION_ADD_PROPERTY_TYPE(Count, (4), "PathPattern", App::Prop_None, "Maximum number of occurrences.");
    Count.setConstraints(&intCount);
    EXTENSION_ADD_PROPERTY_TYPE(
        SpacingMode,
        (static_cast<long>(PathPatternSpacingMode::FixedCount)),
        "PathPattern",
        App::Prop_None,
        "Selects whether occurrences are controlled by count, spacing, or both."
    );
    SpacingMode.setEnums(SpacingModeEnums);
    EXTENSION_ADD_PROPERTY_TYPE(
        Spacing,
        (20.0),
        "PathPattern",
        App::Prop_None,
        "Distance between occurrences."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        StartOffset,
        (0.0),
        "PathPattern",
        App::Prop_None,
        "Unused distance at path start."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        EndOffset,
        (0.0),
        "PathPattern",
        App::Prop_None,
        "Unused distance at path end."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        CyclicOffset,
        (0.0),
        "PathPattern",
        App::Prop_None,
        "Closed paths only: Offsets all transformations by this distance."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        ReversePath,
        (false),
        "PathPattern",
        App::Prop_None,
        "Traverses the path in reverse."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        Align,
        (false),
        "PathPattern",
        App::Prop_None,
        "Aligns the occurrence to the path using the tangent direction."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        VerticalAxis,
        (nullptr),
        "PathPattern",
        App::Prop_None,
        "The axis representing the vertical direction of the first occurence."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        FlipVerticalAxis,
        (false),
        "PathPattern",
        App::Prop_None,
        "Reverse the vertical axis' direction."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        TangentAxis,
        (nullptr),
        "PathPattern",
        App::Prop_None,
        "The axis representing the tangent of the first occurence."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        FlipTangentAxis,
        (false),
        "PathPattern",
        App::Prop_None,
        "Reverse the tangent axis' direction."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        OriginPointIndex,
        (0),
        "PathPattern",
        App::Prop_None,
        "The origin point index, used for the identity transformation."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        RelativeToFirst,
        (false),
        "PathPattern",
        App::Prop_None,
        "Moves all transformations relative to the first one."
    );
    updatePropertyStatus();
}

bool PathPatternExtension::identityTransformSuppressed() const
{
    return true;
}

std::list<gp_Trsf> PathPatternExtension::calculateTransformations() const
{
    if (!Path.getValue()) {
        return {};
    }

    const bool reversed = ReversePath.getValue();

    auto edges = getPathEdges(Path);
    if (reversed) {
        std::reverse(edges.begin(), edges.end());
        for (auto& edge : edges) {
            edge.Reverse();
        }
    }

    std::vector<double> ends;
    ends.reserve(edges.size());
    double totalLength = 0.0;
    for (const auto& edge : edges) {
        totalLength += edgeLength(edge);
        ends.push_back(totalLength);
    }
    if (totalLength <= Precision::Confusion()) {
        throw Base::ValueError("Path length must be greater than zero");
    }

    const auto closed = isClosed(edges);

    const auto mode = static_cast<PathPatternSpacingMode>(SpacingMode.getValue());
    auto distances = calculateDistances(
        totalLength,
        closed,
        Count.getValue(),
        mode,
        Spacing.getValue(),
        reversed ? EndOffset.getValue() : StartOffset.getValue(),
        reversed ? StartOffset.getValue() : EndOffset.getValue()
    );
    if (distances.empty()) {
        throw Base::ValueError("Path pattern produced no occurrences");
    }

    if (closed && !distances.empty()) {
        const double offset = CyclicOffset.getValue();
        for (double& distance : distances) {
            distance = std::fmod(distance + offset, totalLength);
            if (distance < 0.0) {
                distance += totalLength;
            }
        }
    }

    const auto getWantedDirection = [&](const App::PropertyLinkSub& tangentAxisProperty,
                                        const App::PropertyBool& flipTangentAxisProperty,
                                        const gp_Vec& defaultDirection) -> gp_Vec {
        const gp_Ax2 tangentAxis
            = getAxis(tangentAxisProperty).value_or(gp_Ax2(gp_Pnt(), gp_Dir(defaultDirection)));

        gp_Vec tangent(tangentAxis.Direction());

        if (flipTangentAxisProperty.getValue()) {
            tangent.Reverse();
        }

        if (tangent.SquareMagnitude() < Precision::Confusion() * Precision::Confusion()) {
            tangent = defaultDirection;
        }

        return tangent;
    };

    const gp_Vec wantedTangent
        = getWantedDirection(TangentAxis, FlipTangentAxis, BaseGeometry::X_POS_VEC);

    const gp_Vec wantedVertical
        = getWantedDirection(VerticalAxis, FlipVerticalAxis, BaseGeometry::Z_POS_VEC);

    const bool relative = RelativeToFirst.getValue();
    const int originIndex = relative
        ? ((OriginPointIndex.getValue() % distances.size()) + distances.size()) % distances.size()
        : 0;

    if (originIndex < 0 || static_cast<std::size_t>(originIndex) >= distances.size()) {
        throw Base::ValueError("Origin point index is out of range");
    }

    // identity transformation for reference
    const auto [originFrame, originPosition, originTangent]
        = frameAt(edges, ends, distances[originIndex], Align.getValue(), wantedVertical);

    const bool needsTangentFlip = (originTangent.Dot(wantedTangent) < 0.0)
        ^ FlipTangentAxis.getValue();

    std::list<gp_Trsf> transformations;
    for (std::size_t i = 0; i < distances.size(); ++i) {
        auto [frame, position, tangent] = frameAt(
            edges,
            ends,
            distances[i],
            Align.getValue(),
            wantedVertical,
            needsTangentFlip,
            relative ? &originPosition : nullptr
        );

        transformations.push_back(frame);
    }

    return transformations;
}

short PathPatternExtension::extensionMustExecute()
{
    return Path.isTouched() || Count.isTouched() || SpacingMode.isTouched() || Spacing.isTouched()
        || StartOffset.isTouched() || EndOffset.isTouched() || ReversePath.isTouched()
        || Align.isTouched() || VerticalAxis.isTouched() || FlipVerticalAxis.isTouched()
        || TangentAxis.isTouched() || FlipTangentAxis.isTouched() || OriginPointIndex.isTouched()
        || RelativeToFirst.isTouched();
}

void PathPatternExtension::extensionOnChanged(const App::Property* prop)
{
    if (prop == &SpacingMode || prop == &Align) {
        updatePropertyStatus();
    }
    App::DocumentObjectExtension::extensionOnChanged(prop);
}

void PathPatternExtension::updatePropertyStatus()
{
    const auto mode = static_cast<PathPatternSpacingMode>(SpacingMode.getValue());
    Count.setStatus(App::Property::Hidden, mode == PathPatternSpacingMode::FixedSpacing);
    Spacing.setStatus(App::Property::Hidden, mode == PathPatternSpacingMode::FixedCount);
    VerticalAxis.setStatus(App::Property::Hidden, !Align.getValue());
    FlipVerticalAxis.setStatus(App::Property::Hidden, !Align.getValue());
    TangentAxis.setStatus(App::Property::Hidden, !Align.getValue());
    FlipTangentAxis.setStatus(App::Property::Hidden, !Align.getValue());
}

std::optional<gp_Ax2> PathPatternExtension::getAxis(const App::PropertyLinkSub& property) const
{
    return Part::getAxis(property, getExtendedObject());
}
