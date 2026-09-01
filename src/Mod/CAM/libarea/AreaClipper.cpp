// SPDX-License-Identifier: BSD-3-Clause

// AreaClipper.cpp

// implements CArea methods using Angus Johnson's "Clipper"

#include "Area.h"
#include "clipper2/clipper.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <vector>

using namespace heeks;
using namespace Clipper2Lib;

bool CArea::HolesLinked()
{
    return false;
}

double CArea::m_clipper_scale = 10000.0;

static const int min_arc_points = 4;

// Convert between PointD (double) and Point64 (int64) with scaling
static Point64 ToPoint64(const PointD& p)
{
    return Point64(
        (int64_t)(floor(p.x * CArea::m_clipper_scale + 0.5)),
        (int64_t)(floor(p.y * CArea::m_clipper_scale + 0.5)),
        p.z
    );
}

static PointD ToPointD(const Point64& p)
{
    return PointD((double)p.x / CArea::m_clipper_scale, (double)p.y / CArea::m_clipper_scale, p.z);
}


// Helper method for recentering an angle in the 2*PI range next to a reference angle
// type = 1 puts phi CCW of phi_ref; type = -1 puts it CW
// final bounds: result is between phi_ref (exclusive) and (phi_ref + 2*pi * type) (inclusive)
static double recenter(double phi, double phi_ref, int type)
{
    while (phi <= phi_ref && phi < phi_ref + 2 * M_PI * type) {
        phi += 2 * M_PI;
    }
    while (phi >= phi_ref && phi > phi_ref + 2 * M_PI * type) {
        phi -= 2 * M_PI;
    }
    return phi;
};

void CArea::Subtract(const CArea& a2)
{
    Clip(ClipType::Difference, a2);
}

void CArea::Intersect(const CArea& a2)
{
    Clip(ClipType::Intersection, a2);
}

void CArea::Union(const CArea& a2)
{
    Clip(ClipType::Union, a2);
}

void CArea::Xor(const CArea& a2)
{
    Clip(ClipType::Xor, a2);
}

void CArea::PopulateClipper(Clipper64& c, bool as_clip, ConversionMetadata& metadata) const
{
    Paths64 closed_paths;
    Paths64 open_paths;

    for (const CCurve& curve : m_curves) {
        bool is_closed = curve.IsClosed();

        if (!is_closed && as_clip) {
            throw std::logic_error("Open curves cannot be used as clip geometry");
        }

        Path64 p = MakePoly(curve, metadata);

        if (is_closed) {
            closed_paths.push_back(p);
        }
        else {
            open_paths.push_back(p);
        }
    }

    if (as_clip) {
        if (!closed_paths.empty()) {
            c.AddClip(closed_paths);
        }
    }
    else {
        if (!closed_paths.empty()) {
            c.AddSubject(closed_paths);
        }
        if (!open_paths.empty()) {
            c.AddOpenSubject(open_paths);
        }
    }
}

// Internal function to apply a clipping operation with clipper. Results (for edges tagged 1,
// or if there are no tags) are stored in `this`.
//
// op: the boolean clipping operation to perform (Union, Difference, etc)
// this: subject geometry
// clip_area: clipping geometry
// fillType: fill rule applied to determine inside/outside (Positive, EvenOdd, etc)
// reverseOpenPathContents: if true, reverse the point order within each open path result
// reverseOpenPathOrder: if true, reverse the ordering of open path results
// cNeg: if provided, edges tagged -1 (i.e. negative-offset segments from NaiveOffset) are
//       stored in cNeg instead of being dropped. Edges tagged 1always go into `this`. End caps
//       (tagged 0) are always dropped.
void CArea::_Clip(
    ClipType op,
    const CArea& clip_area,
    FillRule fillType,
    bool reverseOpenPathContents,
    bool reverseOpenPathOrder,
    std::optional<std::reference_wrapper<CArea>> cNeg
)
{
    // Initialize a clipper object and populate it with subject/clip geometry
    Clipper64 c;
    ConversionMetadata metadata;
    PopulateClipper(c, false, metadata);
    clip_area.PopulateClipper(c, true, metadata);

    // Set up a callback for clipper to log information about points created during
    // the clipper operation.
    c.SetZCallback([&metadata](
                       const Point64& e1bot,
                       const Point64& e1top,
                       const Point64& e2bot,
                       const Point64& e2top,
                       Point64& pt
                   ) {
        if (pt.x == e1bot.x && pt.y == e1bot.y) {
            pt.z = e1bot.z;
        }
        else if (pt.x == e1top.x && pt.y == e1top.y) {
            pt.z = e1top.z;
        }
        else if (pt.x == e2bot.x && pt.y == e2bot.y) {
            pt.z = e2bot.z;
        }
        else if (pt.x == e2top.x && pt.y == e2top.y) {
            pt.z = e2top.z;
        }
        else if (e1bot.z != 0 || e1top.z != 0 || e2bot.z != 0 || e2top.z != 0) {
            pt.z = metadata.z_next++;
        }

        const int64_t e1min = std::min(e1bot.z, e1top.z);
        const int64_t e1max = std::max(e1bot.z, e1top.z);
        const int64_t e2min = std::min(e2bot.z, e2top.z);
        const int64_t e2max = std::max(e2bot.z, e2top.z);
        metadata.intersections.insert({pt.z, std::make_tuple(e1min, e1max, e2min, e2max)});
    });

    // Execute the operation, potentially producing both closed and open path results
    Paths64 closedPaths, openPaths;
    c.Execute(op, fillType, closedPaths, openPaths);

    // Reverse open path contents if requested
    if (reverseOpenPathContents) {
        for (auto& path : openPaths) {
            std::reverse(path.begin(), path.end());
        }
    }

    // Reverse open path order if requested
    if (reverseOpenPathOrder) {
        std::reverse(openPaths.begin(), openPaths.end());
    }

    m_curves.clear();
    SetFromResult(closedPaths, /*is_closed=*/true, metadata, cNeg);
    SetFromResult(openPaths, /*is_closed=*/false, metadata, cNeg);
}

