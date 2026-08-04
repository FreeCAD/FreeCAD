// SPDX-License-Identifier: BSD-3-Clause

// AreaClipper.cpp

// implements CArea methods using Angus Johnson's "Clipper"

#include "Area.h"
#include "clipper2/clipper.h"
#include <algorithm>
#include <cmath>
#include <functional>
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

    std::cerr << "\nMakePoly (" << pts_for_AddVertex.size() << " points):\n";
    p.resize(pts_for_AddVertex.size());
    unsigned int i = 0;
    for (std::list<PointD>::iterator It = pts_for_AddVertex.begin(); It != pts_for_AddVertex.end();
         It++, i++) {
        p[i] = ToPoint64(*It);
        std::cerr << "  MakePoly[" << i << "] (" << p[i].x << ", " << p[i].y << ") z=" << p[i].z
                  << "\n";
    }
}

void CArea::MakePolyPoly(Paths64& pp)
{
    pp.clear();
    for (const CCurve& curve : m_curves) {
        pp.push_back(_MakePoly(curve));
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
    std::cerr << "\n\n_SetFromResult\n";

    if (CArea::m_clipper_clean_distance >= heeks::Point::tolerance) {
        path = SimplifyPath(path, CArea::m_clipper_clean_distance, is_closed);
    }

    if (path.size() == 0) {
        return {};
    }

    const double max_arc_length = 2 * M_PI * .99;  //(7./8);

    // Tracks the orientation of each edge:
    // 0: z is constant and present in openEnds (i.e. remove from open wire offset result)
    // 1: increasing z value, or constant and rotating about the center in the z+ direction
    // -1: decreasing z value, or constant and rotating about the center in the z- direction
    std::vector<int> orientations;

    // Loop through points
    int pe_next_type = 0;
    Point64 prevP64 = {0, 0, -1};  // TODO deduplicate stored info, fix this name
    int64_t prevZ = -1;            // TODO I think prevZ is only used now to check if it's -1,
                                   // simplify/rename/fix this
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

        std::cerr << "pt=(" << pt.x << ", " << pt.y << ", " << pt.z << ")\n";

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
        bool cond_no_fit_arcs = !CArea::m_fit_arcs;
        bool cond_no_prevZ = (prevZ == -1);
        bool cond_z_changed = (prevZ != pt.z);
        bool cond_not_in_map = (centerIt == m_arc_fitting_map.arc_centers.end());
        bool isLine = !CArea::m_fit_arcs || (prevZ == -1)
            || ((edgeInfo.parentEdge.first != edgeInfo.parentEdge.second)
                && (centerIt == m_arc_fitting_map.arc_centers.end()));

        std::cerr << "  [isLine=" << isLine << "]"
                  << " | !m_fit_arcs=" << cond_no_fit_arcs << " | prevZ==-1(" << cond_no_prevZ
                  << ", prevZ=" << prevZ << ")"
                  << " | prevZ!=pt.z(" << cond_z_changed << ", pt.z=" << pt.z << ")"
                  << " | not_in_arc_map=" << cond_not_in_map << " | edge=("
                  << edgeInfo.parentEdge.first << "," << edgeInfo.parentEdge.second << ")"
                  << "\n";

        if (isLine) {
            curve.m_vertices.emplace_back(0, p, heeks::Point {0, 0});
            if (prevZ != -1) {
                orientations.push_back(edgeInfo.orientation);  // TODO maybe I just want this to be
                                                               // a boolean, orientationMatchesParent
                std::cerr << "[orientation=" << edgeInfo.orientation << "] push-line"
                          << " vertex: type=" << curve.m_vertices.back().m_type << " p=("
                          << curve.m_vertices.back().m_p.x << "," << curve.m_vertices.back().m_p.y
                          << ")"
                          << " c=(" << curve.m_vertices.back().m_c.x << ","
                          << curve.m_vertices.back().m_c.y << ")\n\n";
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
                const auto nextEdgeInfo = getEdgeInfo(pt, pt_next);

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
                                std::cerr << "[orientation=" << orientations.back()
                                          << "] push-PE-subsume-arc-overflow"
                                          << " vertex: type=" << curve.m_vertices.back().m_type
                                          << " p=(" << curve.m_vertices.back().m_p.x << ","
                                          << curve.m_vertices.back().m_p.y << ")"
                                          << " c=(" << curve.m_vertices.back().m_c.x << ","
                                          << curve.m_vertices.back().m_c.y << ")\n\n";
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
                            std::cerr
                                << "[orientation=" << edgeInfo.orientation << "] push-partial-PE"
                                << " vertex: type=" << curve.m_vertices.back().m_type << " p=("
                                << curve.m_vertices.back().m_p.x << ","
                                << curve.m_vertices.back().m_p.y << ")"
                                << " c=(" << curve.m_vertices.back().m_c.x << ","
                                << curve.m_vertices.back().m_c.y << ")\n\n";
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
                                std::cerr << "[orientation=" << orientations.back()
                                          << "] push-arc-overflow"
                                          << " vertex: type=" << curve.m_vertices.back().m_type
                                          << " p=(" << curve.m_vertices.back().m_p.x << ","
                                          << curve.m_vertices.back().m_p.y << ")"
                                          << " c=(" << curve.m_vertices.back().m_c.x << ","
                                          << curve.m_vertices.back().m_c.y << ")\n\n";
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
                const auto nextEdgeInfo = getEdgeInfo(pt, pt_next);

                if (nextEdgeInfo.parentEdge.first == nextEdgeInfo.parentEdge.second
                    && m_arc_fitting_map.point_map.at(nextEdgeInfo.parentEdge.first) != center) {
                    // It is. The original arc boundary is at the point expansion's center
                    const heeks::Point& arc_boundary = m_arc_fitting_map.point_map.at(
                        nextEdgeInfo.parentEdge.first
                    );

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
                           && (dj + 2 < num_j || (dj + 2 == num_j && is_closed))) {
                        std::cerr << "    [skip-loop] enter: dj=" << dj << " jnext=" << jnext
                                  << " phi_next=" << phi_next << " phi_boundary=" << phi_boundary
                                  << " p_next=(" << p_next.x << "," << p_next.y << ")"
                                  << " nextGenerated=" << nextGenerated << "\n";
                        const auto nextNextEdgeInfo
                            = getEdgeInfo(pt_next, path[(dj + 2) % path.size()]);
                        if (nextNextEdgeInfo.parentEdge.first != nextNextEdgeInfo.parentEdge.second) {
                            std::cerr << "    [skip-loop] break: nextNextEdgeInfo parentEdge "
                                         "crosses segments"
                                      << " (" << nextNextEdgeInfo.parentEdge.first << ","
                                      << nextNextEdgeInfo.parentEdge.second << ")\n";
                            break;
                        }

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
                        std::cerr << "    [skip-loop] skipped: new dj=" << dj << " jnext=" << jnext
                                  << " phi_next=" << phi_next << " p_next=(" << p_next.x << ","
                                  << p_next.y << ")"
                                  << " nextGenerated=" << nextGenerated << "\n";
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
                            std::cerr << "  [orientation] erase-front new-front vertex: type="
                                      << curve.m_vertices.front().m_type << " p=("
                                      << curve.m_vertices.front().m_p.x << ","
                                      << curve.m_vertices.front().m_p.y << ")"
                                      << " c=(" << curve.m_vertices.front().m_c.x << ","
                                      << curve.m_vertices.front().m_c.y << ")\n\n";
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
                std::cerr << "[orientation=" << edgeInfo.orientation << "] push-arc"
                          << " vertex: type=" << curve.m_vertices.back().m_type << " p=("
                          << curve.m_vertices.back().m_p.x << "," << curve.m_vertices.back().m_p.y
                          << ")"
                          << " c=(" << curve.m_vertices.back().m_c.x << ","
                          << curve.m_vertices.back().m_c.y << ")\n\n";
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
                std::cerr << "  [orientation] pop-back-arc-merge new-back vertex: type="
                          << curve.m_vertices.back().m_type << " p=("
                          << curve.m_vertices.back().m_p.x << "," << curve.m_vertices.back().m_p.y
                          << ")"
                          << " c=(" << curve.m_vertices.back().m_c.x << ","
                          << curve.m_vertices.back().m_c.y << ")\n\n";
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

void CArea::PopulateClipper(Clipper64& c, bool as_clip)
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

        Path64 p = _MakePoly(curve);

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
    PopulateClipper(c, false);
    // TODO FIXME: copying clip_area just to share m_arc_fitting_map is wasteful and ugly;
    // something else should be done
    CArea clip_area_copy = clip_area;
    clip_area_copy.m_arc_fitting_map = m_arc_fitting_map;
    clip_area_copy.PopulateClipper(c, true);
    m_arc_fitting_map = clip_area_copy.m_arc_fitting_map;

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

    m_curves.clear();
    __SetFromResult(closed_paths, /*is_closed=*/true);
    __SetFromResult(open_paths, /*is_closed=*/false);
    m_arc_fitting_map = {};
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
        Path64 p = _MakePoly(curve);

        if (is_closed) {
            closed_paths.push_back(p);
        }
        else {
            open_paths.push_back(p);
        }
    }

    m_curves.clear();
    __SetFromResult(closed_paths, /*is_closed=*/true);
    __SetFromResult(open_paths, /*is_closed=*/false);
    m_arc_fitting_map = {};
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

// Creates the naive offset of curves by offsetting each segment by +-offset
// TODO internal tagging specifies which output segments come from the positive vs negative offset
// If the path isn't closed, round end caps are generated, tagged as end caps
// Adjacent segments are either joined round or by a segment, depending on end direction (TODO write
// this better)
//
// TODO I want to be sure I have a test where the negative offset completely collapses
// TODO check that it's important that input curves are positively oriented (if closed) and document
// that. Uh, probably in the outer offset function; it's important for the union operation not the
// naive offset
// TODO document the expected context/purpose of this function, document the return value
// TODO reimplement OpenOffset using this
// TODO reimplement normal/closed offset using this. Note that other join types are not supported,
// but I think we never use them?
// TODO consider adding join type, for "use" in the Profile op
std::list<std::list<int>> CArea::NaiveOffset(double offset, double arcTolerance)
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
    std::list<std::list<int>> edge_directions;

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
            edge_directions.push_back(directions);

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
            // TODO I'd rather have a cleaner check than this, I should only need to do the
            // conversion from doubles to clipper once
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
                std::cerr << "[NaiveOffset] join(skip): targets match current endpoints in Point64 "
                             "space\n";
                return;
            }

            int joinType = 0;  // 0 = no join, 1 = positive arc, -1 = negative arc

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
                // Check which it is, parallel or anti-parallel. If parallel the no join is required
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
                edge_directions.push_back(directions);
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
                edge_directions.push_back(directions);
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
            edge_directions.push_back(directions);
        }
    }

    m_curves = std::move(offset_curves);
    return edge_directions;
}

