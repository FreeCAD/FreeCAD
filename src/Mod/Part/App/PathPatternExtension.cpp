// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PathPatternExtension.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
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
#include "TopoShape.h"

using namespace Part;

EXTENSION_PROPERTY_SOURCE(Part::PathPatternExtension, App::DocumentObjectExtension)

const App::PropertyIntegerConstraint::Constraints PathPatternExtension::intCount = {
    1,
    std::numeric_limits<int>::max(),
    1
};

const char* PathPatternExtension::SpacingModeEnums[] = {
    "Fixed count",
    "Fixed spacing",
    "Fixed count and spacing",
    nullptr
};

namespace
{

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
        if (subshape.ShapeType() != TopAbs_EDGE) {
            throw Base::TypeError("Path subelements must be edges");
        }
        edges.emplace_back(subshape);
    }

    if (edges.empty()) {
        const auto wires = shape.getSubShapes(TopAbs_WIRE);
        if (!wires.empty()) {
            for (BRepTools_WireExplorer explorer(TopoDS::Wire(wires.front()));
                 explorer.More();
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

            if (BRep_Tool::Pnt(currentLast).Distance(BRep_Tool::Pnt(candidateFirst))
                <= Precision::Confusion()) {
                result.push_back(candidate);
            }
            else if (BRep_Tool::Pnt(currentLast).Distance(BRep_Tool::Pnt(candidateLast))
                     <= Precision::Confusion()) {
                result.push_back(TopoDS::Edge(candidate.Reversed()));
            }
            else if (BRep_Tool::Pnt(currentFirst).Distance(BRep_Tool::Pnt(candidateLast))
                     <= Precision::Confusion()) {
                result.insert(result.begin(), candidate);
            }
            else if (BRep_Tool::Pnt(currentFirst).Distance(BRep_Tool::Pnt(candidateFirst))
                     <= Precision::Confusion()) {
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
        curve, curve.FirstParameter(), curve.LastParameter(), Precision::Confusion()
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
    return BRep_Tool::Pnt(first).Distance(BRep_Tool::Pnt(last)) <= Precision::Confusion();
}

std::vector<double> calculateDistances(double pathLength,
                                       bool closed,
                                       int count,
                                       PathPatternSpacingMode mode,
                                       double spacing,
                                       double startOffset,
                                       double endOffset)
{
    if (startOffset < 0.0 || endOffset < 0.0) {
        throw Base::ValueError("Path offsets cannot be negative");
    }
    if (startOffset + endOffset > pathLength + Precision::Confusion()) {
        throw Base::ValueError("Path offsets exceed the path length");
    }

    const double available = std::max(0.0, pathLength - startOffset - endOffset);
    std::vector<double> distances;

    if (mode == PathPatternSpacingMode::FixedCount) {
        const int segments = std::max(1, closed && startOffset == 0.0 && endOffset == 0.0
                                            ? count
                                            : count - 1);
        const double step = available / segments;
        distances.reserve(count);
        for (int index = 0; index < count; ++index) {
            distances.push_back(startOffset + index * step);
        }
        return distances;
    }

    if (spacing <= Precision::Confusion()) {
        throw Base::ValueError("Path spacing must be greater than zero");
    }

    const int maximum = mode == PathPatternSpacingMode::FixedCountAndSpacing
        ? count
        : std::numeric_limits<int>::max();
    for (int index = 0; index < maximum; ++index) {
        const double distance = startOffset + index * spacing;
        if (distance > startOffset + available + Precision::Confusion()) {
            break;
        }
        distances.push_back(distance);
        if (distances.size() > 10000) {
            throw Base::ValueError("Path pattern would create more than 10000 occurrences");
        }
    }
    return distances;
}

gp_Trsf frameAt(const std::vector<TopoDS_Edge>& edges,
                const std::vector<double>& ends,
                double distance,
                bool align,
                const Base::Vector3d& vertical)
{
    auto end = std::lower_bound(ends.begin(), ends.end(), distance - Precision::Confusion());
    const std::size_t index =
        end == ends.end() ? edges.size() - 1 : static_cast<std::size_t>(end - ends.begin());
    const double edgeStart = index == 0 ? 0.0 : ends[index - 1];
    const double localDistance = std::clamp(distance - edgeStart, 0.0, edgeLength(edges[index]));

    BRepAdaptor_Curve curve(edges[index]);
    const bool reversed = edges[index].Orientation() == TopAbs_REVERSED;
    const double startParameter =
        reversed ? curve.LastParameter() : curve.FirstParameter();
    double parameter = startParameter;
    if (localDistance > Precision::Confusion()) {
        GCPnts_AbscissaPoint point(
            curve, reversed ? -localDistance : localDistance, startParameter
        );
        if (!point.IsDone()) {
            throw Base::CADKernelError("Failed to evaluate point on path");
        }
        parameter = point.Parameter();
    }

    gp_Pnt position;
    gp_Vec tangent;
    curve.D1(parameter, position, tangent);
    if (reversed) {
        tangent.Reverse();
    }

    gp_Trsf frame;
    if (align) {
        if (tangent.Magnitude() <= Precision::Confusion()) {
            throw Base::ValueError("Path tangent is null");
        }
        gp_Dir xDirection(tangent);
        gp_Vec zCandidate(vertical.x, vertical.y, vertical.z);
        zCandidate -= gp_Vec(xDirection) * zCandidate.Dot(gp_Vec(xDirection));
        if (zCandidate.Magnitude() <= Precision::Confusion()) {
            zCandidate = std::abs(xDirection.Z()) < 0.9 ? gp_Vec(0.0, 0.0, 1.0)
                                                       : gp_Vec(0.0, 1.0, 0.0);
            zCandidate -= gp_Vec(xDirection) * zCandidate.Dot(gp_Vec(xDirection));
        }
        const gp_Dir zDirection(zCandidate);
        const gp_Dir yDirection(gp_Vec(zDirection).Crossed(gp_Vec(xDirection)));
        const gp_Mat rotation(
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
        frame.SetRotation(gp_Quaternion(rotation));
    }
    frame.SetTranslationPart(gp_Vec(position.X(), position.Y(), position.Z()));
    return frame;
}

}  // namespace

PathPatternExtension::PathPatternExtension()
{
    initExtensionType(PathPatternExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY_TYPE(
        Path, (nullptr), "PathPattern", App::Prop_None, "Path edges used by the pattern."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        Count, (4), "PathPattern", App::Prop_None, "Maximum number of occurrences."
    );
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
        Spacing, (20.0), "PathPattern", App::Prop_None, "Distance between occurrences."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        StartOffset, (0.0), "PathPattern", App::Prop_None, "Unused distance at path start."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        EndOffset, (0.0), "PathPattern", App::Prop_None, "Unused distance at path end."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        ReversePath, (false), "PathPattern", App::Prop_None, "Traverse the path in reverse."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        Align, (false), "PathPattern", App::Prop_None, "Align occurrence X axes to the path."
    );
    EXTENSION_ADD_PROPERTY_TYPE(
        VerticalVector,
        (Base::Vector3d(0.0, 0.0, 1.0)),
        "PathPattern",
        App::Prop_None,
        "Preferred local Z direction when aligning occurrences."
    );
    updatePropertyStatus();
}

std::list<gp_Trsf> PathPatternExtension::calculateTransformations(bool relativeToFirst) const
{
    if (!Path.getValue()) {
        return {};
    }

    auto edges = getPathEdges(Path);
    if (ReversePath.getValue()) {
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

    const auto mode = static_cast<PathPatternSpacingMode>(SpacingMode.getValue());
    const auto distances = calculateDistances(totalLength,
                                              isClosed(edges),
                                              Count.getValue(),
                                              mode,
                                              Spacing.getValue(),
                                              StartOffset.getValue(),
                                              EndOffset.getValue());
    if (distances.empty()) {
        throw Base::ValueError("Path pattern produced no occurrences");
    }

    std::list<gp_Trsf> transformations;
    for (double distance : distances) {
        transformations.push_back(
            frameAt(edges, ends, distance, Align.getValue(), VerticalVector.getValue())
        );
    }

    if (relativeToFirst) {
        const gp_Trsf& first = transformations.front();
        const gp_Quaternion inverseFirstRotation = first.GetRotation().Inverted();
        const gp_XYZ firstTranslation = first.TranslationPart();
        for (auto& transform : transformations) {
            gp_Quaternion relativeRotation =
                transform.GetRotation().Multiplied(inverseFirstRotation);
            const gp_XYZ relativeTranslation =
                transform.TranslationPart() - firstTranslation;
            transform.SetRotation(relativeRotation);
            transform.SetTranslationPart(gp_Vec(relativeTranslation));
        }
    }
    return transformations;
}

short PathPatternExtension::extensionMustExecute()
{
    if (Path.isTouched() || Count.isTouched() || SpacingMode.isTouched() || Spacing.isTouched()
        || StartOffset.isTouched() || EndOffset.isTouched() || ReversePath.isTouched()
        || Align.isTouched() || VerticalVector.isTouched()) {
        return 1;
    }
    return 0;
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
    VerticalVector.setStatus(App::Property::Hidden, !Align.getValue());
}
