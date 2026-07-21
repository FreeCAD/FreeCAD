// SPDX-License-Identifier: BSD-3-Clause

// AreaClipper.cpp

// implements CArea methods using Angus Johnson's "Clipper"

#include "Area.h"
#include "clipper2/clipper.h"
#include <algorithm>
#include <cmath>
#include <functional>
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
    std::optional<heeks::Point> prev_pt;
    if (arcMap.z_prev != 0) {
        prev_pt = {arcMap.point_map[arcMap.z_prev]};
    }

    auto setOrientation = [&arcMap](
                              int64_t z,
                              const std::pair<double, double>& dirIn,
                              const std::pair<double, double>& dirOut
                          ) {
        arcMap.orientations[z] = dirIn.first * dirOut.second - dirIn.second * dirOut.first > 0;
    };

    auto addPoint = [&](const PointD& cur_pt, bool push) {
        // Add to the list
        if (push) {
            pts_for_AddVertex.push_back(cur_pt);
        }

        // Update map of point orientations, and update prev* values
        if (prev_pt.has_value()) {
            // Compute orientation of point expansion at prev_pt
            const auto dir = std::make_pair(cur_pt.x - prev_pt->x, cur_pt.y - prev_pt->y);
            if (arcMap.direction_prev) {
                setOrientation(arcMap.z_prev, *(arcMap.direction_prev), dir);
            }

            // Update old values
            arcMap.direction_prev = dir;
            if (!arcMap.direction_initial.has_value()) {
                arcMap.direction_initial = dir;
            }
        }
        arcMap.z_prev = cur_pt.z;
        prev_pt = {{cur_pt.x, cur_pt.y}};
    };

    if (vertex.m_type == 0 || prev_vertex == NULL) {
        // Add new point
        if (!zLoop.has_value()) {
            const int64_t z = arcMap.z_next++;
            arcMap.point_map[z] = vertex.m_p;
            addPoint(PointD(vertex.m_p.x, vertex.m_p.y, z), true);
        }
        else {
            addPoint({arcMap.point_map[*zLoop].x, arcMap.point_map[*zLoop].y, *zLoop}, false);
            setOrientation(*zLoop, *(arcMap.direction_prev), *(arcMap.direction_initial));
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
                        addPoint({arcMap.point_map[*zLoop].x, arcMap.point_map[*zLoop].y, *zLoop}, false);
                        setOrientation(*zLoop, *(arcMap.direction_prev), *(arcMap.direction_initial));
                    }
                    else {
                        const int64_t z = arcMap.z_next++;
                        arcMap.point_map[z] = vertex.m_p;
                        arcMap.arc_centers[{arcMap.z_prev, z}] = vertex.m_c;
                        addPoint(PointD(vertex.m_p.x, vertex.m_p.y, z), true);
                    }
                }
                else {
                    const int64_t z = arcMap.z_next++;
                    const double px = vertex.m_c.x + radius * cos(phi0 + dphi * i);
                    const double py = vertex.m_c.y + radius * sin(phi0 + dphi * i);
                    // Store arc center in point_map for intermediate points
                    arcMap.point_map[z] = vertex.m_c;
                    arcMap.arc_centers[{arcMap.z_prev, z}] = vertex.m_c;
                    addPoint(PointD(px, py, z), true);
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

// Populates the Curve from Path64 data. Returns vector<int> size one smaller than the curve marking
// each edge as z-increasing (1), z-decreasing (-1), or openEnd (0). TODO do I need the value to be
// right when wrapping around? Do I need to start at z-min?
std::vector<int> CArea::_SetFromResult(
    CCurve& curve,
    Path64& path,
    bool is_closed,
    const std::set<int64_t>& openEnds
)
{
    if (CArea::m_clipper_clean_distance >= heeks::Point::tolerance) {
        path = SimplifyPath(path, CArea::m_clipper_clean_distance, is_closed);
    }

    if (path.size() == 0) {
        return {};
    }

    const double max_arc_length = 2 * M_PI * .99;

    // Tracks the orientation of each edge:
    // 0: z is constant and present in openEnds (i.e. remove from open wire offset result)
    // 1: increasing z value, or constant and rotating about the center in the z+ direction
    // -1: decreasing z value, or constant and rotating about the center in the z- direction
    std::vector<int> orientations;

    // Loop through points
    int pe_next_type = 0;
    Point64 prevP64 = {0, 0, -1};  // TODO deduplicate stored info, fix this name
    int64_t prevZ = -1;
    heeks::Point prevP;
    double phi_total = 0.0;
    int num_j = path.size() + (is_closed && path.size() > 1 ? 1 : 0);
    for (int dj = 0; dj < num_j; dj++) {
        const int j = dj % path.size();
        const Point64& pt = path[j];
        PointD dp = ToPointD(pt);
        heeks::Point p(dp.x, dp.y);

        int jnext = (dj + 1) % path.size();
        bool hasNext = (dj + 1 < num_j) || is_closed;
        bool nextGenerated = (dj + 1 == num_j) && is_closed;
        Point64 pt_next = path[jnext];
        PointD dp_next = ToPointD(pt_next);
        heeks::Point p_next(dp_next.x, dp_next.y);

        // Construct ordered pair for arc detection
        auto edgeInfo = prevZ == -1 ? EdgeInfo {{prevZ, pt.z}, 0} : getEdgeInfo(prevP64, pt);
        if (edgeInfo.parentEdge.first == edgeInfo.parentEdge.second
            && openEnds.contains(edgeInfo.parentEdge.first)) {
            edgeInfo.orientation = 0;
        }
        prevP64 = pt;

        // Check if this segment is an arc (presence in arc_centers means it's an arc) TODO need to
        // redo a lot of documentation here with the new getEdgeInfo strategy/semantics
        auto centerIt = m_arc_fitting_map.arc_centers.find(edgeInfo.parentEdge);
        bool isLine = !CArea::m_fit_arcs || (prevZ == -1)
            || ((edgeInfo.parentEdge.first != edgeInfo.parentEdge.second)
                && (centerIt == m_arc_fitting_map.arc_centers.end()));

        if (isLine) {
            curve.m_vertices.emplace_back(0, p, heeks::Point {0, 0});
            if (prevZ != -1) {
                orientations.push_back(edgeInfo.orientation);  // TODO maybe I just want this to be
                                                               // a boolean, orientationMatchesParent
            }

            phi_total = 0.0;
            pe_next_type = 0;  // clear it
        }
        else {
            const bool isPointExpansion = edgeInfo.parentEdge.first == edgeInfo.parentEdge.second;
            heeks::Point center = isPointExpansion
                ? m_arc_fitting_map.point_map.at(edgeInfo.parentEdge.first)
                : centerIt->second;

            double phi0 = atan2(prevP.y - center.y, prevP.x - center.x);
            double phi1 = atan2(p.y - center.y, p.x - center.x);
            double dphi = recenter(phi1 - phi0, -M_PI, 1);  // range: (-M_PI to M_PI]
            int seg_type = (dphi > 0) ? 1 : -1;
            if (pe_next_type != 0) {
                // this is an extension of an existing point expansion; use that type instead
                seg_type = pe_next_type;
                pe_next_type = 0;  // clear it
                phi1 = recenter(phi1, phi0, seg_type);
                dphi = phi1 - phi0;
            }

            // When arcs are discretized, the angle of the edge line doesn't
            // quite match the arc tangent. After offsetting radially outwards,
            // this can result in points generated as an arc about the arc's
            // endpoint that should instead have been part of the expanded arc.
            // Here, we check if that happens in this segment and the following
            // segment and correct for it.

            // If the next segment is the same point expansion, skip this point
            // and process it there instead
            // If this segment is a point expansion and next is an original
            // arc, correct the arc endpoints
            if (isPointExpansion && hasNext) {
                const int pe_type = seg_type;

                if (dj + 1 < num_j && pt_next.z == pt.z) {
                    // Merge with the next point expansion. Save the type, so it can be correctly
                    // determined even if the point expansion covers an angle > M_PI.
                    pe_next_type = pe_type;
                    continue;
                }

                std::pair<int64_t, int64_t> zPairNext(
                    std::min(pt.z, pt_next.z),
                    std::max(pt.z, pt_next.z)
                );
                auto centerNextIt = m_arc_fitting_map.arc_centers.find(zPairNext);

                if (centerNextIt != m_arc_fitting_map.arc_centers.end()
                    && centerNextIt->second != center) {
                    // It is, with original boundary at this point expansion's center point
                    // Consider subsuming some or all of the point expansion into the arc
                    const heeks::Point& arc_center = centerNextIt->second;
                    const heeks::Point& arc_boundary = center;

                    // Point expansion current angles: phi0 to phi1
                    // Arc current angles: phi1 to phi_next
                    // Correct arc boundary: phi_boundary
                    // Compute a new version of phi0, phi1 with respect to the arc center
                    double phi1 = atan2(p.y - arc_center.y, p.x - arc_center.x);
                    const double phi_next = atan2(p_next.y - arc_center.y, p_next.x - arc_center.x);
                    const double dphi_next = recenter(phi_next - phi1, -M_PI, 1);  // range: (-M_PI,
                                                                                   // M_PI]
                    const int arc_type = (dphi_next > 0) ? 1 : -1;

                    const double phi0 = recenter(
                        atan2(prevP.y - arc_center.y, prevP.x - arc_center.x),
                        phi1,
                        -arc_type
                    );
                    phi1 = recenter(phi1, phi_next, -arc_type);
                    const double phi_boundary = recenter(
                        atan2(arc_boundary.y - arc_center.y, arc_boundary.x - arc_center.x),
                        phi1 - M_PI,
                        1
                    );

                    // Compute the arc radius and allowed angular error
                    double dx = p.x - arc_center.x;
                    double dy = p.y - arc_center.y;
                    double arc_radius = sqrt(dx * dx + dy * dy);
                    const double angle_error = CArea::m_accuracy / arc_radius;

                    // centering is such that we have phi0 < ph1 < phi_next (arc_type 1)
                    // or phi0 > phi1 > phi_next (arc_type -1)
                    // always: arc_type * phi0 < arc_type * phi1 < arc_type * phi_next
                    if (arc_type * phi_boundary - angle_error < arc_type * phi0) {
                        // Subsume this point expansion with the subsequent arc
                        if (nextGenerated) {
                            // If it has angular capacity then update its start location to prevP
                            // Otherwise, add a new arc
                            const heeks::Point& p_end = std::next(curve.m_vertices.begin())->m_p;
                            double phi_end = recenter(
                                atan2(p_end.y - arc_center.y, p_end.x - arc_center.x),
                                phi1,
                                arc_type
                            );
                            double arc_span = abs(phi_end - phi0);
                            if (arc_span < max_arc_length) {
                                curve.m_vertices.front().m_p = prevP;  // update arc start location
                            }
                            else {
                                // create a new arc, to avoid making the existing arc too long
                                curve.m_vertices.emplace_back(
                                    arc_type,
                                    curve.m_vertices.front().m_p,
                                    arc_center
                                );
                                orientations.push_back(orientations[0]);
                            }
                        }
                        else {
                            // no special handling required; skipping the point expansion will
                            // correctly result in extending the arc to the start of point expansion
                        }
                        continue;
                    }
                    else if (arc_type * phi_boundary + angle_error < arc_type * phi1) {
                        // Replace part of this point expansion with the next arc
                        p.x = arc_center.x + arc_radius * cos(phi_boundary);
                        p.y = arc_center.y + arc_radius * sin(phi_boundary);
                        if (nextGenerated) {
                            // If it has angular capacity then update its start location to p
                            // Otherwise, add a new arc
                            const heeks::Point& p_end = std::next(curve.m_vertices.begin())->m_p;
                            double phi_end = recenter(
                                atan2(p_end.y - arc_center.y, p_end.x - arc_center.x),
                                phi1,
                                arc_type
                            );
                            double arc_span = phi_end - phi_boundary;

                            curve.m_vertices.emplace_back(pe_type, p, center);  // generate partial
                                                                                // point expansion
                            orientations.push_back(edgeInfo.orientation);
                            if (arc_span < max_arc_length) {
                                curve.m_vertices.front().m_p = p;  // update arc start location
                            }
                            else {
                                curve.m_vertices.emplace_back(
                                    arc_type,
                                    curve.m_vertices.front().m_p,
                                    arc_center
                                );  // make a new arc
                                orientations.push_back(orientations[0]);
                            }
                            continue;
                        }
                    }
                    else {
                        // No special action required; full point expansion is correct
                    }
                }
            }

            // If this segment is an original arc and next is a point
            // expansion, correct the arc endpoints
            if (!isPointExpansion && hasNext) {
                const int arc_type = seg_type;

                if (pt_next.z == pt.z && m_arc_fitting_map.point_map.at(pt_next.z) != center) {
                    // It is. The original arc boundary is at the point expansion's center
                    const heeks::Point& arc_boundary = m_arc_fitting_map.point_map.at(pt_next.z);

                    // Arc current angles: phi0 to phi1 (no recompute required)
                    // Point expansion current angles: phi1 to phi_next
                    // Correct arc boundary: phi_boundary
                    double phi_next
                        = recenter(atan2(p_next.y - center.y, p_next.x - center.x), phi0, arc_type);
                    double phi_boundary = recenter(
                        atan2(arc_boundary.y - center.y, arc_boundary.x - center.x),
                        phi0,
                        arc_type
                    );

                    // Compute the arc radius and allowed angular error
                    double dx = p.x - center.x;
                    double dy = p.y - center.y;
                    double radius = sqrt(dx * dx + dy * dy);
                    const double angle_error = CArea::m_accuracy / radius;

                    // If the point expansion is fully subsumed
                    //   and there are more points to process
                    //   and those points are an extension of the same point expansion
                    // Then skip the point, to process the extended point expansion instead
                    while ((arc_type * phi_next < arc_type * phi_boundary + angle_error)
                           && (dj + 2 < num_j || (dj + 2 == num_j && is_closed))
                           && (path[(dj + 2) % path.size()].z == pt.z)) {
                        // skip the next element, and update p_next, phi_next, and nextGeneraed
                        // accordingly
                        dj++;
                        jnext = (dj + 1) % path.size();
                        nextGenerated = (dj + 1 == num_j) && is_closed;
                        pt_next = path[jnext];
                        dp_next = ToPointD(pt_next);
                        p_next = {dp_next.x, dp_next.y};
                        if (nextGenerated) {
                            p_next = std::next(curve.m_vertices.begin())->m_p;
                        }
                        phi_next = recenter(
                            atan2(p_next.y - center.y, p_next.x - center.x),
                            phi0,
                            arc_type
                        );
                    }

                    // centering is such that we have phi0 < ph1 < phi_next (arc_type 1)
                    // or phi0 > phi1 > phi_next (arc_type -1)
                    // always: arc_type * phi0 < arc_type * phi1 < arc_type * phi_next
                    if (arc_type * phi_boundary + angle_error > arc_type * phi_next) {
                        // Subsume the subsequent point expansion with this arc
                        p = p_next;
                        dphi = phi_next - phi0;
                        if (nextGenerated) {
                            // Delete it, and update start location for the next move
                            curve.m_vertices.front().m_p = p;
                            curve.m_vertices.erase(std::next(curve.m_vertices.begin()));
                            orientations.erase(orientations.begin());
                        }
                        else {
                            // Skip the point expansion that comes next
                            dj++;
                        }
                    }
                    else if (arc_type * phi_boundary - angle_error > arc_type * phi1) {
                        // Replace part of the subsequent point expansion with this arc
                        p.x = center.x + radius * cos(phi_boundary);
                        p.y = center.y + radius * sin(phi_boundary);
                        dphi = phi_boundary - phi0;
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

            if (curve.m_vertices.size() > 0 && curve.m_vertices.back().m_type == seg_type
                && curve.m_vertices.back().m_c == center && abs(phi_total + dphi) <= max_arc_length) {
                // Extend the previous CVertex arc
                curve.m_vertices.back().m_p = p;
                phi_total += dphi;
            }
            else {
                // Add a new CVertex for the arc
                curve.m_vertices.emplace_back(seg_type, p, center);
                orientations.push_back(edgeInfo.orientation);
                phi_total = dphi;
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
                orientations.pop_back();
            }
        }
    }

    return orientations;
}

std::vector<std::vector<int>> CArea::_SetFromResult(
    Paths64& pp,                       // clipper data to put in the area
    bool is_closed,                    // flag if the clipper paths are closed
    bool clear_area,                   // flag for clearing the area's curves before populating
    bool clear_arc_map,                // flag for clearing arc metadata when done
    const std::set<int64_t>& openEnds  // z-values of open wire endpoints
)
{
    // delete existing geometry
    if (clear_area) {
        m_curves.clear();
    }

    // Process intersection points before reconstructing curves
    ProcessIntersectionPoints(pp, is_closed);

    std::vector<std::vector<int>> result;
    for (unsigned int i = 0; i < pp.size(); i++) {
        Path64& p = pp[i];

        m_curves.emplace_back();
        CCurve& curve = m_curves.back();
        result.push_back(_SetFromResult(curve, p, is_closed, openEnds));
    }

    // Reset arc fitting map to ensure clean state
    if (clear_arc_map) {
        m_arc_fitting_map = ArcFittingMap();
    }

    return result;
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
    _SetFromResult(
        closed_paths,
        /*is_closed=*/true,
        /*clear_area=*/true,
        /*clear_arc_map=*/false
    );

    // Append open paths to result
    _SetFromResult(
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
    _SetFromResult(
        closed_paths,
        /*is_closed=*/true,
        /*clear_area=*/true,
        /*clear_arc_map=*/false
    );

    // Append open paths to result
    _SetFromResult(
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

// TODO rename to OffsetClosed? Or is it assumed? Definitely document it
void CArea::Offset(double offset, JoinType joinType, EndType unused, double miterLimit, double arcTolerance)
{
    _Offset(offset, joinType, EndType::Polygon, miterLimit, arcTolerance);
    this->Reorder();
}

CArea CArea::OpenOffset(double offset, double arcTolerance)
{
    auto orientations = _Offset(fabs(offset), JoinType::Round, EndType::Round, 0.0, arcTolerance);

    std::list<CCurve> positive;
    std::list<CCurve> negative;

    auto curveIt = m_curves.begin();
    for (const auto& curveOrientations : orientations) {
        CCurve& curve = *curveIt++;
        const std::vector<CVertex> verts(curve.m_vertices.begin(), curve.m_vertices.end());

        // Find start index: first edge where the previous edge (wrapping) has a different
        // orientation, so we always begin a new wire at a transition. Fall back to 0 if all
        // orientations match.
        //
        // For open curves, always start at 0 since there is no wrap-around.
        int n = (int)curveOrientations.size();
        int iEdgeStart = 0;
        if (curve.IsClosed()) {
            for (int i = 0; i < n; ++i) {
                if (curveOrientations[(i - 1 + n) % n] != curveOrientations[i]) {
                    iEdgeStart = i;
                    break;
                }
            }
        }

        CCurve* currentWire = NULL;
        int prevOrientation = 0;
        heeks::Point prevP = verts[iEdgeStart].m_p;

        for (int edgeOffset = 0; edgeOffset < n; ++edgeOffset) {
            int iEdge = (iEdgeStart + edgeOffset) % n;
            const CVertex& v = verts[iEdge + 1];
            int orientation = curveOrientations[iEdge];

            // If the orientation changes, start a new wire
            if (orientation != prevOrientation) {
                if (orientation == 1) {
                    positive.emplace_back();
                    currentWire = &positive.back();
                    currentWire->m_vertices.emplace_back(0, prevP, heeks::Point {0, 0});
                }
                else if (orientation == -1) {
                    negative.emplace_back();
                    currentWire = &negative.back();
                    currentWire->m_vertices.emplace_back(0, prevP, heeks::Point {0, 0});
                }
                else {
                    currentWire = NULL;
                }
            }

            // Add the current vertex to the wire
            if (currentWire) {
                currentWire->m_vertices.push_back(v);
            }

            // Update prev values
            prevP = v.m_p;
            prevOrientation = orientation;
        }
    }

    // We've accumulated all our negative oriented wires in the negative direction, but for open
    // path offset we actually want them traced out in the forwards direction
    for (CCurve& c : negative) {
        c.Reverse();
    }

    // Return results
    CArea result;
    if (offset >= 0) {
        m_curves = std::move(positive);
        result.m_curves = std::move(negative);
    }
    else {
        m_curves = std::move(negative);
        result.m_curves = std::move(positive);
    }
    return result;
}

std::vector<std::vector<int>> CArea::_Offset(
    double offset,
    JoinType joinType,
    EndType endType,
    double miterLimit,
    double arcTolerance
)
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
    std::set<int64_t> openEnds;

    // Add paths with appropriate end types
    int i = 0;
    for (const CCurve& c : m_curves) {
        if (c.IsClosed()) {
            closedPaths.push_back(pp[i]);
        }
        else {
            clipper.AddPath(pp[i], joinType, endType);
            openEnds.insert(pp[i][0].z);
            openEnds.insert(pp[i][pp[i].size() - 1].z);
        }
        i++;
    }
    clipper.AddPaths(closedPaths, joinType, endType);

    // Execute offset
    Paths64 pp2;
    clipper.Execute(offset, pp2);

    return _SetFromResult(pp2, /*is_closed=*/true, /*clear_area=*/true, /*clear_arc_map=*/true, openEnds);
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
    // If pt exactly matches one of the source points in x and y, give it the same z
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
    else {
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

// rename: get edge data?
// return parent edge <int, int>
// return orientation (parent orientation? parent z-increasing?) (1 = moving in the same direction
// as the parent, -1 opposite)
//   - note: for point expansions (equal z), positive offsets always create positive-movement arcs
//   around points, and negative offsets create negative movement. Interesting? I think I can just
//   classify based on change in angle since this is only used for positive offsets
//   - note: orientation is assumed to be positive with increasing z, but when completing a loop z
//   drops back to the start...I only need this for open path offset, where the input is always open
//   so I should be able to ignore that, but documentation needs to be clear
// TODO write/update/fix documentation here
CArea::EdgeInfo CArea::getEdgeInfo(const Point64& p1, const Point64& p2)
{
    const PointD dp1 = ToPointD(p1);
    const PointD dp2 = ToPointD(p2);

    if (p1.z == p2.z) {
        int orientation = m_arc_fitting_map.orientations[p1.z] ? 1 : -1;
        return {{p1.z, p1.z}, orientation};
    }

    auto it1 = m_arc_fitting_map.intersections.find(p1.z);
    auto it2 = m_arc_fitting_map.intersections.find(p2.z);
    bool p1_is_new = it1 != m_arc_fitting_map.intersections.end();
    bool p2_is_new = it2 != m_arc_fitting_map.intersections.end();

    std::optional<std::pair<int64_t, int64_t>> parentEdge;
    if (p1_is_new && p2_is_new) {
        // Both points are intersection points, splitting their common parent edge
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

        if (p1_edge1 == p2_edge1 || p1_edge1 == p2_edge2) {
            parentEdge = p1_edge1;
        }
        else if (p1_edge2 == p2_edge1 || p1_edge2 == p2_edge2) {
            parentEdge = p1_edge2;
        }
    }
    else if (p1_is_new || p2_is_new) {
        // One point splits the parent edge, and the other is at the end of the shared edge
        const Point64& p_new = p1_is_new ? p1 : p2;
        const Point64& p_old = p1_is_new ? p2 : p1;
        auto it_new = p1_is_new ? it1 : it2;

        const auto& [p_new_e1bot, p_new_e1top, p_new_e2bot, p_new_e2top] = it_new->second;

        std::pair<int64_t, int64_t> new_edge = {std::min(p_new.z, p_old.z), std::max(p_new.z, p_old.z)};

        if (p_old.z == p_new_e1bot || p_old.z == p_new_e1top) {
            parentEdge = {std::min(p_new_e1bot, p_new_e1top), std::max(p_new_e1bot, p_new_e1top)};
        }
        else if (p_old.z == p_new_e2bot || p_old.z == p_new_e2top) {
            parentEdge = {std::min(p_new_e2bot, p_new_e2top), std::max(p_new_e2bot, p_new_e2top)};
        }
    }
    else {
        // Points are at either end of the parent edge
        parentEdge = {std::min(p1.z, p2.z), std::max(p1.z, p2.z)};
    }

    if (parentEdge) {
        // Look up the mm-space position of the "first" (lower-z) endpoint of the parent edge
        const heeks::Point& edgeStart = m_arc_fitting_map.point_map[parentEdge->first];

        double dx1 = dp1.x - edgeStart.x;
        double dy1 = dp1.y - edgeStart.y;
        double distsq1 = dx1 * dx1 + dy1 * dy1;

        double dx2 = dp2.x - edgeStart.x;
        double dy2 = dp2.y - edgeStart.y;
        double distsq2 = dx2 * dx2 + dy2 * dy2;

        int orientation = distsq2 > distsq1 ? 1 : -1;
        return {*parentEdge, orientation};
    }

    return {{-2, -3}, 0};  // this should not happen; parent edge should always exist
}

// For any intersection points created, splitting an original edge, update
// the arc fitting data structure with appropriate arc metadata for their edges
//
// Also, for open paths, reorder as needed to produce positively oriented and positively ordered paths
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

            if (p1_is_new || p2_is_new) {
                auto edgeInfo = getEdgeInfo(p1, p2);
                auto parentEdge = edgeInfo.parentEdge;
                std::pair<int64_t, int64_t> new_edge = {std::min(p1.z, p2.z), std::max(p1.z, p2.z)};

                auto arc_it = m_arc_fitting_map.arc_centers.find(parentEdge);
                if (arc_it != m_arc_fitting_map.arc_centers.end()) {
                    // m_arc_fitting_map.arc_centers[new_edge] = arc_it->second;
                    // TODO do we need to update the arc fitting map? I might change SetFromResult
                    // instead to do the getEdgeInfo lookup, such that we can check
                    // arc_centers[parentEdge] instead
                }

                if (!is_closed) {
                    // check if the open path needs reversal
                    const Point& pRef = m_arc_fitting_map.point_map[parentEdge.first];
                    const int64_t p1dsq = (p1.x - pRef.x) * (p1.x - pRef.x)
                        + (p1.y - pRef.y) * (p1.y - pRef.y);
                    const int64_t p2dsq = (p2.x - pRef.x) * (p2.x - pRef.x)
                        + (p2.y - pRef.y) * (p2.y - pRef.y);
                    needsReversal = p1dsq > p2dsq;

                    std::pair<int64_t, int64_t> currentZ = {parentEdge.first, max(p1dsq, p2dsq)};
                    pathOrder.back() = max(pathOrder.back(), currentZ);
                }
            }
            else {
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
