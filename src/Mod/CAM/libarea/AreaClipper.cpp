// SPDX-License-Identifier: BSD-3-Clause

// AreaClipper.cpp

// implements CArea methods using Angus Johnson's "Clipper"

#include "Area.h"
#include "clipper2/clipper.h"
#include <cmath>
#include <functional>

using namespace heeks;
using namespace Clipper2Lib;

bool CArea::HolesLinked()
{
    return false;
}

// static const double PI = 3.1415926535897932;
double CArea::m_clipper_scale = 10000.0;

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

static void AddVertex(const CVertex& vertex, const CVertex* prev_vertex, ArcFittingMap& arcMap)
{
    if (vertex.m_type == 0 || prev_vertex == NULL) {
        const int64_t z = arcMap.z_next++;
        PointD p(vertex.m_p.x * CArea::m_units, vertex.m_p.y * CArea::m_units, z);
        arcMap.point_map[z] = vertex.m_p;
        pts_for_AddVertex.push_back(p);
        arcMap.z_prev = z;
        std::cerr << "AddVertex: Line/First z=" << z << " at (" << vertex.m_p.x << ", "
                  << vertex.m_p.y << ")" << std::endl;
    }
    else {
        if (vertex.m_p != prev_vertex->m_p) {
            const double phi0
                = atan2(prev_vertex->m_p.y - vertex.m_c.y, prev_vertex->m_p.x - vertex.m_c.x);
            double phi1 = atan2(vertex.m_p.y - vertex.m_c.y, vertex.m_p.x - vertex.m_c.x);

            if (vertex.m_type == -1 && phi1 > phi0) {
                // fix to make it clockwise
                phi1 -= 2 * PI;
            }
            else if (vertex.m_type == 1 && phi1 < phi0) {
                // fix to make it counterclockwise
                phi1 += 2 * PI;
            }

            // what is the delta phi to get an accuracy of aber
            const double dx = prev_vertex->m_p.x - vertex.m_c.x;
            const double dy = prev_vertex->m_p.y - vertex.m_c.y;
            const double radius = sqrt(dx * dx + dy * dy);
            const double max_dphi = 2 * acos((radius - CArea::m_accuracy / CArea::m_units) / radius);

            // determine the number of segments
            const int num_segments
                = max(CArea::m_min_arc_points, (int)ceil(abs(phi1 - phi0) / max_dphi));
            const double dphi = (phi1 - phi0) / num_segments;

            const int64_t z_start = arcMap.z_next;
            std::cerr << "AddVertex: Arc z=" << z_start << ".." << (z_start + num_segments - 1)
                      << " (" << num_segments << " segments), endpoint=(" << vertex.m_p.x << ", "
                      << vertex.m_p.y << "), center=(" << vertex.m_c.x << ", " << vertex.m_c.y
                      << ")" << std::endl;

            for (int i = 1; i <= num_segments; i++) {
                // Create a unique z-value for each interpolated point along the arc
                const int64_t z = arcMap.z_next++;

                if (i == num_segments) {
                    pts_for_AddVertex.push_back(
                        PointD(vertex.m_p.x * CArea::m_units, vertex.m_p.y * CArea::m_units, z)
                    );
                    arcMap.point_map[z] = vertex.m_p;
                }
                else {
                    const double px = vertex.m_c.x + radius * cos(phi0 + dphi * i);
                    const double py = vertex.m_c.y + radius * sin(phi0 + dphi * i);
                    pts_for_AddVertex.push_back(PointD(px * CArea::m_units, py * CArea::m_units, z));
                    // Store arc center in point_map for intermediate points
                    arcMap.point_map[z] = vertex.m_c;
                }

                arcMap.arc_centers[{arcMap.z_prev, z}] = vertex.m_c;
                arcMap.z_prev = z;
            }
        }
    }
}

