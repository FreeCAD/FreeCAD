// SPDX-License-Identifier: LGPL-2.1-or-later
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <App/Document.h>
#include <Base/Exception.h>
#include "HatchPattern.h"
#include "SketchObject.h"

#ifndef QT_TRANSLATE_NOOP
# define QT_TRANSLATE_NOOP(scope, text) text
#endif

using namespace Sketcher;

namespace
{
// A hatch may not produce more pattern lines or dashes than this, whatever the spacing.
constexpr double maxHatchLines = 10000;
constexpr double maxHatchStrokes = 200000;
// Pattern dots are drawn as short strokes of this fraction of the spacing.
constexpr double hatchDotLength = 0.1;
}  // namespace

const Annotation* SketchObject::findAnnotation(long id) const
{
    const auto& values = Annotations.getValues();
    const auto it = std::find_if(values.begin(), values.end(), [id](const auto& a) {
        return a.id == id;
    });
    return it == values.end() ? nullptr : &*it;
}

const Annotation& SketchObject::getAnnotation(long id) const
{
    const auto* annotation = findAnnotation(id);
    if (!annotation) {
        throw Base::ValueError("Unknown annotation ID");
    }
    return *annotation;
}

long SketchObject::addAnnotation(Annotation a)
{
    a.validate();
    if (a.kind == Annotation::Kind::Hatch) {
        annotationFace(a);
    }
    if (Annotations.highestAllocatedId() >= std::numeric_limits<int>::max() - 1) {
        throw Base::ValueError("Too many annotations");
    }
    long next = std::max(NextAnnotationId.getValue(), Annotations.highestAllocatedId() + 1);
    for (const auto& existing : Annotations.getValues()) {
        if (existing.id >= std::numeric_limits<int>::max() - 1) {
            throw Base::ValueError("Too many annotations");
        }
        next = std::max(next, existing.id + 1);
    }
    if (next <= 0 || next >= std::numeric_limits<int>::max()) {
        throw Base::ValueError("Too many annotations");
    }
    a.id = next;
    if (a.label.empty()) {
        a.label = "Annotation" + std::to_string(next);
    }
    auto values = Annotations.getValues();
    values.push_back(std::move(a));
    NextAnnotationId.setValue(next + 1);
    Annotations.setValues(std::move(values));
    return next;
}

void SketchObject::updateAnnotation(long id, Annotation a)
{
    const auto& old = getAnnotation(id);
    if (a.kind != old.kind) {
        throw Base::ValueError("Annotation type cannot be changed");
    }
    a.id = id;
    a.validate();
    // Existing broken references may be renamed/moved; newly assigned boundaries must be valid.
    if (a.kind == Annotation::Kind::Hatch && old.boundary != a.boundary) {
        annotationFace(a);
    }
    auto values = Annotations.getValues();
    for (auto& item : values) {
        if (item.id == id) {
            item = std::move(a);
            break;
        }
    }
    Annotations.setValues(std::move(values));
}

void SketchObject::delAnnotations(const std::vector<long>& ids)
{
    if (ids.empty()) {
        return;  // Nothing to change, so no undo step either.
    }
    const std::set<long> removed(ids.begin(), ids.end());
    for (long id : removed) {
        getAnnotation(id);  // Throws for an unknown ID, before anything is removed.
    }
    auto values = Annotations.getValues();
    std::erase_if(values, [&](const auto& a) { return removed.contains(a.id); });
    Annotations.setValues(std::move(values));
}

Part::TopoShape SketchObject::annotationFace(const Annotation& a) const
{
    if (a.kind != Annotation::Kind::Hatch) {
        throw Base::ValueError("Annotation is not a hatch");
    }
    constexpr const char* invalidArea
        = QT_TRANSLATE_NOOP("Exceptions", "The selected loops do not enclose a valid hatch area");
    try {
        std::vector<Part::TopoShape> edges;
        for (long stableId : a.boundary) {
            const auto& geometry = Geometry.getValues();
            auto it = std::find_if(geometry.begin(), geometry.end(), [stableId](const auto* g) {
                return GeometryFacade::getId(g) == stableId;
            });
            if (it == geometry.end()) {
                THROWMT(
                    Base::ValueError,
                    QT_TRANSLATE_NOOP("Exceptions", "Hatch boundary geometry has been deleted")
                );
            }
            edges.emplace_back((*it)->toShape());
        }
        const auto wires = Part::TopoShape().makeCompound(edges).makeWires();
        for (TopExp_Explorer ex(wires.getShape(), TopAbs_WIRE); ex.More(); ex.Next()) {
            if (!ex.Current().Closed()) {
                THROWMT(
                    Base::ValueError,
                    QT_TRANSLATE_NOOP("Exceptions", "The hatch boundary is not a closed loop")
                );
            }
        }
        auto face = wires.makeFace(nullptr, "Part::FaceMakerBullseye");
        if (face.isNull() || !face.isValid() || face.countSubShapes(TopAbs_FACE) == 0) {
            THROWMT(Base::ValueError, invalidArea);
        }
        return face;
    }
    catch (const Standard_Failure&) {
        THROWMT(Base::ValueError, invalidArea);
    }
}

