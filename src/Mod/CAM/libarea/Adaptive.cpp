// SPDX-License-Identifier: LGPL-2.1-or-later

/**************************************************************************
 *   Copyright (c) 2018 Kresimir Tusek <kresimir.tusek@gmail.com>          *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "Adaptive.hpp"
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <numbers>

namespace ClipperLib
{
void TranslatePath(const Path& input, Path& output, IntPoint delta);
}

namespace AdaptivePath
{
using namespace ClipperLib;
using namespace std;
#define SAME_POINT_TOL_SQRD_SCALED 4.0
#define UNUSED(expr) (void)(expr)

//*****************************************
// Utils - inline
//*****************************************

inline double DistanceSqrd(const IntPoint& pt1, const IntPoint& pt2)
{
    double Dx = double(pt1.X - pt2.X);
    double dy = double(pt1.Y - pt2.Y);
    return (Dx * Dx + dy * dy);
}

inline bool SetSegmentLength(const IntPoint& pt1, IntPoint& pt2, double new_length)
{
    double Dx = double(pt2.X - pt1.X);
    double dy = double(pt2.Y - pt1.Y);
    double l = sqrt(Dx * Dx + dy * dy);
    if (l > 0.0) {
        pt2.X = long(pt1.X + new_length * Dx / l);
        pt2.Y = long(pt1.Y + new_length * dy / l);
        return true;
    }
    return false;
}

inline bool HasAnyPath(const Paths& paths)
{
    for (Paths::size_type i = 0; i < paths.size(); i++) {
        if (!paths[i].empty()) {
            return true;
        }
    }
    return false;
}

inline double averageDV(const vector<double>& vec)
{
    double s = 0;
    std::size_t size = vec.size();
    if (size == 0) {
        return 0;
    }
    for (std::size_t i = 0; i < size; i++) {
        s += vec[i];
    }
    return s / double(size);
}

inline DoublePoint rotate(const DoublePoint& in, double rad)
{
    double c = cos(rad);
    double s = sin(rad);
    return DoublePoint(c * in.X - s * in.Y, s * in.X + c * in.Y);
}

// calculates path length for open path
inline double PathLength(const Path& path)
{
    double len = 0;
    if (path.size() < 2) {
        return len;
    }
    for (size_t i = 1; i < path.size(); i++) {
        len += sqrt(DistanceSqrd(path[i - 1], path[i]));
    }
    return len;
}

inline double PointSideOfLine(const IntPoint& p1, const IntPoint& p2, const IntPoint& pt)
{
    return double((pt.X - p1.X) * (p2.Y - p1.Y) - (pt.Y - p2.Y) * (p2.X - p1.X));
}

inline double Angle3Points(const DoublePoint& p1, const DoublePoint& p2, const DoublePoint& p3)
{
    double t1 = atan2(p2.Y - p1.Y, p2.X - p1.X);
    double t2 = atan2(p3.Y - p2.Y, p3.X - p2.X);
    double a = fabs(t2 - t1);
    return min(a, 2 * std::numbers::pi - a);
}

inline DoublePoint DirectionV(const IntPoint& pt1, const IntPoint& pt2)
{
    double DX = double(pt2.X - pt1.X);
    double DY = double(pt2.Y - pt1.Y);
    double l = sqrt(DX * DX + DY * DY);
    if (l < NTOL) {
        return DoublePoint(0, 0);
    }
    return DoublePoint(DX / l, DY / l);
}

inline void NormalizeV(DoublePoint& pt)
{
    double len = sqrt(pt.X * pt.X + pt.Y * pt.Y);
    if (len > NTOL) {
        pt.X /= len;
        pt.Y /= len;
    }
}

inline DoublePoint GetPathDirectionV(const Path& pth, size_t pointIndex)
{
    if (pth.size() < 2) {
        return DoublePoint(0, 0);
    }
    const IntPoint& p1 = pth.at(pointIndex > 0 ? pointIndex - 1 : pth.size() - 1);
    const IntPoint& p2 = pth.at(pointIndex);
    return DirectionV(p1, p2);
}

// Returns true if points 'a' and 'b' are coincident or nearly so.
bool isClose(const IntPoint& a, const IntPoint& b)
{
    return abs(a.X - b.X) <= 1 && abs(a.Y - b.Y) <= 1;
}

// Remove coincident and almost-coincident points from Paths.
void filterCloseValues(Paths& ppg)
{
    for (auto& pth : ppg) {
        while (true) {
            auto i = std::adjacent_find(pth.begin(), pth.end(), isClose);
            if (i == pth.end()) {
                break;
            }
            pth.erase(i);
        }
        // adjacent_find doesn't compare first with last element, so
        // do that manually.
        while (pth.size() > 1 && isClose(pth.front(), pth.back())) {
            pth.pop_back();
        }
    }
}

//*****************************************
// Utils
//*****************************************

class BoundBox
{
public:
    BoundBox()
    {
        minX = 0;
        maxX = 0;
        minY = 0;
        maxY = 0;
    }

    // generic: first point
    BoundBox(const IntPoint& p1)
    {
        minX = p1.X;
        maxX = p1.X;
        minY = p1.Y;
        maxY = p1.Y;
    }

    void SetFirstPoint(const IntPoint& p1)
    {
        minX = p1.X;
        maxX = p1.X;
        minY = p1.Y;
        maxY = p1.Y;
    }

    // generic: subsequent points
    void AddPoint(const IntPoint& pt)
    {
        minX = min(pt.X, minX);
        maxX = max(pt.X, maxX);
        minY = min(pt.Y, minY);
        maxY = max(pt.Y, maxY);
    }

    // line segment: two points
    BoundBox(const IntPoint& p1, const IntPoint& p2)
    {
        if (p1.X < p2.X) {
            minX = p1.X;
            maxX = p2.X;
        }
        else {
            minX = p2.X;
            maxX = p1.X;
        }
        if (p1.Y < p2.Y) {
            minY = p1.Y;
            maxY = p2.Y;
        }
        else {
            minY = p2.Y;
            maxY = p1.Y;
        }
    }

    // for circle: center and radius
    BoundBox(const IntPoint& center, long radius)
    {
        minX = center.X - radius;
        maxX = center.X + radius;
        minY = center.Y - radius;
        maxY = center.Y + radius;
    }

    // bounds check - intersection
    inline bool CollidesWith(const BoundBox& bb2)
    {
        return minX <= bb2.maxX && maxX >= bb2.minX && minY <= bb2.maxY && maxY >= bb2.minY;
    }

    // bounds check -  contains
    inline bool Contains(const BoundBox& bb2)
    {
        return minX <= bb2.minX && maxX >= bb2.maxX && minY <= bb2.minY && maxY >= bb2.maxY;
    }

    ClipperLib::cInt minX;
    ClipperLib::cInt maxX;
    ClipperLib::cInt minY;
    ClipperLib::cInt maxY;
};

std::ostream& operator<<(std::ostream& s, const BoundBox& p)
{
    s << "(" << p.minX << "," << p.minY << ") - (" << p.maxX << "," << p.maxY << ")";
    return s;
}

int getPathNestingLevel(const Path& path, const Paths& paths)
{
    int nesting = 0;
    for (const auto& other : paths) {
        if (!path.empty() && PointInPolygon(path.front(), other) != 0) {
            nesting++;
        }
    }
    return nesting;
}

void appendDirectChildPaths(Paths& outPaths, const Path& path, const Paths& paths)
{
    int nesting = getPathNestingLevel(path, paths);
    for (const auto& other : paths) {
        if (!path.empty() && !other.empty() && PointInPolygon(other.front(), path) != 0) {
            if (getPathNestingLevel(other, paths) == nesting + 1) {
                outPaths.push_back(other);
            }
        }
    }
}

void AverageDirection(const vector<DoublePoint>& unityVectors, DoublePoint& output)
{
    std::size_t size = unityVectors.size();
    output.X = 0;
    output.Y = 0;
    // sum vectors
    for (std::size_t i = 0; i < size; i++) {
        DoublePoint v = unityVectors[i];
        output.X += v.X;
        output.Y += v.Y;
    }
    // normalize
    double magnitude = sqrt(output.X * output.X + output.Y * output.Y);
    output.X /= magnitude;
    output.Y /= magnitude;
}

double DistancePointToLineSegSquared(
    const IntPoint& p1,
    const IntPoint& p2,
    const IntPoint& pt,
    IntPoint& closestPoint,
    double& ptParameter,
    bool clamp = true
)
{
    double D21X = double(p2.X - p1.X);
    double D21Y = double(p2.Y - p1.Y);
    double DP1X = double(pt.X - p1.X);
    double DP1Y = double(pt.Y - p1.Y);
    double lsegLenSqr = D21X * D21X + D21Y * D21Y;
    if (lsegLenSqr == 0) {  // segment is zero length, return point to point distance
        closestPoint = p1;
        ptParameter = 0;
        return DP1X * DP1X + DP1Y * DP1Y;
    }
    double parameter = DP1X * D21X + DP1Y * D21Y;
    if (clamp) {
        // clamp the parameter
        if (parameter < 0) {
            parameter = 0;
        }
        else if (parameter > lsegLenSqr) {
            parameter = lsegLenSqr;
        }
    }
    // point on line at parameter
    ptParameter = parameter / lsegLenSqr;
    closestPoint.X = long(p1.X + ptParameter * D21X);
    closestPoint.Y = long(p1.Y + ptParameter * D21Y);
    // calculate distance from point on line to pt
    double DX = double(pt.X - closestPoint.X);
    double DY = double(pt.Y - closestPoint.Y);
    return DX * DX + DY * DY;  // return distance squared
}

void ScaleUpPaths(Paths& paths, long scaleFactor)
{
    for (auto& pth : paths) {
        for (auto& pt : pth) {
            pt.X *= scaleFactor;
            pt.Y *= scaleFactor;
        }
    }
}

void ScaleDownPaths(Paths& paths, long scaleFactor)
{
    for (auto& pth : paths) {
        for (auto& pt : pth) {
            pt.X /= scaleFactor;
            pt.Y /= scaleFactor;
        }
    }
}


double DistancePointToPathsSqrd(
    const Paths& paths,
    const IntPoint& pt,
    IntPoint& closestPointOnPath,
    size_t& clpPathIndex,
    size_t& clpSegmentIndex,
    double& clpParameter
)
{
    double minDistSq = __DBL_MAX__;
    IntPoint clp;
    // iterate though paths
    for (Path::size_type i = 0; i < paths.size(); i++) {
        const Path* path = &paths[i];
        Path::size_type size = path->size();
        // iterate through segments
        for (Path::size_type j = 0; j < size; j++) {
            double ptPar;
            double distSq = DistancePointToLineSegSquared(
                path->at(j > 0 ? j - 1 : size - 1),
                path->at(j),
                pt,
                clp,
                ptPar
            );
            if (distSq < minDistSq) {
                clpPathIndex = i;
                clpSegmentIndex = j;
                clpParameter = ptPar;
                closestPointOnPath = clp;
                minDistSq = distSq;
            }
        }
    }
    return minDistSq;
}

// joins collinear segments (within the tolerance)
void CleanPath(const Path& inp, Path& outpt, double tolerance)
{
    if (inp.size() < 3) {
        outpt = inp;
        return;
    }
    outpt.clear();
    Path tmp;
    CleanPolygon(inp, tmp, tolerance);
    long size = long(tmp.size());

    // CleanPolygon will have empty result if all points are collinear,
    // 	need to add first and last point to the output
    if (size <= 2) {
        outpt.push_back(inp.front());
        outpt.push_back(inp.back());
        return;
    }

    // restore starting point
    double clpPar = 0;
    size_t clpSegmentIndex = 0;
    size_t clpPathIndex = 0;
    Paths tmpPaths;
    tmpPaths.push_back(tmp);
    IntPoint clp;
    // find point on cleaned poly that is closest to original starting point
    DistancePointToPathsSqrd(tmpPaths, inp.front(), clp, clpPathIndex, clpSegmentIndex, clpPar);


    // if closes point is not one of the polygon points, add it as separate first point
    if (DistanceSqrd(clp, tmp.at(clpSegmentIndex)) > 0
        && DistanceSqrd(clp, tmp.at(clpSegmentIndex > 0 ? clpSegmentIndex - 1 : size - 1)) > 0) {
        outpt.push_back(clp);
    }

    // add remaining points starting from closest
    long index;
    for (long i = 0; i < size; i++) {
        index = static_cast<long>(clpSegmentIndex + i);
        if (index >= size) {
            index -= size;
        }
        outpt.push_back(tmp.at(index));
    }


    if (DistanceSqrd(outpt.front(), inp.front()) > SAME_POINT_TOL_SQRD_SCALED) {
        outpt.insert(outpt.begin(), inp.front());
    }

    if (DistanceSqrd(outpt.back(), inp.back()) > SAME_POINT_TOL_SQRD_SCALED) {
        outpt.push_back(inp.back());
    }
}

bool Circle2CircleIntersect(
    const IntPoint& c1,
    const IntPoint& c2,
    double radius,
    pair<DoublePoint, DoublePoint>& intersections
)
{
    double DX = double(c2.X - c1.X);
    double DY = double(c2.Y - c1.Y);
    double d = sqrt(DX * DX + DY * DY);
    if (d < NTOL) {
        return false;  // same center
    }
    if (d >= radius) {
        return false;  // do not intersect, or intersect in one point (this case not relevant here)
    }
    double a_2 = sqrt(4 * radius * radius - d * d) / 2.0;
    intersections.first
        = DoublePoint(0.5 * (c1.X + c2.X) - DY * a_2 / d, 0.5 * (c1.Y + c2.Y) + DX * a_2 / d);
    intersections.second
        = DoublePoint(0.5 * (c1.X + c2.X) + DY * a_2 / d, 0.5 * (c1.Y + c2.Y) - DX * a_2 / d);
    return true;
}

bool Line2CircleIntersect(
    const IntPoint& c,
    double radius,
    const DoublePoint& p1,
    const DoublePoint& p2,
    vector<DoublePoint>& result,
    bool clamp = true
)
{
    // if more intersections returned, first is closer to p1
    double dx = double(p2.X - p1.X);
    double dy = double(p2.Y - p1.Y);
    double lcx = double(p1.X - c.X);
    double lcy = double(p1.Y - c.Y);
    double a = dx * dx + dy * dy;
    double b = 2 * dx * lcx + 2 * dy * lcy;
    double C = lcx * lcx + lcy * lcy - radius * radius;
    double sq = b * b - 4 * a * C;
    if (sq < 0) {
        return false;  // no solution
    }
    sq = sqrt(sq);
    double t1 = (-b - sq) / (2 * a);
    double t2 = (-b + sq) / (2 * a);
    result.clear();
    if ((t1 >= 0.0 && t1 <= 1.0) || !clamp) {
        result.emplace_back(p1.X + t1 * dx, p1.Y + t1 * dy);
    }
    if ((t2 >= 0.0 && t2 <= 1.0) || !clamp) {
        result.emplace_back(p1.X + t2 * dx, p1.Y + t2 * dy);
    }
    return !result.empty();
}

// calculate center point of polygon
IntPoint Compute2DPolygonCentroid(const Path& vertices)
{
    DoublePoint centroid(0, 0);
    double signedArea = 0.0;
    double x0 = 0.0;  // Current vertex X
    double y0 = 0.0;  // Current vertex Y
    double x1 = 0.0;  // Next vertex X
    double y1 = 0.0;  // Next vertex Y
    double a = 0.0;   // Partial signed area

    // For all vertices
    size_t i = 0;
    Path::size_type size = vertices.size();
    for (i = 0; i < size; ++i) {
        x0 = double(vertices[i].X);
        y0 = double(vertices[i].Y);
        x1 = double(vertices[(i + 1) % size].X);
        y1 = double(vertices[(i + 1) % size].Y);
        a = x0 * y1 - x1 * y0;
        signedArea += a;
        centroid.X += (x0 + x1) * a;
        centroid.Y += (y0 + y1) * a;
    }

    signedArea *= 0.5;
    centroid.X /= (6.0 * signedArea);
    centroid.Y /= (6.0 * signedArea);
    return IntPoint(long(centroid.X), long(centroid.Y));
}

// point must be within first path (boundary) and must not be within all other paths (holes)
bool IsPointWithinCutRegion(const Paths& toolBoundPaths, const IntPoint& point)
{
    bool inside = false;
    for (size_t i = 0; i < toolBoundPaths.size(); i++) {
        int pip = PointInPolygon(point, toolBoundPaths[i]);
        if (pip != 0) {
            inside = !inside;
        }
    }
    return inside;
}

/* finds intersection of line segment with line segment */
bool IntersectionPoint(
    const IntPoint& s1p1,
    const IntPoint& s1p2,
    const IntPoint& s2p1,
    const IntPoint& s2p2,
    IntPoint& intersection
)
{
    double S1DX = double(s1p2.X - s1p1.X);
    double S1DY = double(s1p2.Y - s1p1.Y);
    double S2DX = double(s2p2.X - s2p1.X);
    double S2DY = double(s2p2.Y - s2p1.Y);
    double d = S1DY * S2DX - S2DY * S1DX;
    if (fabs(d) < NTOL) {
        return false;  // lines are parallel
    }

    double LPDX = double(s1p1.X - s2p1.X);
    double LPDY = double(s1p1.Y - s2p1.Y);
    double p1d = S2DY * LPDX - S2DX * LPDY;
    double p2d = S1DY * LPDX - S1DX * LPDY;
    if ((d < 0) && (p1d < d || p1d > 0 || p2d < d || p2d > 0)) {
        return false;  // intersection not within segment1
    }
    if ((d > 0) && (p1d < 0 || p1d > d || p2d < 0 || p2d > d)) {
        return false;  // intersection not within segment2
    }
    double t = p1d / d;
    intersection = IntPoint(long(s1p1.X + S1DX * t), long(s1p1.Y + S1DY * t));
    return true;
}