void CArea::Clip(ClipType op, const CArea& clip_area, FillRule fillType)
{
    _Clip(op, clip_area, fillType);
}

void CArea::ClipperNoop()
{
    ConversionMetadata metadata;
    Paths64 closed_paths;
    Paths64 open_paths;
    for (const CCurve& curve : m_curves) {
        bool is_closed = curve.IsClosed();
        Path64 p = MakePoly(curve, metadata);

        if (is_closed) {
            closed_paths.push_back(p);
        }
        else {
            open_paths.push_back(p);
        }
    }

    m_curves.clear();
    SetFromResult(closed_paths, /*is_closed=*/true, metadata);
    SetFromResult(open_paths, /*is_closed=*/false, metadata);
}

void CArea::Debug_IntersectOpenPathReversal(
    const CArea& clip_area,
    bool reverseOpenPathContents,
    bool reverseOpenPathOrder
)
{
    _Clip(
        ClipType::Intersection,
        clip_area,
        FillRule::EvenOdd,
        reverseOpenPathContents,
        reverseOpenPathOrder
    );
}

// Creates the naive offset of curves by offsetting each segment by +-offset.
//
// The "naive offset" is produced by offsetting each individual segment on its own and
// joining adjacent offset segments to produce a closed curve. Open curves are closed
// with round end caps. The final result of this operation is expected to be self intersecting,
// and should be post-processed with a union operation with positive fill rule.
//
// This function should always be called with positive offset. Also, closed
// input curves should be correctly oriented. These input requirements are
// necessary to ensure that the output has positive winding, so subsequent
// union operations (with positive fill type) behave as expected.
//
// This function operates natively on arcs and line segments/CVertex; no arc approximation/clipper.
//
// The output geometry is written to m_curves, with each curve's m_edgeTags holding one tag per
// edge. A tag of 0 indicates that the segment is an end cap. 1 indicates that it was produced by
// offsetting in the requested/positive direction, and -1 indicates that it was produced by
// offsetting in the opposite/negative direction.
void CArea::NaiveOffset(double offset)
{
    // Positive oriented curves should be CCW (positive area) but some callers use the
    // wrong convention. For backwards compatibility/to support them, we check if the
    // total area is negative and use that as a cue to reverse all closed curves
    if (GetArea() < 0) {
        for (CCurve& curve : m_curves) {
            if (curve.IsClosed()) {
                curve.Reverse();
            }
        }
    }

    std::list<CCurve> offset_curves;

    for (CCurve& curve : m_curves) {
        if (curve.m_vertices.empty()) {
            continue;
        }

        // Handle "curves" of a single point -- positive offset is the circle about that point,
        // and negative offset is empty.
        if (curve.m_vertices.size() == 1) {
            const Point& center = curve.m_vertices.front().m_p;
            const Point right(center.x + offset, center.y);
            const Point left(center.x - offset, center.y);

            // Construct output circle
            CCurve output_curve;
            output_curve.m_vertices.emplace_back(0, right, heeks::Point {0, 0});
            output_curve.m_vertices.emplace_back(1, left, center);
            output_curve.m_vertices.emplace_back(1, right, center);
            for (auto it = std::next(output_curve.m_vertices.begin());
                 it != output_curve.m_vertices.end();
                 ++it) {
                output_curve.m_edgeTags.push_back(1);
            }
            offset_curves.push_back(output_curve);

            continue;
        }

        // Loop over the segments, offsetting and joining
        //
        // cPos is the positive offset; cNeg is the negative offset. Note that both are built
        // forwards, so cNeg will need to be reversed later. End caps will be handled later
        CCurve cPos;
        CCurve cNeg;
        double startDirX = 0, startDirY = 0, startQex = 0;  // initialized below
        double prevDirX = 0, prevDirY = 0;                  // initialized below
        heeks::Point pPrev = curve.m_vertices.front().m_p;
        double enterQ = 0;

        // Utility for joining from the current endpoint to posTarget and negTarget.
        //
        // arcCenter: the un-offset vertex/center of joining arc
        // enterDirX/enterDirY: the tangent direction entering the join
        // exitDirX/exitDirY: the tangent exiting the join
        // enterQ/exitQ: the curvature of the enterance/exit segments
        auto addJoin = [&](const heeks::Point& posTarget,
                           const heeks::Point& negTarget,
                           const heeks::Point& arcCenter,
                           double enterDirX,
                           double enterDirY,
                           double exitDirX,
                           double exitDirY,
                           const double enterQ,
                           const double exitQ) {
            // Skip the join if the points already match in clipper coordinates
            const Point64 posTarget64 = ToPoint64(PointD(posTarget.x, posTarget.y, 0));
            const Point64 negTarget64 = ToPoint64(PointD(negTarget.x, negTarget.y, 0));
            const Point64 posBack64 = ToPoint64(
                PointD(cPos.m_vertices.back().m_p.x, cPos.m_vertices.back().m_p.y, 0)
            );
            const Point64 negBack64 = ToPoint64(
                PointD(cNeg.m_vertices.back().m_p.x, cNeg.m_vertices.back().m_p.y, 0)
            );
            if ((posTarget64.x == posBack64.x && posTarget64.y == posBack64.y)
                || (negTarget64.x == negBack64.x && negTarget64.y == negBack64.y)) {
                // Skip join. I checked if either point is equal in clipper coordinates rather than
                // both because, besides for rounding error, they should agree with each other, and
                // I'm not interested in having single-unit clipper lines anyway. We can round
                // within that range, and the curve data structure ensures connectivity
                return;
            }

            // Determine how the join is done. 0 = no join, 1 = positive arc, -1 = negative arc
            int joinType = 0;

            // Check the angle of the tangents. If not (anti-)parallel, it is easy to choose if the
            // round join belongs to the positive side or the negative side.
            const double cross = enterDirX * exitDirY - enterDirY * exitDirX;
            if (cross > 0) {
                joinType = 1;
            }
            else if (cross < 0) {
                joinType = -1;
            }
            else {
                // Check which it is, parallel or anti-parallel. If parallel, no join is required.
                // In principle parallel tangents should already be filtered out by the check at the
                // top for skipping joins, but I'm not so confident in the numerics that it isn't
                // worth checking.
                const double dot = enterDirX * exitDirX + enterDirY * exitDirY;
                if (dot > 0) {
                    // Parallel --> the ends of the previous/current offset segments already align,
                    // no join required
                    return;
                }
                else {
                    // Anti parallel --> make the decision based on curvature
                    if (enterQ < -exitQ) {
                        joinType = 1;
                    }
                    else {
                        joinType = -1;
                    }
                }
            }

            // Join methodology has been determined; now construct the joining segments
            if (joinType == 1) {
                cPos.m_vertices.emplace_back(1, posTarget, arcCenter);
                cNeg.m_vertices.emplace_back(0, arcCenter, heeks::Point {0, 0});
                cNeg.m_vertices.emplace_back(0, negTarget, heeks::Point {0, 0});
            }
            else if (joinType == -1) {
                cPos.m_vertices.emplace_back(0, arcCenter, heeks::Point {0, 0});
                cPos.m_vertices.emplace_back(0, posTarget, heeks::Point {0, 0});
                cNeg.m_vertices.emplace_back(-1, negTarget, arcCenter);
            }
        };

        for (auto it = std::next(curve.m_vertices.begin()); it != curve.m_vertices.end(); ++it) {
            const CVertex& v = *it;

            // Compute segment start and end normal and tangent directions. Normalize length to offset
            double sTanX, sTanY;
            double sNormX, sNormY;
            double eTanX, eTanY;
            double eNormX, eNormY;
            double radius = 0;  // initialized below
            if (v.m_type == 0) {
                const double dx = v.m_p.x - pPrev.x;
                const double dy = v.m_p.y - pPrev.y;
                const double len = std::hypot(dx, dy);
                if (len == 0) {
                    continue;
                }
                std::tie(sTanX, sTanY) = std::make_pair(dx / len * offset, dy / len * offset);
                std::tie(sNormX, sNormY) = std::make_pair(sTanY, -sTanX);
                std::tie(eTanX, eTanY) = std::make_pair(sTanX, sTanY);
                std::tie(eNormX, eNormY) = std::make_pair(sNormX, sNormY);
            }
            else {
                assert(v.m_type == 1 || v.m_type == -1);

                const double sx = v.m_type * (pPrev.x - v.m_c.x);
                const double sy = v.m_type * (pPrev.y - v.m_c.y);
                const double ex = v.m_type * (v.m_p.x - v.m_c.x);
                const double ey = v.m_type * (v.m_p.y - v.m_c.y);
                radius = std::hypot(sx, sy);
                if (radius == 0) {
                    continue;
                }

                std::tie(sNormX, sNormY) = std::make_pair(sx / radius * offset, sy / radius * offset);
                std::tie(eNormX, eNormY) = std::make_pair(ex / radius * offset, ey / radius * offset);

                std::tie(sTanX, sTanY) = std::make_pair(-sNormY, sNormX);
                std::tie(eTanX, eTanY) = std::make_pair(-eNormY, eNormX);
            }

            // Compute the start and end points of the offset segment
            const heeks::Point pPosS(pPrev.x + sNormX, pPrev.y + sNormY);
            const heeks::Point pNegS(pPrev.x - sNormX, pPrev.y - sNormY);
            const heeks::Point pPosE(v.m_p.x + eNormX, v.m_p.y + eNormY);
            const heeks::Point pNegE(v.m_p.x - eNormX, v.m_p.y - eNormY);

            // If the output curves are empty, intialize the start point and direction
            const bool hasPrev = !cPos.m_vertices.empty();
            double exitQ = v.m_type == 0 ? 0 : v.m_type / radius;
            if (!hasPrev) {
                cPos.m_vertices.emplace_back(0, pPosS, Point(0, 0));
                cNeg.m_vertices.emplace_back(0, pNegS, Point(0, 0));
                std::tie(startDirX, startDirY) = std::make_pair(sTanX, sTanY);
                startQex = exitQ;
            }

            // Join from the previous segment, if there is one
            if (hasPrev) {
                addJoin(pPosS, pNegS, pPrev, prevDirX, prevDirY, sTanX, sTanY, enterQ, exitQ);
            }
            enterQ = v.m_type == 0 ? 0 : v.m_type / radius;

            // Generate the positive and negative offset segments connecting pPosS to pPosE and
            // pNegS to pNegE
            if (v.m_type == 0) {
                cPos.m_vertices.emplace_back(0, pPosE, heeks::Point {0, 0});
                cNeg.m_vertices.emplace_back(0, pNegE, heeks::Point {0, 0});
            }
            else {
                // Check if the offset causes the arc to collapse; if so, generate a straight line
                // instead of an arc because it will be optimized out anyway in a union operation later
                assert(v.m_type == 1 || v.m_type == -1);
                const bool posCollapse = radius + (offset * v.m_type) <= 0;
                const bool negCollapse = radius - (offset * v.m_type) <= 0;

                cPos.m_vertices.emplace_back(posCollapse ? 0 : v.m_type, pPosE, v.m_c);
                cNeg.m_vertices.emplace_back(negCollapse ? 0 : v.m_type, pNegE, v.m_c);
            }

            // Update state variables
            pPrev = v.m_p;
            std::tie(prevDirX, prevDirY) = std::make_pair(eTanX, eTanY);
        }

        // Post processing
        if (curve.IsClosed()) {
            // Add the final join back to the start
            const heeks::Point pPosStart = cPos.m_vertices.front().m_p;
            const heeks::Point pNegStart = cNeg.m_vertices.front().m_p;
            addJoin(pPosStart, pNegStart, pPrev, prevDirX, prevDirY, startDirX, startDirY, enterQ, startQex);

            // Reverse the negative path so together cPos and cNeg enclose the area within `offset`
            // of the original curve
            cNeg.Reverse();

            // Save positive offset to the output list
            for (auto it = std::next(cPos.m_vertices.begin()); it != cPos.m_vertices.end(); ++it) {
                cPos.m_edgeTags.push_back(1);
            }
            offset_curves.push_back(cPos);

            // Save the negative offset to the output list
            for (auto it = std::next(cNeg.m_vertices.begin()); it != cNeg.m_vertices.end(); ++it) {
                cNeg.m_edgeTags.push_back(-1);
            }
            offset_curves.push_back(cNeg);
        }
        else {
            // Reverse cNeg so it's correctly oriented to join with cPos and the end caps
            cNeg.Reverse();

            // Tag positive offset edges
            while (cPos.m_edgeTags.size() < cPos.m_vertices.size() - 1) {
                cPos.m_edgeTags.push_back(1);
            }

            // Append the first end cap
            cPos.m_vertices.emplace_back(1, cNeg.m_vertices.begin()->m_p, curve.m_vertices.back().m_p);
            cPos.m_edgeTags.push_back(0);

            // Concatenate cNeg
            for (auto it = std::next(cNeg.m_vertices.begin()); it != cNeg.m_vertices.end(); ++it) {
                cPos.m_vertices.push_back(*it);
                cPos.m_edgeTags.push_back(-1);
            }

            // Append the final end cap
            cPos.m_vertices.emplace_back(1, cPos.m_vertices.begin()->m_p, curve.m_vertices.front().m_p);
            cPos.m_edgeTags.push_back(0);

            // Save results to the output list
            offset_curves.push_back(cPos);
        }
    }

    m_curves = std::move(offset_curves);
}