namespace
{
/// Leader segments and arrowhead as line pairs, and filled arrowhead parts as triangles.
/// Sizes follow TechDraw's symbols closely enough that a linked page looks the same.
void leaderGeometry(
    const Annotation& a,
    std::vector<Base::Vector3d>* lines,
    std::vector<Base::Vector3d>* fills
)
{
    // Drag, tool and editor previews reach this before validate(), so trust nothing.
    if (a.points.size() < 2) {
        return;
    }
    const auto tip = a.points[0];
    const auto delta = a.points[1] - tip;
    const double length = delta.Length();
    const double size = a.arrowSize;
    const bool arrow = std::isfinite(length) && length >= 1e-9 && std::isfinite(size) && size > 0;
    const auto d = arrow ? delta / length : Base::Vector3d(1, 0, 0);
    const Base::Vector3d n(-d.y, d.x, 0);
    const double halfWidth = size * 0.35;
    const double radius = size * 0.25;
    constexpr int circleSegments = 24;
    auto line = [lines](const Base::Vector3d& p, const Base::Vector3d& q) {
        if (lines) {
            lines->push_back(p);
            lines->push_back(q);
        }
    };
    auto triangle = [fills](const Base::Vector3d& p, const Base::Vector3d& q, const Base::Vector3d& r) {
        if (fills) {
            fills->push_back(p);
            fills->push_back(q);
            fills->push_back(r);
        }
    };
    auto circlePoint = [&](int i) {
        const double angle = 2 * std::acos(-1.0) * i / circleSegments;
        return tip + Base::Vector3d(std::cos(angle), std::sin(angle), 0) * radius;
    };
    // An open circle sits on the point; the leader starts at its rim.
    const bool trimmed = arrow && a.arrowStyle == "Open circle" && length > radius;
    line(trimmed ? tip + d * radius : tip, a.points[1]);
    for (size_t i = 2; i < a.points.size(); ++i) {
        line(a.points[i - 1], a.points[i]);
    }
    if (!arrow) {
        return;  // The segments are still drawable without an arrowhead.
    }
    const auto back = tip + d * size;
    const auto left = back + n * halfWidth;
    const auto right = back - n * halfWidth;
    const auto& style = a.arrowStyle;
    if (style == "Open arrow") {
        line(tip, left);
        line(tip, right);
    }
    else if (style == "Filled arrow") {
        line(tip, left);
        line(left, right);
        line(right, tip);
        triangle(tip, left, right);
    }
    else if (style == "Tick") {
        const auto slant = (d + n) * (size * 0.5 / std::sqrt(2.0));
        line(tip - slant, tip + slant);
    }
    else if (style == "Dot" || style == "Open circle") {
        for (int i = 0; i < circleSegments; ++i) {
            line(circlePoint(i), circlePoint(i + 1));
            if (style == "Dot") {
                triangle(tip, circlePoint(i), circlePoint(i + 1));
            }
        }
    }
    else if (style == "Fork") {
        line(back, tip + n * halfWidth);
        line(back, tip - n * halfWidth);
    }
    else if (style == "Filled triangle") {
        const auto baseLeft = tip + n * halfWidth;
        const auto baseRight = tip - n * halfWidth;
        line(baseLeft, back);
        line(back, baseRight);
        line(baseRight, baseLeft);
        triangle(baseLeft, back, baseRight);
    }
}
}  // namespace

std::vector<Base::Vector3d> SketchObject::annotationFills(const Annotation& a) const
{
    std::vector<Base::Vector3d> result;
    if (a.kind == Annotation::Kind::Leader) {
        leaderGeometry(a, nullptr, &result);
    }
    return result;
}