static void MakeLoop(
    const PointD& pt0,
    const PointD& pt1,
    const PointD& pt2,
    double radius,
    ArcFittingMap& arcMap
)
{
    heeks::Point p0(pt0.x, pt0.y);
    heeks::Point p1(pt1.x, pt1.y);
    heeks::Point p2(pt2.x, pt2.y);
    heeks::Point forward0 = p1 - p0;
    heeks::Point right0(forward0.y, -forward0.x);
    right0.normalize();
    heeks::Point forward1 = p2 - p1;
    heeks::Point right1(forward1.y, -forward1.x);
    right1.normalize();

    int arc_dir = (radius > 0) ? 1 : -1;

    CVertex v0(0, p1 + right0 * radius, heeks::Point(0, 0));
    CVertex v1(arc_dir, p1 + right1 * radius, p1);
    CVertex v2(0, p2 + right1 * radius, heeks::Point(0, 0));

    double save_units = CArea::m_units;
    CArea::m_units = 1.0;

    AddVertex(v1, &v0, arcMap);
    AddVertex(v2, &v1, arcMap);

    CArea::m_units = save_units;
}

static void OffsetWithLoops(
    const Paths64& pp,
    Paths64& pp_new,
    double inwards_value,
    ArcFittingMap& arcMap,
    const ZCallback64& zcallback
)
{
    Clipper64 c;
    c.SetZCallback(zcallback);

    bool inwards = (inwards_value > 0);
    bool reverse = false;
    double radius = -fabs(inwards_value);

    if (inwards) {
        // add a large square on the outside, to be removed later
        Path64 p;
        p.push_back(ToPoint64(PointD(-10000.0, -10000.0)));
        p.push_back(ToPoint64(PointD(-10000.0, 10000.0)));
        p.push_back(ToPoint64(PointD(10000.0, 10000.0)));
        p.push_back(ToPoint64(PointD(10000.0, -10000.0)));

        c.AddSubject({p});
    }
    else {
        reverse = true;
    }

    for (unsigned int i = 0; i < pp.size(); i++) {
        const Path64& p = pp[i];

        pts_for_AddVertex.clear();

        if (p.size() > 2) {
            if (reverse) {
                for (std::size_t j = p.size() - 1; j > 1; j--) {
                    MakeLoop(ToPointD(p[j]), ToPointD(p[j - 1]), ToPointD(p[j - 2]), radius, arcMap);
                }
                MakeLoop(ToPointD(p[1]), ToPointD(p[0]), ToPointD(p[p.size() - 1]), radius, arcMap);
                MakeLoop(
                    ToPointD(p[0]),
                    ToPointD(p[p.size() - 1]),
                    ToPointD(p[p.size() - 2]),
                    radius,
                    arcMap
                );
            }
            else {
                MakeLoop(
                    ToPointD(p[p.size() - 2]),
                    ToPointD(p[p.size() - 1]),
                    ToPointD(p[0]),
                    radius,
                    arcMap
                );
                MakeLoop(ToPointD(p[p.size() - 1]), ToPointD(p[0]), ToPointD(p[1]), radius, arcMap);
                for (std::size_t j = 2; j < p.size(); j++) {
                    MakeLoop(ToPointD(p[j - 2]), ToPointD(p[j - 1]), ToPointD(p[j]), radius, arcMap);
                }
            }

            Path64 loopy_polygon;
            loopy_polygon.reserve(pts_for_AddVertex.size());
            for (std::list<PointD>::iterator It = pts_for_AddVertex.begin();
                 It != pts_for_AddVertex.end();
                 It++) {
                loopy_polygon.push_back(ToPoint64(*It));
            }

            c.AddSubject({loopy_polygon});
            pts_for_AddVertex.clear();
        }
    }

    Paths64 solution;
    c.Execute(ClipType::Union, FillRule::NonZero, solution);

    pp_new = solution;

    if (inwards) {
        // remove the large square
        if (pp_new.size() > 0) {
            pp_new.erase(pp_new.begin());
        }
    }
    else {
        // reverse all the resulting polygons
        Paths64 copy = pp_new;
        pp_new.clear();
        pp_new.resize(copy.size());
        for (unsigned int i = 0; i < copy.size(); i++) {
            const Path64& p = copy[i];
            Path64 p_new;
            p_new.resize(p.size());
            std::size_t size_minus_one = p.size() - 1;
            for (std::size_t j = 0; j < p.size(); j++) {
                p_new[j] = p[size_minus_one - j];
            }
            pp_new[i] = p_new;
        }
    }
}