// TODO deduplicate this with existing MakePoly function
// TODO document this function properly
// If edge_orientations is empty, all edges are assigned orientation 1. This allows _MakePoly to
// be used in code paths that don't go through NaiveOffset (which is the only producer of
// edge_orientations), so that zData gets populated and getEdgeInfo works correctly.
Path64 CArea::_MakePoly(const CCurve& curve, std::list<int> edge_orientations)
{
    if (!curve.m_vertices.size()) {
        return {};
    }

    Path64 result;
    const int curveIndex = m_arc_fitting_map.nextCurveIndex++;

    std::cerr << "_MakePoly: " << curve.m_vertices.size()
              << " vertices, isClosed=" << curve.IsClosed() << ", " << edge_orientations.size()
              << " orientations\n";
    {
        auto oIt = edge_orientations.cbegin();
        for (auto vIt = curve.m_vertices.cbegin(); vIt != curve.m_vertices.cend(); ++vIt) {
            const auto& v = *vIt;
            if (vIt == curve.m_vertices.cbegin()) {
                std::cerr << "  vertex: type=" << v.m_type << " p=(" << v.m_p.x << "," << v.m_p.y
                          << ") c=(" << v.m_c.x << "," << v.m_c.y << ")\n";
            }
            else {
                const int o = edge_orientations.empty() ? 1 : *oIt++;
                std::cerr << "  vertex: type=" << v.m_type << " p=(" << v.m_p.x << "," << v.m_p.y
                          << ") c=(" << v.m_c.x << "," << v.m_c.y << ") orientation=" << o << "\n";
            }
        }
    }

    auto getPoint64 = [&](double x, double y) -> Point64 {
        const Point64 p64 = ToPoint64(PointD(x, y, 0));
        const auto key = std::make_pair(p64.x, p64.y);
        auto it = m_arc_fitting_map.xy_to_z.find(key);
        if (it != m_arc_fitting_map.xy_to_z.end()) {
            std::cerr << "  point (cached): (" << p64.x << "," << p64.y << ") z=" << it->second
                      << "\n";
            return Point64(p64.x, p64.y, it->second);
        }
        const int64_t z = m_arc_fitting_map.z_next++;
        m_arc_fitting_map.xy_to_z[key] = z;
        return Point64(p64.x, p64.y, z);
    };

    Point64 pPrev = getPoint64(curve.m_vertices.front().m_p.x, curve.m_vertices.front().m_p.y);
    result.push_back(pPrev);
    std::cerr << "  point: (" << pPrev.x << "," << pPrev.y << ") z=" << pPrev.z << "\n";
    auto orientationIt = edge_orientations.cbegin();
    int vertexIndex = 0;

    for (auto vIt = std::next(curve.m_vertices.cbegin()); vIt != curve.m_vertices.cend(); vIt++) {
        const CVertex& vertex = *vIt;
        const bool isLoop = std::next(vIt) == curve.m_vertices.end() && curve.IsClosed()
            && curve.m_vertices.size() > 1;
        const int orientation = edge_orientations.empty() ? 1 : *orientationIt;

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
            if (m_arc_fitting_map.zData.count(key)) {
                std::cerr << "_MakePoly: ERROR (line): zData key (" << key.first << ","
                          << key.second << ") already present, orientation "
                          << m_arc_fitting_map.zData[key].orientation << " -> " << orientation
                          << (m_arc_fitting_map.zData[key].orientation != orientation ? " (CHANGED)"
                                                                                      : " (same)")
                          << "\n";
            }
            m_arc_fitting_map.zData[key] = SegmentData {vertex, orientation, curveIndex, vertexIndex};
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
                std::cerr << "_MakePoly: arc zData[(" << key.first << "," << key.second
                          << ")] orientation=" << orientation << "\n";
                if (m_arc_fitting_map.zData.count(key)) {
                    std::cerr << "_MakePoly: ERROR (arc): zData key (" << key.first << ","
                              << key.second << ") already present, orientation "
                              << m_arc_fitting_map.zData[key].orientation << " -> " << orientation
                              << (m_arc_fitting_map.zData[key].orientation != orientation
                                      ? " (CHANGED)"
                                      : " (same)")
                              << "\n";
                }
                m_arc_fitting_map.zData[key]
                    = SegmentData {vertex, orientation, curveIndex, vertexIndex};
                pPrev = newPt;
            }
        }

        if (!edge_orientations.empty()) {
            orientationIt++;
        }
        vertexIndex++;
    }

    return result;
}