std::vector<Base::Vector3d> SketchObject::annotationStrokes(const Annotation& a) const
{
    std::vector<Base::Vector3d> result;
    if (a.kind == Annotation::Kind::Leader) {
        leaderGeometry(a, &result, nullptr);
        return result;
    }
    if (a.kind != Annotation::Kind::Hatch) {
        return result;
    }
    const auto face = annotationFace(a);
    // OCC reports failures (degenerate edges far from the origin, boolean errors) as
    // Standard_Failure, which no GUI caller expects.
    try {
        Bnd_Box box;
        BRepBndLib::Add(face.getShape(), box);
        double x0, y0, z0, x1, y1, z1;
        box.Get(x0, y0, z0, x1, y1, z1);
        const double radius = std::hypot(x1 - x0, y1 - y0) + a.spacing;
        const Base::Vector3d center((x0 + x1) * 0.5, (y0 + y1) * 0.5, 0);
        const auto families = placeHatchPattern(a.pattern, a.position, a.rotation, a.spacing);
        auto tooDense = [] {
            THROWMT(
                Base::ValueError,
                QT_TRANSLATE_NOOP("Exceptions", "Hatch spacing produces too many lines; increase spacing")
            );
        };
        struct Family
        {
            Base::Vector3d origin;
            Base::Vector3d direction;
            Base::Vector3d normal;
            Base::Vector3d offset;
            double pitch;
            double period;
            const std::vector<double>* dashes;
            int first;
            int last;
        };
        std::vector<Family> placed;
        double lineCount = 0;
        for (const auto& lines : families) {
            Family f;
            f.direction = lines.direction;
            f.normal = Base::Vector3d(-f.direction.y, f.direction.x, 0);
            f.offset = lines.offset;
            f.pitch = f.offset.Dot(f.normal);
            f.dashes = &lines.dashes;
            f.period = 0;
            for (double dash : lines.dashes) {
                f.period += std::abs(dash);
            }
            if (!std::isfinite(f.pitch) || std::abs(f.pitch) < 1e-12) {
                tooDense();
            }
            // Move the origin to the lattice point nearest the face, across the lines and then
            // along them by whole dash periods, so a far-away pattern position can neither
            // overflow the line numbers nor lose the strokes to rounding.
            const double shift = std::round((center - lines.origin).Dot(f.normal) / f.pitch);
            f.origin = lines.origin + f.offset * shift;
            const double along = (center - f.origin).Dot(f.direction);
            f.origin += f.direction
                * (f.period > 1e-12 ? std::round(along / f.period) * f.period : along);
            const double c = (center - f.origin).Dot(f.normal);
            const double lo = (c - radius) / f.pitch;
            const double hi = (c + radius) / f.pitch;
            const double from = std::ceil(std::min(lo, hi));
            const double to = std::floor(std::max(lo, hi));
            lineCount += std::max(0.0, to - from + 1);
            if (!std::isfinite(lineCount) || lineCount > maxHatchLines) {
                tooDense();
            }
            f.first = static_cast<int>(from);
            f.last = static_cast<int>(to);
            placed.push_back(f);
        }
        const double dotLength = a.spacing * hatchDotLength;
        for (const auto& f : placed) {
            // One boolean per family: a Common per line costs orders of magnitude more, and
            // the family tells each clipped stroke which dash sequence it follows.
            BRep_Builder builder;
            TopoDS_Compound pattern;
            builder.MakeCompound(pattern);
            if (f.first > f.last) {
                continue;
            }
            for (int i = f.first; i <= f.last; ++i) {
                const auto through = f.origin + f.offset * i;
                const double along = (center - through).Dot(f.direction);
                const auto start = through + f.direction * (along - radius);
                const auto end = through + f.direction * (along + radius);
                builder.Add(
                    pattern,
                    BRepBuilderAPI_MakeEdge(gp_Pnt(start.x, start.y, 0), gp_Pnt(end.x, end.y, 0)).Edge()
                );
            }
            TopTools_ListOfShape arguments, tools;
            arguments.Append(pattern);
            tools.Append(face.getShape());
            BRepAlgoAPI_Common common;
            common.SetArguments(arguments);
            common.SetTools(tools);
            common.Build();
            if (!common.IsDone()) {
                THROWMT(
                    Base::ValueError,
                    QT_TRANSLATE_NOOP("Exceptions", "Failed to clip hatch strokes")
                );
            }
            const double period = f.period;
            for (TopExp_Explorer ex(common.Shape(), TopAbs_EDGE); ex.More(); ex.Next()) {
                TopoDS_Vertex v0, v1;
                TopExp::Vertices(TopoDS::Edge(ex.Current()), v0, v1);
                if (v0.IsNull() || v1.IsNull()) {
                    continue;
                }
                const auto p0 = BRep_Tool::Pnt(v0);
                const auto p1 = BRep_Tool::Pnt(v1);
                const Base::Vector3d a0(p0.X(), p0.Y(), 0);
                const Base::Vector3d a1(p1.X(), p1.Y(), 0);
                if (period <= 1e-12) {
                    result.push_back(a0);
                    result.push_back(a1);
                }
                else {
                    // Dashes restart at every lattice point of the line this stroke lies on.
                    const double line = std::round((a0 - f.origin).Dot(f.normal) / f.pitch);
                    const auto through = f.origin + f.offset * line;
                    double t0 = (a0 - through).Dot(f.direction);
                    double t1 = (a1 - through).Dot(f.direction);
                    if (t0 > t1) {
                        std::swap(t0, t1);
                    }
                    auto emit = [&](double u0, double u1) {
                        if (u1 - u0 > 1e-9) {
                            result.push_back(through + f.direction * u0);
                            result.push_back(through + f.direction * u1);
                        }
                    };
                    for (double cycle = std::floor(t0 / period) * period; cycle < t1;
                         cycle += period) {
                        double at = cycle;
                        for (double dash : *f.dashes) {
                            if (dash > 0) {
                                emit(std::max(at, t0), std::min(at + dash, t1));
                            }
                            else if (dash == 0 && at >= t0 && at <= t1) {
                                emit(at, std::min(at + dotLength, t1));
                            }
                            at += std::abs(dash);
                        }
                        if (result.size() > 2 * maxHatchStrokes) {
                            tooDense();
                        }
                    }
                }
            }
        }
    }
    catch (const Standard_Failure&) {
        THROWMT(Base::ValueError, QT_TRANSLATE_NOOP("Exceptions", "Failed to clip hatch strokes"));
    }
    return result;
}