/* finds one/first intersection of line segment with paths */
bool IntersectionPoint(const Paths& paths, const IntPoint& p1, const IntPoint& p2, IntPoint& intersection)
{
    BoundBox segBB(p1, p2);
    for (size_t i = 0; i < paths.size(); i++) {
        const Path* path = &paths[i];
        size_t size = path->size();
        if (size < 2) {
            continue;
        }
        BoundBox pathBB(path->front());
        for (size_t j = 0; j < size; j++) {

            const IntPoint* pp2 = &path->at(j);

            // box check for performance
            pathBB.AddPoint(*pp2);
            if (!pathBB.CollidesWith(segBB)) {
                continue;
            }

            const IntPoint* pp1 = &path->at(j > 0 ? j - 1 : size - 1);
            double LDY = double(p2.Y - p1.Y);
            double LDX = double(p2.X - p1.X);
            double PDX = double(pp2->X - pp1->X);
            double PDY = double(pp2->Y - pp1->Y);
            double d = LDY * PDX - PDY * LDX;
            if (fabs(d) < NTOL) {
                continue;  // lines are parallel
            }

            double LPDX = double(p1.X - pp1->X);
            double LPDY = double(p1.Y - pp1->Y);
            double p1d = PDY * LPDX - PDX * LPDY;
            double p2d = LDY * LPDX - LDX * LPDY;
            if ((d < 0) && (p1d < d || p1d > 0 || p2d < d || p2d > 0)) {
                continue;  // intersection not within segment
            }
            if ((d > 0) && (p1d < 0 || p1d > d || p2d < 0 || p2d > d)) {
                continue;  // intersection not within segment
            }
            double t = p1d / d;
            intersection = IntPoint(long(p1.X + LDX * t), long(p1.Y + LDY * t));
            return true;
        }
    }
    return false;
}

void SmoothPaths(Paths& paths, double stepSize, long pointCount, long iterations)
{
    Paths output;
    output.resize(paths.size());
    const long scale = 1000;
    const double stepScaled = stepSize * scale;

    ScaleUpPaths(paths, scale);
    vector<pair<size_t /*path index*/, IntPoint>> points;
    for (size_t i = 0; i < paths.size(); i++) {
        for (const auto& pt : paths[i]) {
            if (points.empty()) {
                points.emplace_back(i, pt);
                continue;
            }
            const auto back = points.back();
            const IntPoint& lastPt = back.second;


            const double l = sqrt(DistanceSqrd(lastPt, pt));

            if (l < 0.5 * stepScaled) {
                if (points.size() > 1) {
                    points.pop_back();
                }
                points.emplace_back(i, pt);
                continue;
            }
            size_t lastPathIndex = back.first;
            const long steps = max(long(l / stepScaled), 1L);
            const long left = pointCount * iterations * 2;
            const long right = steps - pointCount * iterations * 2;
            for (long idx = 0; idx <= steps; idx++) {
                if (idx > left && idx < right) {
                    idx = right;
                    continue;
                }
                const double p = double(idx) / steps;
                const IntPoint ptx(
                    long(lastPt.X + double(pt.X - lastPt.X) * p),
                    long(lastPt.Y + double(pt.Y - lastPt.Y) * p)
                );

                if (idx == 0 && DistanceSqrd(back.second, ptx) < scale && points.size() > 1) {
                    points.pop_back();
                }

                if (p < 0.5) {
                    points.emplace_back(lastPathIndex, ptx);
                }
                else {
                    points.emplace_back(i, ptx);
                }
            }
        }
    }
    if (points.empty()) {
        return;
    }
    const long size = long(points.size());
    for (long iter = 0; iter < iterations; iter++) {
        for (long i = 1; i < size - 1; i++) {
            IntPoint& cp = points[i].second;
            IntPoint avgPoint(cp);
            long cnt = 1;

            long ptsToAverage = pointCount;
            if (i <= ptsToAverage) {
                ptsToAverage = max(i - 1, 0L);
            }
            else if (i + ptsToAverage >= size - 1) {
                ptsToAverage = size - 1 - i;
            }
            for (long j = i - ptsToAverage; j <= i + ptsToAverage; j++) {
                if (j == i) {
                    continue;
                }
                long index = j;
                if (index < 0) {
                    index = 0;
                }
                if (index >= size) {
                    index = size - 1;
                }
                IntPoint& p = points[index].second;
                avgPoint.X += p.X;
                avgPoint.Y += p.Y;
                cnt++;
            }
            cp.X = avgPoint.X / cnt;
            cp.Y = avgPoint.Y / cnt;
        }
    }

    for (const auto& pr : points) {
        output[pr.first].push_back(pr.second);
    }
    for (size_t i = 0; i < paths.size(); i++) {
        CleanPath(output[i], paths[i], 1.4 * scale);
    }
    ScaleDownPaths(paths, scale);
}

bool PopPathWithClosestPoint(
    Paths& paths /*closest path is removed from collection and shifted to
                    start with closest point */
    ,
    IntPoint p1,
    Path& result,
    double extraDistanceAround = 0
)
{

    if (paths.empty()) {
        return false;
    }

    double minDistSqrd = __DBL_MAX__;
    size_t closestPathIndex = 0;
    long closestPointIndex = 0;
    for (size_t pathIndex = 0; pathIndex < paths.size(); pathIndex++) {
        Path& path = paths.at(pathIndex);
        for (size_t i = 0; i < path.size(); i++) {
            double dist = DistanceSqrd(p1, path.at(i));
            if (dist < minDistSqrd) {
                minDistSqrd = dist;
                closestPathIndex = pathIndex;
                closestPointIndex = long(i);
            }
        }
    }

    Path& closestPath = paths.at(closestPathIndex);
    while (extraDistanceAround > 0) {
        long nexti = (closestPointIndex + 1) % closestPath.size();
        extraDistanceAround -= sqrt(DistanceSqrd(closestPath[closestPointIndex], closestPath[nexti]));
        closestPointIndex = nexti;
    }

    result.clear();
    // make new path starting with that point
    for (size_t i = 0; i < closestPath.size(); i++) {
        long index = closestPointIndex + long(i);
        index = index % closestPath.size();
        result.push_back(closestPath.at(index));
    }
    // remove the closest path
    paths.erase(paths.begin() + closestPathIndex);
    return true;
}

void DeduplicatePaths(const Paths& inputs, Paths& outputs)
{
    outputs.clear();
    for (const auto& new_pth : inputs) {
        bool duplicate = false;
        // if all points of new path exist on some of the old paths, path is considered duplicate
        for (const auto& old_pth : outputs) {
            bool all_points_exists = true;
            for (const auto pt1 : new_pth) {
                bool pointExists = false;
                for (const auto pt2 : old_pth) {
                    if (DistanceSqrd(pt1, pt2) < SAME_POINT_TOL_SQRD_SCALED) {
                        pointExists = true;
                        break;
                    }
                }
                if (!pointExists) {
                    all_points_exists = false;
                    break;
                }
            }
            if (all_points_exists) {
                duplicate = true;
                break;
            }
        }

        if (!duplicate && !new_pth.empty()) {
            outputs.push_back(new_pth);
        }
    }
}

void ConnectPaths(Paths input, Paths& output)
{
    output.clear();
    bool newPath = true;
    Path joined;
    while (!input.empty()) {
        if (newPath) {
            if (!joined.empty()) {
                output.push_back(joined);
            }
            joined.clear();
            for (auto pt : input.front()) {
                joined.push_back(pt);
            }
            input.erase(input.begin());
            newPath = false;
        }
        bool anyMatch = false;
        for (size_t i = 0; i < input.size(); i++) {
            Path& n = input.at(i);
            if (DistanceSqrd(n.front(), joined.back()) < SAME_POINT_TOL_SQRD_SCALED) {
                for (auto pt : n) {
                    joined.push_back(pt);
                }
                input.erase(input.begin() + i);
                anyMatch = true;
                break;
            }
            else if (DistanceSqrd(n.back(), joined.back()) < SAME_POINT_TOL_SQRD_SCALED) {
                ReversePath(n);
                for (auto pt : n) {
                    joined.push_back(pt);
                }
                input.erase(input.begin() + i);
                anyMatch = true;
                break;
            }
            else if (DistanceSqrd(n.front(), joined.front()) < SAME_POINT_TOL_SQRD_SCALED) {
                for (auto pt : n) {
                    joined.insert(joined.begin(), pt);
                }
                input.erase(input.begin() + i);
                anyMatch = true;
                break;
            }
            else if (DistanceSqrd(n.back(), joined.front()) < SAME_POINT_TOL_SQRD_SCALED) {
                ReversePath(n);
                for (auto pt : n) {
                    joined.insert(joined.begin(), pt);
                }
                input.erase(input.begin() + i);
                anyMatch = true;
                break;
            }
        }
        if (!anyMatch) {
            newPath = true;
        }
    }
    if (!joined.empty()) {
        output.push_back(joined);
    }
}

// helper class for measuring performance
class PerfCounter
{
public:
    PerfCounter(string p_name)
    {
        name = p_name;
        count = 0;
        running = false;
        start_ticks = 0;
        total_ticks = 0;
    }
    inline void Start()
    {
#define DEV_MODE
#ifdef DEV_MODE
        start_ticks = clock();
        if (running) {
            cerr << "PerfCounter already running:" << name << endl;
        }
        running = true;
#endif
    }
    inline void Stop()
    {
#ifdef DEV_MODE
        if (!running) {
            cerr << "PerfCounter not running:" << name << endl;
        }
        total_ticks += clock() - start_ticks;
        start_ticks = clock();
        count++;
        running = false;
#endif
    }
    void DumpResults()
    {
        double total_time = double(total_ticks) / CLOCKS_PER_SEC;
        cout << "Perf: " << name.c_str() << " total_time: " << total_time
             << " sec, call_count:" << count << " per_call:" << double(total_time / count) << endl;
        start_ticks = clock();
        total_ticks = 0;
        count = 0;
    }

private:
    string name;
    clock_t start_ticks;
    clock_t total_ticks;
    size_t count;
    bool running = false;
};

PerfCounter Perf_ProcessPolyNode("ProcessPolyNode");
PerfCounter Perf_CalcCutAreaCirc("CalcCutArea");
PerfCounter Perf_NextEngagePoint("NextEngagePoint");
PerfCounter Perf_PointIterations("PointIterations");
PerfCounter Perf_ExpandCleared("ExpandCleared");
PerfCounter Perf_DistanceToBoundary("DistanceToBoundary");
PerfCounter Perf_AppendToolPath("AppendToolPath");
PerfCounter Perf_IsAllowedToCutTrough("IsAllowedToCutTrough");
PerfCounter Perf_IsClearPath("IsClearPath");

//***********************************
// Cleared area bounding support
//***********************************
class ClearedArea
{
public:
    ClearedArea(ClipperLib::cInt p_toolRadiusScaled)
    {
        toolRadiusScaled = p_toolRadiusScaled;
    };

    void SetClearedPaths(const Paths& paths)
    {
        clearedPaths = paths;
        bboxPathsInvalid = true;
        bboxClippedInvalid = true;
    }

    void AddClearedPaths(const Paths& paths)
    {
        clip.Clear();
        clip.AddPaths(clearedPaths, PolyType::ptSubject, true);
        clip.AddPaths(paths, PolyType::ptClip, true);
        clip.Execute(ClipType::ctUnion, clearedPaths);
        CleanPolygons(clearedPaths);
        bboxPathsInvalid = true;
        bboxClippedInvalid = true;
    }

    void ExpandCleared(const Path toClearToolPath)
    {
        if (toClearToolPath.empty()) {
            return;
        }
        Perf_ExpandCleared.Start();
        clipof.Clear();
        clipof.AddPath(toClearToolPath, JoinType::jtRound, EndType::etOpenRound);
        Paths toolCoverPoly;
        clipof.Execute(toolCoverPoly, toolRadiusScaled + 1);
        clip.Clear();
        clip.AddPaths(clearedPaths, PolyType::ptSubject, true);
        clip.AddPaths(toolCoverPoly, PolyType::ptClip, true);
        clip.Execute(ClipType::ctUnion, clearedPaths);
        CleanPolygons(clearedPaths);
        bboxPathsInvalid = true;
        bboxClippedInvalid = true;
        Perf_ExpandCleared.Stop();
    }

    // get cleared area/poly bounded to toolbox
    Paths& GetBoundedClearedAreaClipped(const IntPoint& toolPos, int delta)
    {
        // first, attempt to serve this query from cache
        BoundBox toolBB(toolPos, delta);
        if (!bboxClippedInvalid && clearedBBClippedInFocus.Contains(toolBB)) {
            return clearedBoundedClipped;
        }

        // second, check if the window needs to be recomputed
        if (bboxClippedInvalid || !clearedBBWindow.Contains(toolBB)) {
            const int deltaWindow = delta * clearedBoundedWindowScale;
            clearedBBWindow.SetFirstPoint(IntPoint(toolPos.X - deltaWindow, toolPos.Y - deltaWindow));
            clearedBBWindow.AddPoint(IntPoint(toolPos.X + deltaWindow, toolPos.Y + deltaWindow));

            Path bbPath;
            bbPath.push_back(IntPoint(toolPos.X - deltaWindow, toolPos.Y - deltaWindow));
            bbPath.push_back(IntPoint(toolPos.X + deltaWindow, toolPos.Y - deltaWindow));
            bbPath.push_back(IntPoint(toolPos.X + deltaWindow, toolPos.Y + deltaWindow));
            bbPath.push_back(IntPoint(toolPos.X - deltaWindow, toolPos.Y + deltaWindow));
            clip.Clear();
            clip.AddPath(bbPath, PolyType::ptSubject, true);
            clip.AddPaths(clearedPaths, PolyType::ptClip, true);
            clip.Execute(ClipType::ctIntersection, clearedBoundedWindow);
        }

        // finally, perform the query using data from the window
        clearedBBClippedInFocus.SetFirstPoint(IntPoint(toolPos.X - delta, toolPos.Y - delta));
        clearedBBClippedInFocus.AddPoint(IntPoint(toolPos.X + delta, toolPos.Y + delta));

        Path bbPath;
        bbPath.push_back(IntPoint(toolPos.X - delta, toolPos.Y - delta));
        bbPath.push_back(IntPoint(toolPos.X + delta, toolPos.Y - delta));
        bbPath.push_back(IntPoint(toolPos.X + delta, toolPos.Y + delta));
        bbPath.push_back(IntPoint(toolPos.X - delta, toolPos.Y + delta));
        clip.Clear();
        clip.AddPath(bbPath, PolyType::ptSubject, true);
        clip.AddPaths(clearedBoundedWindow, PolyType::ptClip, true);
        clip.Execute(ClipType::ctIntersection, clearedBoundedClipped);

        bboxClippedInvalid = false;
        return clearedBoundedClipped;
    }

    // get full cleared area
    Paths& GetCleared()
    {
        return clearedPaths;
    }

private:
    Clipper clip;
    ClipperOffset clipof;
    Paths clearedPaths;
    Paths clearedBoundedWindow;
    Paths clearedBoundedClipped;
    Paths clearedBoundedPaths;

    ClipperLib::cInt toolRadiusScaled;
    BoundBox clearedBBWindow;
    BoundBox clearedBBClippedInFocus;
    BoundBox clearedBBPathsInFocus;

    bool bboxClippedInvalid = false;
    bool bboxPathsInvalid = false;
    int clearedBoundedWindowScale = 10;
};

//***************************************
// Linear Interpolation - area vs angle
//***************************************
struct InterpItem
{
    std::pair<double, IntPoint> angle;
    double error;
    bool isConventional;
};

class Interpolation
{
public:
    const double MIN_ANGLE = -std::numbers::pi / 4;
    const double MAX_ANGLE = std::numbers::pi / 4;

    void clear()
    {
        m_min.reset();
        m_max.reset();
    }
    bool bothSides()
    {
        return m_min && m_max && m_min->error < 0 && m_max->error >= 0
            && (!m_min->isConventional || !m_max->isConventional);
    }
    // adds point keeping the incremental order of areas for interpolation to work correctly
    void addPoint(double error, std::pair<double, IntPoint> angle, bool allowSkip, bool isConventional)
    {
        const InterpItem newItem = {angle, error, isConventional};

        if (!m_min) {
            m_min = newItem;
        }
        else if (!m_max) {
            m_max = newItem;
            if (m_min->error > m_max->error) {
                auto tmp = m_min;
                m_min = m_max;
                m_max = tmp;
            }
        }
        else if (isConventional && (m_min->isConventional ^ m_max->isConventional)) {
            if (!allowSkip) {
                if (m_min->isConventional) {
                    m_min.reset();
                }
                else {
                    m_max.reset();
                }
                addPoint(error, angle, false, isConventional);
            }
        }
        else if (bothSides()) {
            if (error < 0) {
                m_min = newItem;
            }
            else {
                m_max = newItem;
            }
        }
        else {
            if (allowSkip && abs(error) > abs(m_min->error) && abs(error) > abs(m_max->error)
                && (isConventional || !m_min->isConventional || !m_max->isConventional)) {
                return;
            }

            if (m_min->isConventional ^ m_max->isConventional) {
                if (m_min->isConventional) {
                    m_min.reset();
                }
                else {
                    m_max.reset();
                }
            }
            else if (abs(m_min->error) > abs(m_max->error)) {
                m_min.reset();
            }
            else {
                m_max.reset();
            }
            addPoint(error, angle, false, isConventional);
        }
    }

    double interpolateAngle()
    {
        if (!m_min) {
            return MIN_ANGLE;
        }

        if (!m_max) {
            return MAX_ANGLE;
        }
        double p = (0 - m_min->error) / (m_max->error - m_min->error);

        // Ensure search is sufficiently efficient -- this is a compromise
        // between binary search (p = 0.5, guaranteed search completion in log
        // time) and following linear interpolation completely (often faster
        // since area cut is locally linear in movement angle)
        const double minInterp = .2;
        p = max(min(p, 1 - minInterp), minInterp);

        return m_min->angle.first * (1 - p) + m_max->angle.first * p;
    }

