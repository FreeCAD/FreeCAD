// SPDX-License-Identifier: BSD-3-Clause

// AreaClipper.cpp

// implements CArea methods using Angus Johnson's "Clipper"

#include "Area.h"
#include "clipper2/clipper.h"

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
    return Point64((int64_t)(p.x * CArea::m_clipper_scale), (int64_t)(p.y * CArea::m_clipper_scale));
}

static PointD ToPointD(const Point64& p)
{
    return PointD((double)p.x / CArea::m_clipper_scale, (double)p.y / CArea::m_clipper_scale);
}

static std::list<PointD> pts_for_AddVertex;

static void AddPoint(const PointD& p)
{
    pts_for_AddVertex.push_back(p);
}

static void AddVertex(const CVertex& vertex, const CVertex* prev_vertex)
{
    if (vertex.m_type == 0 || prev_vertex == NULL) {
        AddPoint(PointD(vertex.m_p.x * CArea::m_units, vertex.m_p.y * CArea::m_units));
    }
    else {
        if (vertex.m_p != prev_vertex->m_p) {
            double phi, dphi, dx, dy;
            int Segments;
            int i;
            double ang1, ang2, phit;

            dx = (prev_vertex->m_p.x - vertex.m_c.x) * CArea::m_units;
            dy = (prev_vertex->m_p.y - vertex.m_c.y) * CArea::m_units;

            ang1 = atan2(dy, dx);
            if (ang1 < 0) {
                ang1 += 2.0 * PI;
            }
            dx = (vertex.m_p.x - vertex.m_c.x) * CArea::m_units;
            dy = (vertex.m_p.y - vertex.m_c.y) * CArea::m_units;
            ang2 = atan2(dy, dx);
            if (ang2 < 0) {
                ang2 += 2.0 * PI;
            }

            if (vertex.m_type == -1) {  // clockwise
                if (ang2 > ang1) {
                    phit = 2.0 * PI - ang2 + ang1;
                }
                else {
                    phit = ang1 - ang2;
                }
            }
            else {  // counter_clockwise
                if (ang1 > ang2) {
                    phit = -(2.0 * PI - ang1 + ang2);
                }
                else {
                    phit = -(ang2 - ang1);
                }
            }

            // what is the delta phi to get an accuracy of aber
            double radius = sqrt(dx * dx + dy * dy);
            dphi = 2 * acos((radius - CArea::m_accuracy) / radius);

            // set the number of segments
            if (phit > 0) {
                Segments = (int)ceil(phit / dphi);
            }
            else {
                Segments = (int)ceil(-phit / dphi);
            }

            if (Segments < CArea::m_min_arc_points) {
                Segments = CArea::m_min_arc_points;
            }
            // if (Segments > CArea::m_max_arc_points)
            //     Segments=CArea::m_max_arc_points;

            dphi = phit / (Segments);

            double px = prev_vertex->m_p.x * CArea::m_units;
            double py = prev_vertex->m_p.y * CArea::m_units;

            for (i = 1; i <= Segments; i++) {
                dx = px - vertex.m_c.x * CArea::m_units;
                dy = py - vertex.m_c.y * CArea::m_units;
                phi = atan2(dy, dx);

                double nx = vertex.m_c.x * CArea::m_units + radius * cos(phi - dphi);
                double ny = vertex.m_c.y * CArea::m_units + radius * sin(phi - dphi);

                AddPoint(PointD(nx, ny));

                px = nx;
                py = ny;
            }
        }
    }
}

static void MakeLoop(const PointD& pt0, const PointD& pt1, const PointD& pt2, double radius)
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

    AddVertex(v1, &v0);
    AddVertex(v2, &v1);

    CArea::m_units = save_units;
}