// Convert the input CCurve to clipper, populating metadata.
//
// Edge tags are read from curve.m_edgeTags. If that list is empty, all edges
// are treated as if they were tagged 1 (positive offset edge).
Path64 CArea::MakePoly(const CCurve& curve, ConversionMetadata& metadata) const
{
    if (!curve.m_vertices.size()) {
        return {};
    }

    Path64 result;
    const int curveIndex = metadata.nextCurveIndex++;

    // Helper function to convert to clipper units, handling z caching
    auto getPoint64 = [&](double x, double y) -> Point64 {
        const Point64 p64 = ToPoint64(PointD(x, y, 0));
        const auto key = std::make_pair(p64.x, p64.y);
        auto it = metadata.xy_to_z.find(key);
        if (it != metadata.xy_to_z.end()) {
            return Point64(p64.x, p64.y, it->second);
        }
        const int64_t z = metadata.z_next++;
        metadata.xy_to_z[key] = z;
        return Point64(p64.x, p64.y, z);
    };

    // Init the start of the curve
    Point64 pPrev = getPoint64(curve.m_vertices.front().m_p.x, curve.m_vertices.front().m_p.y);
    result.push_back(pPrev);
    heeks::Point ptPrev = curve.m_vertices.front().m_p;
    assert(curve.m_edgeTags.empty() || curve.m_edgeTags.size() == curve.m_vertices.size() - 1);
    auto tagIt = curve.m_edgeTags.cbegin();
    int vertexIndex = 0;

    // Iterate through edges
    for (auto vIt = std::next(curve.m_vertices.cbegin()); vIt != curve.m_vertices.cend(); vIt++) {
        const CVertex& vertex = *vIt;
        const bool isLoop = std::next(vIt) == curve.m_vertices.end() && curve.IsClosed();
        const int edgeTag = tagIt != curve.m_edgeTags.cend() ? *tagIt : 1;

        if (vertex.m_type == 0) {
            // The current edge is a line segment; add a single point to clipper

            Point64 newPt = getPoint64(vertex.m_p.x, vertex.m_p.y);
            if (!isLoop) {
                // Clipper paths are implicitly closed, so only explicitly add non-loop edges
                result.push_back(newPt);
            }

            // Save metadata for the new segment
            const auto key = std::make_pair(std::min(pPrev.z, newPt.z), std::max(pPrev.z, newPt.z));
            metadata.edgeData[key] = SegmentData {vertex, edgeTag, curveIndex, vertexIndex};
            pPrev = newPt;
        }
        else if (vertex.m_p.x != ptPrev.x || vertex.m_p.y != ptPrev.y) {
            // The current edge is an arc; interpolate many lines in clipper
            assert(vertex.m_type == 1 || vertex.m_type == -1);

            // Compute start and end angles
            const double phi0 = atan2(ptPrev.y - vertex.m_c.y, ptPrev.x - vertex.m_c.x);
            double phi1 = atan2(vertex.m_p.y - vertex.m_c.y, vertex.m_p.x - vertex.m_c.x);

            if (vertex.m_type == -1 && phi1 > phi0) {
                // fix to make it clockwise
                phi1 -= 2 * M_PI;
            }
            else if (vertex.m_type == 1 && phi1 < phi0) {
                // fix to make it counterclockwise
                phi1 += 2 * M_PI;
            }

            // Compute the maximum angular step to achieve the required accuracy
            const double dx = ptPrev.x - vertex.m_c.x;
            const double dy = ptPrev.y - vertex.m_c.y;
            const double radius = sqrt(dx * dx + dy * dy);
            const double max_dphi = 2 * acos((radius - CArea::m_accuracy) / radius);

            // Determine the number of segments
            const int num_segments
                = std::max(min_arc_points, (int)ceil(std::abs(phi1 - phi0) / max_dphi));
            const double dphi = (phi1 - phi0) / num_segments;

            // Generate arc points
            for (int i = 1; i <= num_segments; i++) {
                Point64 newPt;
                if (i == num_segments) {
                    // Final segment is special; use the specified endpoint instead of recomputing it
                    newPt = getPoint64(vertex.m_p.x, vertex.m_p.y);
                    if (newPt == pPrev) {
                        continue;
                    }
                    if (!isLoop) {
                        // Clipper paths are implicitly closed, so only explicitly add non-loop edges
                        result.push_back(newPt);
                    }
                }
                else {
                    // Compute the interpoalted point
                    const double px = vertex.m_c.x + radius * cos(phi0 + dphi * i);
                    const double py = vertex.m_c.y + radius * sin(phi0 + dphi * i);
                    newPt = getPoint64(px, py);
                    if (newPt == pPrev) {
                        continue;
                    }
                    result.push_back(newPt);
                }

                const auto key = std::make_pair(std::min(pPrev.z, newPt.z), std::max(pPrev.z, newPt.z));
                metadata.edgeData[key] = SegmentData {vertex, edgeTag, curveIndex, vertexIndex};
                pPrev = newPt;
            }
        }

        ptPrev = vertex.m_p;
        vertexIndex++;
        if (tagIt != curve.m_edgeTags.cend()) {
            tagIt++;
        }
    }

    return result;
}