    double clampAngle(double angle)
    {
        return max(min(angle, MAX_ANGLE), MIN_ANGLE);
    }

    size_t getPointCount()
    {
        return (m_min ? 1 : 0) + (m_max ? 1 : 0);
    }

public:
    // {{angle, clipper point}, error}
    std::optional<InterpItem> m_min;
    std::optional<InterpItem> m_max;
};

//***************************************
// Adaptive2d main class - implementation
//***************************************

Adaptive2d::Adaptive2d()
{}

// Algorithm to compute area inside circle c2 but outside circle c1 and polygons clearedArea:
// All computations are done on doubles (at some point we can transition off of clipper and operate
// on curves!)
//
// Re-express the problem: find area inside all circles and polygons, where c1 and polygons are
// inverted
//
// 0) Extract from clearedArea a set of polygons close enough to potentially affect the bounded area
// 0.5) Rotate all geometry so the vector from c1 to c2 points up (y+)
// 1) Find all x-coordinates of interest:
//   a) All polygon vertices
//   b) Intersection of all polygons with c1
//   c) Intersection of all polygons with c2
//   d) There are no self-intersections or intersection points with other polygons (guarantee from
//   clipper), so we don't have to compute those
//   e) Compute intersection points between c1 and c2
//   f) //   Add c1's and c2's vertical tangents to the list
//   g) x=c2.X
// 2) Sort these x-coordinates. Discard all values before c2-r or after c2+r. We will consider
// ranges of x-values between these points 3) For each non-empty range in x, construct a vertical
// line through its midpoint
//   Over the full (open) x-range, vertical lines cross all polygons/circles in the same order (no
//   topology changes!) Also note that this line is guaranteed to not pass through any polygon
//   vertex, or tangent to any circle. No funny business! a) Compute the intersection point(s)
//   between the line and each polygon and circle. Keep track of which shape crossing each parameter
//   came from (polygon index and edge index, or circle index and top/bottom flag) b) Sort these
//   intersection on their y-coordinate. c) Loop over y-coordinates. At each, we will update state
//   to account for stepping over that crossing:
//     Init (i.e. y=-inf): outsideCount = 1 (outside c2 and inside all other shapes)
//     1) Identify the shape we are crossing; determine check in->out vs out->in
//     2) Update outsideCount and the list of what we're outside.
//       a) If outsideCount=0, totalArea += integral(x0, x1, crossed boundary)
//       b) If outsideCount was 0 and just changed to 1, totalArea -= integral(x0, x1, crossed
//       boundary)
//         ...careful with the signs on those integrals; be sure to add area from c2 and
//         subtract from other shapes
//     3) if x<c2.X, additionally accumulate area in conventionalArea, for detection of conventional
//     cutting
// 4) Return <totalArea, conventionalArea>
std::pair<double, double> Adaptive2d::CalcCutArea(
    Clipper& clip,
    IntPoint c1,
    IntPoint c2,
    ClearedArea& clearedArea
)
{
    double dist = sqrt(DistanceSqrd(c1, c2));
    if (dist < NTOL) {
        return {0, 0};
    }

    Perf_CalcCutAreaCirc.Start();

    // 0) Extract from clearedArea a set of polygons close enough to potentially affect the bounded area
    vector<vector<DoublePoint>> polygons;
    vector<DoublePoint> inters;  // temporary, to hold intersection results
    const BoundBox c2BB(c2, toolRadiusScaled);
    // get curves from slightly enlarged region that will cover all points tested in this iteration
    const bool useC2 = dist > 2 * toolRadiusScaled;
    const Paths& clearedBounded = clearedArea.GetBoundedClearedAreaClipped(
        useC2 ? c2 : c1,
        toolRadiusScaled + (useC2 ? 0 : (int)dist) + 4
    );

    for (const Path& path : clearedBounded) {
        if (path.size() == 0) {
            continue;
        }

        // bound box check
        BoundBox pathBB(path.front());
        for (const auto& pt : path) {
            pathBB.AddPoint(pt);
        }
        if (!pathBB.CollidesWith(c2BB)) {
            continue;  // this path cannot colide with tool
        }

        vector<DoublePoint> polygon;
        for (const auto p : path) {
            polygon.push_back({(double)p.X, (double)p.Y});
        }
        polygons.push_back(polygon);
    }

    // (*fout) << "AREA["
    // 	<< " c1=(" << c1.X << "," << c1.Y << ")"
    // 	<< " c2=(" << c2.X << "," << c2.Y << ")";
    // for (const auto poly : polygons) {
    // 	(*fout) << "POLY[";
    // 	for (const auto p : poly) {
    // 		(*fout) << "(" << p.X << "," << p.Y << ")";
    // 	}
    // 	(*fout) << "] ";
    // }
    // (*fout) << "] ";

    // 0.5) Rotate all geometry so the vector from c1 to c2 points up (y+)
    {
        const double angle = std::numbers::pi / 2 - atan2(c2.Y - c1.Y, c2.X - c1.X);
        const double ca = cos(angle);
        const double sa = sin(angle);
        c1 = {ca * c1.X - sa * c1.Y, sa * c1.X + ca * c1.Y};
        c2 = {ca * c2.X - sa * c2.Y, sa * c2.X + ca * c2.Y};
        vector<vector<DoublePoint>> rotatedPolygons;
        for (vector<DoublePoint>& pgon : polygons) {
            vector<DoublePoint> rotated;
            for (auto& p : pgon) {
                rotated.push_back({ca * p.X - sa * p.Y, sa * p.X + ca * p.Y});
            }
            rotatedPolygons.push_back(rotated);
        }

        polygons = rotatedPolygons;
    }

    // 1) Find all x-coordinates of interest:
    vector<double> xs;
    for (const auto polygon : polygons) {
        // 1.a) All polygon vertices
        for (const auto p : polygon) {
            xs.push_back(p.X);
        }

        // 1.b) Intersection of all polygons with c1
        // 1.c) Intersection of all polygons with c2
        for (int i = 0; i < polygon.size(); i++) {
            const auto p0 = polygon[i];
            const auto p1 = polygon[(i + 1) % polygon.size()];
            if (Line2CircleIntersect(c1, toolRadiusScaled, p0, p1, inters)) {
                for (const auto p : inters) {
                    xs.push_back(p.X);
                }
            }
            if (Line2CircleIntersect(c2, toolRadiusScaled, p0, p1, inters)) {
                for (const auto p : inters) {
                    xs.push_back(p.X);
                }
            }
        }
    }

    // 1.e) Compute intersection points between c1 and c2
    {
        pair<DoublePoint, DoublePoint> res;
        if (Circle2CircleIntersect(c1, c2, toolRadiusScaled, res)) {
            xs.push_back(res.first.X);
            xs.push_back(res.second.X);
        }
    }

    // 1.f) Add c1's and c2's vertical tangents to the list
    xs.push_back((double)c1.X - toolRadiusScaled);
    xs.push_back((double)c1.X + toolRadiusScaled);

    const double xmin = (double)c2.X - toolRadiusScaled;
    const double xmax = (double)c2.X + toolRadiusScaled;
    xs.push_back(xmin);
    xs.push_back(xmax);

    // 1.g) x=c2.X
    xs.push_back(c2.X);

    // 2) Sort these x-coordinates. Discard all values before c2-r or after c2+r
    {
        vector<double> xfilter;
        for (const double x : xs) {
            if (xmin <= x && x <= xmax) {
                xfilter.push_back(x);
            }
        }
        xs = xfilter;
        std::sort(xs.begin(), xs.end());
    }

    const auto interpX = [](const DoublePoint p0, const DoublePoint p1, double x) {
        const double interp = (x - p0.X) / (p1.X - p0.X);
        const double y = p1.Y * interp + p0.Y * (1 - interp);
        return y;
    };

    // 3) For each non-empty range in x, construct a vertical line through its midpoint
    const vector<DoublePoint> circles = {c2, c1};
    double area = 0;
    double conventionalArea = 0;
    for (int ix = 0; ix < xs.size() - 1; ix++) {
        const double x0 = xs[ix];
        const double x1 = xs[ix + 1];
        if (x0 == x1) {
            continue;
        }
        const double xtest = (x0 + x1) / 2;

        // 3.a) Compute the intersection point(s) between the line and each polygon and circle. Keep
        // track of which shape crossing each parameter came from (polygon index and edge index, or
        // circle index and top/bottom flag)

        // y, polygon index (or polygons.size() + circle index), edge index (or 0/1 for top/bottom half)
        vector<tuple<double, int, int>> ys;

        for (int ipolygon = 0; ipolygon < polygons.size(); ipolygon++) {
            const auto polygon = polygons[ipolygon];
            for (int iedge = 0; iedge < polygon.size(); iedge++) {
                const auto p0 = polygon[iedge];
                const auto p1 = polygon[(iedge + 1) % polygon.size()];
                // note: we skip if the edge is vertical, p0.X == p1.X == xtest
                if (min(p0.X, p1.X) < xtest && max(p0.X, p1.X) > xtest) {
                    const double y = interpX(p0, p1, xtest);
                    ys.push_back({y, ipolygon, iedge});
                }
            }
        }

        for (int icircle = 0; icircle < circles.size(); icircle++) {
            const DoublePoint c = circles[icircle];
            const double dx = abs(xtest - c.X);
            if (dx < toolRadiusScaled) {  // skip tangent; xtest can't be a tangent anyway
                const double dy = sqrt(toolRadiusScaled * toolRadiusScaled - dx * dx);
                ys.push_back({c.Y + dy, polygons.size() + icircle, 0});
                ys.push_back({c.Y - dy, polygons.size() + icircle, 1});
            }
        }

        // 3.b) Sort these intersection on their y-coordinate.
        std::sort(
            ys.begin(),
            ys.end(),
            [](std::tuple<double, int, int> a, std::tuple<double, int, int> b) {
                return std::get<0>(a) < std::get<0>(b);
            }
        );

        // 3.c) Loop over y-coordinates. At each, we will update state to account for stepping over
        // that crossing:
        //     Init (i.e. y=-inf): outsideCount = 1 (outside c2 and inside all other shapes)
        std::vector<bool> outside;
        for (int i = 0; i < polygons.size() + circles.size(); i++) {
            outside.push_back(i == (polygons.size()));  // poly_0, ..., poly_n-1, c2, c1
        }
        int outsideCount = 1;
        for (int iy = 0; iy < ys.size(); iy++) {
            const int ishape = std::get<1>(ys[iy]);
            const int ipart = std::get<2>(ys[iy]);

            const bool prevOutside = outside[ishape];
            const int prevCount = outsideCount;
            outside[ishape] = !outside[ishape];
            outsideCount += prevOutside ? -1 : 1;

            // Sign
            // We want to compute integral(exitY - entranceY)
            // We do this in two steps: -integral(entranceY - 0) + integral(exitY - 0)
            const double entranceExitSign = prevOutside ? -1 : 1;

            if (outsideCount == 0 || prevCount == 0) {
                if (ishape < polygons.size()) {
                    // crossed a polygon
                    const auto polygon = polygons[ishape];
                    const auto p0 = polygon[ipart];
                    const auto p1 = polygon[(ipart + 1) % polygon.size()];
                    const auto y0 = interpX(p0, p1, x0);
                    const auto y1 = interpX(p0, p1, x1);
                    const double newArea = (y0 + y1) / 2 * (x1 - x0);
                    area += entranceExitSign * newArea;
                    if (xtest < c2.X) {
                        conventionalArea += entranceExitSign * newArea;
                    }
                    //(*fout) << "Pgon[eeSign=" << entranceExitSign
                    //	<< " x0=(" << x0 << " x1=" << x1 << ")"
                    //	<< " p0=(" << p0.X << "," << p0.Y << ")"
                    //	<< " p1=(" << p1.X << "," << p1.Y << ")"
                    //	<< " newArea(w/o sign)=" << newArea
                    //	<< " totalArea=" << area << "] ";
                }
                else {
                    // crossed a circle
                    const auto c = circles[ishape - polygons.size()];
                    const double circleSign = ipart == 0 ? 1 : -1;

                    // first, compute area of sector - area of triangle = area of segment
                    const auto clamp = [](double a) {
                        return max(-1.0, min(1.0, a));
                    };
                    // clamp is required only because of floating point rounding errors
                    const double phi0 = acos(clamp((x0 - c.X) / toolRadiusScaled)) * circleSign;
                    const double phi1 = acos(clamp((x1 - c.X) / toolRadiusScaled)) * circleSign;
                    const double areaSector = toolRadiusScaled * toolRadiusScaled / 2
                        * abs(phi1 - phi0);

                    const double y0 = c.Y
                        + circleSign
                            * sqrt(toolRadiusScaled * toolRadiusScaled - (x0 - c.X) * (x0 - c.X));
                    const double y1 = c.Y
                        + circleSign
                            * sqrt(toolRadiusScaled * toolRadiusScaled - (x1 - c.X) * (x1 - c.X));
                    const double tbase = sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
                    const double tmidx = (x0 + x1) / 2;
                    const double tmidy = (y0 + y1) / 2;
                    const double th = sqrt(
                        (tmidx - c.X) * (tmidx - c.X) + (tmidy - c.Y) * (tmidy - c.Y)
                    );
                    const double areaTriangle = tbase * th / 2;
                    const double areaSegment = areaSector - areaTriangle;

                    // then add on trapezoid between the segment and 0
                    // the sign of the segment area is negative for bottom half of the circle,
                    // positive for top half
                    const double areaTrapezoid = (x1 - x0) * (y0 + y1) / 2;
                    const double newArea = circleSign * areaSegment + areaTrapezoid;
                    area += entranceExitSign * newArea;
                    if (xtest < c2.X) {
                        conventionalArea += entranceExitSign * newArea;
                    }
                    //(*fout) << "Circle[eeSign=" << entranceExitSign
                    //	<< " x0=(" << x0 << " x1=" << x1 << ")"
                    //	<< " c=(" << c.X << "," << c.Y << ")"
                    //	<< " circleSign=" << circleSign
                    //	<< " areaSector=" << areaSector
                    //	<< " areaTriangle=" << areaTriangle
                    //	<< " areaSegment=" << areaSegment
                    //	<< " areaTrapezoid=" << areaTrapezoid
                    //	<< " newArea(w/o sign)=" << newArea
                    //	<< " totalArea=" << area << "] ";
                }
            }
        }
    }

    Perf_CalcCutAreaCirc.Stop();
    (*fout) << "Area=" << area << " CA=" << conventionalArea << endl;

    return {area, conventionalArea};
}

void Adaptive2d::ApplyStockToLeave(Paths& inputPaths)
{
    ClipperOffset clipof;
    if (stockToLeave > NTOL) {
        clipof.Clear();
        clipof.AddPaths(inputPaths, JoinType::jtRound, EndType::etClosedPolygon);
        if (opType == OperationType::otClearingOutside
            || opType == OperationType::otProfilingOutside) {
            clipof.Execute(inputPaths, stockToLeave * scaleFactor);
        }
        else {
            clipof.Execute(inputPaths, -stockToLeave * scaleFactor);
        }
    }
    else {
        // fix for clipper glitches
        clipof.Clear();
        clipof.AddPaths(inputPaths, JoinType::jtRound, EndType::etClosedPolygon);
        clipof.Execute(inputPaths, -1);
        filterCloseValues(inputPaths);
        clipof.Clear();
        clipof.AddPaths(inputPaths, JoinType::jtRound, EndType::etClosedPolygon);
        clipof.Execute(inputPaths, 1);
        filterCloseValues(inputPaths);
    }
}

//********************************************
// Adaptive2d - Execute
//********************************************