static void MakeObround(const heeks::Point& pt0, const CVertex& vt1, double radius, ArcFittingMap& arcMap)
{
    Span span(pt0, vt1);
    heeks::Point forward0 = span.GetVector(0.0);
    heeks::Point forward1 = span.GetVector(1.0);
    heeks::Point right0(forward0.y, -forward0.x);
    heeks::Point right1(forward1.y, -forward1.x);
    right0.normalize();
    right1.normalize();

    CVertex v0(pt0 + right0 * radius);
    CVertex v1(vt1.m_type, vt1.m_p + right1 * radius, vt1.m_c);
    CVertex v2(1, vt1.m_p + right1 * -radius, vt1.m_p);
    CVertex v3(-vt1.m_type, pt0 + right0 * -radius, vt1.m_c);
    CVertex v4(1, pt0 + right0 * radius, pt0);

    double save_units = CArea::m_units;
    CArea::m_units = 1.0;

    AddVertex(v0, NULL, arcMap);
    AddVertex(v1, &v0, arcMap);
    AddVertex(v2, &v1, arcMap);
    AddVertex(v3, &v2, arcMap);
    AddVertex(v4, &v3, arcMap);

    CArea::m_units = save_units;
}

static void OffsetSpansWithObrounds(
    const CArea& area,
    Paths64& pp_new,
    double radius,
    ArcFittingMap& arcMap,
    const ZCallback64& zcallback
)
{
    Clipper64 c;
    c.SetZCallback(zcallback);
    pp_new.clear();

    for (std::list<CCurve>::const_iterator It = area.m_curves.begin(); It != area.m_curves.end();
         It++) {
        c.Clear();
        // Add existing results back to clipper
        c.AddSubject(pp_new);
        pp_new.clear();
        pts_for_AddVertex.clear();

        const CCurve& curve = *It;
        const CVertex* prev_vertex = NULL;
        for (std::list<CVertex>::const_iterator It2 = curve.m_vertices.begin();
             It2 != curve.m_vertices.end();
             It2++) {
            const CVertex& vertex = *It2;
            if (prev_vertex) {
                MakeObround(prev_vertex->m_p, vertex, radius, arcMap);

                Path64 loopy_polygon;
                loopy_polygon.reserve(pts_for_AddVertex.size());
                for (std::list<PointD>::iterator It = pts_for_AddVertex.begin();
                     It != pts_for_AddVertex.end();
                     It++) {
                    loopy_polygon.push_back(ToPoint64(*It));
                }
                c.AddSubject({loopy_polygon});
                pts_for_AddVertex.clear();
            }
            prev_vertex = &vertex;
        }
        c.Execute(ClipType::Union, FillRule::NonZero, pp_new);
    }


    // reverse all the resulting polygons
    Paths64 copy = pp_new;
    pp_new.clear();
    pp_new.resize(copy.size());
    for (unsigned int i = 0; i < copy.size(); i++) {
        const Path64& p = copy[i];
        Path64 p_new;
        p_new.resize(p.size());
        std::size_t size_minus_one = p.size() - 1;
        for (std::size_t j = 0; j < p.size(); j++) {
            p_new[j] = p[size_minus_one - j];
        }
        pp_new[i] = p_new;
    }
}

static void MakePoly(const CCurve& curve, Path64& p, bool reverse, ArcFittingMap& arcMap)
{
    pts_for_AddVertex.clear();
    const CVertex* prev_vertex = NULL;

    if (!curve.m_vertices.size()) {
        return;
    }
    if (!curve.IsClosed()) {
        AddVertex(curve.m_vertices.front(), NULL, arcMap);
    }

    for (std::list<CVertex>::const_iterator It2 = curve.m_vertices.begin();
         It2 != curve.m_vertices.end();
         It2++) {
        const CVertex& vertex = *It2;
        if (prev_vertex) {
            AddVertex(vertex, prev_vertex, arcMap);
        }
        prev_vertex = &vertex;
    }
    std::cerr << std::endl;

    p.resize(pts_for_AddVertex.size());
    if (reverse) {
        std::size_t i = pts_for_AddVertex.size() - 1;  // clipper wants them the opposite way to CArea
        for (std::list<PointD>::iterator It = pts_for_AddVertex.begin();
             It != pts_for_AddVertex.end();
             It++, i--) {
            p[i] = ToPoint64(*It);
        }
    }
    else {
        unsigned int i = 0;
        for (std::list<PointD>::iterator It = pts_for_AddVertex.begin();
             It != pts_for_AddVertex.end();
             It++, i++) {
            p[i] = ToPoint64(*It);
        }
    }
}