// Convert the provided clipper paths back to CArea/CCurve data, using metadata to correctly
// infer edge type (arc/line) and arc center information. Only edges tagged 1 (i.e. positive offset
// segments from NaiveOffset) are kept in `this` CArea. If cNeg is provided, edges tagged -1 are
// kept there. Edges tagged 0 (end caps from NaiveOffset) are always dropped.
//
// Parameter isClosed specifies if the clipper paths represent open or closed curves. If the curves
// are open, they will be reoriented/ordered using metadata to preserve the original ordering and
// orientation.
void CArea::SetFromResult(
    Paths64& paths,
    bool isClosed,
    ConversionMetadata& metadata,
    std::optional<std::reference_wrapper<CArea>> cNeg
)
{
    // Reorder/reorient open paths
    if (!isClosed) {
        ReorderOpenPaths(paths, metadata);
    }

    // Convert each path back to a CCurve
    for (const Path64& path : paths) {
        if (!path.size()) {
            continue;
        }

        // Preserve single vertex paths. This requires special handling because the code below
        // expects/processes edges, not vertices
        if (path.size() == 1) {
            const PointD pt = ToPointD(path[0]);
            CCurve c;
            c.m_vertices.emplace_back(heeks::Point {pt.x, pt.y});
            m_curves.push_back(c);
            continue;
        }

        // Initialize state variables: the current curve and its tag, and (for final joining of
        // closed curves) the first curve and its tag.
        CCurve c;
        int tag = 0;
        CCurve* firstCurve = nullptr;
        std::optional<int> firstTag;

        // Helper function to save the current curve to the appropriate CArea when done with it,
        // and update firstTag/firstCurve variables
        auto saveCurve = [&]() {
            if (!c.m_vertices.empty()) {
                CCurve* added = nullptr;

                if (tag == 1) {
                    m_curves.push_back(c);
                    added = &m_curves.back();
                }
                else if (tag == -1 && cNeg) {
                    cNeg->get().m_curves.push_back(c);
                    added = &cNeg->get().m_curves.back();
                }

                if (!firstTag) {
                    firstTag = tag;
                    firstCurve = added;
                }
            }
        };

        // For closed paths, start at the smallest z-value
        size_t startVertex = 0;
        if (isClosed) {
            for (size_t i = startVertex + 1; i < path.size(); i++) {
                if (path[i].z < path[startVertex].z) {
                    startVertex = i;
                }
            }
        }

        // Loop through clipper edges, converting to CVertex and building up the current CCurve
        for (size_t edgeNum = 0; edgeNum < (isClosed ? path.size() : path.size() - 1); edgeNum++) {
            // Current edge
            const size_t iEdge = (startVertex + edgeNum) % path.size();
            const Point64& v0 = path[iEdge];
            const Point64& v1 = path[(iEdge + 1) % path.size()];

            // Parent edge (either the same edge, or the edge that was shortened to create this edge)
            const auto parentEdge = getParentEdge(v0, v1, metadata);
            const SegmentData& parentData = metadata.edgeData.find(parentEdge)->second;

            // Check if the tag changed. If it did, end the curve and start a new one
            if (parentData.edgeTag != tag) {
                saveCurve();
                c.m_vertices.clear();
            }
            tag = parentData.edgeTag;

            // If the curve is empty, initialize it with the start point
            if (c.m_vertices.empty()) {
                const PointD start = ToPointD(v0);
                c.m_vertices.emplace_back(heeks::Point {start.x, start.y});
            }

            // Construct the edge to be added based on the end point and the parent's type
            const PointD end = ToPointD(v1);
            CVertex edge(parentData.orig.m_type, {end.x, end.y}, parentData.orig.m_c);
            if (!CArea::m_fit_arcs) {
                edge.m_type = 0;
                edge.m_c = {0, 0};
            }
            CVertex& prev = c.m_vertices.back();

            // Determine if the edge is reversed from the parent (arc), and update type accordingly
            if (edge.m_type == 1 || edge.m_type == -1) {
                const Point64 mc64 = ToPoint64(PointD(parentData.orig.m_c.x, parentData.orig.m_c.y, 0));
                const double phi1 = atan2(v0.y - mc64.y, v0.x - mc64.x);
                const double phi2 = recenter(atan2(v1.y - mc64.y, v1.x - mc64.x), phi1 - M_PI, 1);
                if (phi2 * edge.m_type < phi1 * edge.m_type) {
                    edge.m_type = -edge.m_type;
                }
            }

            // Check if the edge is a continuation of an existing arc
            if (edge.m_type != 0 && edge.m_type == prev.m_type && edge.m_c == prev.m_c) {
                // It is. If the edge does not complete a circle, we should extend the existing
                // CVertex instead of adding a new one.
                const bool fullLoop = std::prev(c.m_vertices.end(), 2)->m_p == edge.m_p;
                if (!fullLoop) {
                    prev.m_p = edge.m_p;
                }
                else {
                    // The edge cannot be extended, because it would complete a circle and CVertex
                    // arcs are supposed to be less than a full circle. Instead, represent the full
                    // circle as 2 semi circles
                    const heeks::Point mid {2 * edge.m_c.x - edge.m_p.x, 2 * edge.m_c.y - edge.m_p.y};
                    prev.m_p = mid;
                    c.m_vertices.push_back(edge);
                }
            }
            else {
                // The edge is not an extension of the previous CVertex; just add it
                c.m_vertices.push_back(edge);
            }
        }

        // Save the final curve
        if (isClosed && firstCurve && firstTag && tag == *firstTag) {
            // Save the curve by joining it with the (distinct!) first curve

            // Remove the first curve's (now redundant) start point
            firstCurve->m_vertices.pop_front();

            // Check if the first CVertex of the first curve is an extension of the last CVertex of
            // the current curve, and if so deduplicate them
            CVertex& edge = firstCurve->m_vertices.front();
            CVertex& prev = c.m_vertices.back();
            if (edge.m_type != 0 && edge.m_type == prev.m_type && edge.m_c == prev.m_c) {
                // It is an extension
                const bool fullLoop = std::prev(c.m_vertices.end(), 2)->m_p == edge.m_p;
                if (!fullLoop) {
                    prev.m_p = edge.m_p;
                    firstCurve->m_vertices.pop_front();
                }
                else {
                    // Full circle; represent it as 2 semi circles
                    const heeks::Point mid {2 * edge.m_c.x - edge.m_p.x, 2 * edge.m_c.y - edge.m_p.y};
                    prev.m_p = mid;
                }
            }

            // ...and finally concatenate them
            firstCurve->m_vertices
                .insert(firstCurve->m_vertices.begin(), c.m_vertices.begin(), c.m_vertices.end());
        }
        else if (!firstTag && isClosed && c.m_vertices.size() >= 3) {
            // Same as above, but the first curve has not been saved yet because the current curve
            // *is* the first curve. Merging the curve to itself requires some special handling

            // First check if the first CVertex of the curve can extend the last CVertex
            CVertex& first = *std::next(c.m_vertices.begin());
            CVertex& last = c.m_vertices.back();
            if (last.m_type != 0 && last.m_type == first.m_type && last.m_c == first.m_c) {
                // It is an extension
                const bool fullLoop = std::prev(c.m_vertices.end(), 2)->m_p == first.m_p;
                if (!fullLoop) {
                    c.m_vertices.front().m_p = std::prev(c.m_vertices.end(), 2)->m_p;
                    c.m_vertices.pop_back();
                }
                else {
                    // Break it into 2 semi circles
                    const heeks::Point mid {
                        2 * first.m_c.x - first.m_p.x,
                        2 * first.m_c.y - first.m_p.y
                    };
                    last.m_p = mid;
                    c.m_vertices.front().m_p = mid;
                }
            }

            // Save it as a new curve
            saveCurve();
        }
        else {
            // Save it as a new curve
            saveCurve();
        }
    }
}


