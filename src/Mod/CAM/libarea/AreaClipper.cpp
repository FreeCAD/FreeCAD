// SPDX-License-Identifier: BSD-3-Clause

// AreaClipper.cpp

// implements CArea methods using Angus Johnson's "Clipper"

#include "Area.h"
#include "clipper2/clipper.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>

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
    return Point64((int64_t)(p.x * CArea::m_clipper_scale), (int64_t)(p.y * CArea::m_clipper_scale), p.z);
}

static PointD ToPointD(const Point64& p)
{
    return PointD((double)p.x / CArea::m_clipper_scale, (double)p.y / CArea::m_clipper_scale, p.z);
}

static std::list<PointD> pts_for_AddVertex;

/**
 * AddVertex: Accumulate points into pts_for_AddVertex. Interpolate arcs and assign z-values as
 * needed for later reconstruction of the arcs.
 *
 * @param vertex The CVertex representing the next movement
 * @param prev_vertex The previous CVertex, or NULL if none. Used only to determine the movement
 * start point
 * @param arcMap The map to populate with arc metadata, for arc reconstruction
 * @param zLoop On closed curves, when looping back to the start, use this parameter to provide the
 * start z index. Connectivity information will be added to arcMap, but no new point will be
 * allocated for the start/end location
 */
static void AddVertex(
    const CVertex& vertex,
    const CVertex* prev_vertex,
    ArcFittingMap& arcMap,
    std::optional<int> zLoop = {}
)
{
    if (vertex.m_type == 0 || prev_vertex == NULL) {
        if (!zLoop.has_value()) {
            const int64_t z = arcMap.z_next++;
            PointD p(vertex.m_p.x, vertex.m_p.y, z);
            arcMap.point_map[z] = vertex.m_p;
            pts_for_AddVertex.push_back(p);
            arcMap.z_prev = z;
        }
    }
    else {
        if (vertex.m_p != prev_vertex->m_p) {
            const double phi0
                = atan2(prev_vertex->m_p.y - vertex.m_c.y, prev_vertex->m_p.x - vertex.m_c.x);
            double phi1 = atan2(vertex.m_p.y - vertex.m_c.y, vertex.m_p.x - vertex.m_c.x);

            if (vertex.m_type == -1 && phi1 > phi0) {
                // fix to make it clockwise
                phi1 -= 2 * M_PI;
            }
            else if (vertex.m_type == 1 && phi1 < phi0) {
                // fix to make it counterclockwise
                phi1 += 2 * M_PI;
            }

            // what is the delta phi to get an accuracy of aber
            const double dx = prev_vertex->m_p.x - vertex.m_c.x;
            const double dy = prev_vertex->m_p.y - vertex.m_c.y;
            const double radius = sqrt(dx * dx + dy * dy);
            const double max_dphi = 2 * acos((radius - CArea::m_accuracy) / radius);

            // determine the number of segments
            const int num_segments = max(min_arc_points, (int)ceil(abs(phi1 - phi0) / max_dphi));
            const double dphi = (phi1 - phi0) / num_segments;

            const int64_t z_start = arcMap.z_next;
            for (int i = 1; i <= num_segments; i++) {
                if (i == num_segments) {
                    if (zLoop.has_value()) {
                        // since zLoop represents the curve start, its z value will be smaller
                        arcMap.arc_centers[{*zLoop, arcMap.z_prev}] = vertex.m_c;
                    }
                    else {
                        const int64_t z = arcMap.z_next++;
                        pts_for_AddVertex.push_back(PointD(vertex.m_p.x, vertex.m_p.y, z));
                        arcMap.point_map[z] = vertex.m_p;
                        arcMap.arc_centers[{arcMap.z_prev, z}] = vertex.m_c;
                        arcMap.z_prev = z;
                    }
                }
                else {
                    const int64_t z = arcMap.z_next++;
                    const double px = vertex.m_c.x + radius * cos(phi0 + dphi * i);
                    const double py = vertex.m_c.y + radius * sin(phi0 + dphi * i);
                    pts_for_AddVertex.push_back(PointD(px, py, z));
                    // Store arc center in point_map for intermediate points
                    arcMap.point_map[z] = vertex.m_c;
                    arcMap.arc_centers[{arcMap.z_prev, z}] = vertex.m_c;
                    arcMap.z_prev = z;
                }
            }
        }
    }
}