std::list<AdaptiveOutput> Adaptive2d::Execute(
    const DPaths& stockPaths,
    const DPaths& paths,
    const DPaths& clearedPaths,
    std::function<bool(TPaths)> progressCallbackFn
)
{
    //**********************************
    // Initializations
    //**********************************

    // keep the tolerance in workable range
    tolerance = max(tolerance, 0.01);
    tolerance = min(tolerance, 1.0);

    // 1/"tolerance" = number of min-size adaptive steps per stepover
    scaleFactor = MIN_STEP_CLIPPER / tolerance / min(1.0, stepOverFactor * toolDiameter);

    current_region = 0;
    cout << "Tool Diameter: " << toolDiameter << endl;
    cout << "Min step size: " << round(MIN_STEP_CLIPPER / scaleFactor * 1000 * 10) / 10 << " um"
         << endl;
    cout << flush;

    toolRadiusScaled = long(toolDiameter * scaleFactor / 2);
    stepOverScaled = toolRadiusScaled * stepOverFactor;
    progressCallback = &progressCallbackFn;
    lastProgressTime = clock();
    stopProcessing = false;

    if (helixRampDiameter < NTOL) {
        helixRampDiameter = 0.75 * toolDiameter;
    }
    if (helixRampDiameter > toolDiameter) {
        helixRampDiameter = toolDiameter;
    }
    if (helixRampDiameter < toolDiameter / 8) {
        helixRampDiameter = toolDiameter / 8;
    }

    helixRampRadiusScaled = long(helixRampDiameter * scaleFactor / 2);
    finishPassOffsetScaled = finishingProfile ? long(stepOverScaled * FINISHING_THICKNESS_SCALE) : 0;

    ClipperOffset clipof;
    Clipper clip;

    // generate tool shape
    clipof.Clear();
    Path p;
    p << IntPoint(0, 0);
    clipof.AddPath(p, JoinType::jtRound, EndType::etOpenRound);
    Paths toolGeometryPaths;
    clipof.Execute(toolGeometryPaths, toolRadiusScaled);
    toolGeometry = toolGeometryPaths[0];
    // calculate reference area
    Path slotCut;
    TranslatePath(toolGeometryPaths[0], slotCut, IntPoint(toolRadiusScaled / 2, 0));
    clip.Clear();
    clip.AddPath(toolGeometryPaths[0], PolyType::ptSubject, true);
    clip.AddPath(slotCut, PolyType::ptClip, true);
    Paths crossing;
    clip.Execute(ClipType::ctDifference, crossing);
    referenceCutArea = fabs(Area(crossing[0]));
    optimalCutAreaPD = 2 * stepOverFactor * referenceCutArea / toolRadiusScaled;
#ifdef DEV_MODE
    cout << "optimalCutAreaPD:" << optimalCutAreaPD << " scaleFactor:" << scaleFactor
         << " toolRadiusScaled:" << toolRadiusScaled
         << " helixRampRadiusScaled:" << helixRampRadiusScaled << endl;
#endif
    //******************************
    // Convert input paths to clipper
    //******************************
    Paths converted;
    for (size_t i = 0; i < paths.size(); i++) {
        Path cpth;
        for (size_t j = 0; j < paths[i].size(); j++) {
            std::pair<double, double> pt = paths[i][j];
            cpth.push_back(IntPoint(long(pt.first * scaleFactor), long(pt.second * scaleFactor)));
        }
        Path cpth2;
        CleanPath(cpth, cpth2, FINISHING_CLEAN_PATH_TOLERANCE);
        converted.push_back(cpth2);
    }

    DeduplicatePaths(converted, inputPaths);
    ConnectPaths(inputPaths, inputPaths);
    SimplifyPolygons(inputPaths);
    ApplyStockToLeave(inputPaths);

    //*************************
    // convert stock paths
    //*************************
    stockInputPaths.clear();
    for (size_t i = 0; i < stockPaths.size(); i++) {
        Path cpth;
        for (size_t j = 0; j < stockPaths[i].size(); j++) {
            std::pair<double, double> pt = stockPaths[i][j];
            cpth.push_back(IntPoint(long(pt.first * scaleFactor), long(pt.second * scaleFactor)));
        }

        stockInputPaths.push_back(cpth);
    }

    // Convert cleared area
    Paths initialClearedPaths;
    for (size_t i = 0; i < clearedPaths.size(); i++) {
        Path p;
        for (size_t j = 0; j < clearedPaths[i].size(); j++) {
            long x = long(clearedPaths[i][j].first * scaleFactor);
            long y = long(clearedPaths[i][j].second * scaleFactor);
            p.push_back({x, y});
        }
        initialClearedPaths.push_back(p);
    }

    SimplifyPolygons(stockInputPaths);

    // Handle different clearing modes (inside/outside, clearing/profiling) and invoke the core
    // adaptive algorithm
    // 1) If outer clearing or outer profile: Reverse all paths
    // 2) If going outside the stock is allowed, add regionOutsideStock to both inputPaths and
    // clearedArea
    // 3) Compute toolBoundsFinished = offset(input paths, -(toolRadius + finishingThickness))
    // TODO apply stock to leave here, not earlier
    // 4a) If clearing: toolBoundsUnfinished = empty
    // 4b) If profiling:
    // toolBoundsUnfinished = reverse(offset(toolBoundsFinished, -profileThickness))
    // 5) toolBounds = toolBoundsFinished + toolBoundsUnfinished
    // 6) Loop over connected components using nesting level. Only draw top-level polygons from
    // toolBoundsFinished (holes can come from anywhere)
    // 7) finishingPass = offset(any located parts from the TBF category, finishingThickness)
    // 8) Compute bounds = offset(finishingPass + any non-finished boundaries, toolRadius)
    // 9) Run core algorithm on (bounds, toolBounds, finishingPass, clearedArea)
    // ---

    // 1) If outer clearing or outer profile: Reverse all paths
    if (opType == OperationType::otClearingOutside || opType == OperationType::otProfilingOutside) {
        ReversePaths(inputPaths);
    }

    ofstream fout("adaptive_debug.txt");
    this->fout = &fout;
    // 2) If going outside the stock is allowed, add regionOutsideStock to both inputPaths and
    // clearedArea
    if (!forceInsideOut) {
        // shrink the stock boundary to ensure overlap with input paths that hit the boundary
        Paths stockRev;
        clipof.Clear();
        clipof.AddPaths(stockInputPaths, JoinType::jtRound, EndType::etClosedPolygon);
        clipof.Execute(stockRev, -2);
        ReversePaths(stockRev);
        fout << "Stock input paths" << endl;
        for (Path& path : stockInputPaths) {
            fout << "[" << endl;
            for (IntPoint& p : path) {
                fout << "(" << p.X << ", " << p.Y << ")" << endl;
            }
            fout << "]" << endl;
        }
        fout << "Stock rev" << endl;
        for (Path& path : stockRev) {
            fout << "[" << endl;
            for (IntPoint& p : path) {
                fout << "(" << p.X << ", " << p.Y << ")" << endl;
            }
            fout << "]" << endl;
        }
        fout << "Input paths" << endl;
        for (Path& path : inputPaths) {
            fout << "[" << endl;
            for (IntPoint& p : path) {
                fout << "(" << p.X << ", " << p.Y << ")" << endl;
            }
            fout << "]" << endl;
        }

        // input paths
        Paths outsideOfStock;
        double overshootDistance = 4 * toolRadiusScaled
            + stockToLeave
                * scaleFactor;  // TODO think about this stock to leave thing, probably delete
        clipof.Clear();
        clipof.AddPaths(stockInputPaths, JoinType::jtSquare, EndType::etClosedPolygon);
        clipof.Execute(outsideOfStock, overshootDistance);

        fout << "outsideOfStock" << endl;
        for (Path& path : outsideOfStock) {
            fout << "[" << endl;
            for (IntPoint& p : path) {
                fout << "(" << p.X << ", " << p.Y << ")" << endl;
            }
            fout << "]" << endl;
        }

        clip.Clear();
        clip.AddPaths(inputPaths, PolyType::ptSubject, true);
        clip.AddPaths(stockRev, PolyType::ptClip, true);
        clip.AddPaths(outsideOfStock, PolyType::ptClip, true);
        clip.Execute(ClipType::ctUnion, inputPaths);

        fout << "Unioned input paths" << endl;
        for (Path& path : inputPaths) {
            fout << "[" << endl;
            for (IntPoint& p : path) {
                fout << "(" << p.X << ", " << p.Y << ")" << endl;
            }
            fout << "]" << endl;
        }

        // cleared area
        clipof.Clear();
        clipof.AddPaths(stockInputPaths, JoinType::jtSquare, EndType::etClosedPolygon);
        clipof.Execute(outsideOfStock, 100 * toolRadiusScaled);

        clip.Clear();
        clip.AddPaths(initialClearedPaths, PolyType::ptSubject, true);
        clip.AddPaths(stockRev, PolyType::ptClip, true);
        clip.AddPaths(outsideOfStock, PolyType::ptClip, true);
        clip.Execute(ClipType::ctUnion, initialClearedPaths);
    }

    // 3) Compute toolBoundsFinished = offset(input paths, -(toolRadius + finishingThickness))
    Paths toolBoundsFinished;
    clipof.Clear();
    clipof.AddPaths(inputPaths, JoinType::jtRound, EndType::etClosedPolygon);
    clipof.Execute(toolBoundsFinished, -(toolRadiusScaled + finishPassOffsetScaled));

    // 4a) If clearing: toolBoundsUnfinished = empty
    // 4b) If profiling: toolBoundsUnfinished = reverse(offset(toolBoundsFinished, -profileThickness))
    Paths toolBoundsUnfinished;
    if (opType == OperationType::otProfilingInside || opType == OperationType::otProfilingOutside) {
        clipof.Clear();
        clipof.AddPaths(toolBoundsFinished, JoinType::jtRound, EndType::etClosedPolygon);
        double offset = 2 * (helixRampRadiusScaled + toolRadiusScaled) + MIN_STEP_CLIPPER;
        clipof.Execute(toolBoundsUnfinished, -offset);
        ReversePaths(toolBoundsUnfinished);
    }

    // 5) toolBounds = toolBoundsFinished + toolBoundsUnfinished
    Paths toolBounds;
    for (auto path : toolBoundsFinished) {
        toolBounds.push_back(path);
    }
    for (auto path : toolBoundsUnfinished) {
        toolBounds.push_back(path);
    }

    // 6) Loop over connected components using nesting level. Only draw top-level polygons from
    // toolBoundsFinished (holes can come from anywhere)
    for (const auto& current : toolBoundsFinished) {
        int nesting = getPathNestingLevel(current, toolBounds);  // counts itself and the number of
                                                                 // polygons containing it
        if (nesting % 2 != 0) {                                  // current is an exterior boundary
            // current is an exterior boundary; now find all the holes directly inside it
            Paths currentTBF,
                currentTBU;  // finished/unfinished paths for the current connected component
            currentTBF.push_back(current);

            for (int iother = 0; iother < toolBounds.size(); iother++) {
                const auto& other = toolBounds[iother];
                const bool needsFinishingPass = iother < toolBoundsFinished.size();

                if (PointInPolygon(other.front(), current) != 0) {
                    if (getPathNestingLevel(other, toolBounds) == nesting + 1) {
                        if (needsFinishingPass) {
                            currentTBF.push_back(other);
                        }
                        else {
                            currentTBU.push_back(other);
                        }
                    }
                }
            }

            Paths currentTBP;
            for (Path& p : currentTBF) {
                currentTBP.push_back(p);
            }
            for (Path& p : currentTBU) {
                currentTBP.push_back(p);
            }

            // 7) finishingPass = offset(any located parts from the TBF category, finishingThickness)
            Paths finishingPass;
            clipof.Clear();
            clipof.AddPaths(currentTBF, JoinType::jtRound, EndType::etClosedPolygon);
            clipof.Execute(finishingPass, finishPassOffsetScaled);

            // 8) Compute bounds = offset(finishingPass + any non-finished boundaries, toolRadius)
            Paths boundPath;
            clipof.Clear();
            clipof.AddPaths(finishingPass, JoinType::jtRound, EndType::etClosedPolygon);
            clipof.AddPaths(currentTBU, JoinType::jtRound, EndType::etClosedPolygon);
            // 3 = number of offsets in computing boundPaths > max error in boundary
            // Subtracting 3 ensures that rounding in boundPath is guaranteed to shrink it
            // This is important for successfully filtering out boundPaths that should be fully
            // covered by initialClearPaths
            clipof.Execute(boundPath, toolRadiusScaled - 3);

            // Skip path generation if bounds are fully cleared
            {
                Paths boundsToClear;
                clip.Clear();
                clip.AddPaths(boundPath, PolyType::ptSubject, true);
                clip.AddPaths(initialClearedPaths, PolyType::ptClip, true);
                clip.Execute(ClipType::ctDifference, boundsToClear);
                if (!boundsToClear.size()) {
                    continue;
                }
            }

            // 9) Run core algorithm on (bounds, toolBounds, finishingPass, clearedArea)
            ProcessPolyNode(boundPath, currentTBP, finishingPass, initialClearedPaths);
        }
    }

    return results;
}