// TODO consider changing implementation so that if cNeg is not provided then orientation is ignored
// ...but also consider if we just did that behavior is zDat is empty/omitted? tbd
void CArea::__SetFromResult(
    Paths64& paths,
    bool isClosed,
    std::optional<std::reference_wrapper<CArea>> cNeg
)
{
    std::cerr << "__SetFromResult: " << paths.size() << " paths, isClosed=" << isClosed
              << ", m_arc_fitting_map.zData.size()=" << m_arc_fitting_map.zData.size() << "\n";
    if (!isClosed) {
        ReorderOpenPaths(paths);
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
        // Keep track of the current curve and its orientation. Also track the original curve and
        // its orientation so the final curve (if different) can be joined to it (if the path is
        // closed and orientation matches)
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
            // TODO actually we only need the parentEdge, so delete excess code from this function
            // and rename it
            const auto parentEdge = getEdgeInfo(v0, v1).parentEdge;
            const auto zIt = m_arc_fitting_map.zData.find(parentEdge);
            const bool inZData = zIt != m_arc_fitting_map.zData.end();
            static const SegmentData kMissingSegment {};
            const SegmentData& parentData = inZData ? zIt->second : kMissingSegment;

            std::cerr << "  edge[" << edgeNum << "]: z=(" << v0.z << "," << v1.z << ") parentEdge=("
                      << parentEdge.first << "," << parentEdge.second << ") inZData=" << inZData
                      << " orientation=" << parentData.orientation << "\n";
            if (!inZData) {
                std::cerr << "\n\n    ERROR MISSING Z DATA! \n\n\n";
            }

            // Check if the orientation changed; if it did, end the curve
            if (parentData.orientation != orientation) {
                saveCurve();
                c.m_vertices.clear();
            }
            orientation = parentData.orientation;

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

// TODO deduplicate this with existing Union/Clip function
void CArea::_Union(
    const std::list<std::list<int>>& orientations,
    std::optional<std::reference_wrapper<CArea>> cNeg
)
{
    Clipper64 c;
    c.SetZCallback(MakeZCallback());

    // PopulateClipper(c, false, m_arc_fitting_map);
    {
        Paths64 closed_paths;
        Paths64 open_paths;
        int skipped = 0;
        bool as_clip = false;

        auto orientationIt = orientations.cbegin();
        for (const CCurve& curve : m_curves) {
            bool is_closed = curve.IsClosed();

            if (!is_closed && as_clip) {
                ++skipped;
                continue;
            }

            Path64 p = _MakePoly(curve, *orientationIt);

            if (is_closed) {
                closed_paths.push_back(p);
            }
            else {
                open_paths.push_back(p);
            }

            orientationIt++;
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

    // Execute to get both closed and open paths
    Paths64 closed_paths;
    Paths64 open_paths;
    c.Execute(ClipType::Union, FillRule::Positive, closed_paths, open_paths);

    // Convert clipper paths back to CArea; clean up clipper path metadata
    m_curves.clear();
    __SetFromResult(closed_paths, /*is_closed=*/true, cNeg);
    __SetFromResult(open_paths, /*is_closed=*/false, cNeg);
    m_arc_fitting_map = {};
}

// TODO rename to OffsetClosed? Or is it assumed? Definitely document it
void CArea::Offset(double offset, JoinType joinType, EndType unused, double miterLimit, double arcTolerance)
{
    if (offset == 0) {
        return;
    }

    // Perform the naive offset, offsetting each edge and joining
    std::list<std::list<int>> edge_directions = NaiveOffset(abs(offset), arcTolerance);

    // If we want to keep the negative edges, flip all the edge labels
    if (offset < 0) {
        for (auto& curve_dirs : edge_directions) {
            for (auto& dir : curve_dirs) {
                dir = -dir;
            }
        }
    }

    // Union (fill rule positive), keeping positive edges and dropping negative edges
    _Union(edge_directions);

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
    std::list<std::list<int>> edge_directions = NaiveOffset(abs(offset), arcTolerance);

    // Union (fill rule positive), separating out the positive and negative edges
    _Union(edge_directions, std::ref(cNeg));

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
    std::cerr << "_Offset arcTolerance (scaled)=" << arcTolerance << "\n";

    ClipperOffset clipper(miterLimit, arcTolerance);
    clipper.SetZCallback(MakeZCallback());

    Paths64 pp;
    MakePolyPoly(pp);

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
    // Perform the naive offset, offsetting each edge and joining
    std::list<std::list<int>> edge_directions = NaiveOffset(abs(value));

    // We want to keep all offset curves, so clear the edge tags
    for (auto& curve_dirs : edge_directions) {
        curve_dirs.clear();
    }

    // Union (fill rule positive), keeping positive edges and dropping negative edges
    _Union(edge_directions);
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

    // Add to all_intersections map
    const int64_t e1min = std::min(e1bot.z, e1top.z);
    const int64_t e1max = std::max(e1bot.z, e1top.z);
    const int64_t e2min = std::min(e2bot.z, e2top.z);
    const int64_t e2max = std::max(e2bot.z, e2top.z);
    m_arc_fitting_map.all_intersections.insert({pt.z, std::make_tuple(e1min, e1max, e2min, e2max)});
    std::cerr << "[intersection] pt=(" << pt.x << "," << pt.y << "," << pt.z << ")"
              << " e1bot=(" << e1bot.x << "," << e1bot.y << "," << e1bot.z << ")"
              << " e1top=(" << e1top.x << "," << e1top.y << "," << e1top.z << ")"
              << " e2bot=(" << e2bot.x << "," << e2bot.y << "," << e2bot.z << ")"
              << " e2top=(" << e2top.x << "," << e2top.y << "," << e2top.z << ")" << "\n";
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
        // TODO update behavior/documentation: this should be impossible once I fully switch over to
        // the NaiveOffset implementation of offsetting
        int orientation = m_arc_fitting_map.orientations[p1.z] ? 1 : -1;
        return {{p1.z, p1.z}, orientation};
    }

    // Helper for checking if a pair of z values is a known edge
    auto tryEdge = [this](int64_t z1, int64_t z2) {
        if (z2 < z1) {
            std::swap(z1, z2);
        }

        std::pair<int64_t, int64_t> key = {z1, z2};
        if (m_arc_fitting_map.zData.count(key)) {
            return std::optional<EdgeInfo> {{key, m_arc_fitting_map.zData[key].orientation}};
        }

        return std::optional<EdgeInfo> {};
    };

    // Check for a direct edge p1.z to p2.z
    auto info = tryEdge(p1.z, p2.z);
    if (info) {
        return *info;
    }

    // Check for an edge from p1.z to the intersection log of p2,
    // or from p2.z to the intersection log of z1
    auto z1its = m_arc_fitting_map.all_intersections.equal_range(p1.z);
    for (auto z1it = z1its.first; z1it != z1its.second; z1it++) {
        const auto& [e1min, e1max, e2min, e2max] = z1it->second;
        if (p2.z == e1min || p2.z == e1max) {
            info = tryEdge(e1min, e1max);
            if (info) {
                return *info;
            }
        }
        if (p2.z == e2min || p2.z == e2max) {
            info = tryEdge(e2min, e2max);
            if (info) {
                return *info;
            }
        }
    }

    auto z2its = m_arc_fitting_map.all_intersections.equal_range(p2.z);
    for (auto z2it = z2its.first; z2it != z2its.second; z2it++) {
        const auto& [e1min, e1max, e2min, e2max] = z2it->second;
        if (p1.z == e1min || p1.z == e1max) {
            info = tryEdge(e1min, e1max);
            if (info) {
                return *info;
            }
        }
        if (p1.z == e2min || p1.z == e2max) {
            info = tryEdge(e2min, e2max);
            if (info) {
                return *info;
            }
        }
    }

    // Check for any shared edge in the intersection logs of p1 and p2
    for (auto z1it = z1its.first; z1it != z1its.second; z1it++) {
        const auto& [e1min, e1max, e2min, e2max] = z1it->second;
        for (auto z2it = z2its.first; z2it != z2its.second; z2it++) {
            const auto& [e3min, e3max, e4min, e4max] = z2it->second;
            if ((e1min == e3min && e1max == e3max) || (e1min == e4min && e1max == e4max)) {
                info = tryEdge(e1min, e1max);
                if (info) {
                    return *info;
                }
            }
            if ((e2min == e3min && e2max == e3max) || (e2min == e4min && e2max == e4max)) {
                info = tryEdge(e2min, e2max);
                if (info) {
                    return *info;
                }
            }
        }
    }

    // Failed to find the parent edge. This should not happen; parent edge should always exist
    std::cerr << "    ERROR: no parent edge found for z=(" << p1.z << "," << p2.z << ")"
              << " hits=(" << m_arc_fitting_map.all_intersections.count(p1.z) << ","
              << m_arc_fitting_map.all_intersections.count(p2.z) << "), returning sentinel\n";
    return {{-2, -3}, 0};

    // auto it1 = m_arc_fitting_map.intersections.find(p1.z);
    // auto it2 = m_arc_fitting_map.intersections.find(p2.z);
    // bool p1_is_new = it1 != m_arc_fitting_map.intersections.end();
    // bool p2_is_new = it2 != m_arc_fitting_map.intersections.end();

    // std::optional<std::pair<int64_t, int64_t>> parentEdge;

    // if (p1_is_new && p2_is_new) {
    //     // Both points are intersection points, splitting their common parent edge
    //     const auto& [p1_e1bot, p1_e1top, p1_e2bot, p1_e2top] = it1->second;
    //     const auto& [p2_e1bot, p2_e1top, p2_e2bot, p2_e2top] = it2->second;


    //     // Sort each edge by increasing z for easy comparison
    //     std::pair<int64_t, int64_t> p1_edge1
    //         = {std::min(p1_e1bot, p1_e1top), std::max(p1_e1bot, p1_e1top)};
    //     std::pair<int64_t, int64_t> p1_edge2
    //         = {std::min(p1_e2bot, p1_e2top), std::max(p1_e2bot, p1_e2top)};
    //     std::pair<int64_t, int64_t> p2_edge1
    //         = {std::min(p2_e1bot, p2_e1top), std::max(p2_e1bot, p2_e1top)};
    //     std::pair<int64_t, int64_t> p2_edge2
    //         = {std::min(p2_e2bot, p2_e2top), std::max(p2_e2bot, p2_e2top)};

    //     if (p1_edge1 == p2_edge1 || p1_edge1 == p2_edge2) {
    //         parentEdge = p1_edge1;
    //     }
    //     else if (p1_edge2 == p2_edge1 || p1_edge2 == p2_edge2) {
    //         parentEdge = p1_edge2;
    //     }
    // }
    // else if (p1_is_new || p2_is_new) {
    //     // One point splits the parent edge, and the other is at the end of the shared edge
    //     const Point64& p_new = p1_is_new ? p1 : p2;
    //     const Point64& p_old = p1_is_new ? p2 : p1;
    //     auto it_new = p1_is_new ? it1 : it2;

    //     const auto& [p_new_e1bot, p_new_e1top, p_new_e2bot, p_new_e2top] = it_new->second;

    //     std::pair<int64_t, int64_t> new_edge = {std::min(p_new.z, p_old.z), std::max(p_new.z,
    //     p_old.z)};

    //     if (p_old.z == p_new_e1bot || p_old.z == p_new_e1top) {
    //         parentEdge = {std::min(p_new_e1bot, p_new_e1top), std::max(p_new_e1bot, p_new_e1top)};
    //     }
    //     else if (p_old.z == p_new_e2bot || p_old.z == p_new_e2top) {
    //         parentEdge = {std::min(p_new_e2bot, p_new_e2top), std::max(p_new_e2bot, p_new_e2top)};
    //     }
    // }
    // else {
    //     // Points are at either end of the parent edge
    //     parentEdge = {std::min(p1.z, p2.z), std::max(p1.z, p2.z)};
    // }

    // if (parentEdge) {
    //     // TODO FIXME: if it's in arc_centers, then evaluate orientation differently
    //     // for type 1 (ccw) increased radius = positive orientation, decreased =
    //     // negative and for type -1 (cw) it's reversed

    //     // Look up the mm-space position of the "first" (lower-z) endpoint of the parent edge
    //     const heeks::Point& edgeStart = m_arc_fitting_map.point_map[parentEdge->first];

    //     double dx1 = dp1.x - edgeStart.x;
    //     double dy1 = dp1.y - edgeStart.y;
    //     double distsq1 = dx1 * dx1 + dy1 * dy1;

    //     double dx2 = dp2.x - edgeStart.x;
    //     double dy2 = dp2.y - edgeStart.y;
    //     double distsq2 = dx2 * dx2 + dy2 * dy2;

    //     int orientation = distsq2 > distsq1 ? 1 : -1;

    //     // std::cerr << "    edgeStart=(" << edgeStart.x << "," << edgeStart.y << ")"
    //     //           << " distsq1=" << distsq1 << " distsq2=" << distsq2
    //     //           << " orientation=" << orientation << "\n";

    //     return {*parentEdge, orientation};
    // }

    // std::cerr << "    ERROR: no parent edge found, returning sentinel\n";
    // return {{-2, -3}, 0};  // this should not happen; parent edge should always exist
}

// For open paths, reorder as needed to produce positively oriented and positively ordered paths
void CArea::ReorderOpenPaths(Paths64& paths)
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
            const auto edgeInfo = getEdgeInfo(p1, p2);
            const auto it = m_arc_fitting_map.zData.find(edgeInfo.parentEdge);
            if (it == m_arc_fitting_map.zData.end()) {
                // Error, this should not happen; there should always be zData for parent edges
                std::cerr << "ReorderOpenPaths: ERROR: no zData for parentEdge ("
                          << edgeInfo.parentEdge.first << "," << edgeInfo.parentEdge.second << ")\n";
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