static void MakePoly(const CCurve& curve, Path64& p, ArcFittingMap& arcMap)
{
    pts_for_AddVertex.clear();
    const CVertex* prev_vertex = NULL;

    if (!curve.m_vertices.size()) {
        return;
    }

    const int z0 = arcMap.z_next;
    for (std::list<CVertex>::const_iterator It2 = curve.m_vertices.begin();
         It2 != curve.m_vertices.end();
         It2++) {
        const CVertex& vertex = *It2;
        const bool isLoop = std::next(It2) == curve.m_vertices.end() && curve.IsClosed()
            && curve.m_vertices.size() > 1;
        auto zLoop = isLoop ? std::optional<int>(z0) : std::nullopt;
        AddVertex(vertex, prev_vertex, arcMap, zLoop);
        prev_vertex = &vertex;
    }

    p.resize(pts_for_AddVertex.size());
    unsigned int i = 0;
    for (std::list<PointD>::iterator It = pts_for_AddVertex.begin(); It != pts_for_AddVertex.end();
         It++, i++) {
        p[i] = ToPoint64(*It);
    }
}

static void MakePolyPoly(const CArea& area, Paths64& pp, ArcFittingMap& arcMap)
{
    pp.clear();

    for (std::list<CCurve>::const_iterator It = area.m_curves.begin(); It != area.m_curves.end();
         It++) {
        pp.push_back(Path64());
        MakePoly(*It, pp.back(), arcMap);
    }
}

// Helper method for recentering an angle on a target reference angle
double recenter(double phi, double phi_ref = 0)
{
    while (phi - phi_ref > M_PI) {
        phi -= 2 * M_PI;
    }
    while (phi - phi_ref < -M_PI) {
        phi += 2 * M_PI;
    }
    return phi;
};