static void MakePolyPoly(const CArea& area, Paths64& pp, bool reverse, ArcFittingMap& arcMap)
{
    pp.clear();

    for (std::list<CCurve>::const_iterator It = area.m_curves.begin(); It != area.m_curves.end();
         It++) {
        pp.push_back(Path64());
        MakePoly(*It, pp.back(), reverse, arcMap);
    }
}

static void SetFromResult(CCurve& curve, Path64& p, bool reverse, bool is_closed, const ArcFittingMap& arcMap)
{
    std::cerr << "\n=== SetFromResult: Path with " << p.size() << " points ===" << std::endl;

    if (CArea::m_clipper_clean_distance >= heeks::Point::tolerance) {
        p = SimplifyPath(p, CArea::m_clipper_clean_distance, is_closed);
    }

    // TODO for open paths start at one end and iterate in the direction of
    // decreasing z (which may be a nuanced notion given newly generated z
    // values)
    // Actually this order issue may need to be tagged/documented somehow when
    // mapping new zs back to the old ones they came from; the new path may *only* have new zs.
    // Yeah, do that. But only for open paths. Also write a test for it.

    // Loop through points
    int64_t prevZ = -1;
    heeks::Point prevP;
    double phi_total = 0.0;
    int num_j = p.size() + (is_closed ? 1 : 0);
    for (int dj = 0; dj < num_j; dj++) {
        const int j = ((reverse ? -1 : 1) * dj + 2 * p.size()) % p.size();
        const Point64& pt = p[j];
        PointD dp = ToPointD(pt);
        heeks::Point p(dp.x / CArea::m_units, dp.y / CArea::m_units);

        // Construct ordered pair for arc detection
        std::pair<int64_t, int64_t> zPair(std::min(prevZ, pt.z), std::max(prevZ, pt.z));

        // Check if this segment is an arc (presence in arc_centers means it's an arc)
        auto centerIt = arcMap.arc_centers.find(zPair);
        bool isLine = !CArea::m_fit_arcs || (prevZ == -1)
            || ((prevZ != pt.z) && (centerIt == arcMap.arc_centers.end()));

        if (isLine) {
            std::cerr << std::endl;  // Extra newline before adding new vertex
            std::cerr << "  [" << dj << "] j=" << j << " (" << p.x << ", " << p.y << ", " << pt.z
                      << ") ";
            std::cerr << "Edge (" << prevZ << ", " << pt.z << "): LINE" << std::endl;
            curve.m_vertices.emplace_back(0, p, heeks::Point {0, 0});
            phi_total = 0.0;
        }
        else {
            heeks::Point center;
            if (prevZ == pt.z) {
                auto point_it = arcMap.point_map.find(pt.z);
                if (point_it != arcMap.point_map.end()) {
                    center = point_it->second;
                }
                else {
                    std::cerr << std::endl
                              << "ERROR: prevZ == pt.z (" << pt.z << ") but not in point_map."
                              << std::endl;
                    throw std::runtime_error("prevZ == pt.z but z not in point_map");
                }
            }
            else {
                center = centerIt->second;
            }

            const double phi0 = atan2(prevP.y - center.y, prevP.x - center.x);
            const double phi1 = atan2(p.y - center.y, p.x - center.x);

            double dphi = phi1 - phi0;
            if (dphi > M_PI) {
                dphi -= 2 * M_PI;
            }
            else if (dphi < -M_PI) {
                dphi += 2 * M_PI;
            }

            // Positive dphi means ccw arc, negative means cw arc
            int type = (dphi > 0) ? 1 : -1;

            if (curve.m_vertices.size() > 0 && curve.m_vertices.back().m_type == type
                && curve.m_vertices.back().m_c == center && phi_total + abs(dphi) <= 2 * M_PI) {
                // Extend the previous CVertex arc
                std::cerr << "  [" << dj << "] j=" << j << " (" << p.x << ", " << p.y << ", "
                          << pt.z << ") ";
                std::cerr << "Edge (" << prevZ << ", " << pt.z << "): ARC";
                if (prevZ == pt.z) {
                    std::cerr << " (prevZ==pt.z), center from point_map=(" << center.x << ", "
                              << center.y << ")";
                }
                else {
                    std::cerr << ", center from arc_centers=(" << center.x << ", " << center.y << ")";
                }
                std::cerr << " -> EXTEND" << std::endl;
                curve.m_vertices.back().m_p = p;
                phi_total += abs(dphi);
            }
            else {
                // Add a new CVertex for the arc
                std::cerr << std::endl;  // Extra newline before adding new vertex
                std::cerr << "  [" << dj << "] j=" << j << " (" << p.x << ", " << p.y << ", "
                          << pt.z << ") ";
                std::cerr << "Edge (" << prevZ << ", " << pt.z << "): ARC";
                if (prevZ == pt.z) {
                    std::cerr << " (prevZ==pt.z), center from point_map=(" << center.x << ", "
                              << center.y << ")";
                }
                else {
                    std::cerr << ", center from arc_centers=(" << center.x << ", " << center.y << ")";
                }
                std::cerr << " -> NEW" << std::endl;
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

            // Calculate total arc angle to ensure it doesn't exceed 2*PI
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

            // Check if total exceeds a full circle; if not, then combine them
            if (abs(dphi_last) + abs(dphi_first) < 2 * M_PI) {
                curve.m_vertices.pop_back();
                curve.m_vertices.front().m_p = p_prev;

                std::cerr << "Merged final arc with second arc: moved start to (" << p_prev.x
                          << ", " << p_prev.y << ")" << std::endl;
            }
        }
    }
}

static void SetFromResult(CArea& area, Paths64& pp, bool reverse, bool is_closed, bool clear)
{
    // delete existing geometry
    if (clear) {
        area.m_curves.clear();
    }

    // Process intersection points before reconstructing curves
    area.ProcessIntersectionPoints(pp, is_closed);

    for (unsigned int i = 0; i < pp.size(); i++) {
        Path64& p = pp[i];

        area.m_curves.emplace_back();
        CCurve& curve = area.m_curves.back();
        SetFromResult(curve, p, reverse, is_closed, area.m_arc_fitting_map);
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

void CArea::Offset(double inwards_value)
{
    Paths64 pp, pp2;
    MakePolyPoly(*this, pp, false, m_arc_fitting_map);
    OffsetWithLoops(pp, pp2, inwards_value * m_units, m_arc_fitting_map, MakeZCallback());
    SetFromResult(*this, pp2, false, true, true);
    this->Reorder();
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
        MakePoly(curve, p, false, arcMap);

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

void CArea::Clip(ClipType op, const CArea& clip_area, FillRule subjFillType, FillRule clipFillType)
{
    Clipper64 c;
    c.SetZCallback(MakeZCallback());
    PopulateClipper(c, false, m_arc_fitting_map);
    clip_area.PopulateClipper(c, true, m_arc_fitting_map);

    // Execute to get both closed and open paths
    Paths64 closed_paths;
    Paths64 open_paths;
    c.Execute(op, subjFillType, closed_paths, open_paths);

    // Set closed paths as result
    SetFromResult(*this, closed_paths, true, true, true);

    // Append open paths to result
    SetFromResult(*this, open_paths, false, false, false);
}

void CArea::OffsetWithClipper(
    double offset,
    JoinType joinType,
    EndType endType,
    double miterLimit,
    double arcTolerance
)
{
    offset *= m_units * m_clipper_scale;
    if (arcTolerance == 0.0) {
        // Clipper arc tolerance definition: https://goo.gl/4odfQh
        double dphi = acos(1.0 - m_accuracy * m_clipper_scale / fabs(offset));
        int Segments = (int)ceil(PI / dphi);
        if (Segments < 2 * CArea::m_min_arc_points) {
            Segments = 2 * CArea::m_min_arc_points;
        }
        // if (Segments > CArea::m_max_arc_points)
        //     Segments=CArea::m_max_arc_points;
        dphi = PI / Segments;
        arcTolerance = (1.0 - cos(dphi)) * fabs(offset);
    }
    else {
        arcTolerance *= m_clipper_scale;
    }

    ClipperOffset clipper(miterLimit, arcTolerance);
    clipper.SetZCallback(MakeZCallback());

    Paths64 pp;
    MakePolyPoly(*this, pp, false, m_arc_fitting_map);

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

    SetFromResult(*this, pp2, false, true, true);
    this->Reorder();
}

void CArea::Thicken(double value)
{
    Paths64 pp;
    OffsetSpansWithObrounds(*this, pp, value * m_units, m_arc_fitting_map, MakeZCallback());
    SetFromResult(*this, pp, false, true, true);
    this->Reorder();
}

void UnFitArcs(CCurve& curve, ArcFittingMap& arcMap)
{
    pts_for_AddVertex.clear();
    const CVertex* prev_vertex = NULL;
    for (std::list<CVertex>::const_iterator It2 = curve.m_vertices.begin();
         It2 != curve.m_vertices.end();
         It2++) {
        const CVertex& vertex = *It2;
        AddVertex(vertex, prev_vertex, arcMap);
        prev_vertex = &vertex;
    }

    curve.m_vertices.clear();

    for (std::list<PointD>::iterator It = pts_for_AddVertex.begin(); It != pts_for_AddVertex.end();
         It++) {
        PointD& pt = *It;
        CVertex vertex(
            0,
            heeks::Point(pt.x / CArea::m_units, pt.y / CArea::m_units),
            heeks::Point(0.0, 0.0)
        );
        curve.m_vertices.push_back(vertex);
    }
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
        m_arc_fitting_map.point_map[pt.z] = heeks::Point(dp.x / m_units, dp.y / m_units);
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

void CArea::ProcessIntersectionPoints(const Paths64& paths, bool is_closed)
{
    // Process each path
    for (const Path64& path : paths) {
        if (path.empty()) {
            continue;
        }

        // Loop over edges, including wraparound edge if closed
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

                // Check if p1_edge1 matches either p2 edge
                if (p1_edge1 == p2_edge1 || p1_edge1 == p2_edge2) {
                    auto arc_it = m_arc_fitting_map.arc_centers.find(p1_edge1);
                    if (arc_it != m_arc_fitting_map.arc_centers.end()) {
                        m_arc_fitting_map.arc_centers[new_edge] = arc_it->second;
                    }
                }

                // Check if p1_edge2 matches either p2 edge
                if (p1_edge2 == p2_edge1 || p1_edge2 == p2_edge2) {
                    auto arc_it = m_arc_fitting_map.arc_centers.find(p1_edge2);
                    if (arc_it != m_arc_fitting_map.arc_centers.end()) {
                        m_arc_fitting_map.arc_centers[new_edge] = arc_it->second;
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

                // Check if p_old.z is an endpoint of edge1
                if (p_old.z == p_new_e1bot || p_old.z == p_new_e1top) {
                    std::pair<int64_t, int64_t> parent_edge
                        = {std::min(p_new_e1bot, p_new_e1top), std::max(p_new_e1bot, p_new_e1top)};
                    auto arc_it = m_arc_fitting_map.arc_centers.find(parent_edge);
                    if (arc_it != m_arc_fitting_map.arc_centers.end()) {
                        m_arc_fitting_map.arc_centers[new_edge] = arc_it->second;
                    }
                }

                // Check if p_old.z is an endpoint of edge2
                if (p_old.z == p_new_e2bot || p_old.z == p_new_e2top) {
                    std::pair<int64_t, int64_t> parent_edge
                        = {std::min(p_new_e2bot, p_new_e2top), std::max(p_new_e2bot, p_new_e2top)};
                    auto arc_it = m_arc_fitting_map.arc_centers.find(parent_edge);
                    if (arc_it != m_arc_fitting_map.arc_centers.end()) {
                        m_arc_fitting_map.arc_centers[new_edge] = arc_it->second;
                    }
                }
            }
        }
    }
}