void CArea::Offset(double offset)
{
    if (offset == 0) {
        return;
    }

    // Perform the naive offset, offsetting each edge and joining
    NaiveOffset(std::abs(offset));

    // If we want to keep the negative edges, flip all the edge labels
    if (offset < 0) {
        for (CCurve& curve : m_curves) {
            for (int& tag : curve.m_edgeTags) {
                tag = -tag;
            }
        }
    }

    // Union (fill rule positive), keeping positive edges and dropping negative edges
    _Clip(ClipType::Union, CArea {}, FillRule::Positive);

    // Note that this code currently has no impact because we call Reorder afterwards, but
    // (to be vetted in a future PR) I think the curves from the previous step have known
    // orientation and this simpler/lighter loop should replace the Reorder call
    //
    // // If negative offset, reverse the curves to put them in the forward direction
    // if (offset < 0) {
    //     for (CCurve& c : m_curves) {
    //         c.Reverse();
    //     }
    // }

    // I'm preserving this Reorder() call to preserve old behavior, but imo this should not be part
    // of Offset's spec
    this->Reorder();
}

CArea CArea::OpenOffset(double offset)
{
    CArea cNeg;
    if (offset == 0) {
        return cNeg;
    }

    // Perform the naive offset, offsetting each edge and joining
    NaiveOffset(std::abs(offset));

    // Union (fill rule positive), separating out the positive and negative edges
    _Clip(ClipType::Union, CArea {}, FillRule::Positive, false, false, std::ref(cNeg));

    // The negative curves are oriented in reverse (to make the union operation work) but we
    // actually want them oriented forwards when we return. Reverse them.
    for (CCurve& c : cNeg.m_curves) {
        c.Reverse();
    }

    // If the offset was supposed to be in the negative direction, swap the negative and positive results
    if (offset < 0) {
        std::swap(m_curves, cNeg.m_curves);
    }
    return cNeg;
}