static void SetFromResult(CCurve& curve, Path64& path, bool is_closed, const ArcFittingMap& arcMap)
{
    if (CArea::m_clipper_clean_distance >= heeks::Point::tolerance) {
        path = SimplifyPath(path, CArea::m_clipper_clean_distance, is_closed);
    }

    if (path.size() == 0) {
        return;
    }

    const double max_arc_length = 2 * M_PI * .99;

    // Loop through points
    int64_t prevZ = -1;
    heeks::Point prevP;
    double phi_total = 0.0;
    int num_j = path.size() + (is_closed && path.size() > 1 ? 1 : 0);
    for (int dj = 0; dj < num_j; dj++) {
        const int j = dj % path.size();
        const Point64& pt = path[j];
        PointD dp = ToPointD(pt);
        heeks::Point p(dp.x, dp.y);

        const int jnext = (dj + 1) % path.size();
        const bool hasNext = (dj + 1 < num_j) || is_closed;
        const bool nextGenerated = (dj + 1 == num_j) && is_closed;
        const Point64& pt_next = path[jnext];
        PointD dp_next = ToPointD(pt_next);
        heeks::Point p_next(dp_next.x, dp_next.y);

        // Construct ordered pair for arc detection
        std::pair<int64_t, int64_t> zPair(std::min(prevZ, pt.z), std::max(prevZ, pt.z));

        // Check if this segment is an arc (presence in arc_centers means it's an arc)
        auto centerIt = arcMap.arc_centers.find(zPair);
        bool isLine = !CArea::m_fit_arcs || (prevZ == -1)
            || ((prevZ != pt.z) && (centerIt == arcMap.arc_centers.end()));

        if (isLine) {

            curve.m_vertices.emplace_back(0, p, heeks::Point {0, 0});
            phi_total = 0.0;
        }
        else {
            const bool isPointExpansion = prevZ == pt.z;
            heeks::Point center = isPointExpansion ? arcMap.point_map.at(pt.z) : centerIt->second;

            double phi0 = atan2(prevP.y - center.y, prevP.x - center.x);
            double phi1 = atan2(p.y - center.y, p.x - center.x);
            double dphi = recenter(phi1 - phi0);
            int type = (dphi > 0) ? 1 : -1;

            // When arcs are discretized, the angle of the edge line doesn't
            // quite match the arc tangent. After offsetting radially outwards,
            // this can result in points generated as an arc about the arc's
            // endpoint that should instead have been part of the expanded arc.
            // Here, we check if that happens in this segment and the following
            // segment and correct for it.

            // If this segment is a point expansion and next is an original
            // arc, correct the arc endpoints
            if (isPointExpansion && hasNext) {
                std::pair<int64_t, int64_t> zPairNext(
                    std::min(pt.z, pt_next.z),
                    std::max(pt.z, pt_next.z)
                );
                auto centerNextIt = arcMap.arc_centers.find(zPairNext);

                if (centerNextIt != arcMap.arc_centers.end() && centerNextIt->second != center) {
                    // It is, with original boundary at this point expansion's center point
                    const heeks::Point& arc_center = centerNextIt->second;
                    const heeks::Point& arc_boundary = center;
                    const double phi_boundary
                        = atan2(arc_boundary.y - arc_center.y, arc_boundary.x - arc_center.x);

                    // Point expansion current angles: phi0 to phi1
                    // Arc current angles: phi1 to phi_next
                    // Correct arc boundary: phi_boundary
                    const double phi0 = recenter(
                        atan2(prevP.y - arc_center.y, prevP.x - arc_center.x),
                        phi_boundary
                    );
                    const double phi1
                        = recenter(atan2(p.y - arc_center.y, p.x - arc_center.x), phi_boundary);
                    const double phi_next = recenter(
                        atan2(p_next.y - arc_center.y, p_next.x - arc_center.x),
                        phi_boundary
                    );
                    const int type = (phi1 - phi0) > 0 ? 1 : -1;
                    const int type_next = (phi_next - phi1 > 0) ? 1 : -1;

                    if (type == type_next) {
                        // Compute the arc radius and allowed angular error
                        double dx = p.x - arc_center.x;
                        double dy = p.y - arc_center.y;
                        double arc_radius = sqrt(dx * dx + dy * dy);
                        const double angle_error = CArea::m_accuracy / arc_radius;

                        // matching type means we have phi0 < ph1 < phi_next (type 1)
                        // or phi0 > phi1 > phi_next (type -1)
                        // always: type * phi0 < type * phi1 < type * phi_next
                        if (type * phi_boundary - angle_error < type * phi0) {
                            // Subsume this point expansion with the subsequent arc
                            if (nextGenerated) {
                                // Update its start location to prevP
                                curve.m_vertices.front().m_p = prevP;
                            }
                            continue;
                        }
                        else if (type * phi_boundary + angle_error < type * phi1) {
                            // Replace part of this point expansion with the next arc
                            p.x = arc_center.x + arc_radius * cos(phi_boundary);
                            p.y = arc_center.y + arc_radius * sin(phi_boundary);
                            if (nextGenerated) {
                                // Update its start location to p
                                curve.m_vertices.front().m_p = p;
                            }
                        }
                        else {
                            // No action required; full point expansion is correct
                        }
                    }
                }
            }

            // If this segment is an original arc and next is a point
            // expansion, correct the arc endpoints
            if (!isPointExpansion && hasNext) {
                if (pt_next.z == pt.z && arcMap.point_map.at(pt_next.z) != center) {
                    // It is. The original arc boundary is at the point expansion's center
                    const heeks::Point& arc_boundary = arcMap.point_map.at(pt_next.z);
                    double phi_boundary = atan2(arc_boundary.y - center.y, arc_boundary.x - center.x);

                    // Arc current angles: phi0 to phi1 (no recompute required)
                    // Point expansion current angles: phi1 to phi_next
                    // Correct arc boundary: phi_boundary
                    double phi_next = atan2(p_next.y - center.y, p_next.x - center.x);
                    phi0 = recenter(phi0, phi_boundary);
                    phi1 = recenter(phi1, phi_boundary);
                    phi_next = recenter(phi_next, phi_boundary);
                    int type_next = (phi_next - phi1 > 0) ? 1 : -1;

                    if (type == type_next) {
                        // Compute the arc radius and allowed angular error
                        double dx = p.x - center.x;
                        double dy = p.y - center.y;
                        double radius = sqrt(dx * dx + dy * dy);
                        const double angle_error = CArea::m_accuracy / radius;

                        // matching type means we have phi0 < ph1 < phi_next (type 1)
                        // or phi0 > phi1 > phi_next (type -1)
                        // always: type * phi0 < type * phi1 < type * phi_next
                        if (type * phi_boundary + angle_error > type * phi_next) {
                            // Subsume the subsequent point expansion with this arc
                            p = p_next;
                            if (nextGenerated) {
                                // Delete it, and update start location for the next move
                                curve.m_vertices.front().m_p = p;
                                curve.m_vertices.erase(std::next(curve.m_vertices.begin()));
                            }
                            else {
                                // Skip the point expansion that comes next
                                dj += 1;
                            }
                        }
                        else if (type * phi_boundary - angle_error > type * phi1) {
                            // Replace part of the subsequent point expansion with this arc
                            p.x = center.x + radius * cos(phi_boundary);
                            p.y = center.y + radius * sin(phi_boundary);
                            if (nextGenerated) {
                                // Update its start location to p
                                curve.m_vertices.front().m_p = p;
                            }
                        }
                        else {
                            // No action required; full point expansion is correct
                        }
                    }
                }
            }

            if (curve.m_vertices.size() > 0 && curve.m_vertices.back().m_type == type
                && curve.m_vertices.back().m_c == center && phi_total + abs(dphi) <= max_arc_length) {
                // Extend the previous CVertex arc
                curve.m_vertices.back().m_p = p;
                phi_total += abs(dphi);
            }
            else {
                // Add a new CVertex for the arc
                curve.m_vertices.emplace_back(type, p, center);
                phi_total = abs(dphi);
            }
        }

        prevZ = dp.z;
        prevP = p;
    }

    // For closed paths, check if it starts and ends with the same arc:
    // [0] line vertex with starting point
    // [1] arc vertex
    // [n-1] arc vertex, for the same arc
    // If they are the same and aren't too long to be merged, then merge them
    if (is_closed && curve.m_vertices.size() >= 3) {
        const CVertex& last_vertex = curve.m_vertices.back();
        const CVertex& second_vertex = *std::next(curve.m_vertices.begin());

        // Check if both are arcs with the same type and center
        if (last_vertex.m_type != 0 && second_vertex.m_type != 0
            && last_vertex.m_type == second_vertex.m_type && last_vertex.m_c == second_vertex.m_c) {

            // Calculate total arc angle to ensure it doesn't exceed max_arc_length
            const heeks::Point& p0 = curve.m_vertices.front().m_p;
            const heeks::Point& p1 = second_vertex.m_p;
            auto second_to_last_it = std::prev(curve.m_vertices.end(), 2);
            const heeks::Point& p_prev = second_to_last_it->m_p;
            const heeks::Point& center = last_vertex.m_c;

            // Compute the ngular span of each arc (from p_prev to p0)
            double phi_prev = atan2(p_prev.y - center.y, p_prev.x - center.x);
            double phi0 = atan2(p0.y - center.y, p0.x - center.x);
            double phi1 = atan2(p1.y - center.y, p1.x - center.x);

            double dphi_last = phi0 - phi_prev;
            double dphi_first = phi1 - phi0;

            // Ensure dphi sign matches vertex type (CCW=1 should be positive, CW=-1 should be negative)
            if (last_vertex.m_type == 1) {
                if (dphi_last < 0) {
                    dphi_last += 2 * M_PI;
                }
                if (dphi_first < 0) {
                    dphi_first += 2 * M_PI;
                }
            }
            else {
                if (dphi_last > 0) {
                    dphi_last -= 2 * M_PI;
                }
                if (dphi_first > 0) {
                    dphi_first -= 2 * M_PI;
                }
            }

            // Check if total exceeds max_arc_length; if not, then combine them
            if (abs(dphi_last) + abs(dphi_first) < max_arc_length) {
                curve.m_vertices.pop_back();
                curve.m_vertices.front().m_p = p_prev;
            }
        }
    }
}

