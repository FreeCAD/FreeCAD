// SPDX-License-Identifier: BSD-3-Clause

// written by g.j.hawkesford 2006 for Camtek Gmbh
//
// This program is released under the BSD license. See the file COPYING for details.
//

#include "geometry.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//            finite intersections
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace geoff_geometry
{
int Intof(const Span& sp0, const Span& sp1, Point& p0, Point& p1, double t[4])
{
    // returns the number of intersects (lying within spans sp0, sp1)
    if (sp0.box.outside(sp1.box)) {
        return 0;
    }
    if (!sp0.dir) {
        if (!sp1.dir) {
            // line line
            return LineLineIntof(sp0, sp1, p0, t);
        }
        else {
            // line arc
            return LineArcIntof(sp0, sp1, p0, p1, t);
        }
    }
    else {
        if (!sp1.dir) {
            // arc line
            return LineArcIntof(sp1, sp0, p0, p1, t);
        }
        else {
            // arc arc
            return ArcArcIntof(sp0, sp1, p0, p1);
        }
    }
}

int LineLineIntof(const Span& sp0, const Span& sp1, Point& p, double t[2])
{
    // intersection between 2 Line2d
    // returns 0 for no intersection in range of either span
    // returns 1 for intersction in range of both spans
    // t[0] is parameter on sp0,
    // t[1] is parameter on sp1
    Vector2d v0(sp0.p0, sp0.p1);
    Vector2d v1(sp1.p0, sp1.p1);
    Vector2d v2(sp0.p0, sp1.p0);

    double cp = v1 ^ v0;

    if (fabs(cp) < UNIT_VECTOR_TOLERANCE) {
        p = INVALID_POINT;
        return 0;  // parallel or degenerate lines
    }

    t[0] = (v1 ^ v2) / cp;
    p = v0 * t[0] + sp0.p0;
    p.ok = true;
    double toler = geoff_geometry::TOLERANCE / sp0.length;  // calc a parametric tolerance

    t[1] = (v0 ^ v2) / cp;
    if (t[0] < -toler || t[0] > 1 + toler) {  // intersection on first?
        return 0;
    }
    toler = geoff_geometry::TOLERANCE / sp1.length;  // calc a parametric tolerance
    if (t[1] < -toler || t[1] > 1 + toler) {         // intersection on second?
        return 0;
    }
    return 1;
}

int LineArcIntof(const Span& line, const Span& arc, Point& p0, Point& p1, double t[4])
{
    // inters of line arc
    // solving	x = x0 + dx * t			x = y0 + dy * t
    //			x = xc + R * cos(a)		y = yc + R * sin(a)		for t
    // gives :-  t² (dx² + dy²) + 2t(dx*dx0 + dy*dy0) + (x0-xc)² + (y0-yc)² - R² = 0
    int nRoots;
    Vector2d v0(arc.pc, line.p0);
    Vector2d v1(line.p0, line.p1);
    double s = v1.magnitudesqd();

    p0.ok = p1.ok = false;
    if ((nRoots = quadratic(s, 2 * (v0 * v1), v0.magnitudesqd() - arc.radius * arc.radius, t[0], t[1]))
        != 0) {
        double toler = geoff_geometry::TOLERANCE / sqrt(s);  // calc a parametric tolerance
        if (t[0] > -toler && t[0] < 1 + toler) {
            p0 = v1 * t[0] + line.p0;
            p0.ok = arc.OnSpan(p0, &t[2]);
        }
        if (nRoots == 2) {
            if (t[1] > -toler && t[1] < 1 + toler) {
                p1 = v1 * t[1] + line.p0;
                p1.ok = arc.OnSpan(p1, &t[3]);
            }
        }
        if (!p0.ok && p1.ok) {
            p0 = p1;
            p1.ok = false;
        }
        nRoots = (int)p0.ok + (int)p1.ok;
    }
    return nRoots;
}

int ArcArcIntof(const Span& arc0, const Span& arc1, Point& pLeft, Point& pRight)
{
    // Intof 2 arcs
    int numInts = Intof(Circle(arc0.pc, arc0.radius), Circle(arc1.pc, arc1.radius), pLeft, pRight);

    if (numInts == 0) {
        pLeft = arc0.p1;
        pLeft.ok = false;
        return 0;
    }
    int nLeft = arc0.OnSpan(pLeft) && arc1.OnSpan(pLeft);
    int nRight = (numInts == 2) ? arc0.OnSpan(pRight) && arc1.OnSpan(pRight) : 0;
    if (nLeft == 0 && nRight) {
        pLeft = pRight;
    }
    return nLeft + nRight;
}

bool Span::OnSpan(const Point& p) const
{
    double t;
    return OnSpan(p, &t);
}

bool Span::OnSpan(const Point& p, double* t) const
{
    // FAST OnSpan test - assumes that p lies ON the unbounded span

    bool ret;

    if (dir == LINEAR) {
        Vector2d v0(p0, p);
        *t = vs * v0;
        *t = *t / length;
        ret = (*t >= 0 && *t <= 1.0);
    }
    else {
        // true if p lies on arc span sp (p must be on circle of span)
        Vector2d v = ~Vector2d(pc, p);
        v.normalise();
        if (dir == CW) {
            v = -v;
        }

        double ang = IncludedAngle(vs, v, dir);
        *t = ang / angle;
        ret = (*t >= 0 && *t <= 1.0);
    }

    return ret;
}

// box class
bool Box::outside(const Box& b) const
{
    // returns true if this box is outside b
    if (!b.ok || !this->ok) {  // no box set
        return false;
    }
    if (this->max.x < b.min.x) {
        return true;
    }
    if (this->max.y < b.min.y) {
        return true;
    }
    if (this->min.x > b.max.x) {
        return true;
    }
    if (this->min.y > b.max.y) {
        return true;
    }
    return false;
}

}  // namespace geoff_geometry