void CArea::Thicken(double value)
{
    // Perform the naive offset, offsetting each edge and joining
    NaiveOffset(std::abs(value));

    // We want to keep all offset curves, so clear the edge tags
    for (CCurve& curve : m_curves) {
        curve.m_edgeTags.clear();
    }

    // Union (fill rule positive), keeping positive edges and dropping negative edges
    _Clip(ClipType::Union, CArea {}, FillRule::Positive);
}


// Return the parent of the provided edge, specified as (zMin, zMax) of its endpoints
std::pair<int64_t, int64_t> CArea::getParentEdge(
    const Point64& p1,
    const Point64& p2,
    const ConversionMetadata& metadata
)
{
    // Check for a direct edge p1.z to p2.z
    std::pair<int64_t, int64_t> testEdge = {std::min(p1.z, p2.z), std::max(p1.z, p2.z)};
    if (metadata.edgeData.count(testEdge)) {
        return testEdge;
    }

    // Check for an edge from p1.z to the intersection log of p2,
    // or from p2.z to the intersection log of z1
    auto z1its = metadata.intersections.equal_range(p1.z);
    for (auto z1it = z1its.first; z1it != z1its.second; z1it++) {
        const auto& [e1min, e1max, e2min, e2max] = z1it->second;
        if (p2.z == e1min || p2.z == e1max) {
            testEdge = {e1min, e1max};
            if (metadata.edgeData.count(testEdge)) {
                return testEdge;
            }
        }
        if (p2.z == e2min || p2.z == e2max) {
            testEdge = {e2min, e2max};
            if (metadata.edgeData.count(testEdge)) {
                return testEdge;
            }
        }
    }

    auto z2its = metadata.intersections.equal_range(p2.z);
    for (auto z2it = z2its.first; z2it != z2its.second; z2it++) {
        const auto& [e1min, e1max, e2min, e2max] = z2it->second;
        if (p1.z == e1min || p1.z == e1max) {
            testEdge = {e1min, e1max};
            if (metadata.edgeData.count(testEdge)) {
                return testEdge;
            }
        }
        if (p1.z == e2min || p1.z == e2max) {
            testEdge = {e2min, e2max};
            if (metadata.edgeData.count(testEdge)) {
                return testEdge;
            }
        }
    }

    // Check for any shared edge in the intersection logs of p1 and p2
    for (auto z1it = z1its.first; z1it != z1its.second; z1it++) {
        const auto& [e1min, e1max, e2min, e2max] = z1it->second;
        for (auto z2it = z2its.first; z2it != z2its.second; z2it++) {
            const auto& [e3min, e3max, e4min, e4max] = z2it->second;
            if ((e1min == e3min && e1max == e3max) || (e1min == e4min && e1max == e4max)) {
                testEdge = {e1min, e1max};
                if (metadata.edgeData.count(testEdge)) {
                    return testEdge;
                }
            }
            if ((e2min == e3min && e2max == e3max) || (e2min == e4min && e2max == e4max)) {
                testEdge = {e2min, e2max};
                if (metadata.edgeData.count(testEdge)) {
                    return testEdge;
                }
            }
        }
    }

    // Failed to find the parent edge. This should not happen; the parent edge should always exist.
    throw std::logic_error(
        "No parent edge found for z=(" + std::to_string(p1.z) + "," + std::to_string(p2.z) + ")"
        + " hits=(" + std::to_string(metadata.intersections.count(p1.z)) + ","
        + std::to_string(metadata.intersections.count(p2.z)) + ")"
    );
}