static void SetFromResult(
    CArea& area,               // area to populate
    Paths64& pp,               // clipper data to put in the area
    bool is_closed = true,     // flag if the clipper paths are closed
    bool clear_area = true,    // flag for clearing the area's curves before populating
    bool clear_arc_map = true  // flag for clearing arc metadata when done
)
{
    // delete existing geometry
    if (clear_area) {
        area.m_curves.clear();
    }

    // Process intersection points before reconstructing curves
    area.ProcessIntersectionPoints(pp, is_closed);

    for (unsigned int i = 0; i < pp.size(); i++) {
        Path64& p = pp[i];

        area.m_curves.emplace_back();
        CCurve& curve = area.m_curves.back();
        SetFromResult(curve, p, is_closed, area.m_arc_fitting_map);
    }

    // Reset arc fitting map to ensure clean state
    if (clear_arc_map) {
        area.m_arc_fitting_map = ArcFittingMap();
    }
}

void CArea::Subtract(const CArea& a2)
{
    Clip(ClipType::Difference, a2, FillRule::EvenOdd, FillRule::EvenOdd);
}

void CArea::Intersect(const CArea& a2)
{
    Clip(ClipType::Intersection, a2, FillRule::EvenOdd, FillRule::EvenOdd);
}

void CArea::Union(const CArea& a2)
{
    Clip(ClipType::Union, a2, FillRule::EvenOdd, FillRule::EvenOdd);
}

