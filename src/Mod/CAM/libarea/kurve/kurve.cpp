// SPDX-License-Identifier: BSD-3-Clause

// written by g.j.hawkesford 2006 for Camtek Gmbh
//
// This program is released under the BSD license. See the file COPYING for details.
//

#include "geometry.h"
using namespace geoff_geometry;

////////////////////////////////////////////////////////////////////////////////////////////////
// kurve
////////////////////////////////////////////////////////////////////////////////////////////////

namespace geoff_geometry
{

void Span::minmax(Box& box, bool start)
{
    minmax(box.min, box.max, start);
}

void Span::minmax(Point& min, Point& max, bool start)
{
    // box a span (min/max)
    if (start) {
        MinMax(p0, min, max);
    }
    MinMax(p1, min, max);

    if (dir) {
        // check the quadrant points
        double dx1 = p1.x - p0.x;
        double dy1 = p1.y - p0.y;

        double dx = pc.x - p0.x;
        double dy = pc.y - p0.y;

        double dx0 = dx + radius;  // 0deg

        if (dir * (dx0 * dy1 - dx1 * dy) > 0) {
            if (pc.x + radius > max.x) {
                max.x = pc.x + radius;
            }
        }
        dx0 = dx - radius;  // 180deg
        if (dir * (dx0 * dy1 - dx1 * dy) > 0) {
            if (pc.x - radius < min.x) {
                min.x = pc.x - radius;
            }
        }
        double dy0 = dy + radius;  // 90deg
        if (dir * (dx * dy1 - dx1 * dy0) > 0) {
            if (pc.y + radius > max.y) {
                max.y = pc.y + radius;
            }
        }

        dy0 = dy - radius;  // 270deg
        if (dir * (dx * dy1 - dx1 * dy0) > 0) {
            if (pc.y - radius < min.y) {
                min.y = pc.y - radius;
            }
        }
    }
}

int Span::Intof(const Span& sp, Point& pInt1, Point& pInt2, double t[4]) const
{
    // Intof 2 spans
    return geoff_geometry::Intof(*this, sp, pInt1, pInt2, t);
}

void Span::SetProperties()
{
    if (dir) {
        // arc properties
        vs = ~Vector2d(pc, p0);  // tangent at start ( perp to radial vector)
        Vector2d ve = ~Vector2d(pc, p1);
        if (dir == CW) {
            vs = -vs;  // reverse directions for CW arc
            ve = -ve;
        }

        radius = vs.normalise();
        double radCheck = ve.normalise();
        //				if(FNE(radius, radCheck, geoff_geometry::TOLERANCE * 0.5)){
        if (FNE(radius, radCheck, geoff_geometry::TOLERANCE)) {
            throw "Invalid Geometry - Radii mismatch - SetProperties";
        }

        length = 0.0;
        angle = 0.0;
        if (radius > geoff_geometry::TOLERANCE) {
            if (p0.Dist(p1) <= geoff_geometry::TOLERANCE) {
                dir = LINEAR;
            }
            else {
                // arc length & included angle
                length = fabs(angle = IncludedAngle(vs, ve, dir)) * radius;
            }
        }
    }
    else {
        // straight properties
        vs = Vector2d(p0, p1);
        length = vs.normalise();
    }
    minmax(box, true);
}

}  // namespace geoff_geometry