// For open paths, reorder as needed to produce positively oriented and positively ordered paths
void CArea::ReorderOpenPaths(Paths64& paths, const ConversionMetadata& metadata)
{
    std::vector<std::tuple<int, int, double>> pathOrder;  // max (curveIndex, vertexIndex, progress)
                                                          // across edges
    pathOrder.reserve(paths.size());

    for (Path64& path : paths) {
        pathOrder.push_back({-1, 0, 0.0});
        if (path.empty()) {
            continue;
        }

        bool needsReversal = false;

        for (size_t i = 0; i + 1 < path.size(); i++) {
            const Point64& p1 = path[i];
            const Point64& p2 = path[i + 1];

            // Look up parent edge metadata
            const auto parentEdge = getParentEdge(p1, p2, metadata);
            const auto it = metadata.edgeData.find(parentEdge);
            if (it == metadata.edgeData.end()) {
                // This should not happen; there should always be edgeData for parent edges.
                throw std::logic_error(
                    "ReorderOpenPaths: no edgeData for parentEdge ("
                    + std::to_string(parentEdge.first) + "," + std::to_string(parentEdge.second) + ")"
                );
            }
            const SegmentData& seg = it->second;

            // Convert seg endpoint/center to Point64 for consistent units
            const Point64 mp64 = ToPoint64(PointD(seg.orig.m_p.x, seg.orig.m_p.y, 0));
            const Point64 mc64 = ToPoint64(PointD(seg.orig.m_c.x, seg.orig.m_c.y, 0));

            // Check if the current edge points forwards or backwards on the parent edge
            if (seg.orig.m_type == 0) {
                // For lines, compare Euclidean distance to the parent line's end point
                const double d1 = std::hypot(p1.x - mp64.x, p1.y - mp64.y);
                const double d2 = std::hypot(p2.x - mp64.x, p2.y - mp64.y);
                needsReversal = d1 < d2;
                const double progress = std::max(-d1, -d2);
                pathOrder.back()
                    = std::max(pathOrder.back(), {seg.curveIndex, seg.vertexIndex, progress});
            }
            else {
                assert(seg.orig.m_type == 1 || seg.orig.m_type == -1);
                // For arcs, use angular distance. Clipper segments representing lines are
                // always small angles, so center phi1 and phi2 together
                double phi1 = atan2(p1.y - mc64.y, p1.x - mc64.x);
                double phi2 = recenter(atan2(p2.y - mc64.y, p2.x - mc64.x), phi1 - M_PI, 1);
                needsReversal = phi2 * seg.orig.m_type < phi1 * seg.orig.m_type;

                // Then recenter them colectively relative to phi_end
                const double phi_end = atan2(mp64.y - mc64.y, mp64.x - mc64.x);
                while ((phi1 + phi2) / 2 * seg.orig.m_type > phi_end * seg.orig.m_type) {
                    phi1 -= 2 * M_PI * seg.orig.m_type;
                    phi2 -= 2 * M_PI * seg.orig.m_type;
                }
                while ((phi1 + phi2) / 2 * seg.orig.m_type + 2 * M_PI < phi_end * seg.orig.m_type) {
                    phi1 += 2 * M_PI * seg.orig.m_type;
                    phi2 += 2 * M_PI * seg.orig.m_type;
                }

                const double progress = std::max(-std::abs(phi_end - phi1), -std::abs(phi_end - phi2));
                pathOrder.back()
                    = std::max(pathOrder.back(), {seg.curveIndex, seg.vertexIndex, progress});
            }
        }

        if (needsReversal) {
            std::reverse(path.begin(), path.end());
        }
    }

    // Now put the paths in order. Do the sorting in a std::vector copy and then copy them back
    std::vector<std::pair<std::tuple<int, int, double>, Path64>> vpaths;
    vpaths.reserve(paths.size());

    for (size_t i = 0; i < paths.size(); i++) {
        vpaths.emplace_back(pathOrder[i], std::move(paths[i]));
    }

    std::sort(vpaths.begin(), vpaths.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    paths.clear();
    for (auto& [key, path] : vpaths) {
        paths.push_back(std::move(path));
    }
}