void CArea::Xor(const CArea& a2)
{
    Clip(ClipType::Xor, a2, FillRule::EvenOdd, FillRule::EvenOdd);
}

void CArea::OffsetInward(double inwards_value)
{
    Offset(-inwards_value);
}

void CArea::PopulateClipper(Clipper64& c, bool as_clip, ArcFittingMap& arcMap) const
{
    Paths64 closed_paths;
    Paths64 open_paths;
    int skipped = 0;

    for (const CCurve& curve : m_curves) {
        bool is_closed = curve.IsClosed();

        if (!is_closed && as_clip) {
            ++skipped;
            continue;
        }

        Path64 p;
        MakePoly(curve, p, arcMap);

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

void CArea::_Clip(
    ClipType op,
    const CArea& clip_area,
    FillRule subjFillType,
    FillRule clipFillType,
    bool reverseOpenPathContents,
    bool reverseOpenPathOrder
)
{
    Clipper64 c;
    c.SetZCallback(MakeZCallback());
    PopulateClipper(c, false, m_arc_fitting_map);
    clip_area.PopulateClipper(c, true, m_arc_fitting_map);

    // Execute to get both closed and open paths
    Paths64 closed_paths;
    Paths64 open_paths;
    c.Execute(op, subjFillType, closed_paths, open_paths);

    // Reverse open path contents if requested
    if (reverseOpenPathContents) {
        for (auto& path : open_paths) {
            std::reverse(path.begin(), path.end());
        }
    }

    // Reverse open path order if requested
    if (reverseOpenPathOrder) {
        std::reverse(open_paths.begin(), open_paths.end());
    }

    // Set closed paths as result
    SetFromResult(
        *this,
        closed_paths,
        /*is_closed=*/true,
        /*clear_area=*/true,
        /*clear_arc_map=*/false
    );

    // Append open paths to result
    SetFromResult(
        *this,
        open_paths,
        /*is_closed=*/false,
        /*clear_area=*/false,
        /*clear_arc_map=*/true
    );
}

void CArea::Clip(ClipType op, const CArea& clip_area, FillRule subjFillType, FillRule clipFillType)
{
    _Clip(op, clip_area, subjFillType, clipFillType, false, false);
}

void CArea::ClipperNoop()
{
    Paths64 closed_paths;
    Paths64 open_paths;
    for (const CCurve& curve : m_curves) {
        bool is_closed = curve.IsClosed();
        Path64 p;
        MakePoly(curve, p, m_arc_fitting_map);

        if (is_closed) {
            closed_paths.push_back(p);
        }
        else {
            open_paths.push_back(p);
        }
    }

    // Set closed paths as result
    SetFromResult(
        *this,
        closed_paths,
        /*is_closed=*/true,
        /*clear_area=*/true,
        /*clear_arc_map=*/false
    );

    // Append open paths to result
    SetFromResult(
        *this,
        open_paths,
        /*is_closed=*/false,
        /*clear_area=*/false,
        /*clear_arc_map=*/true
    );
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
        FillRule::EvenOdd,
        reverseOpenPathContents,
        reverseOpenPathOrder
    );
}

void CArea::Offset(double offset, JoinType joinType, EndType endType, double miterLimit, double arcTolerance)
{
    offset *= m_clipper_scale;
    if (arcTolerance == 0.0) {
        // Clipper arc tolerance definition: https://goo.gl/4odfQh
        double dphi = acos(1.0 - m_accuracy * m_clipper_scale / fabs(offset));
        int Segments = max(2 * min_arc_points, (int)ceil(M_PI / dphi));
        dphi = M_PI / Segments;
        arcTolerance = (1.0 - cos(dphi)) * fabs(offset);
    }
    else {
        arcTolerance *= m_clipper_scale;
    }

    ClipperOffset clipper(miterLimit, arcTolerance);
    clipper.SetZCallback(MakeZCallback());

    Paths64 pp;
    MakePolyPoly(*this, pp, m_arc_fitting_map);

    // Collect closed paths to add together (holes must be added with outer boundary)
    Paths64 closedPaths;

    // Add paths with appropriate end types
    int i = 0;
    for (const CCurve& c : m_curves) {
        if (c.IsClosed()) {
            closedPaths.push_back(pp[i]);
        }
        else {
            clipper.AddPath(pp[i], joinType, endType);
        }
        i++;
    }
    clipper.AddPaths(closedPaths, joinType, EndType::Polygon);

    // Execute offset
    Paths64 pp2;
    clipper.Execute(offset, pp2);

    SetFromResult(*this, pp2);
    this->Reorder();
}

void CArea::Thicken(double value)
{
    // Create inward offset on a copy
    CArea inner(*this);
    inner.Offset(-value);

    // Create outward offset on current area
    this->Offset(value);

    // Subtract inner from outer to create the thickened band
    this->Subtract(inner);
}

void CArea::ZCallback(
    const Point64& e1bot,
    const Point64& e1top,
    const Point64& e2bot,
    const Point64& e2top,
    Point64& pt
)
{
    // If z values are present, generate a new one for the new point
    if (e1bot.z != 0 || e1top.z != 0 || e2bot.z != 0 || e2top.z != 0) {
        // Allocate a new z-label for this intersection point
        pt.z = m_arc_fitting_map.z_next++;

        // Record the intersection: which edges intersected to create this point
        m_arc_fitting_map.intersections[pt.z] = std::make_tuple(e1bot.z, e1top.z, e2bot.z, e2top.z);

        // Add the new point to the point map
        PointD dp = ToPointD(pt);
        m_arc_fitting_map.point_map[pt.z] = heeks::Point(dp.x, dp.y);
    }
}

ZCallback64 CArea::MakeZCallback()
{
    return std::bind(
        &CArea::ZCallback,
        this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3,
        std::placeholders::_4,
        std::placeholders::_5
    );
}

void CArea::ProcessIntersectionPoints(Paths64& paths, bool is_closed)
{
    // Process each path
    // For open paths, check z-order ensure that paths come in order
    std::vector<std::pair<int64_t, int64_t>> pathOrder;  // (zMax, dsqMax)
    pathOrder.reserve(paths.size());
    for (Path64& path : paths) {
        pathOrder.push_back({0, 0});
        if (path.empty()) {
            continue;
        }

        // Loop over edges, including wraparound edge if closed
        // For open paths, check z-order to ensure points come in order
        bool needsReversal = false;
        size_t num_edges = is_closed ? path.size() : path.size() - 1;
        for (size_t i = 0; i < num_edges; i++) {
            size_t idx1 = i;
            size_t idx2 = (i + 1) % path.size();

            const Point64& p1 = path[idx1];
            const Point64& p2 = path[idx2];

            // Check if either endpoint is from an intersection
            auto it1 = m_arc_fitting_map.intersections.find(p1.z);
            auto it2 = m_arc_fitting_map.intersections.find(p2.z);
            bool p1_is_new = it1 != m_arc_fitting_map.intersections.end();
            bool p2_is_new = it2 != m_arc_fitting_map.intersections.end();

            if (p1_is_new && p2_is_new) {
                // Both points are intersections - check if they share a parent edge
                const auto& [p1_e1bot, p1_e1top, p1_e2bot, p1_e2top] = it1->second;
                const auto& [p2_e1bot, p2_e1top, p2_e2bot, p2_e2top] = it2->second;

                // Sort each edge by increasing z for easy comparison
                std::pair<int64_t, int64_t> p1_edge1
                    = {std::min(p1_e1bot, p1_e1top), std::max(p1_e1bot, p1_e1top)};
                std::pair<int64_t, int64_t> p1_edge2
                    = {std::min(p1_e2bot, p1_e2top), std::max(p1_e2bot, p1_e2top)};
                std::pair<int64_t, int64_t> p2_edge1
                    = {std::min(p2_e1bot, p2_e1top), std::max(p2_e1bot, p2_e1top)};
                std::pair<int64_t, int64_t> p2_edge2
                    = {std::min(p2_e2bot, p2_e2top), std::max(p2_e2bot, p2_e2top)};

                std::pair<int64_t, int64_t> new_edge = {std::min(p1.z, p2.z), std::max(p1.z, p2.z)};

                std::optional<std::pair<int64_t, int64_t>> sharedEdge;
                if (p1_edge1 == p2_edge1 || p1_edge1 == p2_edge2) {
                    // p1_edge1 matches one of the p2 edges
                    sharedEdge = p1_edge1;
                }
                else if (p1_edge2 == p2_edge1 || p1_edge2 == p2_edge2) {
                    // p1_edge2 matches one of the p2 edges
                    sharedEdge = p1_edge2;
                }

                if (sharedEdge) {
                    auto arc_it = m_arc_fitting_map.arc_centers.find(*sharedEdge);
                    if (arc_it != m_arc_fitting_map.arc_centers.end()) {
                        m_arc_fitting_map.arc_centers[new_edge] = arc_it->second;
                    }

                    if (!is_closed) {
                        // check if the open path needs reversal
                        const Point& pRef = m_arc_fitting_map.point_map[sharedEdge->first];
                        const int64_t p1dsq = (p1.x - pRef.x) * (p1.x - pRef.x)
                            + (p1.y - pRef.y) * (p1.y - pRef.y);
                        const int64_t p2dsq = (p2.x - pRef.x) * (p2.x - pRef.x)
                            + (p2.y - pRef.y) * (p2.y - pRef.y);
                        needsReversal = p1dsq > p2dsq;

                        std::pair<int64_t, int64_t> currentZ = {sharedEdge->first, max(p1dsq, p2dsq)};
                        pathOrder.back() = max(pathOrder.back(), currentZ);
                    }
                }
            }
            else if (p1_is_new || p2_is_new) {
                // One point is new, one is old
                const Point64& p_new = p1_is_new ? p1 : p2;
                const Point64& p_old = p1_is_new ? p2 : p1;
                auto it_new = p1_is_new ? it1 : it2;

                const auto& [p_new_e1bot, p_new_e1top, p_new_e2bot, p_new_e2top] = it_new->second;

                std::pair<int64_t, int64_t> new_edge
                    = {std::min(p_new.z, p_old.z), std::max(p_new.z, p_old.z)};

                std::optional<std::pair<int64_t, int64_t>> sharedEdge;
                if (p_old.z == p_new_e1bot || p_old.z == p_new_e1top) {
                    // p_old.z is an endpoint of edge1
                    sharedEdge
                        = {std::min(p_new_e1bot, p_new_e1top), std::max(p_new_e1bot, p_new_e1top)};
                }
                else if (p_old.z == p_new_e2bot || p_old.z == p_new_e2top) {
                    // p_old.z is an endpoint of edge2
                    sharedEdge
                        = {std::min(p_new_e2bot, p_new_e2top), std::max(p_new_e2bot, p_new_e2top)};
                }

                if (sharedEdge) {
                    auto arc_it = m_arc_fitting_map.arc_centers.find(*sharedEdge);
                    if (arc_it != m_arc_fitting_map.arc_centers.end()) {
                        m_arc_fitting_map.arc_centers[new_edge] = arc_it->second;
                    }

                    if (!is_closed) {
                        // check if the open path needs reversal
                        const Point& pRef = m_arc_fitting_map.point_map[sharedEdge->first];
                        const int64_t p1dsq = (p1.x - pRef.x) * (p1.x - pRef.x)
                            + (p1.y - pRef.y) * (p1.y - pRef.y);
                        const int64_t p2dsq = (p2.x - pRef.x) * (p2.x - pRef.x)
                            + (p2.y - pRef.y) * (p2.y - pRef.y);
                        needsReversal = p1dsq > p2dsq;

                        std::pair<int64_t, int64_t> currentZ = {sharedEdge->first, max(p1dsq, p2dsq)};
                        pathOrder.back() = max(pathOrder.back(), currentZ);
                    }
                }
            }
            else {
                // neither point is new; just check if open path reversal is required
                if (!is_closed) {
                    needsReversal = p1.z > p2.z;

                    std::pair<int64_t, int64_t> currentZ = {max(p1.z, p2.z), 0};
                    pathOrder.back() = max(pathOrder.back(), currentZ);
                }
            }
        }

        if (needsReversal) {
            std::reverse(path.begin(), path.end());
        }
    }

    if (!is_closed) {
        // sort paths based on pathOrder value
        std::vector<std::pair<std::pair<int64_t, int64_t>, Path64>> sorter;
        sorter.reserve(paths.size());
        for (int i = 0; i < paths.size(); i++) {
            sorter.emplace_back(pathOrder[i], std::move(paths[i]));
        }

        std::sort(sorter.begin(), sorter.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        paths.clear();
        for (auto& pair : sorter) {
            paths.push_back(std::move(pair.second));
        }
    }
}