static void OffsetWithLoops(const Paths64& pp, Paths64& pp_new, double inwards_value)
{
    Clipper64 c;

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
                    MakeLoop(ToPointD(p[j]), ToPointD(p[j - 1]), ToPointD(p[j - 2]), radius);
                }
                MakeLoop(ToPointD(p[1]), ToPointD(p[0]), ToPointD(p[p.size() - 1]), radius);
                MakeLoop(ToPointD(p[0]), ToPointD(p[p.size() - 1]), ToPointD(p[p.size() - 2]), radius);
            }
            else {
                MakeLoop(ToPointD(p[p.size() - 2]), ToPointD(p[p.size() - 1]), ToPointD(p[0]), radius);
                MakeLoop(ToPointD(p[p.size() - 1]), ToPointD(p[0]), ToPointD(p[1]), radius);
                for (std::size_t j = 2; j < p.size(); j++) {
                    MakeLoop(ToPointD(p[j - 2]), ToPointD(p[j - 1]), ToPointD(p[j]), radius);
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

static void MakeObround(const heeks::Point& pt0, const CVertex& vt1, double radius)
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

    AddVertex(v0, NULL);
    AddVertex(v1, &v0);
    AddVertex(v2, &v1);
    AddVertex(v3, &v2);
    AddVertex(v4, &v3);

    CArea::m_units = save_units;
}

static void OffsetSpansWithObrounds(const CArea& area, Paths64& pp_new, double radius)
{
    Clipper64 c;
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
                MakeObround(prev_vertex->m_p, vertex, radius);

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

static void MakePoly(const CCurve& curve, Path64& p, bool reverse = false)
{
    pts_for_AddVertex.clear();
    const CVertex* prev_vertex = NULL;

    if (!curve.m_vertices.size()) {
        return;
    }
    if (!curve.IsClosed()) {
        AddVertex(curve.m_vertices.front(), NULL);
    }

    for (std::list<CVertex>::const_iterator It2 = curve.m_vertices.begin();
         It2 != curve.m_vertices.end();
         It2++) {
        const CVertex& vertex = *It2;
        if (prev_vertex) {
            AddVertex(vertex, prev_vertex);
        }
        prev_vertex = &vertex;
    }

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

static void MakePolyPoly(const CArea& area, Paths64& pp, bool reverse = true)
{
    pp.clear();

    for (std::list<CCurve>::const_iterator It = area.m_curves.begin(); It != area.m_curves.end();
         It++) {
        pp.push_back(Path64());
        MakePoly(*It, pp.back(), reverse);
    }
}

static void SetFromResult(CCurve& curve, Path64& p, bool reverse = true, bool is_closed = true)
{
    if (CArea::m_clipper_clean_distance >= heeks::Point::tolerance) {
        p = SimplifyPath(p, CArea::m_clipper_clean_distance, is_closed);
    }

    for (unsigned int j = 0; j < p.size(); j++) {
        const Point64& pt = p[j];
        PointD dp = ToPointD(pt);
        CVertex vertex(
            0,
            heeks::Point(dp.x / CArea::m_units, dp.y / CArea::m_units),
            heeks::Point(0.0, 0.0)
        );
        if (reverse) {
            curve.m_vertices.push_front(vertex);
        }
        else {
            curve.m_vertices.push_back(vertex);
        }
    }
    if (is_closed) {
        // make a copy of the first point at the end
        if (reverse) {
            curve.m_vertices.push_front(curve.m_vertices.back());
        }
        else {
            curve.m_vertices.push_back(curve.m_vertices.front());
        }
    }

    if (CArea::m_fit_arcs) {
        curve.FitArcs();
    }
}

static void SetFromResult(
    CArea& area,
    Paths64& pp,
    bool reverse = true,
    bool is_closed = true,
    bool clear = true
)
{
    // delete existing geometry
    if (clear) {
        area.m_curves.clear();
    }

    for (unsigned int i = 0; i < pp.size(); i++) {
        Path64& p = pp[i];

        area.m_curves.emplace_back();
        CCurve& curve = area.m_curves.back();
        SetFromResult(curve, p, reverse, is_closed);
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
    MakePolyPoly(*this, pp, false);
    OffsetWithLoops(pp, pp2, inwards_value * m_units);
    SetFromResult(*this, pp2, false);
    this->Reorder();
}

void CArea::PopulateClipper(Clipper64& c, bool as_clip) const
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
        MakePoly(curve, p, false);

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

void CArea::Clip(ClipType op, const CArea& clip_area, FillRule subjFillType, [[maybe_unused]] FillRule clipFillType)
{
    Clipper64 c;
    PopulateClipper(c, false);
    clip_area.PopulateClipper(c, true);

    // Execute to get both closed and open paths
    Paths64 closed_paths;
    Paths64 open_paths;
    c.Execute(op, subjFillType, closed_paths, open_paths);

    // Set closed paths as result
    SetFromResult(*this, closed_paths);

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

    Paths64 pp;
    MakePolyPoly(*this, pp, false);

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

    SetFromResult(*this, pp2, false);
    this->Reorder();
}

void CArea::Thicken(double value)
{
    Paths64 pp;
    OffsetSpansWithObrounds(*this, pp, value * m_units);
    SetFromResult(*this, pp, false);
    this->Reorder();
}

void UnFitArcs(CCurve& curve)
{
    pts_for_AddVertex.clear();
    const CVertex* prev_vertex = NULL;
    for (std::list<CVertex>::const_iterator It2 = curve.m_vertices.begin();
         It2 != curve.m_vertices.end();
         It2++) {
        const CVertex& vertex = *It2;
        AddVertex(vertex, prev_vertex);
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