bool Adaptive2d::FindEntryPoint(
    TPaths& progressPaths,
    const Paths& toolBoundPaths,
    const Paths& boundPaths,
    ClearedArea& clearedArea /*output-initial cleared area by helix*/,
    IntPoint& entryPoint /*output*/,
    IntPoint& toolPos,
    DoublePoint& toolDir
)
{
    Paths incOffset;
    Paths lastValidOffset;
    Clipper clip;
    ClipperOffset clipof;
    bool found = false;
    Paths clearedPaths;

    Paths checkPaths;
    clip.Clear();
    clip.AddPaths(toolBoundPaths, PolyType::ptSubject, true);
    clip.AddPaths(clearedArea.GetCleared(), PolyType::ptClip, true);
    clip.Execute(ClipType::ctDifference, checkPaths);

    for (int iter = 0; iter < 10; iter++) {
        clipof.Clear();
        clipof.AddPaths(checkPaths, JoinType::jtSquare, EndType::etClosedPolygon);
        double step = MIN_STEP_CLIPPER;
        double currentDelta = -1;
        clipof.Execute(incOffset, currentDelta);
        while (!incOffset.empty()) {
            clipof.Execute(incOffset, currentDelta);
            if (!incOffset.empty()) {
                lastValidOffset = incOffset;
            }
            currentDelta -= step;
        }
        for (size_t i = 0; i < lastValidOffset.size(); i++) {
            if (!lastValidOffset[i].empty()) {
                entryPoint = Compute2DPolygonCentroid(lastValidOffset[i]);
                found = true;
                break;
            }
        }
        // check if the start point is in any of the holes
        // this may happen in case when toolBoundPaths are symmetric (boundary + holes)
        // we need to break simetry and try again
        for (size_t j = 0; j < checkPaths.size(); j++) {
            int pip = PointInPolygon(entryPoint, checkPaths[j]);
            if ((j == 0 && pip == 0) || (j > 0 && pip != 0)) {
                found = false;
                break;
            }
        }
        // check if helix fits
        if (found) {
            // make initial polygon cleared by helix ramp
            clipof.Clear();
            Path p1;
            p1.push_back(entryPoint);
            clipof.AddPath(p1, JoinType::jtRound, EndType::etOpenRound);
            clipof.Execute(clearedPaths, helixRampRadiusScaled + toolRadiusScaled);
            CleanPolygons(clearedPaths);
            // we got first cleared area - check if it is crossing boundary
            clip.Clear();
            clip.AddPaths(clearedPaths, PolyType::ptSubject, true);
            clip.AddPaths(boundPaths, PolyType::ptClip, true);
            Paths crossing;
            clip.Execute(ClipType::ctDifference, crossing);
            if (!crossing.empty()) {
                // helix does not fit to the cutting area
                found = false;
            }
            else {
                clearedArea.AddClearedPaths(clearedPaths);
            }
        }

        if (!found) {  // break simetry and try again
            clip.Clear();
            clip.AddPaths(checkPaths, PolyType::ptSubject, true);
            auto bounds = clip.GetBounds();
            clip.Clear();
            Path rect;
            rect << IntPoint(bounds.left, bounds.bottom);
            rect << IntPoint(bounds.left, (bounds.top + bounds.bottom) / 2);
            rect << IntPoint((bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2);
            rect << IntPoint((bounds.left + bounds.right) / 2, bounds.bottom);
            clip.AddPath(rect, PolyType::ptSubject, true);
            clip.AddPaths(checkPaths, PolyType::ptClip, true);
            clip.Execute(ClipType::ctIntersection, checkPaths);
        }
        if (found) {
            break;
        }
    }

    if (!found) {
        cerr << "Start point not found!" << endl;
        cout << "Tool bound paths (" << toolBoundPaths.size() << "):" << endl;
        for (const Path& path : toolBoundPaths) {
            cout << "[" << endl;
            for (const IntPoint& p : path) {
                cout << "(" << p.X << ", " << p.Y << ")" << endl;
            }
            cout << "]" << endl;
        }
    }
    if (found) {
        // visualize/progress for helix
        clipof.Clear();
        Path hp;
        hp << entryPoint;
        clipof.AddPath(hp, JoinType::jtRound, EndType::etOpenRound);
        Paths hps;
        clipof.Execute(hps, helixRampRadiusScaled);
        AddPathsToProgress(progressPaths, hps);

        toolPos = IntPoint(entryPoint.X, entryPoint.Y - helixRampRadiusScaled);
        toolDir = DoublePoint(1.0, 0.0);
    }
    return found;
}

//************************************************************
//  IsClearPath - returns true if path is clear from obstacles
//***********************************************************
bool Adaptive2d::IsClearPath(const Path& tp, ClearedArea& cleared, double safetyClearance)
{
    Perf_IsClearPath.Start();
    Clipper clip;
    ClipperOffset clipof;
    clipof.AddPath(tp, JoinType::jtRound, EndType::etOpenRound);
    Paths toolShape;
    clipof.Execute(toolShape, toolRadiusScaled + safetyClearance);
    clip.AddPaths(toolShape, PolyType::ptSubject, true);
    clip.AddPaths(cleared.GetCleared(), PolyType::ptClip, true);
    Paths crossing;
    clip.Execute(ClipType::ctDifference, crossing);
    double collisionArea = 0;
    for (auto& p : crossing) {
        collisionArea += fabs(Area(p));
    }
    Perf_IsClearPath.Stop();
    return collisionArea < 1.0;
}

bool Adaptive2d::IsAllowedToCutTrough(
    const IntPoint& p1,
    const IntPoint& p2,
    ClearedArea& cleared,
    const Paths& toolBoundPaths,
    double areaFactor,
    bool skipBoundsCheck
)
{
    Perf_IsAllowedToCutTrough.Start();

    if (!skipBoundsCheck && !IsPointWithinCutRegion(toolBoundPaths, p2)) {
        // last point outside boundary - its not clear to cut
        Perf_IsAllowedToCutTrough.Stop();
        return false;
    }
    else if (!skipBoundsCheck && !IsPointWithinCutRegion(toolBoundPaths, p1)) {
        // first point outside boundary - its not clear to cut
        Perf_IsAllowedToCutTrough.Stop();
        return false;
    }
    else {
        Clipper clip;
        double distance = sqrt(DistanceSqrd(p1, p2));
        double stepSize = min(0.5 * stepOverScaled, 8 * MIN_STEP_CLIPPER);
        if (distance < stepSize / 2) {  // not significant cut
            Perf_IsAllowedToCutTrough.Stop();
            return true;
        }
        if (distance < stepSize) {  // adjust for numeric instability with small distances
            areaFactor *= 2;
        }

        IntPoint toolPos1 = p1;
        long steps = long(distance / stepSize) + 1;
        stepSize = distance / steps;
        for (long i = 1; i <= steps; i++) {
            double p = double(i) / steps;
            IntPoint toolPos2(
                long(p1.X + double(p2.X - p1.X) * p),
                long(p1.Y + double(p2.Y - p1.Y) * p)
            );
            double area = CalcCutArea(clip, toolPos1, toolPos2, cleared).first;
            // if we are cutting above optimal -> not clear to cut
            if (area > areaFactor * stepSize * optimalCutAreaPD) {
                Perf_IsAllowedToCutTrough.Stop();
                return false;
            }
            // if tool is outside boundary -> its not clear to cut
            if (!skipBoundsCheck && !IsPointWithinCutRegion(toolBoundPaths, toolPos2)) {
                Perf_IsAllowedToCutTrough.Stop();
                return false;
            }
            toolPos1 = toolPos2;
        }
    }
    Perf_IsAllowedToCutTrough.Stop();
    return true;
}

bool Adaptive2d::ResolveLinkPath(
    const IntPoint& startPoint,
    const IntPoint& endPoint,
    ClearedArea& clearedArea,
    Path& output
)
{
    // Future work: reimplement this function with a modified version of the
    // algorithm from https://annas-archive.li/scidb/10.1002/net.3230140304/
    // 1) offset cleared area by tool radius, to effectively reduce the tool to a point
    // 2) actually further offset by whatever distance we want G0 to be from stock
    // 3) if start/end are outside that area, generate straight-line paths to them
    // 4) perform delaunay triangulation with clipper 2 (needs upgraded clipper!)
    // 5) do shortest path search along the triangulation from start to end,
    // producing a triangle strip (exact details tbd)
    // 6) finish the shortest path algorithm as specified in the paper
    // The result is O(n log n) shortest path (up to selection of the
    // appropriate triangle strip), which should be an improvement in both run
    // time and result over our current algorithm

    vector<pair<IntPoint, IntPoint>> queue;
    queue.emplace_back(startPoint, endPoint);
    Path checkPath;
    double totalLength = 0;
    double directDistance = sqrt(DistanceSqrd(startPoint, endPoint));
    Paths linkPaths;

    double scanStep = 2 * MIN_STEP_CLIPPER;
    if (scanStep > scaleFactor * 0.1) {
        scanStep = scaleFactor * 0.1;
    }
    if (scanStep < scaleFactor * 0.01) {
        scanStep = scaleFactor * 0.01;
    }
    long limit = 10000;

    double clearance = stepOverScaled;
    double offClearance = 2 * stepOverScaled;
    if (offClearance > directDistance / 2) {
        offClearance = directDistance / 2;
        clearance = 0;
    }

    long cnt = 0;

    // to hold CLP results
    IntPoint clp;
    size_t pindex;
    size_t sindex;
    double par;

    // put a time limit on the resolving the link path
    clock_t time_limit = (clock_t)(max(keepToolDownDistRatio, 3.0) * CLOCKS_PER_SEC / 6);

    clock_t time_out = clock() + time_limit;

    while (!queue.empty()) {
        if (stopProcessing) {
            return false;
        }
        if (clock() > time_out) {
            cout << "Unable to resolve tool down linking path (limit reached)." << endl;
            return false;
        }

        cnt++;
        if (cnt > limit) {
            cout << "Unable to resolve tool down linking path @(" << endPoint.X / scaleFactor << ","
                 << endPoint.Y / scaleFactor << ") (" << limit << " points limit reached)." << endl;
            return false;
        }
        pair<IntPoint, IntPoint> pointPair = queue.back();
        queue.pop_back();

        // check for self intersections - if found discard the link path
        for (size_t i = 0; i < linkPaths.size(); i++) {
            if (linkPaths[i].front() != pointPair.first && linkPaths[i].back() != pointPair.first
                && linkPaths[i].front() != pointPair.second && linkPaths[i].back() != pointPair.second
                && IntersectionPoint(
                    linkPaths[i].front(),
                    linkPaths[i].back(),
                    pointPair.first,
                    pointPair.second,
                    clp
                )) {
                cout << "Unable to resolve tool down linking path (self-intersects)." << endl;
                return false;
            }
        }

        DoublePoint direction = DirectionV(pointPair.first, pointPair.second);
        checkPath.clear();
        if (pointPair.first == startPoint) {
            checkPath.push_back(IntPoint(
                pointPair.first.X + offClearance * direction.X,
                pointPair.first.Y + offClearance * direction.Y
            ));
        }
        else {
            checkPath.push_back(pointPair.first);
        }
        if (pointPair.second == endPoint) {
            checkPath.push_back(IntPoint(
                pointPair.second.X - offClearance * direction.X,
                pointPair.second.Y - offClearance * direction.Y
            ));
        }
        else {
            checkPath.push_back(pointPair.second);
        }

        if (IsClearPath(checkPath, clearedArea, clearance)) {
            totalLength += sqrt(DistanceSqrd(pointPair.first, pointPair.second));
            if (totalLength > keepToolDownDistRatio * directDistance) {
                return false;
            }
            Path link;
            link.push_back(pointPair.first);
            link.push_back(pointPair.second);
            linkPaths.push_back(link);
        }
        else {
            if (sqrt(DistanceSqrd(pointPair.first, pointPair.second)) < 4) {
                // segment became too short but still not clear
                return false;
            }
            DoublePoint pDir(-direction.Y, direction.X);
            // find mid point
            IntPoint midPoint(
                0.5 * double(pointPair.first.X + pointPair.second.X),
                0.5 * double(pointPair.first.Y + pointPair.second.Y)
            );
            for (long i = 1;; i++) {
                if (stopProcessing) {
                    return false;
                }
                double offset = i * scanStep;
                IntPoint checkPoint1(midPoint.X + offset * pDir.X, midPoint.Y + offset * pDir.Y);
                IntPoint checkPoint2(midPoint.X - offset * pDir.X, midPoint.Y - offset * pDir.Y);

                if (DistancePointToPathsSqrd(clearedArea.GetCleared(), checkPoint1, clp, pindex, sindex, par)
                    < DistancePointToPathsSqrd(
                        clearedArea.GetCleared(),
                        checkPoint2,
                        clp,
                        pindex,
                        sindex,
                        par
                    )) {
                    // exchange points
                    IntPoint tmp = checkPoint2;
                    checkPoint2 = checkPoint1;
                    checkPoint1 = tmp;
                }

                checkPath.clear();
                checkPath.push_back(checkPoint1);
                if (IsClearPath(checkPath, clearedArea, clearance + 1)) {  // check if point clear
                    queue.emplace_back(pointPair.first, checkPoint1);
                    queue.emplace_back(checkPoint1, pointPair.second);
                    break;
                }
                else {  // check the other side

                    checkPath.clear();
                    checkPath.push_back(checkPoint2);
                    if (IsClearPath(checkPath, clearedArea, clearance + 1)) {
                        queue.emplace_back(pointPair.first, checkPoint2);
                        queue.emplace_back(checkPoint2, pointPair.second);
                        break;
                    }
                }
                if (offset > keepToolDownDistRatio * directDistance) {
                    return false;  // can't find keep tool down link
                }
            }
        }
    }
    if (linkPaths.empty()) {
        return false;
    }
    ConnectPaths(linkPaths, linkPaths);
    output = linkPaths[0];
    return true;
}

bool Adaptive2d::MakeLeadPath(
    bool leadIn,
    const IntPoint& startPoint,
    const DoublePoint& startDir,
    IntPoint beaconPoint,
    ClearedArea& clearedAreaOriginal,
    const Paths& toolBoundPaths,
    Path& output
)
{
    output.push_back(startPoint);
    double stepSize = min(MIN_STEP_CLIPPER * 8, 0.2 * stepOverScaled + 1);

    // make a copy of clearedArea to update as the path progresses (for lead out only)
    ClearedArea clearedArea(toolRadiusScaled);
    clearedArea.SetClearedPaths(clearedAreaOriginal.GetCleared());

    // compute acceptable tool end locations
    ClipperOffset clipof;
    clipof.Clear();
    Paths cleared;
    clipof.AddPaths(clearedArea.GetCleared(), JoinType::jtRound, EndType::etClosedPolygon);
    clipof.Execute(cleared, -(toolRadiusScaled + stepSize));

    // move the beacon to an acceptable location if necessary
    if (cleared.size() == 0) {
        (*fout) << "MakeLeadPath (in? " << leadIn << ") No valid beacon locations!!" << endl;
        return false;
    }

    (*fout) << "MakeLeadPath (in? " << leadIn << ") start (" << startPoint.X << ", " << startPoint.Y
            << ") dir (" << startDir.X << ", " << startDir.Y << ") beacon (" << beaconPoint.X
            << ", " << beaconPoint.Y << ")" << endl;

    if (getPathNestingLevel({beaconPoint}, cleared) % 2 == 0) {
        IntPoint clp;  // to store closest point
        size_t clpPathIndex;
        size_t clpSegmentIndex;
        double clpParameter;
        DistancePointToPathsSqrd(cleared, beaconPoint, clp, clpPathIndex, clpSegmentIndex, clpParameter);
        beaconPoint = clp;
        (*fout) << "Moved beacon point to (" << beaconPoint.X << ", " << beaconPoint.Y << ")" << endl;
    }

    IntPoint currentPoint = startPoint;
    DoublePoint targetDir = DirectionV(currentPoint, beaconPoint);

    (*fout) << "beaconDir (" << targetDir.X << ", " << targetDir.Y << ")" << endl;

    double distanceToBeacon = sqrt(DistanceSqrd(startPoint, beaconPoint));
    double minExitLength = min(toolRadiusScaled / 5., min(stepOverScaled, distanceToBeacon / 2));
    double maxLength = max(distanceToBeacon * 2, stepSize * 10);
    std::optional<double> clearedStartLen;
    DoublePoint nextDir = startDir;
    IntPoint nextPoint
        = IntPoint(currentPoint.X + nextDir.X * stepSize, currentPoint.Y + nextDir.Y * stepSize);
    Path checkPath;
    double adaptFactor = 0.4;
    double alfa = std::numbers::pi / 64;
    double pathLen = 0;
    checkPath.push_back(currentPoint);
    for (int i = 0; i < 10000; i++) {
        if (IsAllowedToCutTrough(currentPoint, nextPoint, clearedArea, toolBoundPaths)) {
            if (!leadIn) {
                // For lead out paths, update/recompute the cleared area
                checkPath.push_back(nextPoint);
                clearedArea.ExpandCleared(checkPath);
                checkPath.clear();
                checkPath.push_back(nextPoint);

                clipof.Clear();
                clipof.AddPaths(clearedArea.GetCleared(), JoinType::jtRound, EndType::etClosedPolygon);
                clipof.Execute(cleared, -(toolRadiusScaled + stepSize));
            }

            output.push_back(nextPoint);
            currentPoint = nextPoint;
            pathLen += stepSize;
            targetDir = DirectionV(currentPoint, beaconPoint);
            (*fout) << "\tCut to (" << nextPoint.X << ", " << nextPoint.Y << ") dir (" << nextDir.X
                    << ", " << nextDir.Y << ") targetDir (" << targetDir.X << ", " << targetDir.Y
                    << ")" << endl;
            nextDir = DoublePoint(
                nextDir.X + adaptFactor * targetDir.X,
                nextDir.Y + adaptFactor * targetDir.Y
            );
            NormalizeV(nextDir);

            // check if cleared
            if (getPathNestingLevel({currentPoint}, cleared) % 2 == 1) {
                if (!clearedStartLen) {
                    clearedStartLen = {pathLen};
                }

                // if the path is long enough, exit with success
                if (pathLen > minExitLength && pathLen - *clearedStartLen > MIN_STEP_CLIPPER) {
                    (*fout) << "Success" << endl;
                    return true;
                }
            }
            else {
                clearedStartLen = {};
            }

            // if traveled too far without getting to a clear area, exit with failure
            if (pathLen > maxLength) {
                if (getPathNestingLevel({currentPoint}, clearedArea.GetCleared()) % 2 == 1) {
                    (*fout) << "Success, barely into cleared area" << endl;
                    return true;
                }
                else {
                    (*fout) << "Failed: overtravel without getting to cleared area" << endl;
                    cerr << "MakeLeadPath failed: overtravel without getting to cleared area" << endl;
                    return false;
                }
            }
        }
        else {
            (*fout) << "\tFailed cut to (" << nextPoint.X << ", " << nextPoint.Y << "), rotating"
                    << endl;
            nextDir = rotate(nextDir, leadIn ? -alfa : alfa);
        }
        nextPoint
            = IntPoint(currentPoint.X + nextDir.X * stepSize, currentPoint.Y + nextDir.Y * stepSize);
    }

    (*fout) << "Failed: iterations" << endl;
    return false;
}

std::optional<TPaths> Adaptive2d::FindLinkPath(
    const std::optional<IntPoint>& prevPoint,
    const IntPoint& pathStart,
    const DoublePoint& pathDir,
    ClearedArea& cleared,
    const Paths& toolBoundPaths
)
{
    Perf_AppendToolPath.Start();
    TPaths result;

    IntPoint endPoint(pathStart);

    // if the link distance is very short, no special linking is required
    double linkDistance = prevPoint ? sqrt(DistanceSqrd(*prevPoint, endPoint)) : stepOverScaled;
    if (linkDistance >= NTOL) {
        size_t clpPathIndex;
        size_t clpSegmentIndex;
        double clpParameter;
        IntPoint clp;

        double beaconOffset = max(min(stepOverScaled, linkDistance / 2) * 1.5, 8 * MIN_STEP_CLIPPER);

        // plan the lead in, as a reverse-direction lead out
        double eDistToBounds = DistancePointToPathsSqrd(
            toolBoundPaths,
            endPoint,
            clp,
            clpPathIndex,
            clpSegmentIndex,
            clpParameter
        );

        DoublePoint revEndDir = {-pathDir.X, -pathDir.Y};

        DoublePoint endBoundaryDir = GetPathDirectionV(toolBoundPaths[clpPathIndex], clpSegmentIndex);
        if (eDistToBounds > beaconOffset) {
            endBoundaryDir = pathDir;  // if boundary is far away, use beacon to leave the path
        }
        DoublePoint endBeaconDir = {revEndDir.X - endBoundaryDir.Y, revEndDir.Y + endBoundaryDir.X};
        NormalizeV(endBeaconDir);

        IntPoint endBeacon(
            endPoint.X + beaconOffset * endBeaconDir.X,
            endPoint.Y + beaconOffset * endBeaconDir.Y
        );

        Path leadInPath;
        bool leadInOk
            = MakeLeadPath(true, endPoint, revEndDir, endBeacon, cleared, toolBoundPaths, leadInPath);
        ReversePath(leadInPath);
        cout << "MakeLeadIn:" << endl;
        cout << "\trevEndDir (" << revEndDir.X << ", " << revEndDir.Y << ")" << endl;
        cout << "\tendBoundaryDir (" << endBoundaryDir.X << ", " << endBoundaryDir.Y << ")" << endl;
        cout << "\tendBeaconDir (" << endBeaconDir.X << ", " << endBeaconDir.Y << ")" << endl;
        cout << "\tok? " << leadInOk << endl;
        for (auto& p : leadInPath) {
            cout << "\t(" << p.X << "," << p.Y << ")" << endl;
        }
        cout << endl;

        if (!leadInOk) {
            Perf_AppendToolPath.Stop();
            return {};
        }

        // Compute linking path
        Path linkPath;
        MotionType linkType = MotionType::mtCutting;

        if (prevPoint) {
            if (ResolveLinkPath(*prevPoint, leadInPath.front(), cleared, linkPath)) {
                linkType = MotionType::mtLinkClear;
                double remainingLeadInExtension = stepOverScaled / 2;
                while (linkPath.size() >= 2 && remainingLeadInExtension > NTOL) {
                    IntPoint p1 = linkPath.at(linkPath.size() - 2);
                    IntPoint p2 = linkPath.at(linkPath.size() - 1);
                    double l = sqrt(DistanceSqrd(p1, p2));
                    if (l >= remainingLeadInExtension) {
                        IntPoint splitPoint(
                            p1.X + (p2.X - p1.X) * (l - remainingLeadInExtension) / l,
                            p1.Y + (p2.Y - p1.Y) * (l - remainingLeadInExtension) / l
                        );
                        linkPath.pop_back();
                        linkPath.push_back(splitPoint);
                        leadInPath.insert(leadInPath.begin(), splitPoint);
                        remainingLeadInExtension = 0;
                        Path checkPath;
                        checkPath.push_back(p2);
                        checkPath.push_back(splitPoint);
                        if (!IsClearPath(checkPath, cleared, 0)) {
                            remainingLeadInExtension = stepOverScaled / 2;
                        }
                    }
                    else {
                        linkPath.pop_back();
                        leadInPath.insert(leadInPath.begin(), p1);
                        remainingLeadInExtension -= l;
                        if (remainingLeadInExtension < NTOL) {
                            Path checkPath;
                            checkPath.push_back(p2);
                            checkPath.push_back(p1);
                            if (!IsClearPath(checkPath, cleared, 0)) {
                                remainingLeadInExtension = stepOverScaled / 2;
                            }
                        }
                    }
                }
            }
            else {
                linkType = MotionType::mtLinkNotClear;
                double dist = sqrt(DistanceSqrd(*prevPoint, leadInPath.front()));
                if (dist < 2 * stepOverScaled
                    && IsAllowedToCutTrough(
                        IntPoint(
                            prevPoint->X + (leadInPath.front().X - prevPoint->X) / dist,
                            prevPoint->Y + (leadInPath.front().Y - prevPoint->Y) / dist
                        ),
                        IntPoint(
                            leadInPath.front().X - (leadInPath.front().X - prevPoint->X) / dist,
                            leadInPath.front().Y - (leadInPath.front().Y - prevPoint->Y) / dist
                        ),
                        cleared,
                        toolBoundPaths
                    )) {
                    linkType = MotionType::mtCutting;
                }
                // add direct linking move at clear height

                linkPath.clear();
                linkPath.push_back(*prevPoint);
                linkPath.push_back(leadInPath.front());
            }
        }

        /* paths smoothing*/
        Paths linkPaths;
        linkPaths.push_back(linkPath);
        linkPaths.push_back(leadInPath);

        if (linkType == MotionType::mtLinkClear) {
            SmoothPaths(linkPaths, 0.1 * stepOverScaled, 1, 4);
        }

        linkPath = linkPaths[0];
        leadInPath = linkPaths[1];

        if (prevPoint) {
            // add linking path
            TPath linkPath1;
            linkPath1.first = linkType;
            for (const auto& pt : linkPath) {
                linkPath1.second.emplace_back(double(pt.X) / scaleFactor, double(pt.Y) / scaleFactor);
            }
            result.push_back(linkPath1);
        }

        // add lead-in move
        TPath linkPath2;
        linkPath2.first = MotionType::mtCutting;
        for (const auto& pt : leadInPath) {
            linkPath2.second.emplace_back(double(pt.X) / scaleFactor, double(pt.Y) / scaleFactor);
        }

        result.push_back(linkPath2);
    }

    Perf_AppendToolPath.Stop();
    return {result};
}

std::optional<std::pair<IntPoint, DoublePoint>> Adaptive2d::AppendToolPath(
    AdaptiveOutput& output,
    const Path& passToolPath,
    TPaths& linkPath,
    ClearedArea& cleared,
    const Paths& toolBoundPaths
)
{
    std::optional<std::pair<IntPoint, DoublePoint>> result;  // new toolPos and toolDir after lead out

    Perf_AppendToolPath.Start();

    for (TPath& lp : linkPath) {
        output.AdaptivePaths.push_back(lp);
    }

    TPath cutPath;
    cutPath.first = MotionType::mtCutting;
    for (const auto& p : passToolPath) {
        DPoint nextT;
        nextT.first = double(p.X) / scaleFactor;
        nextT.second = double(p.Y) / scaleFactor;
        cutPath.second.push_back(nextT);
    }

    if (!cutPath.second.empty()) {
        output.AdaptivePaths.push_back(cutPath);

        // plan the lead out
        if (passToolPath.size() >= 2) {
            IntPoint prevPoint = passToolPath.back();
            DoublePoint prevDir = GetPathDirectionV(passToolPath, passToolPath.size() - 1);

            size_t clpPathIndex;
            size_t clpSegmentIndex;
            double clpParameter;
            IntPoint clp;
            double distToBounds = DistancePointToPathsSqrd(
                toolBoundPaths,
                prevPoint,
                clp,
                clpPathIndex,
                clpSegmentIndex,
                clpParameter
            );

            DoublePoint boundaryDir = GetPathDirectionV(toolBoundPaths[clpPathIndex], clpSegmentIndex);
            double beaconOffset
                = max(min(stepOverScaled, PathLength(passToolPath) / 2) * 1.5, 8 * MIN_STEP_CLIPPER);
            if (distToBounds > beaconOffset) {
                boundaryDir = prevDir;  // if boundary is far away, use beacon to leave the path
            }
            DoublePoint beaconDir = {prevDir.X - boundaryDir.Y, prevDir.Y + boundaryDir.X};
            NormalizeV(beaconDir);

            IntPoint beacon(
                prevPoint.X + beaconOffset * beaconDir.X,
                prevPoint.Y + beaconOffset * beaconDir.Y
            );

            Path leadOutPath;
            bool ok
                = MakeLeadPath(false, prevPoint, prevDir, beacon, cleared, toolBoundPaths, leadOutPath);
            cout << "MakeLeadOut:" << endl;
            cout << "\tstartDir (" << prevDir.X << ", " << prevDir.Y << ")" << endl;
            cout << "\tstartBoundaryDir (" << boundaryDir.X << ", " << boundaryDir.Y << ")" << endl;
            cout << "\tstartBeaconDir (" << beaconDir.X << ", " << beaconDir.Y << ")" << endl;
            cout << "\tok? " << ok << endl;
            for (auto& p : leadOutPath) {
                cout << "\t(" << p.X << "," << p.Y << ")" << endl;
            }
            cout << endl;

            if (ok && leadOutPath.size() >= 1) {
                // smooth path
                Paths linkPaths;
                linkPaths.push_back(leadOutPath);
                SmoothPaths(linkPaths, 0.1 * stepOverScaled, 1, 4);
                leadOutPath = linkPaths[0];

                // scale and output
                TPath out;
                out.first = MotionType::mtCutting;
                for (auto& p : leadOutPath) {
                    out.second.push_back({((double)p.X) / scaleFactor, ((double)p.Y) / scaleFactor});
                }
                output.AdaptivePaths.push_back(out);
                cleared.ExpandCleared(leadOutPath);

                IntPoint p2 = leadOutPath.back();
                IntPoint p1 = leadOutPath.size() >= 2 ? leadOutPath[leadOutPath.size() - 2]
                                                      : prevPoint;
                DoublePoint dir = DirectionV(p1, p2);

                result = {{p2, dir}};
            }
        }
    }
    Perf_AppendToolPath.Stop();

    return result;
}

void Adaptive2d::CheckReportProgress(TPaths& progressPaths, bool force)
{
    if (!force && (clock() - lastProgressTime < PROGRESS_TICKS)) {
        return;  // not yet
    }
    lastProgressTime = clock();
    if (progressPaths.empty()) {
        return;
    }
    if (progressCallback) {
        if ((*progressCallback)(progressPaths)) {
            stopProcessing = true;  // call python function, if returns true signal stop processing
        }
    }
    // clean the paths - keep the last point
    if (progressPaths.back().second.empty()) {
        return;
    }
    TPath* lastPath = &progressPaths.back();
    DPoint* lastPoint = &lastPath->second.back();
    DPoint next(lastPoint->first, lastPoint->second);
    while (progressPaths.size() > 1) {
        progressPaths.pop_back();
    }
    while (!progressPaths.front().second.empty()) {
        progressPaths.front().second.pop_back();
    }
    progressPaths.front().first = MotionType::mtCutting;
    progressPaths.front().second.push_back(next);
}

void Adaptive2d::AddPathsToProgress(TPaths& progressPaths, Paths paths, MotionType mt)
{
    for (const auto& pth : paths) {
        if (!pth.empty()) {
            progressPaths.push_back(TPath());
            progressPaths.back().first = mt;
            for (const auto pt : pth) {
                progressPaths.back().second.emplace_back(
                    double(pt.X) / scaleFactor,
                    double(pt.Y) / scaleFactor
                );
            }
            progressPaths.back().second.emplace_back(
                double(pth.front().X) / scaleFactor,
                double(pth.front().Y) / scaleFactor
            );
        }
    }
}

void Adaptive2d::AddPathToProgress(TPaths& progressPaths, const Path pth, MotionType mt)
{
    if (!pth.empty()) {
        progressPaths.push_back(TPath());
        progressPaths.back().first = mt;
        for (const auto pt : pth) {
            progressPaths.back().second.emplace_back(
                double(pt.X) / scaleFactor,
                double(pt.Y) / scaleFactor
            );
        }
    }
}

// performs the intersection of the closed path (subject) and the area (obj), preserving
// orientation and (closed-path) connectivity
Paths PathIntersectArea(Clipper& clip, Path& subject, const Paths& obj, ofstream& fout)
{
    subject.push_back(subject[0]);  // close path explicitly before treating it as open

    // init z-data: p[i].z = 2 * i + 1, and new points are the average of their neighbors
    // this ensures new points have unique z but come between the points they're made from
    for (int i = 0; i < subject.size(); i++) {
        subject[i].Z = i * 2 + 1;
    }
    auto zfill = [](IntPoint& e1b, IntPoint& e1t, IntPoint& e2b, IntPoint& e2t, IntPoint& p) {
        if (e1b.Z != 0 && e1t.Z != 0) {
            p.Z = (e1b.Z + e1t.Z) / 2;
        }
        else if (e2b.Z != 0 && e2t.Z != 0) {
            p.Z = (e2b.Z + e2t.Z) / 2;
        }
    };
    clip.ZFillFunction(zfill);

    PolyTree diffTree;
    Paths diff;
    clip.Clear();
    clip.AddPath(subject, PolyType::ptSubject, false);
    clip.AddPaths(obj, PolyType::ptClip, true);
    clip.Execute(ClipType::ctIntersection, diffTree);
    clip.ZFillFunction(0);
    OpenPathsFromPolyTree(diffTree, diff);

    // restore orientation
    for (Path& p : diff) {
        bool needsReverse = false;
        if (p.size() >= 2) {
            if (p[0].Z + 1 != p[1].Z && p[0].Z + 2 != p[1].Z) {
                ReversePath(p);
            }
        }
    }

    // collect result, joining any path that goes through the end point
    const int zstart = 1;
    const int zend = subject.size() * 2 - 1;
    std::optional<Path> start, end;
    Paths result;
    for (Path& p : diff) {
        if (p[0].Z == zstart) {
            start = {p};
        }
        else if (p.back().Z == zend) {
            end = {p};
        }
        else {
            result.push_back(p);
        }
    }
    if (start && end) {
        Path joined = *end;
        // append points from start, skipping the first, which is a repeat
        for (int i = 1; i < start->size(); i++) {
            joined.push_back((*start)[i]);
        }
        result.push_back(joined);
    }
    else {
        if (start) {
            result.push_back(*start);
        }
        if (end) {
            result.push_back(*end);
        }
    }

    // debug
    fout << "Subject: [" << endl;
    for (IntPoint& p : subject) {
        fout << "\t(" << p.X << ", " << p.Y << ", " << p.Z << ")" << endl;
    }
    fout << "]" << endl;

    fout << "Diff:" << endl;
    for (Path& path : diff) {
        fout << "[" << endl;
        for (IntPoint& p : path) {
            fout << "\t(" << p.X << ", " << p.Y << ", " << p.Z << ")" << endl;
        }
        fout << "]" << endl;
    }

    fout << "Result:" << endl;
    for (Path& path : result) {
        fout << "[" << endl;
        for (IntPoint& p : path) {
            fout << "\t(" << p.X << ", " << p.Y << ", " << p.Z << ")" << endl;
        }
        fout << "]" << endl;
    }

    return result;
}

struct IterateNextStepOutput
{
    std::optional<double> iterationAngle;
    bool tooManyIterations = false;
    bool failed;
    double area;
    double errorFraction;
    IntPoint newToolPos;
    DoublePoint newToolDir;
};

void Adaptive2d::ProcessPolyNode(
    Paths boundPaths,
    Paths toolBoundPaths,
    Paths finishingPaths,
    Paths initialClearedPaths
)
{
    ofstream& fout = *this->fout;
    fout << "\n" << "\n" << "----------------------" << "\n";
    fout << "Start ProcessPolyNode (tbp size " << toolBoundPaths.size() << ")" << "\n";
    Perf_ProcessPolyNode.Start();
    current_region++;
    cout << "** Processing region: " << current_region << endl;

    // node paths are already constrained to tool boundary path for adaptive path before finishing
    // pass
    Clipper clip;
    ClipperOffset clipof;

    IntPoint entryPoint;
    TPaths progressPaths;
    progressPaths.reserve(10000);

    CleanPolygons(toolBoundPaths);
    SimplifyPolygons(toolBoundPaths);

    CleanPolygons(boundPaths);
    SimplifyPolygons(boundPaths);

    AddPathsToProgress(progressPaths, toolBoundPaths, MotionType::mtLinkClear);

    IntPoint toolPos;
    DoublePoint toolDir;

    // Initialize cleared area from previously cleared paths
    ClearedArea cleared(toolRadiusScaled);
    cleared.SetClearedPaths(initialClearedPaths);

    cout << "Tool Radius Scaled: " << toolRadiusScaled << endl;
    cout << "stepOverScaled: " << stepOverScaled << endl;

    long stepScaled = long(MIN_STEP_CLIPPER);

    CheckReportProgress(progressPaths, true);

    Path passToolPath;  // to store pass toolpath
    Path toClearPath;
    IntPoint clp;  // to store closest point
    size_t clpPathIndex;
    size_t clpSegmentIndex;
    double clpParameter;
    vector<DoublePoint> gyro;     // used to average tool direction
    vector<double> angleHistory;  // use to predict deflection angle
    double angle = std::numbers::pi;
    Interpolation interp;  // interpolation instance

    long total_iterations = 0;
    long total_points = 0;
    long total_exceeded = 0;
    long total_output_points = 0;
    long over_cut_count = 0;
    long bad_engage_count = 0;

    double perf_total_len = 0;
#ifdef DEV_MODE
    clock_t start_clock = clock();
#endif
    ClearedArea clearedBeforePass(toolRadiusScaled);
    clearedBeforePass.SetClearedPaths(cleared.GetCleared());
    fout << "Tool radius scaled: " << toolRadiusScaled << "\n";
    fout << "toolBoundPaths:";
    for (auto path : toolBoundPaths) {
        fout << " [";
        for (auto p : path) {
            fout << "(" << p.X << "," << p.Y << ")_";
        }
        fout << "]";
    }
    fout << "\n";

    DoublePoint lastExpandToolDir = toolDir;

    const auto iterateNextStep = [&](const IntPoint& toolPos,
                                     const DoublePoint& toolDir,
                                     bool warnRotate) {
        IterateNextStepOutput out;
        out.tooManyIterations = false;
        out.failed = false;

        Perf_DistanceToBoundary.Start();

        double distanceToBoundary = sqrt(
            DistancePointToPathsSqrd(toolBoundPaths, toolPos, clp, clpPathIndex, clpSegmentIndex, clpParameter)
        );
        DoublePoint boundaryDir = GetPathDirectionV(toolBoundPaths[clpPathIndex], clpSegmentIndex);

        Perf_DistanceToBoundary.Stop();
        double distanceToEngage = sqrt(DistanceSqrd(toolPos, entryPoint));

        double targetAreaPD = optimalCutAreaPD;

        // set the step size: 1x to 8x base size
        double slowDownDistance = max(double(toolRadiusScaled) / 4, MIN_STEP_CLIPPER * 8);
        if (distanceToBoundary < slowDownDistance || distanceToEngage < slowDownDistance) {
            stepScaled = long(MIN_STEP_CLIPPER);
        }
        else if (fabs(angle) > NTOL) {
            stepScaled = long(MIN_STEP_CLIPPER / fabs(angle));
        }
        else {
            stepScaled = long(MIN_STEP_CLIPPER * 8);
        }

        // clamp the step size - for stability
        if (stepScaled > min(long(toolRadiusScaled / 4), long(MIN_STEP_CLIPPER * 8))) {
            stepScaled = min(long(toolRadiusScaled / 4), long(MIN_STEP_CLIPPER * 8));
        }
        if (stepScaled < MIN_STEP_CLIPPER) {
            stepScaled = long(MIN_STEP_CLIPPER);
        }
        fout << "\tstepScaled " << stepScaled << "\n";

        //*****************************
        // ANGLE vs AREA ITERATIONS
        //*****************************
        double predictedAngle = averageDV(angleHistory);
        double maxError = AREA_ERROR_FACTOR * optimalCutAreaPD;
        double errorFraction = 1;
        fout << "optimal area " << optimalCutAreaPD << " maxError " << maxError << "\n";
        double area = 0;
        bool isConventional = false;
        const double conventionalCutoff = 0.51;  // allow some room for rounding, but otherwise < 50%
        double areaPD = 0;
        interp.clear();
        /******************************/
        Perf_PointIterations.Start();
        int iteration;
        double prev_error = __DBL_MAX__;
        bool pointNotInterp;
        bool foundArea = false;
        IntPoint newToolPos;
        DoublePoint newToolDir;
        for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
            total_iterations++;
            fout << "It " << iteration << " ";
            if (iteration == 0) {
                angle = predictedAngle;
                pointNotInterp = true;
                fout << "case predicted ";
            }
            else if (iteration == 1) {
                angle = interp.MIN_ANGLE;  // max engage
                pointNotInterp = true;
                fout << "case minimum ";
            }
            else if (iteration == 2) {
                if (interp.bothSides()) {
                    angle = interp.interpolateAngle();
                    fout << "(" << interp.m_min->angle.first << ", " << interp.m_min->error << ","
                         << interp.m_min->isConventional << ") ~ (" << interp.m_max->angle.first
                         << ", " << interp.m_max->error << "," << interp.m_max->isConventional
                         << ") ";
                    pointNotInterp = false;
                    fout << "case interp ";
                }
                else {
                    angle = interp.MAX_ANGLE;  // min engage
                    fout << "case maximum ";
                    pointNotInterp = true;
                }
            }
            else if (iteration == 3 && !foundArea) {
                fout << "case nearby ";
                // Expand cleared area
                cleared.ExpandCleared(toClearPath);
                toClearPath.clear();
                lastExpandToolDir = toolDir;

                // Find nearby uncleared area in the forward direction
                Paths clearedArea = cleared.GetCleared();

                // 1.5 > sqrt(2) for the constructed triangle to contain possible steps
                double dist = (stepScaled + toolRadiusScaled) * 1.5;
                Path triangle = {toolPos};
                DoublePoint leftAngle = rotate(toolDir, -std::numbers::pi / 4);
                DoublePoint rightAngle = rotate(toolDir, std::numbers::pi / 4);
                triangle.push_back({toolPos.X + rightAngle.X * dist, toolPos.Y + rightAngle.Y * dist});
                triangle.push_back({toolPos.X + leftAngle.X * dist, toolPos.Y + leftAngle.Y * dist});

                clip.Clear();
                clip.AddPath(triangle, PolyType::ptSubject, true);
                clip.AddPaths(clearedArea, PolyType::ptClip, true);
                clip.Execute(ClipType::ctDifference, clearedArea);

                if (clearedArea.size() == 0) {
                    continue;
                }

                // Find the closest point on the boundary, and try stepping towards it
                DistancePointToPathsSqrd(
                    clearedArea,
                    toolPos,
                    clp,
                    clpPathIndex,
                    clpSegmentIndex,
                    clpParameter
                );
                double dy = clp.Y - toolPos.Y;
                double dx = clp.X - toolPos.X;
                double len = sqrt(dx * dx + dy * dy);
                angle = asin((dy * toolDir.X - dx * toolDir.Y) / len);
            }
            else if (!foundArea) {
                // if the previous iteration didn't cut area then nothing will; exit early
                angle = 0;
                area = 0;
                areaPD = 0;
                break;
            }
            else {
                angle = interp.interpolateAngle();
                fout << "(" << interp.m_min->angle.first << ", " << interp.m_min->error << ","
                     << interp.m_min->isConventional << ") ~ (" << interp.m_max->angle.first << ", "
                     << interp.m_max->error << "," << interp.m_max->isConventional << ") ";
                pointNotInterp = false;
                fout << "case interp ";
            }
            fout << "raw " << angle << " ";
            angle = interp.clampAngle(angle);
            fout << "clamped " << angle << " ";

            newToolDir = rotate(toolDir, angle);
            newToolPos = IntPoint(
                long(toolPos.X + newToolDir.X * stepScaled),
                long(toolPos.Y + newToolDir.Y * stepScaled)
            );
            fout << "int pos " << newToolPos << " ";

            // Skip iteration if this IntPoint has already been processed
            bool intRepeat = false;
            if (interp.m_min && newToolPos == interp.m_min->angle.second) {
                interp.m_min = {{angle, newToolPos}, interp.m_min->error, interp.m_min->isConventional};
                intRepeat = true;
            }
            if (interp.m_max && newToolPos == interp.m_max->angle.second) {
                interp.m_max = {{angle, newToolPos}, interp.m_max->error, interp.m_max->isConventional};
                intRepeat = true;
            }

            if (intRepeat) {
                if (interp.m_min && interp.m_max
                    && abs(interp.m_min->angle.second.X - interp.m_max->angle.second.X) <= 1
                    && abs(interp.m_min->angle.second.Y - interp.m_max->angle.second.Y) <= 1) {
                    if (pointNotInterp) {
                        // if this happens while testing min/max of the range it doesn't mean
                        // anything; only exit early if interpolation is down to adjacent
                        // integers
                        continue;
                    }
                    fout << "hit integer floor" << "\n";
                    // exit early, selecting the better of the two adjacent integers
                    double error;
                    if (interp.m_min->isConventional ^ interp.m_max->isConventional) {
                        if (!interp.m_min->isConventional) {
                            newToolDir = rotate(toolDir, interp.m_min->angle.first);
                            newToolPos = interp.m_min->angle.second;
                            error = interp.m_min->error;
                            isConventional = interp.m_min->isConventional;
                        }
                        else {
                            newToolDir = rotate(toolDir, interp.m_max->angle.first);
                            newToolPos = interp.m_max->angle.second;
                            error = interp.m_max->error;
                            isConventional = interp.m_max->isConventional;
                        }
                    }
                    else if (abs(interp.m_min->error) < abs(interp.m_max->error)) {
                        newToolDir = rotate(toolDir, interp.m_min->angle.first);
                        newToolPos = interp.m_min->angle.second;
                        error = interp.m_min->error;
                        isConventional = interp.m_min->isConventional;
                    }
                    else {
                        newToolDir = rotate(toolDir, interp.m_max->angle.first);
                        newToolPos = interp.m_max->angle.second;
                        error = interp.m_max->error;
                        isConventional = interp.m_max->isConventional;
                    }
                    areaPD = error + targetAreaPD;
                    area = areaPD * double(stepScaled);
                    out.iterationAngle = angle;
                    break;
                }
                fout << "skip area calc " << "\n";
                continue;
            }

            const auto caRet = CalcCutArea(clip, toolPos, newToolPos, cleared);
            area = std::get<0>(caRet);
            double conventionalArea = std::get<1>(caRet);
            double fractionConventional = (area == 0) ? 0 : conventionalArea / area;
            isConventional = fractionConventional >= conventionalCutoff;
            if (area > 0) {
                foundArea = true;
            }

            areaPD = area / double(stepScaled);  // area per distance
            fout << "addPoint " << areaPD << " " << angle << " ";
            double error = areaPD - targetAreaPD;
            errorFraction = abs(error / optimalCutAreaPD);
            interp.addPoint(error, {angle, newToolPos}, pointNotInterp, isConventional);
            fout << "areaPD " << areaPD << " error " << error << " conventional? " << isConventional
                 << " ";
            if (fabs(error) < maxError && !isConventional) {
                out.iterationAngle = angle;
                fout << "small enough" << "\n";
                break;
            }
            if (iteration == MAX_ITERATIONS - 1) {
                fout << "too many iterations!" << "\n";
                out.tooManyIterations = true;
            }
            fout << "\n";
            prev_error = error;
        }
        Perf_PointIterations.Stop();
        fout << "Iterations: " << iteration << "\n";

        bool recalcArea = false;

        if (area > 0) {
            //**********************************************
            // CHECK AND RECORD NEW TOOL POS
            //**********************************************
            long rotateStep = 0;
            double rotateIncrement;
            {
                double boundaryAngle = atan2(boundaryDir.Y, boundaryDir.X);
                double toolAngle = atan2(newToolDir.Y, newToolDir.X);
                double delta = boundaryAngle - toolAngle;
                if (delta > std::numbers::pi) {
                    delta -= 2 * std::numbers::pi;
                }
                if (delta < -std::numbers::pi) {
                    delta += 2 * std::numbers::pi;
                }
                rotateIncrement = (delta > 0 ? 1 : -1) * std::numbers::pi / 90;
            }
            while (!IsPointWithinCutRegion(toolBoundPaths, newToolPos) && rotateStep < 180) {
                rotateStep++;
                // if new tool pos. outside boundary rotate until back in
                recalcArea = true;
                newToolDir = rotate(newToolDir, rotateIncrement);
                newToolPos = IntPoint(
                    long(toolPos.X + newToolDir.X * stepScaled),
                    long(toolPos.Y + newToolDir.Y * stepScaled)
                );
                fout << "\tMoving tool back within boundary..."
                     << "(" << newToolPos.X << ", " << newToolPos.Y << ")" << "\n";
            }
            if (rotateStep >= 180) {
#ifdef DEV_MODE
                if (warnRotate) {
                    cerr << "Warning: unexpected number of rotate iterations." << endl;
                    fout << "Warning: unexpected number of rotate iterations." << endl;
                }
#endif
                out.failed = true;
            }

            if (recalcArea) {
                const auto caRet = CalcCutArea(clip, toolPos, newToolPos, cleared);
                area = std::get<0>(caRet);
                areaPD = area / double(stepScaled);  // area per distance
                double error = areaPD - targetAreaPD;
                errorFraction = abs(error / optimalCutAreaPD);

                double conventionalArea = std::get<1>(caRet);
                double fractionConventional = area == 0 ? 0 : conventionalArea / area;
                isConventional = fractionConventional >= conventionalCutoff;

                fout << "\tRecalc area: " << area << "areaPD " << areaPD << " error " << error
                     << " conventional? " << isConventional << "\n";
            }

            // safety condition
            if (area > stepScaled * optimalCutAreaPD && areaPD > 2 * optimalCutAreaPD) {
                over_cut_count++;
                fout << "\tCut area too big!!!" << "\n";
                out.failed = true;
            }
        }

        fout << "itResult: area=" << area << " isConventional=" << isConventional
             << " otherwiseFailed=" << out.failed << "\n";
        out.area = area;
        out.failed |= isConventional;
        out.failed |= area < 1;
        out.newToolPos = newToolPos;
        out.newToolDir = newToolDir;
        out.errorFraction = errorFraction;
        return out;
    };

    const auto initToolDir = [&](const IntPoint& toolPos, const DoublePoint& baseDir) {
        DoublePoint testDirs[] = {
            {baseDir.X, baseDir.Y},
            {-baseDir.Y, baseDir.X},
            {-baseDir.X, -baseDir.Y},
            {baseDir.Y, -baseDir.X}
        };
        std::optional<std::pair<DoublePoint, double>> bestDir;
        bool allZero = true;
        for (const auto& testDir : testDirs) {
            fout << endl << "testing dir (" << testDir.X << "," << testDir.Y << ")" << endl;
            const auto itResult = iterateNextStep(toolPos, testDir, false);
            if (itResult.area != 0) {
                allZero = false;
            }
            if (!itResult.failed) {
                fout << "Found a candidate tool direction\n";
                if (!bestDir || itResult.errorFraction < bestDir->second) {
                    bestDir = {itResult.newToolDir, itResult.errorFraction};
                }
            }
        }

        if (bestDir) {
            return std::optional<DoublePoint> {bestDir->first};
        }
        else if (allZero) {
            Paths clearedArea = cleared.GetCleared();
            if (DistancePointToPathsSqrd(clearedArea, toolPos, clp, clpPathIndex, clpSegmentIndex, clpParameter)
                < toolRadiusScaled * toolRadiusScaled) {
                IntPoint p2 = Compute2DPolygonCentroid(clearedArea[clpPathIndex]);
                DoublePoint dir = DirectionV(toolPos, p2);
                return std::optional<DoublePoint> {dir};
            }
            return std::optional<DoublePoint> {};
        }
        else {
            return std::optional<DoublePoint> {};
        }
    };

    const auto _getEngagePoint = [&](const std::optional<IntPoint>& prevPos,
                                     const std::optional<DoublePoint>& prevDir,
                                     long engagementProtrusion) {
        // engagePoint, engageDir, heuristicCost
        std::vector<std::tuple<IntPoint, DoublePoint, double>> engagePoints;

        const auto addEngagePoint = [&](const IntPoint& engagePoint, const DoublePoint& engageDir) {
            double cost_mm = (prevPos ? sqrt(DistanceSqrd(*prevPos, engagePoint)) / scaleFactor : 0);
            engagePoints.emplace_back(
                std::tuple<IntPoint, DoublePoint, double> {engagePoint, engageDir, cost_mm}
            );
        };

        Paths clearedArea = cleared.GetCleared();

        // offset inward to find places the tool can start
        long engageBuffer = 2;  // smooths out the integer rounding in the two offsets, +toolRadius
                                // and -toolRadius
        Paths preEngage;
        clipof.Clear();
        clipof.AddPaths(clearedArea, JoinType::jtRound, EndType::etClosedPolygon);
        clipof.Execute(preEngage, -(toolRadiusScaled + engageBuffer));

        // offset outward to find places that would protrude outside the cleared area
        Paths engagePaths;
        clipof.Clear();
        clipof.AddPaths(preEngage, JoinType::jtRound, EndType::etClosedPolygon);
        clipof.Execute(engagePaths, engageBuffer + engagementProtrusion);

        // clip engage candidates with tool bounds
        if (prevPos) {
            fout << "Prev Pos: (" << prevPos->X << ", " << prevPos->Y << ")" << endl;
        }
        for (Path& engagePath : engagePaths) {
            // rotate the closed path so it starts with the closest point
            // this is useful because if the path does not get clipped, any point on
            // the path is a valid start location (not just the first) but we want to
            // test against the closest one
            Path rotated;
            if (!prevPos) {
                rotated = engagePath;
            }
            else {
                int iClosest = 0;
                double dsqClosest = __DBL_MAX__;
                for (int i = 0; i < engagePath.size(); i++) {
                    double dsq = DistanceSqrd(*prevPos, engagePath[i]);
                    if (dsq < dsqClosest) {
                        dsqClosest = dsq;
                        iClosest = i;
                    }
                }

                fout << "Closest i=" << iClosest << " (" << engagePath[iClosest].X << ", "
                     << engagePath[iClosest].Y << ")" << endl;
                for (int i = 0; i < engagePath.size(); i++) {
                    rotated.push_back(engagePath[(i + iClosest) % engagePath.size()]);
                }
            }

            Paths openPaths = PathIntersectArea(clip, rotated, toolBoundPaths, fout);

            for (Path& open : openPaths) {
                fout << "Start path (" << open[0].X << ", " << open[0].Y << ")" << endl;
                bool added = false;
                double dToGo = 0;  // first step is 0 -- start point
                int seg = 0;
                double segD = 0;
                DoublePoint segDir = {1, 0};
                while (!added && seg < open.size() - 1) {
                    // step to next point
                    IntPoint p;
                    while (dToGo > 0 && seg < open.size() - 1) {
                        IntPoint p1 = open[seg];
                        IntPoint p2 = open[seg + 1];
                        double segLen = sqrt(DistanceSqrd(p1, p2));
                        segDir = {(p2.X - p1.X) / segLen, (p2.Y - p1.Y) / segLen};
                        if (segLen - segD > dToGo) {
                            // interpolate current segment
                            segD += dToGo;
                            dToGo = 0;
                            double interp = segD / segLen;
                            p = {
                                p2.X * interp + p1.X * (1 - interp),
                                p2.Y * interp + p1.Y * (1 - interp)
                            };
                        }
                        else {
                            dToGo -= segLen - segD;
                            segD = 0;
                            seg++;
                            p = p2;  // ensures that we try the endpoint too
                        }
                    }

                    // Attempt to add the point
                    const auto toolDir = initToolDir(p, segDir);
                    if (toolDir) {
                        addEngagePoint(p, *toolDir);
                        added = true;
                        fout << "Open path adds point: (" << p.X << ", " << p.Y << ", " << p.Z
                             << ") [";
                        for (IntPoint p : open) {
                            fout << "(" << p.X << "," << p.Y << ")_";
                        }
                        fout << "], ";
                        fout << "entry at seg=" << seg << " segD=" << segD << ", dir: ("
                             << toolDir->X << "," << toolDir->Y << ")" << endl;
                        fout << "ZZOpen path: [";
                        for (IntPoint p : open) {
                            fout << p.Z << ",";
                        }
                        fout << "]" << endl;
                    }

                    dToGo = MIN_STEP_CLIPPER;  // all subsequent steps are MIN_STEP_CLIPPER
                }

                if (!added) {
                    fout << "Open path: [";
                    for (IntPoint p : open) {
                        fout << "(" << p.X << "," << p.Y << ")_";
                    }
                    fout << "], ";

                    fout << "failed to find a tool dir" << endl;
                }
            }
        }

        // sort engagePoints based on connection cost
        std::sort(
            engagePoints.begin(),
            engagePoints.end(),
            [](std::tuple<IntPoint, DoublePoint, double> aa,
               std::tuple<IntPoint, DoublePoint, double> bb) {
                return std::get<double>(aa) < std::get<double>(bb);
            }
        );

        cout << "Engagement points: [";
        fout << "Engagement points: [";
        for (const auto& p : engagePoints) {
            cout << "(" << std::get<IntPoint>(p).X << "," << std::get<IntPoint>(p).Y << ")_";
            fout << "(" << std::get<IntPoint>(p).X << "," << std::get<IntPoint>(p).Y << ")_";
        }
        cout << "]" << endl;
        fout << "]" << endl;

        double bestCost = __DBL_MAX__;
        TPaths bestLink;
        IntPoint bestPos;
        DoublePoint bestDir;

        for (const auto& ep : engagePoints) {
            if (std::get<double>(ep) < bestCost) {
                std::optional<TPaths> link = FindLinkPath(
                    prevPos,
                    std::get<IntPoint>(ep),
                    std::get<DoublePoint>(ep),
                    cleared,
                    toolBoundPaths
                );
                if (!link) {
                    continue;
                }

                double cost_mm = 0;
                std::optional<DPoint> prev = prevPos
                    ? std::optional<DPoint> {{prevPos->X / (double)scaleFactor, prevPos->Y / (double)scaleFactor}}
                    : std::optional<DPoint> {};
                for (TPath tp : *link) {
                    fout << "TP type " << tp.first << ": cost ";
                    if (tp.first == MotionType::mtLinkNotClear) {
                        fout << "retraction 10000 ";
                        cost_mm += 10000;  // prioritize links that don't require retraction
                    }
                    for (int i = 0; i < tp.second.size(); i++) {
                        DPoint cur = tp.second[i];
                        if (prev) {
                            double dx = cur.first - prev->first;
                            double dy = cur.second - prev->second;
                            double dist = sqrt(dx * dx + dy * dy);
                            fout << "+ " << dist << " (" << cur.first * scaleFactor << ", "
                                 << cur.second * scaleFactor << ") ";
                            cost_mm += dist;
                        }
                        prev = {cur};
                    }
                    fout << endl;
                }
                fout << "Cost heuristic " << std::get<double>(ep) << " and actual " << cost_mm
                     << " for (" << std::get<IntPoint>(ep).X << ", " << std::get<IntPoint>(ep).Y
                     << ")";
                if (prevPos) {
                    fout << " from (" << prev->first << ", " << prev->second << ")";
                }
                fout << endl;

                if (cost_mm < bestCost) {
                    bestCost = cost_mm;
                    bestLink = *link;
                    bestPos = std::get<IntPoint>(ep);
                    bestDir = std::get<DoublePoint>(ep);
                }
            }
        }

        if (bestCost < __DBL_MAX__) {
            return std::optional<std::tuple<IntPoint, DoublePoint, TPaths>> {
                {bestPos, bestDir, bestLink}
            };
        }
        else {
            return std::optional<std::tuple<IntPoint, DoublePoint, TPaths>> {};
        }
    };

    const auto getEngagePoint = [&](const std::optional<IntPoint>& prevPos,
                                    const std::optional<DoublePoint>& prevDir) {
        Perf_NextEngagePoint.Start();

        // Compute how far into the material the first engagement should be
        const double targetArea = optimalCutAreaPD * MIN_STEP_CLIPPER;
        // Area of a segment of a circle: A = R^2 / 2 * (theta - sin(theta))
        // 2nd order Taylor expansion: A = R^2 / 2 * (theta^3/6) = R^2 * theta^3 / 12
        // Solve for theta: theta = (12 * A / R^2)^(1/3)
        const double theta = std::pow(12 * targetArea / toolRadiusScaled / toolRadiusScaled, 1 / 3.);
        const double protrusion = toolRadiusScaled - cos(theta / 2) * toolRadiusScaled;
        const long engagementProtrusion
            = (long)min(protrusion, stepOverScaled * FINISHING_THICKNESS_SCALE);

        // Get engagement point. First attempt with the desired offsets, then fallback
        auto result = _getEngagePoint(prevPos, prevDir, engagementProtrusion);
        if (!result) {
            // TODO consider retry at tiny protrusion?
            // result = _getEngagePoint(prevPos, prevDir, 4);
        }

        // update cleared area
        if (result) {
            for (const TPath& linkPath : std::get<TPaths>(*result)) {
                if (linkPath.first == MotionType::mtCutting) {
                    Path p;
                    for (const DPoint& dp : linkPath.second) {
                        p.push_back({dp.first * scaleFactor, dp.second * scaleFactor});
                    }
                    cleared.ExpandCleared(p);
                }
            }
        }

        Perf_NextEngagePoint.Stop();
        return result;
    };

    AdaptiveOutput output;
    std::optional<std::tuple<IntPoint, DoublePoint, TPaths>> engagePoint = getEngagePoint({}, {});
    TPaths linkPath;
    if (engagePoint) {
        toolPos = std::get<IntPoint>(*engagePoint);
        toolDir = std::get<DoublePoint>(*engagePoint);
        linkPath = std::get<TPaths>(*engagePoint);
        entryPoint = linkPath.size() > 0
            ? IntPoint {linkPath[0].second[0].first * scaleFactor, linkPath[0].second[0].second * scaleFactor}
            : toolPos;
        cout << "link path size " << linkPath.size() << endl;
        output.StartPoint
            = DPoint(double(entryPoint.X) / scaleFactor, double(entryPoint.Y) / scaleFactor);
    }
    else {
        // Engagement failed; instead helix down
        fout << "Helix entry " << entryPoint << "\n";
        cout << "No enage, helixing down\n";
        if (!FindEntryPoint(progressPaths, toolBoundPaths, boundPaths, cleared, entryPoint, toolPos, toolDir)) {
            Perf_ProcessPolyNode.Stop();
            return;
        }
        output.StartPoint = DPoint(double(toolPos.X) / scaleFactor, double(toolPos.Y) / scaleFactor);
    }

    output.ReturnMotionType = 0;
    output.HelixCenterPoint.first = double(entryPoint.X) / scaleFactor;
    output.HelixCenterPoint.second = double(entryPoint.Y) / scaleFactor;

    cout << "Entry point: (" << entryPoint.X << "," << entryPoint.Y << ") Start point: ("
         << toolPos.X << "," << toolPos.Y << ")" << endl;
    fout << "Entry point: (" << entryPoint.X << "," << entryPoint.Y << ") Start point: ("
         << toolPos.X << "," << toolPos.Y << ")" << endl;

    //*******************************
    // LOOP - PASSES
    //*******************************
    for (long pass = 0; pass < PASSES_LIMIT; pass++) {
        fout << "New pass! " << pass << "\n";
        if (stopProcessing) {
            break;
        }
        fout << "start point in bounds? " << IsPointWithinCutRegion(toolBoundPaths, toolPos) << endl;

        passToolPath.clear();
        toClearPath.clear();
        angleHistory.clear();
        angleHistory.push_back(0);

        // include linking path in cleared area
        for (TPath lp : linkPath) {
            if (lp.first == MotionType::mtCutting || lp.first == MotionType::mtLinkClear) {
                Path scaledP;
                for (auto& p : lp.second) {
                    scaledP.push_back({p.first * scaleFactor, p.second * scaleFactor});
                }
                cleared.ExpandCleared(scaledP);
            }
        }

        // append a new path to progress info paths
        if (progressPaths.empty()) {
            progressPaths.push_back(TPath());
        }
        else {
            // append new path if previous not empty
            if (!progressPaths.back().second.empty()) {
                progressPaths.push_back(TPath());
            }
        }

        angle = std::numbers::pi / 4;  // initial pass angle
        double cumulativeCutArea = 0;
        // init gyro
        gyro.clear();
        for (int i = 0; i < DIRECTION_SMOOTHING_BUFLEN; i++) {
            gyro.push_back(toolDir);
        }

        double passLength = 0;
        //*******************************
        // LOOP - POINTS
        //*******************************
        for (long point_index = 0; point_index < POINTS_PER_PASS_LIMIT; point_index++) {
            fout << "\n" << "Point " << point_index << "\n";
            if (stopProcessing) {
                break;
            }

            total_points++;
            AverageDirection(gyro, toolDir);

            const auto itResult = iterateNextStep(toolPos, toolDir, true);

            if (itResult.tooManyIterations) {
                total_exceeded++;
            }

            if (!itResult.failed) {  // cut is ok - record it
                fout << "\tFinal cut acceptance (" << itResult.newToolPos.X << ","
                     << itResult.newToolPos.Y << ") dir (" << itResult.newToolDir.X << ","
                     << itResult.newToolDir.Y << ")\n";

                if (itResult.iterationAngle) {
                    angleHistory.push_back(*itResult.iterationAngle);
                    if (angleHistory.size() > ANGLE_HISTORY_POINTS) {
                        angleHistory.erase(angleHistory.begin());
                    }
                }

                // if the path has changed direction by more than 45 degrees (such that we consider
                // continuations (>45)+45>90 degrees from the original direction) then we need to
                // update cleared paths
                if (lastExpandToolDir.X * itResult.newToolDir.X
                        + lastExpandToolDir.Y * itResult.newToolDir.Y
                    < cos(std::numbers::pi / 4)) {
                    cleared.ExpandCleared(toClearPath);
                    toClearPath.clear();
                    lastExpandToolDir = toolDir;
                }

                if (toClearPath.empty()) {
                    toClearPath.push_back(toolPos);
                }
                toClearPath.push_back(itResult.newToolPos);

                cumulativeCutArea += itResult.area;

                // append to toolpaths
                if (passToolPath.empty()) {
                    passToolPath.push_back(toolPos);
                }
                passToolPath.push_back(itResult.newToolPos);
                perf_total_len += stepScaled;
                passLength += stepScaled;
                toolPos = itResult.newToolPos;

                // append to progress info paths
                if (progressPaths.empty()) {
                    progressPaths.push_back(TPath());
                }
                progressPaths.back().second.emplace_back(
                    double(itResult.newToolPos.X) / scaleFactor,
                    double(itResult.newToolPos.Y) / scaleFactor
                );

                // append gyro
                gyro.push_back(itResult.newToolDir);
                gyro.erase(gyro.begin());
                CheckReportProgress(progressPaths);
            }
            else {
                // cout<<"Break: no cut @" << point_index << endl;
                fout << "\tFailed to accept point" << "\n";
                fout << "Points: " << point_index << "\n";
                break;
            }
        } /* end of points loop*/

        if (!toClearPath.empty()) {
            cleared.ExpandCleared(toClearPath);
            toClearPath.clear();
        }

        Paths newlyClearedAreas;
        clip.Clear();
        clip.AddPaths(cleared.GetCleared(), PolyType::ptSubject, true);
        clip.AddPaths(clearedBeforePass.GetCleared(), PolyType::ptClip, true);
        clip.Execute(ClipType::ctDifference, newlyClearedAreas);
        cumulativeCutArea = 0;
        for (Path& a : newlyClearedAreas) {
            int nesting = getPathNestingLevel(a, newlyClearedAreas);
            cumulativeCutArea += (nesting <= 1 ? 1 : -1) * Area(a);
            fout << "accumulating area " << (nesting <= 1 ? 1 : -1) * Area(a) << " [" << endl;
            for (IntPoint& p : a) {
                fout << "\t(" << p.X << ", " << p.Y << ")" << endl;
            }
            fout << "]" << endl;
        }

        if (cumulativeCutArea >= 1) {
            Path cleaned;
            CleanPath(passToolPath, cleaned, CLEAN_PATH_TOLERANCE);
            total_output_points += long(cleaned.size());
            auto newPos = AppendToolPath(output, cleaned, linkPath, cleared, toolBoundPaths);
            if (newPos) {
                toolPos = newPos->first;
                toolDir = newPos->second;
            }
            CheckReportProgress(progressPaths);
            bad_engage_count = 0;
            fout << "Accepted pass, area " << cumulativeCutArea << "\n\n";
        }
        else {
            fout << "Rejected pass, too little area " << cumulativeCutArea << "\n\n";
            cout << "Rejected pass, too little area " << cumulativeCutArea << "\n\n";
            bad_engage_count++;
        }

        fout << endl << "Previously cleared:" << endl;
        for (Path& path : clearedBeforePass.GetCleared()) {
            fout << path.size() << " points [";
            for (IntPoint& p : path) {
                fout << "(" << p.X << ", " << p.Y << ")  ";
            }
            fout << "]" << endl;
        }
        fout << endl << "Now cleared:" << endl;
        for (Path& path : cleared.GetCleared()) {
            fout << path.size() << " points [";
            for (IntPoint& p : path) {
                fout << "(" << p.X << ", " << p.Y << ")  ";
            }
            fout << "]" << endl;
        }
        fout << endl;

        if (bad_engage_count > 10000) {
            cerr << "Break (next valid engage point not found)." << endl;
            break;
        }

        clearedBeforePass.SetClearedPaths(cleared.GetCleared());
        engagePoint = getEngagePoint({toolPos}, {toolDir});
        if (engagePoint) {
            toolPos = std::get<IntPoint>(*engagePoint);
            toolDir = std::get<DoublePoint>(*engagePoint);
            linkPath = std::get<TPaths>(*engagePoint);
            lastExpandToolDir = toolDir;
        }
        else {
            // check if there are any uncleared area left
            Paths remaining;
            for (const auto& p : cleared.GetCleared()) {
                if (!p.empty() && IsPointWithinCutRegion(toolBoundPaths, p.front())
                    && DistancePointToPathsSqrd(
                           boundPaths,
                           p.front(),
                           clp,
                           clpPathIndex,
                           clpSegmentIndex,
                           clpParameter
                       ) > 4 * toolRadiusScaled * toolRadiusScaled) {
                    remaining.push_back(p);
                }
            };
            if (remaining.empty()) {
                cout << "All cleared." << endl;
                break;
            }

            cerr << "NO ENGAGEMENTS LEFT BUT NOT ALL CELARED!!! " << endl;
            for (Path& path : remaining) {
                cout << "[" << endl;
                for (IntPoint& p : path) {
                    cout << "(" << p.X << ", " << p.Y << ")" << endl;
                }
                cout << "]" << endl;
            }
            cout << endl;
            break;
        }
    }

    // sanity check for finishing paths - check the area of finishing cut
    Paths clearedLocations;
    clipof.Clear();
    clipof.AddPaths(cleared.GetCleared(), JoinType::jtRound, EndType::etClosedPolygon);
    clipof.Execute(clearedLocations, long(-toolRadiusScaled));

    Paths tbpShrink;
    clipof.Clear();
    clipof.AddPaths(toolBoundPaths, JoinType::jtRound, EndType::etClosedPolygon);
    clipof.Execute(tbpShrink, long(-stepOverScaled * FINISHING_THICKNESS_SCALE - MIN_STEP_CLIPPER));

    Paths uncut;
    clip.Clear();
    clip.AddPaths(tbpShrink, PolyType::ptSubject, true);
    clip.AddPaths(clearedLocations, PolyType::ptClip, true);
    clip.Execute(ClipType::ctDifference, uncut);

    if (uncut.size() > 0) {
        cerr << "Warning: some cuts may be above optimal step-over. Please double check the "
                "results."
             << endl
             << "Hint: try to modify accuracy and/or step-over." << endl;

        fout << "REMAINING tbpShrink" << endl;
        for (auto& path : tbpShrink) {
            fout << "[" << endl;
            for (IntPoint& p : path) {
                fout << "\t(" << p.X << ", " << p.Y << ")" << endl;
            }
            fout << "]" << endl << endl;
        }

        fout << "REMAINING clearedLocations" << endl;
        for (auto& path : clearedLocations) {
            fout << "[" << endl;
            for (IntPoint& p : path) {
                fout << "\t(" << p.X << ", " << p.Y << ")" << endl;
            }
            fout << "]" << endl << endl;
        }

        fout << "REMAINING uncut" << endl;
        for (auto& path : uncut) {
            fout << "[" << endl;
            for (IntPoint& p : path) {
                fout << "\t(" << p.X << ", " << p.Y << ")" << endl;
            }
            fout << "]" << endl << endl;
        }
    }


    //**********************************
    //*  FINISHING PASS                *
    //**********************************
    if (finishingProfile) {
        // update tool bound paths to correspond to the finishing paths instead of the interior
        {
            Paths tbpModified;
            for (const Path& fp : finishingPaths) {
                clipof.Clear();
                clipof.AddPath(fp, JoinType::jtRound, EndType::etClosedPolygon);
                int offset = (getPathNestingLevel(fp, finishingPaths) % 2 == 1) ? 3 : -3;
                Paths out;
                clipof.Execute(out, offset);

                int orientation = Orientation(fp);
                for (Path& p : out) {
                    if (Orientation(p) != orientation) {
                        ReversePath(p);
                    }
                    tbpModified.push_back(p);
                }
            }

            toolBoundPaths = tbpModified;
        }

        Path finShiftedPath;

        while (!stopProcessing
               && PopPathWithClosestPoint(finishingPaths, toolPos, finShiftedPath, stepOverScaled)) {
            if (finShiftedPath.empty()) {
                continue;
            }
            // skip finishing passes outside the stock boundary - no sense to cut where is no
            // material
            bool allPointsOutside = true;
            IntPoint p1 = finShiftedPath.front();
            for (const auto& pt : finShiftedPath) {

                // midpoint
                if (IsPointWithinCutRegion(
                        stockInputPaths,
                        IntPoint((p1.X + pt.X) / 2, (p1.Y + pt.Y) / 2)
                    )) {
                    allPointsOutside = false;
                    break;
                }
                // current point
                if (IsPointWithinCutRegion(stockInputPaths, pt)) {
                    allPointsOutside = false;
                    break;
                }

                p1 = pt;
            }
            if (allPointsOutside) {
                continue;
            }

            progressPaths.push_back(TPath());
            // show in progress cb
            for (auto& pt : finShiftedPath) {
                progressPaths.back().second.emplace_back(
                    double(pt.X) / scaleFactor,
                    double(pt.Y) / scaleFactor
                );
            }

            if (!finShiftedPath.empty()) {
                finShiftedPath << finShiftedPath.front();  // make sure its closed
            }

            Path finCleaned;
            CleanPath(finShiftedPath, finCleaned, FINISHING_CLEAN_PATH_TOLERANCE);

            // make sure it's closed, but don't ruin the final direction
            if (sqrt(DistanceSqrd(finCleaned.front(), finCleaned.back()))
                < FINISHING_CLEAN_PATH_TOLERANCE) {
                finCleaned.pop_back();
            }
            finCleaned.push_back(finCleaned.front());
            std::optional<TPaths> linkPath = FindLinkPath(
                toolPos,
                finCleaned[0],
                GetPathDirectionV(finCleaned, 1),
                cleared,
                toolBoundPaths
            );
            if (!linkPath) {
                cerr << "Failed to generate lead-in for finishing pass; skipping pass" << endl;
                fout << "Failed to generate lead-in for finishing pass; skipping pass" << endl;
            }
            else {
                auto newPos = AppendToolPath(output, finCleaned, *linkPath, cleared, toolBoundPaths);
                if (newPos) {
                    toolPos = newPos->first;
                    toolDir = newPos->second;
                }
                else {
                    toolPos = finCleaned.back();
                    toolDir = GetPathDirectionV(finCleaned, finCleaned.size() - 1);
                }

                cleared.ExpandCleared(finCleaned);
                for (TPath lp : *linkPath) {
                    if (lp.first == MotionType::mtCutting) {
                        Path scaledP;
                        for (auto& p : lp.second) {
                            scaledP.push_back({p.first * scaleFactor, p.second * scaleFactor});
                        }
                        cleared.ExpandCleared(scaledP);
                    }
                }
            }
        }

        Path returnPath;
        returnPath << toolPos;
        returnPath << entryPoint;
        output.ReturnMotionType = IsClearPath(returnPath, cleared) ? MotionType::mtLinkClear
                                                                   : MotionType::mtLinkNotClear;

        // dump performance results
#ifdef DEV_MODE
        Perf_ProcessPolyNode.Stop();
        Perf_ProcessPolyNode.DumpResults();
        Perf_PointIterations.DumpResults();
        Perf_CalcCutAreaCirc.DumpResults();
        Perf_NextEngagePoint.DumpResults();
        Perf_ExpandCleared.DumpResults();
        Perf_DistanceToBoundary.DumpResults();
        Perf_AppendToolPath.DumpResults();
        Perf_IsAllowedToCutTrough.DumpResults();
        Perf_IsClearPath.DumpResults();
#endif
        CheckReportProgress(progressPaths, true);
#ifdef DEV_MODE
        double duration = ((double)(clock() - start_clock)) / CLOCKS_PER_SEC;
        cout << "PolyNode perf:" << perf_total_len / double(scaleFactor) / duration << " mm/sec"
             << " processed_points:" << total_points << " output_points:" << total_output_points
             << " total_iterations:" << total_iterations
             << " iter_per_point:" << (double(total_iterations) / ((double(total_points) + 0.001)))
             << " total_exceeded:" << total_exceeded << " ("
             << 100 * double(total_exceeded) / double(total_points) << "%)" << endl;
#else
        (void)total_output_points;
        (void)over_cut_count;
        (void)total_exceeded;
        (void)total_points;
        (void)total_iterations;
        (void)perf_total_len;
#endif
    }
    results.push_back(output);
}

}  // namespace AdaptivePath
