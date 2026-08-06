// SPDX-License-Identifier: BSD-3-Clause

// AreaClipper.cpp

// implements CArea methods using Angus Johnson's "Clipper"

#include "Area.h"
#include "clipper2/clipper.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
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
double recenter(double phi, double phi_ref, int type)
{
    while (phi <= phi_ref && phi < phi_ref + 2 * PI * type) {
        phi += 2 * M_PI;
    }
    while (phi >= phi_ref && phi > phi_ref + 2 * PI * type) {
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

void CArea::OffsetInward(double inwards_value)
{
    Offset(-inwards_value);
}

void CArea::PopulateClipper(Clipper64& c, bool as_clip, ConversionMetadata& metadata) const
{
    Paths64 closed_paths;
    Paths64 open_paths;
    int skipped = 0;

    auto tagIt = m_edgeTags.cbegin();
    for (const CCurve& curve : m_curves) {
        const std::optional<std::list<int>> tags = m_edgeTags.empty() ? std::nullopt
                                                                      : std::make_optional(*tagIt++);
        bool is_closed = curve.IsClosed();

        if (!is_closed && as_clip) {
            ++skipped;
            continue;
        }

        Path64 p = MakePoly(curve, metadata, tags.value_or(std::list<int> {}));

        if (is_closed) {
            closed_paths.push_back(p);
        }
        else {
            open_paths.push_back(p);
        }
    }

    if (skipped) {
        std::cerr << "libarea: warning skipped " << skipped << " open wires" << std::endl;
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
        std::cerr << "[intersection] pt=(" << pt.x << "," << pt.y << "," << pt.z << ")"
                  << " e1bot=(" << e1bot.x << "," << e1bot.y << "," << e1bot.z << ")"
                  << " e1top=(" << e1top.x << "," << e1top.y << "," << e1top.z << ")"
                  << " e2bot=(" << e2bot.x << "," << e2bot.y << "," << e2bot.z << ")"
                  << " e2top=(" << e2top.x << "," << e2top.y << "," << e2top.z << ")" << "\n";
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
    m_edgeTags = {};
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

void CArea::TestIntersectOpenPathReversal(
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
// The output geometry is written to m_curves, and tags are written to m_edgeTags with one tag per
// edge of each curve, matching the layout of m_curves. A tag of 0 indicates that the segment is an
// end cap. 1 indicates that it was produced by offsetting in the requested/positive direction, and
// -1 indicates that it was produced by offsetting in the opposite/negative direction.
void CArea::NaiveOffset(double offset, double arcTolerance)
{
    {
        std::cerr << "\n\n[NaiveOffset] entry: offset=" << offset
                  << ", m_curves.size()=" << m_curves.size() << ", m_vertices.size()=[";
        bool first = true;
        for (const CCurve& c : m_curves) {
            if (!first) {
                std::cerr << ", ";
            }
            std::cerr << c.m_vertices.size();
            first = false;
        }
        std::cerr << "]\n\n";
    }

    // Positive oriented curves should be CCW (positive area) but some callers use the
    // wrong convention. For backwards compatibility/to support them, we check if the
    // total area is negative and use that as a cue to reverse all closed curves
    if (GetArea() < 0) {
        std::cerr << "[NaiveOffset] area=" << GetArea() << " < 0, reversing all curves\n";
        for (CCurve& curve : m_curves) {
            if (curve.IsClosed()) {
                curve.Reverse();
            }
        }
    }

    std::list<CCurve> offset_curves;
    m_edgeTags = {};

    int curveIdx = 0;
    for (CCurve& curve : m_curves) {
        if (curve.m_vertices.empty()) {
            continue;
        }

        {
            std::cerr << "[NaiveOffset] curve[" << curveIdx++ << "]: area=" << curve.GetArea()
                      << "\n";
            for (const CVertex& v : curve.m_vertices) {
                std::cerr << "  type=" << v.m_type << " p=(" << v.m_p.x << "," << v.m_p.y << ")"
                          << " c=(" << v.m_c.x << "," << v.m_c.y << ")\n";
            }
            std::cerr << "\n";
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
            std::cerr << "[NaiveOffset] single-point start: output_curve += type=0 p=(" << right.x
                      << "," << right.y << ")\n";
            output_curve.m_vertices.emplace_back(1, left, center);
            std::cerr << "[NaiveOffset] single-point upper arc: output_curve += type=1 p=("
                      << left.x << "," << left.y << ") c=(" << center.x << "," << center.y << ")\n";
            output_curve.m_vertices.emplace_back(1, right, center);
            std::cerr << "[NaiveOffset] single-point lower arc: output_curve += type=1 p=("
                      << right.x << "," << right.y << ") c=(" << center.x << "," << center.y
                      << ")\n";
            std::cerr << "\n[NaiveOffset] saving single-point output_curve to offset_curves ("
                      << output_curve.m_vertices.size() << " vertices)\n\n";
            offset_curves.push_back(output_curve);

            // Construct direction labels
            std::list<int> directions;
            for (auto it = std::next(output_curve.m_vertices.begin());
                 it != output_curve.m_vertices.end();
                 ++it) {
                directions.push_back(1);
            }
            m_edgeTags.push_back(directions);

            continue;
        }

        // Loop over the segments, offsetting and joining
        double startDirX, startDirY, startQex;
        double prevDirX, prevDirY;
        heeks::Point pPrev = curve.m_vertices.front().m_p;
        double enterQ = 0;

        // cPos is the positive offset; cNeg is the negative offset. Note that both are built
        // forwards, so cNeg will need to be reversed later. End caps will be handled later
        CCurve cPos;
        CCurve cNeg;

        auto dbgPair = [](const char* ctx,
                          int posType,
                          const heeks::Point& posP,
                          const heeks::Point& posC,
                          int negType,
                          const heeks::Point& negP,
                          const heeks::Point& negC) {
            auto& s = std::cerr;
            const Point64 posP64 = ToPoint64(PointD(posP.x, posP.y, 0));
            const Point64 posC64 = ToPoint64(PointD(posC.x, posC.y, 0));
            const Point64 negP64 = ToPoint64(PointD(negP.x, negP.y, 0));
            const Point64 negC64 = ToPoint64(PointD(negC.x, negC.y, 0));
            s << "[NaiveOffset] " << std::left << std::setw(20) << ctx << std::right
              << ": cPos+=type=" << std::setw(2) << posType << " p=(" << std::setw(12) << posP64.x
              << "," << std::setw(12) << posP64.y << ")"
              << " c=(" << std::setw(12) << posC64.x << "," << std::setw(12) << posC64.y << ")"
              << " | cNeg+=type=" << std::setw(2) << negType << " p=(" << std::setw(12) << negP64.x
              << "," << std::setw(12) << negP64.y << ")"
              << " c=(" << std::setw(12) << negC64.x << "," << std::setw(12) << negC64.y << ")\n";
        };
        auto dbgOne = [](const char* ctx,
                         const char* curveName,
                         int type,
                         const heeks::Point& p,
                         const heeks::Point& c) {
            auto& s = std::cerr;
            const Point64 p64 = ToPoint64(PointD(p.x, p.y, 0));
            const Point64 c64 = ToPoint64(PointD(c.x, c.y, 0));
            s << "[NaiveOffset] " << std::left << std::setw(20) << ctx << std::right << ": "
              << curveName << " += type=" << std::setw(2) << type << " p=(" << std::setw(12)
              << p64.x << "," << std::setw(12) << p64.y << ")"
              << " c=(" << std::setw(12) << c64.x << "," << std::setw(12) << c64.y << ")\n";
        };

        // Append a join from the current endpoint to posTarget/negTarget, using arcCenter as the
        // arc centerDir. enterDirX/enterDirY is the tangent entering the join; exitDirX/exitDirY is
        // the tangent exiting the join. enterQ and exitQ are the curvatures of the enter/exit
        // segments
        auto addJoin = [&](double enterDirX,
                           double enterDirY,
                           double exitDirX,
                           double exitDirY,
                           const heeks::Point& posTarget,
                           const heeks::Point& negTarget,
                           const heeks::Point& arcCenter,
                           const double enterQ,
                           const double exitQ,
                           const heeks::Point& pOrig) {
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
                std::cerr << "[NaiveOffset] join(skip): targets match current endpoints in Point64 "
                             "space\n";
                return;
            }

            // Determine how the join is done. 0 = no join, 1 = positive arc, -1 = negative arc
            int joinType = 0;

            // Check the angle of the tangents. If not (anti-)parallel, it is easy to choose if the
            // round join belongs to the positive side or the negative side.
            const double cross = enterDirX * exitDirY - enterDirY * exitDirX;
            if (cross > 0) {
                joinType = 1;
                dbgPair("join(pArc/nLine)", 1, posTarget, arcCenter, 0, negTarget, heeks::Point {0, 0});
            }
            else if (cross < 0) {
                joinType = -1;
                dbgPair("join(pLine/nArc)", 0, posTarget, heeks::Point {0, 0}, -1, negTarget, arcCenter);
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
                    std::cerr << "[NaiveOffset] join(parallel): no join needed\n";
                    return;
                }
                else {
                    // Anti parallel --> make the decision based on curvature
                    if (enterQ < -exitQ) {
                        joinType = 1;
                        dbgPair(
                            "join(Q:pArc/nLine)",
                            1,
                            posTarget,
                            arcCenter,
                            0,
                            negTarget,
                            heeks::Point {0, 0}
                        );
                    }
                    else {
                        joinType = -1;
                        dbgPair(
                            "join(Q:pLine/nArc)",
                            0,
                            posTarget,
                            heeks::Point {0, 0},
                            -1,
                            negTarget,
                            arcCenter
                        );
                    }
                }
            }

            // Join methodology has been determined; now construct the joining segments
            if (joinType == 1) {
                cPos.m_vertices.emplace_back(1, posTarget, arcCenter);
                cNeg.m_vertices.emplace_back(0, pOrig, heeks::Point {0, 0});
                cNeg.m_vertices.emplace_back(0, negTarget, heeks::Point {0, 0});
            }
            else if (joinType == -1) {
                cPos.m_vertices.emplace_back(0, pOrig, heeks::Point {0, 0});
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
            double radius;
            if (v.m_type == 0) {
                const double dx = v.m_p.x - pPrev.x;
                const double dy = v.m_p.y - pPrev.y;
                const double len = std::hypot(dx, dy);
                tie(sTanX, sTanY) = make_pair(dx / len * offset, dy / len * offset);
                tie(sNormX, sNormY) = make_pair(sTanY, -sTanX);
                tie(eTanX, eTanY) = make_pair(sTanX, sTanY);
                tie(eNormX, eNormY) = make_pair(sNormX, sNormY);
            }
            else {
                [[assume(v.m_type == 1 || v.m_type == -1)]];

                const double sx = v.m_type * (pPrev.x - v.m_c.x);
                const double sy = v.m_type * (pPrev.y - v.m_c.y);
                const double ex = v.m_type * (v.m_p.x - v.m_c.x);
                const double ey = v.m_type * (v.m_p.y - v.m_c.y);
                radius = std::hypot(sx, sy);

                tie(sNormX, sNormY) = make_pair(sx / radius * offset, sy / radius * offset);
                tie(eNormX, eNormY) = make_pair(ex / radius * offset, ey / radius * offset);

                tie(sTanX, sTanY) = make_pair(-sNormY, sNormX);
                tie(eTanX, eTanY) = make_pair(-eNormY, eNormX);
            }

            // Compute the start and end points of the offset segment
            const heeks::Point pPosS(pPrev.x + sNormX, pPrev.y + sNormY);
            const heeks::Point pNegS(pPrev.x - sNormX, pPrev.y - sNormY);
            const heeks::Point pPosE(v.m_p.x + eNormX, v.m_p.y + eNormY);
            const heeks::Point pNegE(v.m_p.x - eNormX, v.m_p.y - eNormY);

            // If the output curves are empty, intialize the start point and direction
            const bool hasPrev = !cPos.m_vertices.empty();
            double exitQ = v.m_type == 0
                ? 0
                : v.m_type / std::hypot(pPrev.x - v.m_c.x, pPrev.y - v.m_c.y);
            if (!hasPrev) {
                cPos.m_vertices.emplace_back(0, pPosS, Point(0, 0));
                cNeg.m_vertices.emplace_back(0, pNegS, Point(0, 0));
                dbgPair("init", 0, pPosS, Point(0, 0), 0, pNegS, Point(0, 0));
                tie(startDirX, startDirY) = make_pair(sTanX, sTanY);
                startQex = exitQ;
            }

            // Join from the previous segment, if there is one
            if (hasPrev) {
                addJoin(prevDirX, prevDirY, sTanX, sTanY, pPosS, pNegS, pPrev, enterQ, exitQ, pPrev);
            }
            enterQ = v.m_type == 0 ? 0 : v.m_type / std::hypot(v.m_p.x - v.m_c.x, v.m_p.y - v.m_c.y);

            // Generate the positive and negative offset segments connecting pPosS to pPosE and
            // pNegS to pNegE
            if (v.m_type == 0) {
                cPos.m_vertices.emplace_back(0, pPosE, heeks::Point {0, 0});
                cNeg.m_vertices.emplace_back(0, pNegE, heeks::Point {0, 0});
                dbgPair("line segment", 0, pPosE, heeks::Point {0, 0}, 0, pNegE, heeks::Point {0, 0});
            }
            else {
                // Check if the offset causes the arc to collapse; if so, generate a straight line
                // instead of an arc because it will be optimized out anyway in a union operation later
                [[assume(v.m_type == 1 || v.m_type == -1)]];
                const bool posCollapse = radius + (offset * v.m_type) <= 0;
                const bool negCollapse = radius - (offset * v.m_type) <= 0;

                cPos.m_vertices.emplace_back(posCollapse ? 0 : v.m_type, pPosE, v.m_c);
                cNeg.m_vertices.emplace_back(negCollapse ? 0 : v.m_type, pNegE, v.m_c);
                const char* posCtx = posCollapse ? "arc segment (pos collapsed->line)"
                                                 : "arc segment";
                const char* negCtx = negCollapse ? "arc segment (neg collapsed->line)"
                                                 : "arc segment";
                if (posCollapse == negCollapse) {
                    dbgPair(
                        posCtx,
                        posCollapse ? 0 : v.m_type,
                        pPosE,
                        v.m_c,
                        negCollapse ? 0 : v.m_type,
                        pNegE,
                        v.m_c
                    );
                }
                else {
                    dbgOne(posCtx, "cPos", posCollapse ? 0 : v.m_type, pPosE, v.m_c);
                    dbgOne(negCtx, "cNeg", negCollapse ? 0 : v.m_type, pNegE, v.m_c);
                }
            }

            // Update state variables
            pPrev = v.m_p;
            tie(prevDirX, prevDirY) = make_pair(eTanX, eTanY);
        }

        // Post processing
        if (curve.IsClosed()) {
            // Add the final join back to the start
            const heeks::Point pPosStart = cPos.m_vertices.front().m_p;
            const heeks::Point pNegStart = cNeg.m_vertices.front().m_p;
            addJoin(
                prevDirX,
                prevDirY,
                startDirX,
                startDirY,
                pPosStart,
                pNegStart,
                pPrev,
                enterQ,
                startQex,
                curve.m_vertices.front().m_p
            );

            // Reverse the negative path so together cPos and cNeg enclose the area within `offset`
            // of the original curve
            cNeg.Reverse();

            // Save results to the output list
            {
                std::cerr << "\n[NaiveOffset] saving cPos (closed) to offset_curves ("
                          << cPos.m_vertices.size() << " vertices):\n";
                for (const auto& v : cPos.m_vertices) {
                    std::cerr << "  type=" << v.m_type << " p=(" << v.m_p.x << "," << v.m_p.y
                              << ") c=(" << v.m_c.x << "," << v.m_c.y << ")\n";
                }
                std::cerr << "\n";
                offset_curves.push_back(cPos);

                std::list<int> directions;
                for (auto it = std::next(cPos.m_vertices.begin()); it != cPos.m_vertices.end(); ++it) {
                    directions.push_back(1);
                }
                m_edgeTags.push_back(directions);
            }
            {
                std::cerr << "\n[NaiveOffset] saving cNeg (closed) to offset_curves ("
                          << cNeg.m_vertices.size() << " vertices):\n";
                for (const auto& v : cNeg.m_vertices) {
                    std::cerr << "  type=" << v.m_type << " p=(" << v.m_p.x << "," << v.m_p.y
                              << ") c=(" << v.m_c.x << "," << v.m_c.y << ")\n";
                }
                std::cerr << "\n";
                offset_curves.push_back(cNeg);

                std::list<int> directions;
                for (auto it = std::next(cNeg.m_vertices.begin()); it != cNeg.m_vertices.end(); ++it) {
                    directions.push_back(-1);
                }
                m_edgeTags.push_back(directions);
            }
        }
        else {
            // Reverse cNeg so it's correctly oriented to join with cPos and the end caps
            cNeg.Reverse();

            // Label positive direction edges
            std::list<int> directions;
            while (directions.size() < cPos.m_vertices.size() - 1) {
                directions.push_back(1);
            }

            // Append the first end cap
            cPos.m_vertices.emplace_back(1, cNeg.m_vertices.begin()->m_p, curve.m_vertices.back().m_p);
            dbgOne(
                "end cap (start)",
                "cPos",
                1,
                cNeg.m_vertices.begin()->m_p,
                curve.m_vertices.back().m_p
            );
            directions.push_back(0);

            // Concatenate cNeg
            for (auto it = std::next(cNeg.m_vertices.begin()); it != cNeg.m_vertices.end(); ++it) {
                cPos.m_vertices.push_back(*it);
                dbgOne("cNeg concat", "cPos", it->m_type, it->m_p, it->m_c);
                directions.push_back(-1);
            }

            // Append the final end cap
            cPos.m_vertices.emplace_back(1, cPos.m_vertices.begin()->m_p, curve.m_vertices.front().m_p);
            dbgOne(
                "end cap (end)",
                "cPos",
                1,
                cPos.m_vertices.begin()->m_p,
                curve.m_vertices.front().m_p
            );
            directions.push_back(0);

            // Save results to the output list
            std::cerr << "\n[NaiveOffset] saving cPos (open) to offset_curves ("
                      << cPos.m_vertices.size() << " vertices):\n";
            for (const auto& v : cPos.m_vertices) {
                std::cerr << "  type=" << v.m_type << " p=(" << v.m_p.x << "," << v.m_p.y << ") c=("
                          << v.m_c.x << "," << v.m_c.y << ")\n";
            }
            std::cerr << "\n";
            offset_curves.push_back(cPos);
            m_edgeTags.push_back(directions);
        }
    }

    m_curves = std::move(offset_curves);
}

// Convert the input CCurve to clipper, populating metadata.
//
// Parameter edgeTags may be empty. If non-empty, there must be as many entries
// as edges in the curve. Tags are meant to track positive/negative offset edges
// from NaiveOffset, and are stored appropriately in the metadata. If tags are
// not provided, all edges are tagged as positive offset edges.
Path64 CArea::MakePoly(const CCurve& curve, ConversionMetadata& metadata, std::list<int> edgeTags) const
{
    if (!curve.m_vertices.size()) {
        return {};
    }

    Path64 result;
    const int curveIndex = metadata.nextCurveIndex++;

    std::cerr << "MakePoly: " << curve.m_vertices.size()
              << " vertices, isClosed=" << curve.IsClosed() << ", " << edgeTags.size() << " tags\n";
    {
        auto tagIt = edgeTags.cbegin();
        for (auto vIt = curve.m_vertices.cbegin(); vIt != curve.m_vertices.cend(); ++vIt) {
            const auto& v = *vIt;
            if (vIt == curve.m_vertices.cbegin()) {
                std::cerr << "  vertex: type=" << v.m_type << " p=(" << v.m_p.x << "," << v.m_p.y
                          << ") c=(" << v.m_c.x << "," << v.m_c.y << ")\n";
            }
            else {
                const int edgeTag = edgeTags.empty() ? 1 : *tagIt++;
                std::cerr << "  vertex: type=" << v.m_type << " p=(" << v.m_p.x << "," << v.m_p.y
                          << ") c=(" << v.m_c.x << "," << v.m_c.y << ") edgeTag=" << edgeTag << "\n";
            }
        }
    }

    auto getPoint64 = [&](double x, double y) -> Point64 {
        const Point64 p64 = ToPoint64(PointD(x, y, 0));
        const auto key = std::make_pair(p64.x, p64.y);
        auto it = metadata.xy_to_z.find(key);
        if (it != metadata.xy_to_z.end()) {
            std::cerr << "  point (cached): (" << p64.x << "," << p64.y << ") z=" << it->second
                      << "\n";
            return Point64(p64.x, p64.y, it->second);
        }
        const int64_t z = metadata.z_next++;
        metadata.xy_to_z[key] = z;
        return Point64(p64.x, p64.y, z);
    };

    Point64 pPrev = getPoint64(curve.m_vertices.front().m_p.x, curve.m_vertices.front().m_p.y);
    result.push_back(pPrev);
    std::cerr << "  point: (" << pPrev.x << "," << pPrev.y << ") z=" << pPrev.z << "\n";
    auto tagIt = edgeTags.cbegin();
    int vertexIndex = 0;

    for (auto vIt = std::next(curve.m_vertices.cbegin()); vIt != curve.m_vertices.cend(); vIt++) {
        const CVertex& vertex = *vIt;
        const bool isLoop = std::next(vIt) == curve.m_vertices.end() && curve.IsClosed()
            && curve.m_vertices.size() > 1;
        const int edgeTag = edgeTags.empty() ? 1 : *tagIt;

        const PointD pPrevD = ToPointD(pPrev);
        std::cerr << "  processing vertex: type=" << vertex.m_type << " isLoop=" << isLoop
                  << " pPrev=(" << pPrev.x << "," << pPrev.y << ")\n";

        if (vertex.m_type == 0) {
            // Add new point
            Point64 newPt;
            if (!isLoop) {
                newPt = getPoint64(vertex.m_p.x, vertex.m_p.y);
                result.push_back(newPt);
            }
            else {
                newPt = Point64(result[0].x, result[0].y, result[0].z);
            }

            const auto key = std::make_pair(std::min(pPrev.z, newPt.z), std::max(pPrev.z, newPt.z));
            std::cerr << " point (line[" << key.first << "," << key.second << "]): (" << newPt.x
                      << "," << newPt.y << ")\n";
            if (metadata.edgeData.count(key)) {
                std::cerr << "MakePoly: ERROR (line): edgeData key (" << key.first << ","
                          << key.second << ") already present, edgeTag "
                          << metadata.edgeData[key].edgeTag << " -> " << edgeTag
                          << (metadata.edgeData[key].edgeTag != edgeTag ? " (CHANGED)" : " (same)")
                          << "\n";
            }
            metadata.edgeData[key] = SegmentData {vertex, edgeTag, curveIndex, vertexIndex};
            pPrev = newPt;
        }
        else if (vertex.m_p.x != pPrevD.x || vertex.m_p.y != pPrevD.y) {
            [[assume(vertex.m_type == 1 || vertex.m_type == -1)]];

            // Compute start and end angles
            const double phi0 = atan2(pPrevD.y - vertex.m_c.y, pPrevD.x - vertex.m_c.x);
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
            const double dx = pPrevD.x - vertex.m_c.x;
            const double dy = pPrevD.y - vertex.m_c.y;
            const double radius = sqrt(dx * dx + dy * dy);
            const double max_dphi = 2 * acos((radius - CArea::m_accuracy) / radius);

            // Determine the number of segments
            const int num_segments = max(min_arc_points, (int)ceil(abs(phi1 - phi0) / max_dphi));
            const double dphi = (phi1 - phi0) / num_segments;

            // Generate arc points
            for (int i = 1; i <= num_segments; i++) {
                Point64 newPt;
                if (i == num_segments) {
                    if (isLoop) {
                        newPt = result[0];
                        if (newPt == pPrev) {
                            continue;
                        }
                        std::cerr << "  point (arc loop): (" << newPt.x << "," << newPt.y
                                  << ") z=" << newPt.z << "\n";
                    }
                    else {
                        newPt = getPoint64(vertex.m_p.x, vertex.m_p.y);
                        if (newPt == pPrev) {
                            continue;
                        }
                        result.push_back(newPt);
                        std::cerr << "  point (arc end): (" << newPt.x << "," << newPt.y
                                  << ") z=" << newPt.z << "\n";
                    }
                }
                else {
                    const double px = vertex.m_c.x + radius * cos(phi0 + dphi * i);
                    const double py = vertex.m_c.y + radius * sin(phi0 + dphi * i);
                    newPt = getPoint64(px, py);
                    if (newPt == pPrev) {
                        continue;
                    }
                    result.push_back(newPt);
                    std::cerr << "  point (arc): (" << newPt.x << "," << newPt.y
                              << ") z=" << newPt.z << "\n";
                }

                const auto key = std::make_pair(std::min(pPrev.z, newPt.z), std::max(pPrev.z, newPt.z));
                std::cerr << "MakePoly: arc edgeData[(" << key.first << "," << key.second
                          << ")] edgeTag=" << edgeTag << "\n";
                if (metadata.edgeData.count(key)) {
                    std::cerr << "MakePoly: ERROR (arc): edgeData key (" << key.first << ","
                              << key.second << ") already present, edgeTag "
                              << metadata.edgeData[key].edgeTag << " -> " << edgeTag
                              << (metadata.edgeData[key].edgeTag != edgeTag ? " (CHANGED)" : " (same)")
                              << "\n";
                }
                metadata.edgeData[key] = SegmentData {vertex, edgeTag, curveIndex, vertexIndex};
                pPrev = newPt;
            }
        }

        if (!edgeTags.empty()) {
            tagIt++;
        }
        vertexIndex++;
    }

    return result;
}


// Convert the provided clipper paths back to CArea/CCurve data, using metadata to correctly
// infer edge type (arc/line) and arc center information. Only edges tagged 1 (i.e. positive offset
// segments from NaiveOffset) are kept in `this` CArea. If cNeg is provided, edges tagged -1 are
// kept there. Edges tagged 0 (end caps from NaiveOffset) are always dropped.
//
// Parameter isClosed specifies if the clipper paths represent open or closed curves. If the curves
// are open, they will be reoriented/ordered using metadata to match the ordering of the parent
// edges they are derived from.
void CArea::SetFromResult(
    Paths64& paths,
    bool isClosed,
    ConversionMetadata& metadata,
    std::optional<std::reference_wrapper<CArea>> cNeg
)
{
    std::cerr << "SetFromResult: " << paths.size() << " paths, isClosed=" << isClosed
              << ", metadata.edgeData.size()=" << metadata.edgeData.size() << "\n";
    if (!isClosed) {
        ReorderOpenPaths(paths, metadata);
    }

    for (const Path64& path : paths) {
        if (!path.size()) {
            continue;
        }

        // Preserve single vertex paths. They require special code because the code below
        // expects/processes edges, not vertices
        if (path.size() == 1) {
            const PointD pt = ToPointD(path[0]);
            CCurve c;
            c.m_vertices.emplace_back(heeks::Point {pt.x, pt.y});
            m_curves.push_back(c);
            continue;
        }

        std::cerr << "  path: " << path.size() << " points\n";
        // Keep track of the current curve and its edge tag. Also track the original curve and
        // its curve/edge tag so the final curve (if different) can be joined to it (if the path is
        // closed and tag matches)
        int orientation = 0;
        std::optional<CVertex> lastParentCVertex;
        CCurve c;
        CCurve* firstCurve = nullptr;
        std::optional<int> firstOrientation;
        std::optional<CVertex> firstParentCVertex;

        // Helper function to save the current curve to the appropriate CArea when done with it
        auto saveCurve = [&]() {
            if (!c.m_vertices.empty()) {
                std::cerr << "\n    saveCurve: orientation=" << orientation
                          << " vertices=" << c.m_vertices.size() << "\n";
                for (const auto& v : c.m_vertices) {
                    std::cerr << "      type=" << v.m_type << " p=(" << v.m_p.x << "," << v.m_p.y
                              << ") c=(" << v.m_c.x << "," << v.m_c.y << ")\n";
                }

                if (orientation == 1) {
                    m_curves.push_back(c);
                    if (!firstOrientation) {
                        firstCurve = &m_curves.back();
                        std::cerr << "    -> pushed to m_curves (firstCurve set), total="
                                  << m_curves.size() << "\n";
                    }
                    else {
                        std::cerr << "    -> pushed to m_curves, total=" << m_curves.size() << "\n";
                    }
                }
                else if (orientation == -1 && cNeg) {
                    cNeg->get().m_curves.push_back(c);
                    if (!firstOrientation) {
                        firstCurve = &cNeg->get().m_curves.back();
                        std::cerr << "    -> pushed to cNeg (firstCurve set), total="
                                  << cNeg->get().m_curves.size() << "\n";
                    }
                    else {
                        std::cerr << "    -> pushed to cNeg, total=" << cNeg->get().m_curves.size()
                                  << "\n\n";
                    }
                }
                else {
                    std::cerr << "    -> DROPPED (orientation=" << orientation
                              << ", cNeg=" << cNeg.has_value() << ")\n";
                }
                if (!firstOrientation) {
                    firstOrientation = orientation;
                }
                std::cerr << "\n";

                lastParentCVertex = {};
            }
        };

        // For closed paths, start at the smallest z-value
        int startVertex = 0;
        if (isClosed) {
            for (int i = startVertex + 1; i < path.size(); i++) {
                if (path[i].z < path[startVertex].z) {
                    startVertex = i;
                }
            }
        }

        // Loop through clipper edges, converting to CVertex and building up the current CCurve
        for (int edgeNum = 0; edgeNum < (isClosed ? path.size() : path.size() - 1); edgeNum++) {
            const int iEdge = (startVertex + edgeNum) % path.size();
            const Point64& v0 = path[iEdge];
            const Point64& v1 = path[(iEdge + 1) % path.size()];
            const auto parentEdge = getParentEdge(v0, v1, metadata);
            const auto zIt = metadata.edgeData.find(parentEdge);
            const bool inZData = zIt != metadata.edgeData.end();
            static const SegmentData kMissingSegment {};
            const SegmentData& parentData = inZData ? zIt->second : kMissingSegment;

            std::cerr << "  edge[" << edgeNum << "]: z=(" << v0.z << "," << v1.z << ") parentEdge=("
                      << parentEdge.first << "," << parentEdge.second << ") inZData=" << inZData
                      << " orientation=" << parentData.edgeTag << "\n";
            if (!inZData) {
                std::cerr << "\n\n    ERROR MISSING Z DATA! \n\n\n";
            }

            // Check if the orientation changed; if it did, end the curve
            if (parentData.edgeTag != orientation) {
                saveCurve();
                c.m_vertices.clear();
            }
            orientation = parentData.edgeTag;

            // Append the edge to the curve
            if (c.m_vertices.empty()) {
                const PointD start = ToPointD(v0);
                c.m_vertices.emplace_back(heeks::Point {start.x, start.y});
                std::cerr << "    start vertex: (" << start.x << "," << start.y << ")\n";
            }

            const PointD end = ToPointD(v1);
            CVertex edge(parentData.orig.m_type, {end.x, end.y}, parentData.orig.m_c);
            CVertex& prev = c.m_vertices.back();

            // Determine if the edge is reversed from the parent, and update type accordingly
            if (edge.m_type == 1 || edge.m_type == -1) {
                const Point64 mc64 = ToPoint64(PointD(parentData.orig.m_c.x, parentData.orig.m_c.y, 0));
                const double phi1 = atan2(v0.y - mc64.y, v0.x - mc64.x);
                const double phi2 = recenter(atan2(v1.y - mc64.y, v1.x - mc64.x), phi1 - M_PI, 1);
                if (phi2 * edge.m_type < phi1 * edge.m_type) {
                    edge.m_type = -edge.m_type;
                }
            }

            if (edge.m_type != 0 && edge.m_type == prev.m_type && edge.m_c == prev.m_c) {
                // This is a continuation of the same curve
                const bool fullLoop = c.m_vertices.size() > 1
                    && std::prev(c.m_vertices.end(), 2)->m_p == edge.m_p;
                if (fullLoop) {
                    // Break it into 2 semi circles
                    const heeks::Point mid {2 * edge.m_c.x - edge.m_p.x, 2 * edge.m_c.y - edge.m_p.y};
                    prev.m_p = mid;
                    c.m_vertices.push_back(edge);
                    std::cerr << "    full-loop split: type=" << edge.m_type << " mid=(" << mid.x
                              << "," << mid.y << ") c=(" << edge.m_c.x << "," << edge.m_c.y << ")\n";
                }
                else {
                    // Extend the existing segment instead of appending a new one
                    prev.m_p = edge.m_p;
                    std::cerr << "    extend vertex: type=" << edge.m_type << " p=(" << edge.m_p.x
                              << "," << edge.m_p.y << ") c=(" << edge.m_c.x << "," << edge.m_c.y
                              << ")\n";
                }
            }
            else {
                c.m_vertices.push_back(edge);
                std::cerr << "    push vertex: type=" << edge.m_type << " p=(" << edge.m_p.x << ","
                          << edge.m_p.y << ") c=(" << edge.m_c.x << "," << edge.m_c.y << ")"
                          << " reason=" << (!lastParentCVertex ? "first-edge" : "new-parent-segment");
                if (lastParentCVertex) {
                    std::cerr << " old-parent=type:" << lastParentCVertex->m_type << " p:("
                              << lastParentCVertex->m_p.x << "," << lastParentCVertex->m_p.y << ")"
                              << " c:(" << lastParentCVertex->m_c.x << ","
                              << lastParentCVertex->m_c.y << ")"
                              << " new-parent=type:" << parentData.orig.m_type << " p:("
                              << parentData.orig.m_p.x << "," << parentData.orig.m_p.y << ")"
                              << " c:(" << parentData.orig.m_c.x << "," << parentData.orig.m_c.y
                              << ")";
                }
                std::cerr << "\n";
            }

            lastParentCVertex = {parentData.orig};
            if (!firstParentCVertex) {
                firstParentCVertex = {parentData.orig};
            }
        }

        // Save the final curve
        std::cerr << "\n  final: isClosed=" << isClosed << " firstCurve=" << (firstCurve != nullptr)
                  << " firstOrientation=" << (firstOrientation ? *firstOrientation : -99)
                  << " orientation=" << orientation << "\n";
        if (isClosed && firstCurve && firstOrientation && orientation == *firstOrientation) {
            // Save the curve by joining it with the first curve
            std::cerr << "  -> joining with firstCurve (" << c.m_vertices.size()
                      << " vertices prepended)\n\n";

            // Remove the first curve's (now redundant) start point
            firstCurve->m_vertices.pop_front();

            // Check if the first CVertex of the first curve can extend the last CVertex of the
            // current curve
            CVertex& edge = firstCurve->m_vertices.front();
            CVertex& prev = c.m_vertices.back();
            if (edge.m_type != 0 && edge.m_type == prev.m_type && edge.m_c == prev.m_c) {
                // It is an extension
                const bool fullLoop = c.m_vertices.size() > 1
                    && std::prev(c.m_vertices.end(), 2)->m_p == edge.m_p;
                if (fullLoop) {
                    // Break it into 2 semi circles
                    const heeks::Point mid {2 * edge.m_c.x - edge.m_p.x, 2 * edge.m_c.y - edge.m_p.y};
                    prev.m_p = mid;
                    std::cerr << "    full-loop split at join: type=" << edge.m_type << " mid=("
                              << mid.x << "," << mid.y << ") c=(" << edge.m_c.x << "," << edge.m_c.y
                              << ")\n";
                }
                else {
                    // Join them
                    prev.m_p = edge.m_p;
                    firstCurve->m_vertices.pop_front();
                    std::cerr << "    join at join: type=" << edge.m_type << " p=(" << edge.m_p.x
                              << "," << edge.m_p.y << ") c=(" << edge.m_c.x << "," << edge.m_c.y
                              << ")\n";
                }
            }

            // Finally, concatenate them
            firstCurve->m_vertices
                .insert(firstCurve->m_vertices.begin(), c.m_vertices.begin(), c.m_vertices.end());
        }
        else if (!firstOrientation && isClosed && c.m_vertices.size() >= 3) {
            // This is the only curve so it needs to be saved as a new curve, but first try to
            // connect its first/last edges

            // Check if the first CVertex of the curve can extend the last CVertex
            CVertex& first = *std::next(c.m_vertices.begin());
            CVertex& last = c.m_vertices.back();
            if (last.m_type != 0 && last.m_type == first.m_type && last.m_c == first.m_c) {
                // It is an extension
                const bool fullLoop = std::prev(c.m_vertices.end(), 2)->m_p == first.m_p;
                if (fullLoop) {
                    // Break it into 2 semi circles
                    const heeks::Point mid {
                        2 * first.m_c.x - first.m_p.x,
                        2 * first.m_c.y - first.m_p.y
                    };
                    last.m_p = mid;
                    c.m_vertices.front().m_p = mid;
                    std::cerr << "    full-loop split at wrap: type=" << first.m_type << " mid=("
                              << mid.x << "," << mid.y << ") c=(" << first.m_c.x << ","
                              << first.m_c.y << ")\n";
                }
                else {
                    // Join them by merging the last edge into the first one
                    c.m_vertices.front().m_p = std::prev(c.m_vertices.end(), 2)->m_p;
                    c.m_vertices.pop_back();
                    std::cerr << "    join at wrap: type=" << first.m_type << " p=(" << first.m_p.x
                              << "," << first.m_p.y << ") c=(" << first.m_c.x << "," << first.m_c.y
                              << ")\n";
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


void CArea::Offset(double offset, double arcTolerance)
{
    if (offset == 0) {
        return;
    }

    // Perform the naive offset, offsetting each edge and joining
    NaiveOffset(abs(offset), arcTolerance);

    // If we want to keep the negative edges, flip all the edge labels
    if (offset < 0) {
        for (auto& curve_dirs : m_edgeTags) {
            for (auto& dir : curve_dirs) {
                dir = -dir;
            }
        }
    }

    // Union (fill rule positive), keeping positive edges and dropping negative edges
    _Clip(ClipType::Union, CArea {}, FillRule::Positive);

    // If negative offset, reverse the curves to put them in the forward direction
    if (offset < 0) {
        for (CCurve& c : m_curves) {
            c.Reverse();
        }
    }

    // I'm preserving this Reorder() call to preserve old behavior, but imo this should not be part
    // of Offset's spec
    this->Reorder();
}

CArea CArea::OpenOffset(double offset, double arcTolerance)
{
    CArea cNeg;
    if (offset == 0) {
        return cNeg;
    }

    // Perform the naive offset, offsetting each edge and joining
    NaiveOffset(abs(offset), arcTolerance);

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
    NaiveOffset(abs(value));

    // We want to keep all offset curves, so clear the edge tags
    for (auto& curve_dirs : m_edgeTags) {
        curve_dirs.clear();
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
    std::pair<int64_t, int64_t> testEdge = {min(p1.z, p2.z), max(p1.z, p2.z)};
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

    // Failed to find the parent edge. This should not happen; parent edge should always exist.
    // Print an error and return a sentinel value
    std::cerr << "    ERROR: no parent edge found for z=(" << p1.z << "," << p2.z << ")"
              << " hits=(" << metadata.intersections.count(p1.z) << ","
              << metadata.intersections.count(p2.z) << "), returning sentinel\n";
    return {-2, -3};
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
                // Error, this should not happen; there should always be edgeData for parent edges
                std::cerr << "ReorderOpenPaths: ERROR: no edgeData for parentEdge ("
                          << parentEdge.first << "," << parentEdge.second << ")\n";
                continue;
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
                [[assume(seg.orig.m_type == 1 || seg.orig.m_type == -1)]];
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

                const double progress = max(-abs(phi_end - phi1), -abs(phi_end - phi2));
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
