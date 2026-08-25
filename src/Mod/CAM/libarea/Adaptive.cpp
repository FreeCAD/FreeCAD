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
#include <cstddef>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <ctime>
#include <climits>
#include <algorithm>
#include <map>
#include <numbers>
#include <string>


namespace AdaptivePath
{
using namespace std;
using namespace Clipper2Lib;
#define SAME_POINT_TOL_SQRD_SCALED 4.0
#define UNUSED(expr) (void)(expr)

// Matches CleanPolygons default distance in Clipper1 (sqrt(2))
const double CLEAN_POLYGONS_TOLERANCE = 1.415;

//*****************************************
// Clipper 1 <-> Clipper 2 conversion helpers (temporary)
//*****************************************

inline Path64 ToPath64(const ClipperLib::Path& p)
{
    Path64 out;
    out.reserve(p.size());
    for (const auto& pt : p)
        out.emplace_back(pt.X, pt.Y, pt.Z);
    return out;
}

inline Paths64 ToPaths64(const ClipperLib::Paths& pp)
{
    Paths64 out;
    out.reserve(pp.size());
    for (const auto& p : pp)
        out.emplace_back(ToPath64(p));
    return out;
}

inline ClipperLib::Path ToC1Path(const Path64& p)
{
    ClipperLib::Path out;
    out.reserve(p.size());
    for (const auto& pt : p)
        out.emplace_back(pt.x, pt.y, pt.z);
    return out;
}

inline ClipperLib::Paths ToC1Paths(const Paths64& pp)
{
    ClipperLib::Paths out;
    out.reserve(pp.size());
    for (const auto& p : pp)
        out.emplace_back(ToC1Path(p));
    return out;
}

inline Point64 ToPoint64(const ClipperLib::IntPoint& p)
{
    return Point64(p.X, p.Y, p.Z);
}

inline ClipperLib::IntPoint ToC1Point(const Point64& p)
{
    return ClipperLib::IntPoint(p.x, p.y, p.z);
}

inline PointD ToPointD(const ClipperLib::DoublePoint& p)
{
    return PointD(p.X, p.Y);
}

inline ClipperLib::DoublePoint ToC1PointD(const PointD& p)
{
    return ClipperLib::DoublePoint(p.x, p.y);
}

// Wrappers that preserve Clipper1 default offset behavior (arc_tolerance=0.25 absolute)
inline Paths64 OffsetPaths(const Paths64& paths, double delta, EndType et = EndType::Polygon, JoinType jt = JoinType::Round)
{
    return InflatePaths(paths, delta, jt, et, 2.0, 0.25);
}

inline Paths64 OffsetPath(const Path64& path, double delta, EndType et = EndType::Polygon, JoinType jt = JoinType::Round)
{
    return InflatePaths(Paths64{path}, delta, jt, et, 2.0, 0.25);
}

//*****************************************
// Utils - inline
//*****************************************

inline double DistanceSqrd(const Point64& pt1, const Point64& pt2)
{
    double Dx = double(pt1.x - pt2.x);
    double dy = double(pt1.y - pt2.y);
    return (Dx * Dx + dy * dy);
}

inline bool SetSegmentLength(const ClipperLib::IntPoint& pt1, ClipperLib::IntPoint& pt2, double new_length)
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

inline bool HasAnyPath(const Paths64& paths_p64)
{
    ClipperLib::Paths paths = ToC1Paths(paths_p64);
    for (ClipperLib::Paths::size_type i = 0; i < paths.size(); i++) {
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

inline ClipperLib::DoublePoint rotate(const ClipperLib::DoublePoint& in, double rad)
{
    double c = cos(rad);
    double s = sin(rad);
    return ClipperLib::DoublePoint(c * in.X - s * in.Y, s * in.X + c * in.Y);
}

// calculates path length for open path
inline double PathLength(const Path64& path_p64)
{
    ClipperLib::Path path = ToC1Path(path_p64);
    double len = 0;
    if (path.size() < 2) {
        return len;
    }
    for (size_t i = 1; i < path.size(); i++) {
        len += sqrt(DistanceSqrd(ToPoint64(path[i - 1]), ToPoint64(path[i])));
    }
    return len;
}

inline double PointSideOfLine(const ClipperLib::IntPoint& p1, const ClipperLib::IntPoint& p2, const ClipperLib::IntPoint& pt)
{
    return double((pt.X - p1.X) * (p2.Y - p1.Y) - (pt.Y - p2.Y) * (p2.X - p1.X));
}

inline double Angle3Points(const ClipperLib::DoublePoint& p1, const ClipperLib::DoublePoint& p2, const ClipperLib::DoublePoint& p3)
{
    double t1 = atan2(p2.Y - p1.Y, p2.X - p1.X);
    double t2 = atan2(p3.Y - p2.Y, p3.X - p2.X);
    double a = fabs(t2 - t1);
    return min(a, 2 * std::numbers::pi - a);
}

inline ClipperLib::DoublePoint DirectionV(const ClipperLib::IntPoint& pt1, const ClipperLib::IntPoint& pt2)
{
    double DX = double(pt2.X - pt1.X);
    double DY = double(pt2.Y - pt1.Y);
    double l = sqrt(DX * DX + DY * DY);
    if (l < NTOL) {
        return ClipperLib::DoublePoint(0, 0);
    }
    return ClipperLib::DoublePoint(DX / l, DY / l);
}

inline void NormalizeV(ClipperLib::DoublePoint& pt)
{
    double len = sqrt(pt.X * pt.X + pt.Y * pt.Y);
    if (len > NTOL) {
        pt.X /= len;
        pt.Y /= len;
    }
}

inline ClipperLib::DoublePoint GetPathDirectionV(const Path64& pth_p64, size_t pointIndex)
{
    ClipperLib::Path pth = ToC1Path(pth_p64);
    if (pth.size() < 2) {
        return ClipperLib::DoublePoint(0, 0);
    }
    const ClipperLib::IntPoint& p1 = pth.at(pointIndex > 0 ? pointIndex - 1 : pth.size() - 1);
    const ClipperLib::IntPoint& p2 = pth.at(pointIndex);
    return DirectionV(p1, p2);
}

// Returns true if points 'a' and 'b' are coincident or nearly so.
bool isClose(const ClipperLib::IntPoint& a, const ClipperLib::IntPoint& b)
{
    return abs(a.X - b.X) <= 1 && abs(a.Y - b.Y) <= 1;
}

// Remove coincident and almost-coincident points from ClipperLib::Paths.
void filterCloseValues(Paths64& ppg_p64)
{
    ClipperLib::Paths ppg = ToC1Paths(ppg_p64);
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
    ppg_p64 = ToPaths64(ppg);
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
    BoundBox(const ClipperLib::IntPoint& p1)
    {
        minX = p1.X;
        maxX = p1.X;
        minY = p1.Y;
        maxY = p1.Y;
    }

    void SetFirstPoint(const ClipperLib::IntPoint& p1)
    {
        minX = p1.X;
        maxX = p1.X;
        minY = p1.Y;
        maxY = p1.Y;
    }

    // generic: subsequent points
    void AddPoint(const ClipperLib::IntPoint& pt)
    {
        minX = min(pt.X, minX);
        maxX = max(pt.X, maxX);
        minY = min(pt.Y, minY);
        maxY = max(pt.Y, maxY);
    }

    // line segment: two points
    BoundBox(const ClipperLib::IntPoint& p1, const ClipperLib::IntPoint& p2)
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
    BoundBox(const ClipperLib::IntPoint& center, long radius)
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

int getPathNestingLevel(const Path64& path_p64, const Paths64& paths_p64)
{
    ClipperLib::Path path = ToC1Path(path_p64);
    ClipperLib::Paths paths = ToC1Paths(paths_p64);
    int nesting = 0;
    for (const auto& other : paths) {
        if (!path.empty() && PointInPolygon(path.front(), other) != 0) {
            nesting++;
        }
    }
    return nesting;
}

void appendDirectChildPaths(Paths64& outPaths_p64, const Path64& path_p64, const Paths64& paths_p64)
{
    ClipperLib::Paths outPaths = ToC1Paths(outPaths_p64);
    ClipperLib::Path path = ToC1Path(path_p64);
    ClipperLib::Paths paths = ToC1Paths(paths_p64);
    int nesting = getPathNestingLevel(path_p64, paths_p64);
    for (const auto& other : paths) {
        if (!path.empty() && !other.empty() && PointInPolygon(other.front(), path) != 0) {
            if (getPathNestingLevel(ToPath64(other), paths_p64) == nesting + 1) {
                outPaths.push_back(other);
            }
        }
    }
    outPaths_p64 = ToPaths64(outPaths);
}

void AverageDirection(const vector<PointD>& unityVectors, PointD& output)
{
    std::size_t size = unityVectors.size();
    output.x = 0;
    output.y = 0;
    // sum vectors
    for (std::size_t i = 0; i < size; i++) {
        PointD v = unityVectors[i];
        output.x += v.x;
        output.y += v.y;
    }
    // normalize
    double magnitude = sqrt(output.x * output.x + output.y * output.y);
    output.x /= magnitude;
    output.y /= magnitude;
}

double DistancePointToLineSegSquared(
    const ClipperLib::IntPoint& p1,
    const ClipperLib::IntPoint& p2,
    const ClipperLib::IntPoint& pt,
    ClipperLib::IntPoint& closestPoint,
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

void ScaleUpPaths(Paths64& paths_p64, long scaleFactor)
{
    ClipperLib::Paths paths = ToC1Paths(paths_p64);
    for (auto& pth : paths) {
        for (auto& pt : pth) {
            pt.X *= scaleFactor;
            pt.Y *= scaleFactor;
        }
    }
    paths_p64 = ToPaths64(paths);
}

void ScaleDownPaths(Paths64& paths_p64, long scaleFactor)
{
    ClipperLib::Paths paths = ToC1Paths(paths_p64);
    for (auto& pth : paths) {
        for (auto& pt : pth) {
            pt.X /= scaleFactor;
            pt.Y /= scaleFactor;
        }
    }
    paths_p64 = ToPaths64(paths);
}


double DistancePointToPathsSqrd(
    const Paths64& paths_p64,
    const Point64& pt_p64,
    Point64& closestPointOnPath,
    size_t& clpPathIndex,
    size_t& clpSegmentIndex,
    double& clpParameter
)
{
    ClipperLib::Paths paths = ToC1Paths(paths_p64);
    ClipperLib::IntPoint pt = ToC1Point(pt_p64);
    double minDistSq = __DBL_MAX__;
    ClipperLib::IntPoint clp;
    // iterate though paths
    for (ClipperLib::Path::size_type i = 0; i < paths.size(); i++) {
        const ClipperLib::Path* path = &paths[i];
        ClipperLib::Path::size_type size = path->size();
        // iterate through segments
        for (ClipperLib::Path::size_type j = 0; j < size; j++) {
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
                closestPointOnPath = ToPoint64(clp);
                minDistSq = distSq;
            }
        }
    }
    return minDistSq;
}

// joins collinear segments (within the tolerance)
// TODO when migrating to ClipperLib::Clipper 2, this function can be deleted and replaced with SimplifyPath(s),
// which also handles open paths correctly
void CleanPath(const Path64& inp_p64, Path64& outpt_p64, double tolerance)
{
    ClipperLib::Path inp = ToC1Path(inp_p64);
    ClipperLib::Path outpt = ToC1Path(outpt_p64);
    if (inp.size() < 3) {
        outpt = inp;
        outpt_p64 = ToPath64(outpt);
        return;
    }
    outpt.clear();
    ClipperLib::Path tmp;
    CleanPolygon(inp, tmp, tolerance);
    long size = long(tmp.size());

    // CleanPolygon will have empty result if all points are collinear,
    // 	need to add first and last point to the output
    if (size <= 2) {
        outpt.push_back(inp.front());
        outpt.push_back(inp.back());
        outpt_p64 = ToPath64(outpt);
        return;
    }

    // restore starting point
    double clpPar = 0;
    size_t clpSegmentIndex = 0;
    size_t clpPathIndex = 0;
    ClipperLib::Paths tmpPaths;
    tmpPaths.push_back(tmp);
    Point64 clp;
    // find point on cleaned poly that is closest to original starting point
    DistancePointToPathsSqrd(ToPaths64(tmpPaths), ToPoint64(inp.front()), clp, clpPathIndex, clpSegmentIndex, clpPar);


    // if closes point is not one of the polygon points, add it as separate first point
    if (DistanceSqrd(clp, ToPoint64(tmp.at(clpSegmentIndex))) > 0
        && DistanceSqrd(clp, ToPoint64(tmp.at(clpSegmentIndex > 0 ? clpSegmentIndex - 1 : size - 1))) > 0) {
        outpt.push_back(ToC1Point(clp));
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


    if (DistanceSqrd(ToPoint64(outpt.front()), ToPoint64(inp.front())) > SAME_POINT_TOL_SQRD_SCALED) {
        outpt.insert(outpt.begin(), inp.front());
    }

    if (DistanceSqrd(ToPoint64(outpt.back()), ToPoint64(inp.back())) > SAME_POINT_TOL_SQRD_SCALED) {
        outpt.push_back(inp.back());
    }
    outpt_p64 = ToPath64(outpt);
}

bool Circle2CircleIntersect(
    const ClipperLib::IntPoint& c1,
    const ClipperLib::IntPoint& c2,
    double radius,
    pair<ClipperLib::DoublePoint, ClipperLib::DoublePoint>& intersections
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
        = ClipperLib::DoublePoint(0.5 * (c1.X + c2.X) - DY * a_2 / d, 0.5 * (c1.Y + c2.Y) + DX * a_2 / d);
    intersections.second
        = ClipperLib::DoublePoint(0.5 * (c1.X + c2.X) + DY * a_2 / d, 0.5 * (c1.Y + c2.Y) - DX * a_2 / d);
    return true;
}

bool Line2CircleIntersect(
    const ClipperLib::IntPoint& c,
    double radius,
    const ClipperLib::DoublePoint& p1,
    const ClipperLib::DoublePoint& p2,
    vector<ClipperLib::DoublePoint>& result,
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
ClipperLib::IntPoint Compute2DPolygonCentroid(const Path64& vertices_p64)
{
    ClipperLib::Path vertices = ToC1Path(vertices_p64);
    ClipperLib::DoublePoint centroid(0, 0);
    double signedArea = 0.0;
    double x0 = 0.0;  // Current vertex X
    double y0 = 0.0;  // Current vertex Y
    double x1 = 0.0;  // Next vertex X
    double y1 = 0.0;  // Next vertex Y
    double a = 0.0;   // Partial signed area

    // For all vertices
    size_t i = 0;
    ClipperLib::Path::size_type size = vertices.size();
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
    return ClipperLib::IntPoint(long(centroid.X), long(centroid.Y));
}

// point must be within first path (boundary) and must not be within all other paths (holes)
bool IsPointWithinCutRegion(const Paths64& toolBoundPaths_p64, const Point64& point_p64)
{
    ClipperLib::Paths toolBoundPaths = ToC1Paths(toolBoundPaths_p64);
    ClipperLib::IntPoint point = ToC1Point(point_p64);
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
    const Point64& s1p1,
    const Point64& s1p2,
    const Point64& s2p1,
    const Point64& s2p2,
    Point64& intersection
)
{
    double S1DX = double(s1p2.x - s1p1.x);
    double S1DY = double(s1p2.y - s1p1.y);
    double S2DX = double(s2p2.x - s2p1.x);
    double S2DY = double(s2p2.y - s2p1.y);
    double d = S1DY * S2DX - S2DY * S1DX;
    if (fabs(d) < NTOL) {
        return false;  // lines are parallel
    }

    double LPDX = double(s1p1.x - s2p1.x);
    double LPDY = double(s1p1.y - s2p1.y);
    double p1d = S2DY * LPDX - S2DX * LPDY;
    double p2d = S1DY * LPDX - S1DX * LPDY;
    if ((d < 0) && (p1d < d || p1d > 0 || p2d < d || p2d > 0)) {
        return false;  // intersection not within segment1
    }
    if ((d > 0) && (p1d < 0 || p1d > d || p2d < 0 || p2d > d)) {
        return false;  // intersection not within segment2
    }
    double t = p1d / d;
    intersection = Point64(long(s1p1.x + S1DX * t), long(s1p1.y + S1DY * t));
    return true;
}

/* finds one/first intersection of line segment with paths */
bool IntersectionPoint(const Paths64& paths_p64, const Point64& p1, const Point64& p2, Point64& intersection)
{
    ClipperLib::Paths paths = ToC1Paths(paths_p64);
    BoundBox segBB(ToC1Point(p1), ToC1Point(p2));
    for (size_t i = 0; i < paths.size(); i++) {
        const ClipperLib::Path* path = &paths[i];
        size_t size = path->size();
        if (size < 2) {
            continue;
        }
        BoundBox pathBB(path->front());
        for (size_t j = 0; j < size; j++) {

            const ClipperLib::IntPoint* pp2 = &path->at(j);

            // box check for performance
            pathBB.AddPoint(*pp2);
            if (!pathBB.CollidesWith(segBB)) {
                continue;
            }

            const ClipperLib::IntPoint* pp1 = &path->at(j > 0 ? j - 1 : size - 1);
            double LDY = double(p2.y - p1.y);
            double LDX = double(p2.x - p1.x);
            double PDX = double(pp2->X - pp1->X);
            double PDY = double(pp2->Y - pp1->Y);
            double d = LDY * PDX - PDY * LDX;
            if (fabs(d) < NTOL) {
                continue;  // lines are parallel
            }

            double LPDX = double(p1.x - pp1->X);
            double LPDY = double(p1.y - pp1->Y);
            double p1d = PDY * LPDX - PDX * LPDY;
            double p2d = LDY * LPDX - LDX * LPDY;
            if ((d < 0) && (p1d < d || p1d > 0 || p2d < d || p2d > 0)) {
                continue;  // intersection not within segment
            }
            if ((d > 0) && (p1d < 0 || p1d > d || p2d < 0 || p2d > d)) {
                continue;  // intersection not within segment
            }
            double t = p1d / d;
            intersection = Point64(long(p1.x + LDX * t), long(p1.y + LDY * t));
            return true;
        }
    }
    return false;
}

void SmoothPaths(Paths64& paths_p64, double stepSize, long pointCount, long iterations)
{
    ClipperLib::Paths paths = ToC1Paths(paths_p64);
    ClipperLib::Paths output;
    output.resize(paths.size());
    const long scale = 1000;
    const double stepScaled = stepSize * scale;

    Paths64 tmp = ToPaths64(paths);
    ScaleUpPaths(tmp, scale);
    paths = ToC1Paths(tmp);
    vector<pair<size_t /*path index*/, ClipperLib::IntPoint>> points;
    for (size_t i = 0; i < paths.size(); i++) {
        for (const auto& pt : paths[i]) {
            if (points.empty()) {
                points.emplace_back(i, pt);
                continue;
            }
            const auto back = points.back();
            const ClipperLib::IntPoint& lastPt = back.second;


            const double l = sqrt(DistanceSqrd(ToPoint64(lastPt), ToPoint64(pt)));

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
                const ClipperLib::IntPoint ptx(
                    long(lastPt.X + double(pt.X - lastPt.X) * p),
                    long(lastPt.Y + double(pt.Y - lastPt.Y) * p)
                );

                if (idx == 0 && DistanceSqrd(ToPoint64(back.second), ToPoint64(ptx)) < scale && points.size() > 1) {
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
        paths_p64 = ToPaths64(paths);
        return;
    }
    const long size = long(points.size());
    for (long iter = 0; iter < iterations; iter++) {
        for (long i = 1; i < size - 1; i++) {
            ClipperLib::IntPoint& cp = points[i].second;
            ClipperLib::IntPoint avgPoint(cp);
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
                ClipperLib::IntPoint& p = points[index].second;
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
        Path64 out_p64 = ToPath64(output[i]);
        Path64 paths_i_p64 = ToPath64(paths[i]);
        CleanPath(out_p64, paths_i_p64, 1.4 * scale);
        paths[i] = ToC1Path(paths_i_p64);
    }
    Paths64 tmp2 = ToPaths64(paths);
    ScaleDownPaths(tmp2, scale);
    paths = ToC1Paths(tmp2);
    paths_p64 = ToPaths64(paths);
}

// PopNextFinishingPass: Select and remove the next finishing pass to execute
//
// This function examines all available finishing paths (both closed and open) and selects
// the best one to execute next based on proximity to the current tool position (p1).
//
// Requirements:
//   - At least one finishing path must be available (either closed or open)
//
// Behavior:
//   - Searches through both closedFinishingPaths and openFinishingPaths to find the
//     closest point to the current tool position
//   - For CLOSED paths: Can start at any point along the path. The path is rotated
//     (shifted) so that execution begins at the closest point. The extraDistanceAround
//     parameter allows advancing further along the path before starting.
//   - For OPEN paths: Must start at the first vertex (index 0). No rotation is allowed.
//     These paths represent segments where only part of the geometry needs finishing.
//
// Return:
//   - true: A closed finishing path was selected and returned in 'result'. The path is
//     removed from closedFinishingPaths.
//   - false: An open finishing path was selected and returned in 'result'. The path is
//     removed from openFinishingPaths.
//   - If no paths remain in either list, the function returns false with an empty result.
//
bool PopNextFinishingPass(
    Paths64& closedFinishingPaths_p64,
    Paths64& openFinishingPaths_p64,
    Point64 p1,
    Path64& result_p64,
    double extraDistanceAround = 0
)
{
    ClipperLib::Paths closedFinishingPaths = ToC1Paths(closedFinishingPaths_p64);
    ClipperLib::Paths openFinishingPaths = ToC1Paths(openFinishingPaths_p64);
    ClipperLib::Path result = ToC1Path(result_p64);
    // Early return if no paths remain in either list
    if (closedFinishingPaths.empty() && openFinishingPaths.empty()) {
        result.clear();
        result_p64 = ToPath64(result);
        closedFinishingPaths_p64 = ToPaths64(closedFinishingPaths);
        openFinishingPaths_p64 = ToPaths64(openFinishingPaths);
        return false;
    }

    double minDistSqrd = __DBL_MAX__;
    size_t closestPathIndex = 0;
    size_t closestPointIndex = 0;
    bool closestIsClosed = closedFinishingPaths.size() > 0;

    // Search through closed finishing paths (can start at any point)
    for (size_t pathIndex = 0; pathIndex < closedFinishingPaths.size(); pathIndex++) {
        ClipperLib::Path& path = closedFinishingPaths.at(pathIndex);
        for (size_t i = 0; i < path.size(); i++) {
            double dist = DistanceSqrd(p1, ToPoint64(path.at(i)));
            if (dist < minDistSqrd) {
                minDistSqrd = dist;
                closestPathIndex = pathIndex;
                closestPointIndex = i;
                closestIsClosed = true;
            }
        }
    }

    // Search through open finishing paths (can only start at first vertex)
    for (size_t pathIndex = 0; pathIndex < openFinishingPaths.size(); pathIndex++) {
        ClipperLib::Path& path = openFinishingPaths.at(pathIndex);
        if (!path.empty()) {
            double dist = DistanceSqrd(p1, ToPoint64(path.at(0)));
            if (dist < minDistSqrd) {
                minDistSqrd = dist;
                closestPathIndex = pathIndex;
                closestPointIndex = 0;
                closestIsClosed = false;
            }
        }
    }

    // Handle open path: copy as-is (no rotation) and remove from list
    if (!closestIsClosed) {
        ClipperLib::Path& closestPath = openFinishingPaths.at(closestPathIndex);
        result = closestPath;  // Copy path as-is, starting at first vertex
        openFinishingPaths.erase(openFinishingPaths.begin() + closestPathIndex);
        result_p64 = ToPath64(result);
        closedFinishingPaths_p64 = ToPaths64(closedFinishingPaths);
        openFinishingPaths_p64 = ToPaths64(openFinishingPaths);
        return false;  // Return false to indicate open path
    }

    // Handle closed path: rotate to start at closest point and remove from list
    // Copy the path before erasing to avoid dangling reference
    ClipperLib::Path closestPath = closedFinishingPaths.at(closestPathIndex);
    closedFinishingPaths.erase(closedFinishingPaths.begin() + closestPathIndex);
    result.clear();

    // Apply extraDistanceAround to advance further along the path before starting
    // Interpolate as needed
    while (extraDistanceAround > 0) {
        size_t nexti = (closestPointIndex + 1) % closestPath.size();
        const ClipperLib::IntPoint& p1 = closestPath[closestPointIndex];
        const ClipperLib::IntPoint& p2 = closestPath[nexti];
        double distToNext = sqrt(DistanceSqrd(ToPoint64(p1), ToPoint64(p2)));
        closestPointIndex = nexti;

        if (distToNext <= extraDistanceAround) {
            extraDistanceAround -= distToNext;
        }
        else {
            // Interpolate to produce the first vertex
            double interp = extraDistanceAround / distToNext;  // Fraction along the edge
            ClipperLib::IntPoint interpolated(
                ClipperLib::cInt(p1.X * (1 - interp) + p2.X * interp),
                ClipperLib::cInt(p1.Y * (1 - interp) + p2.Y * interp),
                p1.Z
            );
            result.push_back(interpolated);
            extraDistanceAround = 0;
        }
    }

    // Output the remaining points from that start poitn
    for (size_t offset = 0; offset < closestPath.size(); offset++) {
        size_t index = (closestPointIndex + offset) % closestPath.size();
        result.push_back(closestPath.at(index));
    }

    result_p64 = ToPath64(result);
    closedFinishingPaths_p64 = ToPaths64(closedFinishingPaths);
    openFinishingPaths_p64 = ToPaths64(openFinishingPaths);
    return true;  // Return true to indicate closed path
}

void DeduplicatePaths(const Paths64& inputs_p64, Paths64& outputs_p64)
{
    ClipperLib::Paths inputs = ToC1Paths(inputs_p64);
    ClipperLib::Paths outputs = ToC1Paths(outputs_p64);
    outputs.clear();
    for (const auto& new_pth : inputs) {
        bool duplicate = false;
        // if all points of new path exist on some of the old paths, path is considered duplicate
        for (const auto& old_pth : outputs) {
            bool all_points_exists = true;
            for (const auto pt1 : new_pth) {
                bool pointExists = false;
                for (const auto pt2 : old_pth) {
                    if (DistanceSqrd(ToPoint64(pt1), ToPoint64(pt2)) < SAME_POINT_TOL_SQRD_SCALED) {
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
    outputs_p64 = ToPaths64(outputs);
}

void ConnectPaths(Paths64 input_p64, Paths64& output_p64)
{
    ClipperLib::Paths input = ToC1Paths(input_p64);
    ClipperLib::Paths output = ToC1Paths(output_p64);
    output.clear();
    bool newPath = true;
    ClipperLib::Path joined;
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
            ClipperLib::Path& n = input.at(i);
            if (DistanceSqrd(ToPoint64(n.front()), ToPoint64(joined.back())) < SAME_POINT_TOL_SQRD_SCALED) {
                for (auto pt : n) {
                    joined.push_back(pt);
                }
                input.erase(input.begin() + i);
                anyMatch = true;
                break;
            }
            else if (DistanceSqrd(ToPoint64(n.back()), ToPoint64(joined.back())) < SAME_POINT_TOL_SQRD_SCALED) {
                ReversePath(n);
                for (auto pt : n) {
                    joined.push_back(pt);
                }
                input.erase(input.begin() + i);
                anyMatch = true;
                break;
            }
            else if (DistanceSqrd(ToPoint64(n.front()), ToPoint64(joined.front())) < SAME_POINT_TOL_SQRD_SCALED) {
                for (auto pt : n) {
                    joined.insert(joined.begin(), pt);
                }
                input.erase(input.begin() + i);
                anyMatch = true;
                break;
            }
            else if (DistanceSqrd(ToPoint64(n.back()), ToPoint64(joined.front())) < SAME_POINT_TOL_SQRD_SCALED) {
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
    output_p64 = ToPaths64(output);
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

    void SetClearedPaths(const Paths64& paths_p64)
    {
        ClipperLib::Paths paths = ToC1Paths(paths_p64);
        clearedPaths = paths;
        bboxPathsInvalid = true;
        bboxClippedInvalid = true;
    }

    void AddClearedPaths(const Paths64& paths_p64)
    {
        ClipperLib::Paths paths = ToC1Paths(paths_p64);
        clip.Clear();
        clip.AddPaths(clearedPaths, ClipperLib::PolyType::ptSubject, true);
        clip.AddPaths(paths, ClipperLib::PolyType::ptClip, true);
        clip.Execute(ClipperLib::ClipType::ctUnion, clearedPaths);
        CleanPolygons(clearedPaths);
        bboxPathsInvalid = true;
        bboxClippedInvalid = true;
    }

    void ExpandCleared(const Path64 toClearToolPath_p64)
    {
        if (toClearToolPath_p64.empty()) {
            return;
        }
        ClipperLib::Path toClearToolPath = ToC1Path(toClearToolPath_p64);
        Perf_ExpandCleared.Start();
        clipof.Clear();
        clipof.AddPath(toClearToolPath, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etOpenRound);
        ClipperLib::Paths toolCoverPoly;
        clipof.Execute(toolCoverPoly, toolRadiusScaled + 1);
        clip.Clear();
        clip.AddPaths(clearedPaths, ClipperLib::PolyType::ptSubject, true);
        clip.AddPaths(toolCoverPoly, ClipperLib::PolyType::ptClip, true);
        clip.Execute(ClipperLib::ClipType::ctUnion, clearedPaths);
        CleanPolygons(clearedPaths);
        bboxPathsInvalid = true;
        bboxClippedInvalid = true;
        Perf_ExpandCleared.Stop();
    }

    // get cleared area/poly bounded to toolbox
    ClipperLib::Paths& GetBoundedClearedAreaClipped(const ClipperLib::IntPoint& toolPos, int delta)
    {
        // first, attempt to serve this query from cache
        BoundBox toolBB(toolPos, delta);
        if (!bboxClippedInvalid && clearedBBClippedInFocus.Contains(toolBB)) {
            return clearedBoundedClipped;
        }

        // second, check if the window needs to be recomputed
        if (bboxClippedInvalid || !clearedBBWindow.Contains(toolBB)) {
            const int deltaWindow = delta * clearedBoundedWindowScale;
            clearedBBWindow.SetFirstPoint(ClipperLib::IntPoint(toolPos.X - deltaWindow, toolPos.Y - deltaWindow));
            clearedBBWindow.AddPoint(ClipperLib::IntPoint(toolPos.X + deltaWindow, toolPos.Y + deltaWindow));

            ClipperLib::Path bbPath;
            bbPath.push_back(ClipperLib::IntPoint(toolPos.X - deltaWindow, toolPos.Y - deltaWindow));
            bbPath.push_back(ClipperLib::IntPoint(toolPos.X + deltaWindow, toolPos.Y - deltaWindow));
            bbPath.push_back(ClipperLib::IntPoint(toolPos.X + deltaWindow, toolPos.Y + deltaWindow));
            bbPath.push_back(ClipperLib::IntPoint(toolPos.X - deltaWindow, toolPos.Y + deltaWindow));
            clip.Clear();
            clip.AddPath(bbPath, ClipperLib::PolyType::ptSubject, true);
            clip.AddPaths(clearedPaths, ClipperLib::PolyType::ptClip, true);
            clip.Execute(ClipperLib::ClipType::ctIntersection, clearedBoundedWindow);
        }

        // finally, perform the query using data from the window
        clearedBBClippedInFocus.SetFirstPoint(ClipperLib::IntPoint(toolPos.X - delta, toolPos.Y - delta));
        clearedBBClippedInFocus.AddPoint(ClipperLib::IntPoint(toolPos.X + delta, toolPos.Y + delta));

        ClipperLib::Path bbPath;
        bbPath.push_back(ClipperLib::IntPoint(toolPos.X - delta, toolPos.Y - delta));
        bbPath.push_back(ClipperLib::IntPoint(toolPos.X + delta, toolPos.Y - delta));
        bbPath.push_back(ClipperLib::IntPoint(toolPos.X + delta, toolPos.Y + delta));
        bbPath.push_back(ClipperLib::IntPoint(toolPos.X - delta, toolPos.Y + delta));
        clip.Clear();
        clip.AddPath(bbPath, ClipperLib::PolyType::ptSubject, true);
        clip.AddPaths(clearedBoundedWindow, ClipperLib::PolyType::ptClip, true);
        clip.Execute(ClipperLib::ClipType::ctIntersection, clearedBoundedClipped);

        bboxClippedInvalid = false;
        return clearedBoundedClipped;
    }

    // get full cleared area
    Paths64 GetCleared()
    {
        return ToPaths64(clearedPaths);
    }

private:
    ClipperLib::Clipper clip;
    ClipperLib::ClipperOffset clipof;
    ClipperLib::Paths clearedPaths;
    ClipperLib::Paths clearedBoundedWindow;
    ClipperLib::Paths clearedBoundedClipped;
    ClipperLib::Paths clearedBoundedPaths;

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
    std::pair<double, ClipperLib::IntPoint> angle;
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
    void addPoint(double error, std::pair<double, ClipperLib::IntPoint> angle, bool allowSkip, bool isConventional)
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
std::pair<double, double> Adaptive2d::CalcCutArea(Point64 c1_p64, Point64 c2_p64, ClearedArea& clearedArea)
{
    ClipperLib::IntPoint c1 = ToC1Point(c1_p64);
    ClipperLib::IntPoint c2 = ToC1Point(c2_p64);
    double dist = sqrt(DistanceSqrd(ToPoint64(c1), ToPoint64(c2)));
    if (dist < NTOL) {
        return {0, 0};
    }

    Perf_CalcCutAreaCirc.Start();

    // 0) Extract from clearedArea a set of polygons close enough to potentially affect the bounded area
    vector<vector<ClipperLib::DoublePoint>> polygons;
    vector<ClipperLib::DoublePoint> inters;  // temporary, to hold intersection results
    const BoundBox c2BB(c2, toolRadiusScaled);
    // get curves from slightly enlarged region that will cover all points tested in this iteration
    const bool useC2 = dist > 2 * toolRadiusScaled;
    const ClipperLib::Paths& clearedBounded = clearedArea.GetBoundedClearedAreaClipped(
        useC2 ? c2 : c1,
        toolRadiusScaled + (useC2 ? 0 : (int)dist) + 4
    );

    for (const ClipperLib::Path& path : clearedBounded) {
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

        vector<ClipperLib::DoublePoint> polygon;
        for (const auto p : path) {
            polygon.emplace_back((double)p.X, (double)p.Y);
        }
        polygons.push_back(polygon);
    }

    // 0.5) Rotate all geometry so the vector from c1 to c2 points up (y+)
    {
        const double angle = std::numbers::pi / 2 - atan2(c2.Y - c1.Y, c2.X - c1.X);
        const double ca = cos(angle);
        const double sa = sin(angle);
        c1 = {(long long)(ca * c1.X - sa * c1.Y), (long long)(sa * c1.X + ca * c1.Y)};
        c2 = {(long long)(ca * c2.X - sa * c2.Y), (long long)(sa * c2.X + ca * c2.Y)};
        vector<vector<ClipperLib::DoublePoint>> rotatedPolygons;
        for (vector<ClipperLib::DoublePoint>& pgon : polygons) {
            vector<ClipperLib::DoublePoint> rotated;
            for (auto& p : pgon) {
                rotated.push_back({ca * p.X - sa * p.Y, sa * p.X + ca * p.Y});
            }
            rotatedPolygons.push_back(rotated);
        }

        polygons = rotatedPolygons;
    }

    // 1) Find all x-coordinates of interest:
    vector<double> xs;
    for (const auto& polygon : polygons) {
        // 1.a) All polygon vertices
        for (const auto& p : polygon) {
            xs.push_back(p.X);
        }

        // 1.b) Intersection of all polygons with c1
        // 1.c) Intersection of all polygons with c2
        for (size_t i = 0; i < polygon.size(); i++) {
            const auto& p0 = polygon[i];
            const auto& p1 = polygon[(i + 1) % polygon.size()];
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
        pair<ClipperLib::DoublePoint, ClipperLib::DoublePoint> res;
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

    const auto interpX = [](const ClipperLib::DoublePoint p0, const ClipperLib::DoublePoint p1, double x) {
        const double interp = (x - p0.X) / (p1.X - p0.X);
        const double y = (p1.Y * interp) + (p0.Y * (1 - interp));
        return y;
    };

    // 3) For each non-empty range in x, construct a vertical line through its midpoint
    const vector<ClipperLib::DoublePoint> circles = {c2, c1};
    double area = 0;
    double conventionalArea = 0;
    for (size_t ix = 0; ix + 1 < xs.size(); ix++) {
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
        vector<tuple<double, size_t, size_t>> ys;

        for (size_t ipolygon = 0; ipolygon < polygons.size(); ipolygon++) {
            const auto& polygon = polygons[ipolygon];
            for (size_t iedge = 0; iedge < polygon.size(); iedge++) {
                const auto& p0 = polygon[iedge];
                const auto& p1 = polygon[(iedge + 1) % polygon.size()];
                // note: we skip if the edge is vertical, p0.X == p1.X == xtest
                if (min(p0.X, p1.X) < xtest && max(p0.X, p1.X) > xtest) {
                    const double y = interpX(p0, p1, xtest);
                    ys.emplace_back(y, ipolygon, iedge);
                }
            }
        }

        for (size_t icircle = 0; icircle < circles.size(); icircle++) {
            const ClipperLib::DoublePoint c = circles[icircle];
            const double dx = abs(xtest - c.X);
            if (dx < toolRadiusScaled) {  // skip tangent; xtest can't be a tangent anyway
                const double dy = sqrt((toolRadiusScaled * toolRadiusScaled) - (dx * dx));
                ys.emplace_back(c.Y + dy, polygons.size() + icircle, 0);
                ys.emplace_back(c.Y - dy, polygons.size() + icircle, 1);
            }
        }

        // 3.b) Sort these intersection on their y-coordinate.
        std::sort(
            ys.begin(),
            ys.end(),
            [](const std::tuple<double, size_t, size_t>& a,
               const std::tuple<double, size_t, size_t>& b) {
                return std::get<0>(a) < std::get<0>(b);
            }
        );

        // 3.c) Loop over y-coordinates. At each, we will update state to account for stepping over
        // that crossing:
        //     Init (i.e. y=-inf): outsideCount = 1 (outside c2 and inside all other shapes)
        std::vector<bool> outside;
        outside.reserve(polygons.size() + circles.size());
        for (size_t i = 0; i < polygons.size() + circles.size(); i++) {
            outside.push_back(i == (polygons.size()));  // poly_0, ..., poly_n-1, c2, c1
        }
        int outsideCount = 1;
        for (const auto& [_, ishape, ipart] : ys) {
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
                    const auto& polygon = polygons[ishape];
                    const auto& p0 = polygon[ipart];
                    const auto& p1 = polygon[(ipart + 1) % polygon.size()];
                    const auto y0 = interpX(p0, p1, x0);
                    const auto y1 = interpX(p0, p1, x1);
                    const double newArea = (y0 + y1) / 2 * (x1 - x0);
                    area += entranceExitSign * newArea;
                    if (xtest < c2.X) {
                        conventionalArea += entranceExitSign * newArea;
                    }
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
                        ((tmidx - c.X) * (tmidx - c.X)) + ((tmidy - c.Y) * (tmidy - c.Y))
                    );
                    const double areaTriangle = tbase * th / 2;
                    const double areaSegment = areaSector - areaTriangle;

                    // then add on trapezoid between the segment and 0
                    // the sign of the segment area is negative for bottom half of the circle,
                    // positive for top half
                    const double areaTrapezoid = (x1 - x0) * (y0 + y1) / 2;
                    const double newArea = (circleSign * areaSegment) + areaTrapezoid;
                    area += entranceExitSign * newArea;
                    if (xtest < c2.X) {
                        conventionalArea += entranceExitSign * newArea;
                    }
                }
            }
        }
    }

    Perf_CalcCutAreaCirc.Stop();

    return {area, conventionalArea};
}

void Adaptive2d::ApplyStockToLeave(Paths64& inputPaths_p64)
{
    ClipperLib::Paths inputPaths = ToC1Paths(inputPaths_p64);
    ClipperLib::ClipperOffset clipof;
    if (stockToLeave > NTOL) {
        clipof.Clear();
        clipof.AddPaths(inputPaths, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
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
        clipof.AddPaths(inputPaths, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
        clipof.Execute(inputPaths, -1);
        Paths64 tmp = ToPaths64(inputPaths);
        filterCloseValues(tmp);
        inputPaths = ToC1Paths(tmp);
        clipof.Clear();
        clipof.AddPaths(inputPaths, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
        clipof.Execute(inputPaths, 1);
        Paths64 tmp2 = ToPaths64(inputPaths);
        filterCloseValues(tmp2);
        inputPaths = ToC1Paths(tmp2);
    }
    inputPaths_p64 = ToPaths64(inputPaths);
}

// SVG Debug Info
//*****************************************
// Write combined SVG with all accumulated paths from all regions
void writePreprocessingSVG(const DebugSVGInfo& svgInfo)
{
    // Calculate overall bounding box
    int64_t minX = INT64_MAX, minY = INT64_MAX, maxX = INT64_MIN, maxY = INT64_MIN;

    auto updateBBox = [&](const Paths64& paths) {
        for (const Path64& path : paths) {
            for (const auto& pt : path) {
                minX = std::min(minX, pt.x);
                minY = std::min(minY, pt.y);
                maxX = std::max(maxX, pt.x);
                maxY = std::max(maxY, pt.y);
            }
        }
    };

    // Regional collections (need unwrapping)
    for (const Paths64& regionPaths : svgInfo.allFinishingPaths) {
        updateBBox(regionPaths);
    }
    for (const Paths64& regionPaths : svgInfo.allToolBoundPaths) {
        updateBBox(regionPaths);
    }

    // Single collections (direct)
    updateBBox(svgInfo.step3Paths);
    updateBBox(svgInfo.step4Paths);
    updateBBox(svgInfo.step5e_inputPaths);

    long long padding = std::max(maxX - minX, maxY - minY) / 10;
    long long legendWidth = (maxX - minX) / 5;  // Legend takes 20% of width
    minX -= padding + legendWidth;
    minY -= padding;
    maxX += padding;
    maxY += padding;

    std::ofstream svg("adaptive_preprocessing.svg");
    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"" << minX << " " << -maxY << " "
        << (maxX - minX) << " " << (maxY - minY) << "\">\n";

    // Add JavaScript for toggling visibility
    svg << "<script type=\"text/javascript\"><![CDATA[\n";
    svg << "function toggle(id) {\n";
    svg << "  var el = document.getElementById(id);\n";
    svg << "  var box = document.getElementById(id + '-box');\n";
    svg << "  if (el.style.display === 'none') {\n";
    svg << "    el.style.display = 'inline';\n";
    svg << "    box.setAttribute('fill', box.getAttribute('data-color'));\n";
    svg << "  } else {\n";
    svg << "    el.style.display = 'none';\n";
    svg << "    box.setAttribute('fill', '#cccccc');\n";
    svg << "  }\n";
    svg << "}\n";
    svg << "]]></script>\n";

    auto renderPathGroup =
        [&](const std::string& groupId, const std::string& strokeColor, const Paths64& paths) {
            svg << "<g id=\"" << groupId << "\">\n";
            for (size_t i = 0; i < paths.size(); i++) {
                const Path64& ip = paths[i];
                if (!ip.empty()) {
                    // Merge consecutive edges of the same dashedness into one <path> so that
                    // stroke-dasharray renders continuously across short segments.
                    bool inPath = false;
                    bool curDash = false;
                    for (size_t j = 0; j < ip.size(); j++) {
                        const Point64& p1 = ip[j];
                        const Point64& p2 = (j + 1 < ip.size()) ? ip[j + 1] : ip[0];
                        bool edgeDash = !(p1.z == 1 && p2.z == 1);

                        if (!inPath || edgeDash != curDash) {
                            if (inPath) {
                                svg << "\"/>\n";
                            }
                            curDash = edgeDash;
                            inPath = true;
                            svg << "<path stroke=\"" << strokeColor << "\" stroke-width=\""
                                << (padding / 100) << "\" fill=\"none\"";
                            if (curDash) {
                                svg << " stroke-dasharray=\"" << (padding / 8) << ","
                                    << (padding / 8) << "\"";
                            }
                            svg << " d=\"M" << p1.x << "," << -p1.y;
                        }
                        svg << " L" << p2.x << "," << -p2.y;
                    }
                    if (inPath) {
                        svg << "\"/>\n";
                    }
                    // Add markers for Z=1 vertices
                    for (const auto& pt : ip) {
                        if (pt.z == 1) {
                            svg << "<circle cx=\"" << pt.x << "\" cy=\"" << -pt.y << "\" r=\""
                                << (3 * padding / 50) << "\" fill=\"" << strokeColor << "\"/>\n";
                        }
                    }
                }
            }
            svg << "</g>\n";
        };

    // Render all path groups
    renderPathGroup("step3-paths", "green", svgInfo.step3Paths);
    renderPathGroup("step4-paths", "orange", svgInfo.step4Paths);
    renderPathGroup("step5a-paths", "cyan", svgInfo.step5a_stockRev);
    renderPathGroup("step5b-paths", "magenta", svgInfo.step5b_outsideOfStock);
    renderPathGroup("step5c-paths", "yellow", svgInfo.step5c_inputPathsUnion);
    renderPathGroup("step5d-paths", "brown", svgInfo.step5d_clearedArea);
    renderPathGroup("step5-paths", "purple", svgInfo.step5e_inputPaths);
    for (size_t regionIdx = 0; regionIdx < svgInfo.allToolBoundPaths.size(); regionIdx++) {
        renderPathGroup(
            "tool-bound-paths-" + std::to_string(regionIdx + 1),
            "blue",
            svgInfo.allToolBoundPaths[regionIdx]
        );
    }
    for (size_t regionIdx = 0; regionIdx < svgInfo.allFinishingPaths.size(); regionIdx++) {
        renderPathGroup(
            "finishing-paths-" + std::to_string(regionIdx + 1),
            "red",
            svgInfo.allFinishingPaths[regionIdx]
        );
    }
    for (size_t regionIdx = 0; regionIdx < svgInfo.allFinalClearedPaths.size(); regionIdx++) {
        renderPathGroup(
            "final-cleared-paths-" + std::to_string(regionIdx + 1),
            "lime",
            svgInfo.allFinalClearedPaths[regionIdx]
        );
    }

    // Legend (positioned in top-left within viewBox)
    long long legendX = minX + padding / 2;
    long long legendY = -maxY + padding;
    long long boxSize = legendWidth / 15;
    long long lineHeight = boxSize * 2;

    // Calculate legend height based on number of entries
    size_t numRegions = svgInfo.allToolBoundPaths.size();
    size_t numLegendItems = 7 + numRegions * 3;  // 7 for steps 3,4,5a,5b,5c,5d,5 + 3 per region
                                                 // (step 6, step 8, final cleared)
    long long headingOffset = lineHeight;        // Space for heading

    svg << "<g id=\"legend\">\n";

    // Background for legend
    svg << "<rect x=\"" << legendX << "\" y=\"" << legendY << "\" width=\"" << legendWidth
        << "\" height=\"" << (lineHeight * numLegendItems + padding + headingOffset)
        << "\" fill=\"white\" stroke=\"black\" stroke-width=\"" << (padding / 200)
        << "\" opacity=\"0.9\"/>\n";

    // Legend heading
    svg << "<text x=\"" << (legendX + legendWidth / 2) << "\" y=\"" << (legendY + boxSize)
        << "\" font-size=\"" << (boxSize * 1.2) << "\" text-anchor=\"middle\" font-weight=\"bold\""
        << ">Click to Toggle</text>\n";

    // Legend items (in step order)
    int legendItem = 0;

    auto renderLegendItem =
        [&](const std::string& groupId, const std::string& color, const std::string& label) {
            svg << "<rect id=\"" << groupId << "-box\" data-color=\"" << color << "\" x=\""
                << (legendX + boxSize) << "\" y=\""
                << (legendY + headingOffset + lineHeight * legendItem + boxSize) << "\" width=\""
                << boxSize << "\" height=\"" << boxSize << "\" fill=\"" << color
                << "\" stroke=\"black\" stroke-width=\"" << (padding / 400)
                << "\" style=\"cursor:pointer\" onclick=\"toggle('" << groupId << "')\"/>\n";
            svg << "<text x=\"" << (legendX + boxSize * 3) << "\" y=\""
                << (legendY + headingOffset + lineHeight * legendItem + boxSize * 1.5)
                << "\" font-size=\"" << boxSize << "\" style=\"cursor:pointer\" onclick=\"toggle('"
                << groupId << "')\">" << label << "</text>\n";
            legendItem++;
        };

    renderLegendItem("step3-paths", "green", "Step 3: Input Paths");
    renderLegendItem("step4-paths", "orange", "Step 4: Profiling Areas");
    renderLegendItem("step5a-paths", "cyan", "Step 5a: Stock Reversed");
    renderLegendItem("step5b-paths", "magenta", "Step 5b: Outside Stock");
    renderLegendItem("step5c-paths", "yellow", "Step 5c: Input Paths Union");
    renderLegendItem("step5d-paths", "brown", "Step 5d: Cleared Area");
    renderLegendItem("step5-paths", "purple", "Step 5e: Input Paths to be Finished");

    for (size_t regionIdx = 0; regionIdx < numRegions; regionIdx++) {
        std::string regionLabel = (numRegions > 1) ? " (Region " + std::to_string(regionIdx + 1) + ")"
                                                   : "";
        renderLegendItem(
            "tool-bound-paths-" + std::to_string(regionIdx + 1),
            "blue",
            "Step 6: Tool Bound Paths" + regionLabel
        );
        renderLegendItem(
            "finishing-paths-" + std::to_string(regionIdx + 1),
            "red",
            "Step 8: Finishing Paths" + regionLabel
        );
        renderLegendItem(
            "final-cleared-paths-" + std::to_string(regionIdx + 1),
            "lime",
            "Final Cleared Area" + regionLabel
        );
    }

    svg << "</g>\n";

    svg << "</svg>";
    svg.close();
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

    if (helixRampTargetDiameter < NTOL) {
        helixRampTargetDiameter = toolDiameter;
    }
    helixRampTargetDiameter = min(helixRampTargetDiameter, toolDiameter);
    helixRampMinDiameter = max(helixRampMinDiameter, toolDiameter / 8);
    helixRampTargetDiameter = max(helixRampTargetDiameter, helixRampMinDiameter);

    helixRampMaxRadiusScaled = long(helixRampTargetDiameter * scaleFactor / 2);
    helixRampMinRadiusScaled = long(helixRampMinDiameter * scaleFactor / 2);
    finishPassOffsetScaled = finishingProfile ? long(stepOverScaled * FINISHING_THICKNESS_SCALE) : 0;

    // Debug variables for SVG visualization
    DebugSVGInfo svgInfo;

    // generate tool shape
    Paths64 toolGeometryPaths = OffsetPath(Path64{{{0, 0}}}, toolRadiusScaled, EndType::Round);
    Path64& toolGeometry = toolGeometryPaths[0];
    // calculate reference area
    Path64 slotCut = TranslatePath(toolGeometry, (int64_t)(toolRadiusScaled / 2), (int64_t)0);
    Paths64 crossing = Difference(Paths64{toolGeometry}, Paths64{slotCut}, FillRule::EvenOdd);
    referenceCutArea = fabs(Area(crossing[0]));
    optimalCutAreaPD = 2 * stepOverFactor * referenceCutArea / toolRadiusScaled;
#ifdef DEV_MODE
    cout << "optimalCutAreaPD:" << optimalCutAreaPD << " scaleFactor:" << scaleFactor
         << " toolRadiusScaled:" << toolRadiusScaled
         << " helixRampMaxRadiusScaled:" << helixRampMaxRadiusScaled << endl;
#endif
    //******************************
    // Convert input paths to clipper
    //******************************
    Paths64 converted;
    for (size_t i = 0; i < paths.size(); i++) {
        Path64 cpth;
        for (size_t j = 0; j < paths[i].size(); j++) {
            std::pair<double, double> pt = paths[i][j];
            cpth.push_back(Point64(long(pt.first * scaleFactor), long(pt.second * scaleFactor)));
        }
        Path64 cpth2;
        CleanPath(cpth, cpth2, FINISHING_CLEAN_PATH_TOLERANCE);
        converted.push_back(cpth2);
    }
    Paths64 inputPaths;
    DeduplicatePaths(converted, inputPaths);
    ConnectPaths(inputPaths, inputPaths);
    inputPaths = Union(inputPaths, FillRule::EvenOdd);
    ApplyStockToLeave(inputPaths);

    //*************************
    // convert stock paths
    //*************************
    for (size_t i = 0; i < stockPaths.size(); i++) {
        Path64 cpth;
        for (size_t j = 0; j < stockPaths[i].size(); j++) {
            std::pair<double, double> pt = stockPaths[i][j];
            cpth.push_back(Point64(long(pt.first * scaleFactor), long(pt.second * scaleFactor)));
        }
        stockInputPaths.push_back(cpth);
    }

    // Convert cleared area
    Paths64 initialClearedPaths;
    for (size_t i = 0; i < clearedPaths.size(); i++) {
        Path64 p;
        for (size_t j = 0; j < clearedPaths[i].size(); j++) {
            long x = long(clearedPaths[i][j].first * scaleFactor);
            long y = long(clearedPaths[i][j].second * scaleFactor);
            p.push_back({x, y});
        }
        initialClearedPaths.push_back(p);
    }
    initialClearedPaths = Union(initialClearedPaths, FillRule::EvenOdd);

    stockInputPaths = Union(stockInputPaths, FillRule::EvenOdd);

    // Handle different clearing modes (inside/outside, clearing/profiling) and invoke the core
    // adaptive algorithm
    // 1) If outer clearing: add stock path to inputs
    // 2) Fix input path orientation
    // 3) Set Z=1 on all input paths to tag them as needing a finishing pass
    // 4) Turn profiles into areas; mark new paths as unfinished (Z=0) and then union
    // 5) If going outside the stock is allowed, add regionOutsideStock to both inputPaths and
    // 6) Compute toolBounds = offset(input paths, -(toolRadius + finishingThickness)).
    // 7) Loop over connected components using nesting level.
    // 8) finishingPass = offset(currentTBP, finishingThickness), filtered for paths with Z=1
    // 9) Compute bounds = offset(currentTBP, toolRadius)
    // 10) Run core algorithm on (bounds, toolBounds, finishingPass, clearedArea)

    // 1) If outer clearing: add stock path to inputs
    if (opType == OperationType::otClearingOutside) {
        for (const Path64& p : stockInputPaths) {
            inputPaths.push_back(p);
        }
    }

    // 2) Fix input path orientation
    for (Path64& inputPath : inputPaths) {
        // nesting count includes self, so odd is positive orientation
        int nesting = getPathNestingLevel(inputPath, inputPaths);
        if ((nesting % 2 == 1) ^ IsPositive(inputPath)) {
            std::reverse(inputPath.begin(), inputPath.end());
        }
    }

    // 3) Set Z=1 on all input paths to tag them as needing a finishing pass
    //
    // When finishing passes are eventually generated, they will be created only for edges with
    // z=1 on both end vertices. This is a little bit imprecise -- ideally the segments themselves
    // would be tagged instead of their end vertices, but clipper doesn't support that feature
    // natively. From what I've seen this is good enough in practice, but if it becomes problematic
    // we can try to leverage vertex z-tagging to create a more complicated system for indirectly
    // tagging edges.
    for (Path64& inputPath : inputPaths) {
        for (Point64& p : inputPath) {
            p.z = 1;
        }
    }
    svgInfo.step3Paths = inputPaths;

    // 4) Turn profiles into areas; mark new paths as unfinished (Z=0) and then union
    //
    // Adaptive "profiling" clears a small area offset from the input edges, but since the goal of
    // the operation is profiling, rather than area clearing, only the input edge needs to be
    // finished. The far sides of those areas do not require a finishing pass
    if (opType == OperationType::otProfilingOutside || opType == OperationType::otProfilingInside) {
        // offset by an extra finishPassOffsetScaled to compensate for undoing that later
        long offset = 2 * (toolRadiusScaled + helixRampMaxRadiusScaled + finishPassOffsetScaled)
            + MIN_STEP_CLIPPER;
        if (opType == OperationType::otProfilingInside) {
            offset = -offset;
        }

        Paths64 fullPaths;

        for (Path64& inputPath : inputPaths) {
            Paths64 offsetPaths = OffsetPath(inputPath, offset);

            if (IsPositive(inputPath) ^ (offset > 0)) {
                std::reverse(inputPath.begin(), inputPath.end());
            }
            Paths64 clips = {inputPath};
            for (Path64& offsetPath : offsetPaths) {
                for (Point64& p : offsetPath) {
                    p.z = 0;
                }
                if (IsPositive(offsetPath) ^ (offset < 0)) {
                    std::reverse(offsetPath.begin(), offsetPath.end());
                }
                clips.push_back(offsetPath);
            }
            fullPaths = Union(fullPaths, clips, FillRule::EvenOdd);
        }

        inputPaths = fullPaths;
        svgInfo.step4Paths = fullPaths;
    }
    else {
        svgInfo.step4Paths = inputPaths;  // No change for non-profiling operations
    }

    // 5) If going outside the stock is allowed, add regionOutsideStock to both inputPaths and
    // clearedArea. Use Z=0 to mark the stock boundary as not needing to be finished
    //
    // We are working towards generating the toolBoundsPath -- the region in which the tool center
    // is allowed to be. So far we have the basis for allowing the tool within the raw input
    // regions. In this step, if going outside the stock is allowed, we add a boundary region
    // outside the stock to the inputs so the tool will be allowed there too.
    //
    // All new paths created in this process have vertices tagged with Z=0 to prevent them from
    // receiving finishing passes. Also, the new regions are added to the representation of
    // already-cleared area so the algorithm knows that the tool *can* go there but does not need
    // to.
    if (!forceInsideOut) {

        // 5a) Shrink the stock boundary to ensure overlap with input paths that hit the boundary
        Paths64 stockRev = OffsetPaths(stockInputPaths, -2);
        for (Path64& p : stockRev) {
            std::reverse(p.begin(), p.end());
        }
        svgInfo.step5a_stockRev = stockRev;

        // 5b) Create outside-of-stock region
        double overshootDistance = 4 * toolRadiusScaled + stockToLeave * scaleFactor;
        Paths64 outsideOfStock = OffsetPaths(stockInputPaths, overshootDistance, EndType::Polygon, JoinType::Square);
        svgInfo.step5b_outsideOfStock = outsideOfStock;

        // 5c) Union input paths with stock regions
        Clipper64 clip;
        clip.AddSubject(inputPaths);
        clip.AddClip(stockRev);
        clip.AddClip(outsideOfStock);

        // Z callback: output Z=1 if both vertices of either input edge have Z=1.
        // This callback will be invoked on new vertices created while unioning areas (i.e.
        // intersections between segments)
        clip.SetZCallback(
            [](const Point64& e1bot, const Point64& e1top, const Point64& e2bot, const Point64& e2top, Point64& pt) {
                bool edge1HasZ1 = (e1bot.z == 1 && e1top.z == 1);
                bool edge2HasZ1 = (e2bot.z == 1 && e2top.z == 1);
                pt.z = (edge1HasZ1 || edge2HasZ1) ? 1 : 0;
            }
        );

        clip.Execute(ClipType::Union, FillRule::EvenOdd, inputPaths);
        svgInfo.step5c_inputPathsUnion = inputPaths;

        // 5d) Update cleared area
        outsideOfStock = OffsetPaths(stockInputPaths, 100 * toolRadiusScaled, EndType::Polygon, JoinType::Square);

        clip.Clear();
        clip.AddSubject(initialClearedPaths);
        clip.AddClip(stockRev);
        clip.AddClip(outsideOfStock);
        clip.Execute(ClipType::Union, FillRule::EvenOdd, initialClearedPaths);
        svgInfo.step5d_clearedArea = initialClearedPaths;
    }

    // 5e) Use initialClearedPaths to zero out Z-values for parts of the input paths that do not
    // need to be finished. This was added after the fact and I probably ought to make it a
    // high-level step of its own, but I didn't want to renumber everything.
    //
    // So far, "input" paths are tagged as needing to be finished if the user input them, but not if
    // they are generated to allow tool movement outside the stock or if they are generated as the
    // boundary region of a path to be profiled. However, in addition to these exceptions, some
    // (complete or incomplete) sections of user-specified input paths also do not require finishing
    // passes because they protrude into regions that are already clear. For example, if the user
    // runs adaptive on the same face twice, first with a large tool and then again with a small
    // tool (with rest machining enabled), much of the boundary of the small-tool adaptive operation
    // may already be cleared, and does not require a finishing pass. Only the input boundaries that
    // are actually cut in the current operation require a finishing pass. (If the user wants a
    // full-boundary finishing pass with the small tool in this scenario, they should leave some
    // stock uncut at the boundary of the large-tool pass.)
    //
    // This computation is done by computing the initial non-cleared area and intersecting it with
    // the input path boundary. The portions of the boundary present in this intersection need to be
    // finished; the rest do not.
    //
    // Unfortunately, Clipper does not natively support this sort of "update z if in intersection"
    // operation. In order build it out of clipper primitives, we start by creating a copy of the
    // input path with z-values corresponding to the index/position in the original input. This
    // enables us to keep track of which vertex is which after taking the intersection. Then create
    // the updated version of the path by simultaneously iterating through the original input path
    // and the intersection result and re-assembling the input boundary path with appropriately
    // (z=0/z=1) tagged vertices to specify if they require a finishing pass.
    //
    // There is some subtlety to this tracking+reassembly process because new vertices may be
    // generated by the intersection operation, subdividing input edges one or more times. These
    // vertices are assigned new z-values that uniquely identify them, but these z-values do not
    // correspond to the index of any vertex in the initial path. For these newly generated points,
    // we separately track their position along the boundary (represented as a floating point number
    // interpolating the positions of the vertices of their parent edges). We also check if the
    // parent edge needs to be finished (at least as understood prior to this computation). If so,
    // we also tag this splitting vertex as needing to be finished to preserve that property when
    // the edge is split.
    //
    // During path reassembly, information about new/split-edge vertices needs to be looked up by a
    // different process than original vertices, but all of the same information (position/order on
    // the path, potentially requires finishing) is present, so after the information lookup is
    // complete they can be handled the same way as original vertices.
    {
        // Compute noncleared area = stock - cleared
        Clipper64 clipDiff;
        clipDiff.AddSubject(stockInputPaths);
        clipDiff.AddClip(initialClearedPaths);
        Paths64 nonclearedPaths;
        clipDiff.Execute(
            ClipType::Difference,
            FillRule::EvenOdd,
            nonclearedPaths
        );

        Paths64 updatedPaths;
        for (const auto& path : inputPaths) {
            // Create a copy of the path with z value specifying the index. Points are initialized
            // with z=0 by default, so to make uninitialized-z bugs clear we skip z=0 and set z =
            // index + 1
            Path64 indexedPath;
            for (size_t i = 0; i < path.size(); i++) {
                indexedPath.emplace_back(path[i].x, path[i].y, i + 1);
            }
            // Explicitly close the path, before doing a boolean operation on it as an open (well,
            // "not implicitly closed") path. Unfortunately clipper treats points as interchangeable
            // if they have equal x and y even if z is different, so this end vertex also shares the
            // start vertex's z-value.
            indexedPath.push_back(indexedPath[0]);

            // Prepare structures to store information about newly generated/edge-splitting points
            const int firstNewZ = static_cast<int>(path.size()) + 2;
            int nextNewZ = firstNewZ;
            std::map<int, double> newZPosition;
            std::map<int, int64_t> newZNeedsFinishing;

            // Create a new clipper object to avoid overwriting the z callback of the main one
            Clipper64 clip2;
            clip2.SetZCallback([&nextNewZ, &newZPosition, &newZNeedsFinishing, &path](
                                   const Point64& e1bot,
                                   const Point64& e1top,
                                   const Point64& e2bot,
                                   const Point64& e2top,
                                   Point64& pt
                               ) {
                (void)e2bot;
                (void)e2top;
                // If the split-edge point is actuall at an edge then we have no need for a new
                // z-value. Otherwise, generate a new z value
                if (pt.x == e1bot.x && pt.y == e1bot.y) {
                    pt.z = e1bot.z;
                    return;
                }
                if (pt.x == e1top.x && pt.y == e1top.y) {
                    pt.z = e1top.z;
                    return;
                }
                pt.z = nextNewZ++;

                // Compute the position of the new point by interpolating the z-values of its input
                // parent edge (e1 is the parent edge of the subject/input path; e2 comes from the
                // clipping/cleared area path)

                // Compute distance from e1bot to e1top
                double dx_total = e1top.x - e1bot.x;
                double dy_total = e1top.y - e1bot.y;
                double dist_total = sqrt(dx_total * dx_total + dy_total * dy_total);

                // Compute distance from e1bot to pt
                double dx_pt = pt.x - e1bot.x;
                double dy_pt = pt.y - e1bot.y;
                double dist_pt = sqrt(dx_pt * dx_pt + dy_pt * dy_pt);

                // Compute interpolated position Z value. In most cases edges are z=n and z=n+1, and
                // we do a simple interpolation. However, the final edge connects z=path.size() to
                // z=1, and that interpolation result does not correctly specify the position of the
                // new point. Instead, we detect that case and interpolate between path.size() and
                // path.size() + 1.
                double e1bot_effective = (static_cast<std::size_t>(e1bot.z) == 1
                                          && static_cast<std::size_t>(e1top.z) == path.size())
                    ? static_cast<double>((path.size() + 1))
                    : static_cast<double>(e1bot.z);
                double e1top_effective = (e1top.z == 1
                                          && static_cast<std::size_t>(e1bot.z) == path.size())
                    ? (path.size() + 1)
                    : static_cast<double>(e1top.z);
                double interp = dist_pt / dist_total;
                double interpZ = e1bot_effective + interp * (e1top_effective - e1bot_effective);

                // Update data structures for the new point
                newZPosition[pt.z] = interpZ;

                // I'm pretty sure e1bot and e1top are always original vertices, but just in case
                // we can handle the alternative possibility
                int64_t e1bot_finishing = (e1bot.z >= 1
                                        && static_cast<std::size_t>(e1bot.z) <= path.size())
                    ? path[e1bot.z - 1].z
                    : newZNeedsFinishing[static_cast<int>(e1bot.z)];
                int64_t e1top_finishing = (e1top.z >= 1
                                        && static_cast<std::size_t>(e1top.z) <= path.size())
                    ? path[e1top.z - 1].z
                    : newZNeedsFinishing[static_cast<int>(e1top.z)];
                newZNeedsFinishing[pt.z] = e1bot_finishing && e1top_finishing;
            });

            // Intersect the input boundary (i.e. open path intersection) with the noncleared area
            clip2.AddOpenSubject({indexedPath});
            clip2.AddClip(nonclearedPaths);
            Paths64 unusedClosedPaths, nonclearedInputs;
            clip2.Execute(
                ClipType::Intersection,
                FillRule::EvenOdd,
                unusedClosedPaths,
                nonclearedInputs
            );

            // Prepare for the final reassembly process by collecting all points from the
            // intersection result. Also remove duplicate representations of the start/end point,
            // since we will switch back to the clipper default of implicitly-closed paths
            std::vector<Point64> nonclearedPoints;
            bool hasZ1 = false;
            for (const auto& nonclearedPath : nonclearedInputs) {
                for (const auto& pt : nonclearedPath) {
                    if (pt.z != 1 || !hasZ1) {
                        nonclearedPoints.push_back(pt);
                    }
                    hasZ1 |= pt.z == 1;
                }
            }
            indexedPath.pop_back();  // remove duplicate start point from the input

            // Further prepare for reassembly by sorting the collected points by their position
            // (z-value for original points, newZPosition for new points). This allows us to
            // reassemble with a single pass, simultaneously iterating through both the original
            // input points and the intersection results in order of increasing position.
            std::sort(
                nonclearedPoints.begin(),
                nonclearedPoints.end(),
                [&newZPosition, &firstNewZ](const Point64& p1, const Point64& p2) {
                    double val1 = (p1.z >= firstNewZ) ? newZPosition.at(p1.z) : (double)p1.z;
                    double val2 = (p2.z >= firstNewZ) ? newZPosition.at(p2.z) : (double)p2.z;
                    return val1 < val2;
                }
            );

            // Reassemble the updated input path with new z-values.
            //
            // Case 1: Points present in the original input and not the intersection appear in
            // cleared area and do not require finishing (set z=0).
            //
            // Case 2: Points present in the intersection and not the original input are
            // generated/edge-splitting points and need to be added. They appear in the non-cleared
            // area, so they should be finished if their parent edge wanted finishing (i.e. use the
            // value in newZNeedsFinishing).
            //
            // Case 3: Points present in both appear in the non-cleared area, so they should be
            // finished if they are already tagged for finishing (i.e. keep the original z-value
            // from the input).
            Path64 updatedPath;
            auto nonclearedIt = nonclearedPoints.begin();
            auto indexedIt = indexedPath.begin();

            while (indexedIt != indexedPath.end() || nonclearedIt != nonclearedPoints.end()) {
                // Get position of next noncleared point (if any remain)
                double nextNonclearedPos = (nonclearedIt != nonclearedPoints.end())
                    ? ((nonclearedIt->z >= firstNewZ) ? newZPosition.at(nonclearedIt->z)
                                                      : static_cast<double>(nonclearedIt->z))
                    : std::numeric_limits<double>::max();

                // Get position of next indexed path point (if any)
                double nextPathPos = (indexedIt != indexedPath.end())
                    ? static_cast<double>(indexedIt->z)
                    : std::numeric_limits<double>::max();

                if (nextPathPos < nextNonclearedPos) {
                    // Case 1: original point is not in the noncleared region
                    Point64 newPt = path[indexedIt->z - 1];
                    newPt.z = 0;
                    updatedPath.push_back(newPt);
                    indexedIt++;
                }
                else if (nextNonclearedPos < nextPathPos) {
                    // Case 2: newly generated/split-edge point in the noncleared region
                    Point64 newPt = *nonclearedIt;
                    newPt.z = newZNeedsFinishing.at(newPt.z);
                    updatedPath.push_back(newPt);
                    nonclearedIt++;
                }
                else {
                    // Case 3: the same point is present in both lists
                    Point64 newPt = path[indexedIt->z - 1];
                    updatedPath.push_back(newPt);
                    indexedIt++;
                    nonclearedIt++;
                }
            }

            updatedPaths.push_back(updatedPath);
        }

        inputPaths = updatedPaths;
        svgInfo.step5e_inputPaths = inputPaths;
    }

    // 6) Compute toolBounds = offset(input paths, -(toolRadius + finishingThickness)).
    //
    // This represents the locations the tool center is allowed to be during the main adatpive
    // pass, prior to applying finishing profiling.

    // Use ClipperOffset with Z callback to preserve Z=1 marking
    // Note that PreserveCollinear must be set of ClipperOffset (it is already set by default on
    // Clipper64) to avoid accidentally optimizing out collinear points with differing z-values
    ClipperOffset clipof(2.0, 0.25);
    clipof.PreserveCollinear(true);
    clipof.AddPaths(inputPaths, JoinType::Round, EndType::Polygon);

    // Z callback: output Z=1 if both vertices of either input edge have Z=1
    clipof.SetZCallback([](const Point64& e1bot,
                            const Point64& e1top,
                            const Point64& e2bot,
                            const Point64& e2top,
                            Point64& pt) {
        // Check if both vertices of edge 1 have Z=1
        bool edge1HasZ1 = (e1bot.z == 1 && e1top.z == 1);
        // Check if both vertices of edge 2 have Z=1
        bool edge2HasZ1 = (e2bot.z == 1 && e2top.z == 1);
        // Set output Z=1 if either edge has both vertices with Z=1
        pt.z = (edge1HasZ1 || edge2HasZ1) ? 1 : 0;
    });

    Paths64 toolBounds;
    clipof.Execute(-(toolRadiusScaled + finishPassOffsetScaled), toolBounds);

    // 7) Loop over connected components of toolBounds
    //
    // The main adaptive algorithm (ProcessPolyNode) is run on one connected component at a time
    // to facilitate considering a helix-down entrance exclusively for the initial engagement of
    // each component and using entrances from the cleared area for all subsequent reengagements.
    //
    // Note that connectivity here is determined based on the toolBounds representation, which
    // indicates where the tool center is allowed to be. If two regions are connected only by
    // narrow channels that do not fit the tool, they will be split up for independent processing
    // here. This guarantees that the region provided to ProcessPolyNode can be fully cleared by
    // repeated reengagement from the cleared area after the initial entrance (which may require
    // helixing down).
    //
    // When we loop over toolBounds, we look for outer boundaries only. When we find one, we also
    // accumulate all top-level holes inside of them. The boundary path and immediate hole paths are
    // processed together. Any further-nested paths (i.e. appearing inside a hole) will be processed
    // in a separate iteration of the loop.
    for (const auto& current : toolBounds) {
        // Nesting counts itself and the number of polygons containing it, so nesting % 2 == 1
        // specifies exterior boundaries and nesting % 2 == 0 specifies holes
        int nesting = getPathNestingLevel(current, toolBounds);
        if (nesting % 2 != 0) {
            // current is an exterior boundary; now find all the holes directly inside it
            Paths64 currentTBP;
            currentTBP.push_back(current);

            for (size_t iother = 0; iother < toolBounds.size(); iother++) {
                const auto& other = toolBounds[iother];

                if (PointInPolygon(other.front(), current) != PointInPolygonResult::IsOutside) {
                    if (getPathNestingLevel(other, toolBounds) == nesting + 1) {
                        currentTBP.push_back(other);
                    }
                }
            }

            // 8) finishingPass = offset(currentTBP, finishingThickness)
            //
            // Now that we have currentTBP representing where the tool center may be while during
            // the main clearing operation, we need to reconstruct the required finishing passes for
            // that region.
            //
            // Note that it is
            // important that we offset the input inward to create the TBP and then offset part of
            // the way back outwards from the TBP to create the finishing pass, and that we do not
            // directly compute the finishing pass from the input by only offsetting inwards. If the
            // input has narrow channels that are just wide enough to fit the tool for a finishing
            // pass, those regions will not be wide enough for interior clearing. If we actually
            // executed this "finishing" pass, the tool would be slotting when it entered that
            // region. By instead constructing our finising pass by offsetting outward from the TBP,
            // we guarantee that all points on the finishing pass represent a small stepover cut
            // from already-cleared area. Thin channels that just barely/exactly fit the tool will
            // not be cut, but that is a reality of an adaptive cutting process -- such regions must
            // be cut by a slotting operation instead.
            clipof.Clear();
            clipof.AddPaths(currentTBP, JoinType::Round, EndType::Polygon);

            Paths64 finishingPass;
            clipof.Execute(finishPassOffsetScaled, finishingPass);

            // 9) Compute bounds = offset(currentTBP, toolRadius)
            //
            // These paths represent the region that will actually be cleared by the interior
            // clearing process. They do not include the region cleared by the finishing pass.
            //
            // When the interior clearing process completes, we will assert that this region is
            // fully cleared. In order to make the numerics work correctly on this check, we
            // actually need this region to be *slightly* smaller than its ideal/nominal size.
            // Specifically, each offset operation we perform can produce up to 1 unit of error.
            // (This is a necessary result of working on an integer grid because there must be an
            // orientation for input segments where a tiny perturbation in the input orientation
            // (which can always be achieved, even on an integer grid) changes the output location.
            // This is an output change of 1 for an input change of epsilon, but the non-dicritized
            // output would only have changed by epsilon. Therefore, offset operations must have
            // potential to accumulate error of up to 1 unit per offset operation). We perform 2
            // offset operations to produce this path (input -> TBP -> BP), and we will do 1 more
            // when computing the cleared area. So when computing BP, we leave a buffer of 3 units
            // to ensure it will be seen as fully cleared when checking a correct interior clearing
            // pass. This process is a bit finicky -- take great care if you modify it!
            Paths64 boundPath = OffsetPaths(currentTBP, toolRadiusScaled - 3);

            // Skip path generation if bounds are fully cleared
            //
            // Note: should we also be checking that finishing passes (if any are required) are
            // also clear? I'd think so, but I'm just documenting now and not going to change it.
            // If I were to implement that, I'd do subtract(offset(finishing, toolRadius), cleared)
            // and if the result contains any z=1 segments then there is finishing work left to do.
            // I don't know if ProcessPolyNode is well equipped to handle a fully pre-cleared
            // interior region with only finishing work to do; probably it has never been tested and
            // it is not!
            {
                Paths64 boundsToClear = Difference(boundPath, initialClearedPaths, FillRule::EvenOdd);
                if (!boundsToClear.size()) {
                    continue;
                }
            }

            svgInfo.allToolBoundPaths.push_back(currentTBP);
            svgInfo.allFinishingPaths.push_back(finishingPass);

            // 10) Run core algorithm on (bounds, toolBounds, finishingPass, clearedArea)
            ProcessPolyNode(boundPath, currentTBP, finishingPass, initialClearedPaths, svgInfo);
        }
    }

#ifdef DEBUG_SVG
    writePreprocessingSVG(svgInfo);
#else
    UNUSED(svgInfo);
#endif

    return results;
}

bool Adaptive2d::FindEntryPoint(
    TPaths& progressPaths,
    const Paths64& toolBoundPaths_p64,
    const Paths64& boundPaths_p64,
    ClearedArea& clearedArea /*output-initial cleared area by helix*/,
    Point64& entryPoint_p64 /*output*/,
    Point64& toolPos_p64,
    PointD& toolDir_p64,
    long& helixRadiusScaled,
    AdaptiveOutput& adaptiveOutput
)
{
    ClipperLib::Paths toolBoundPaths = ToC1Paths(toolBoundPaths_p64);
    ClipperLib::Paths boundPaths = ToC1Paths(boundPaths_p64);
    ClipperLib::IntPoint entryPoint, toolPos;
    ClipperLib::DoublePoint toolDir;
    ClipperLib::Paths incOffset;
    ClipperLib::Paths lastValidOffset;
    ClipperLib::Clipper clip;
    ClipperLib::ClipperOffset clipof;
    bool found = false;
    ClipperLib::Paths clearedPaths;

    ClipperLib::Paths checkPaths;
    clip.Clear();
    clip.AddPaths(toolBoundPaths, ClipperLib::PolyType::ptSubject, true);
    clip.AddPaths(ToC1Paths(clearedArea.GetCleared()), ClipperLib::PolyType::ptClip, true);
    clip.Execute(ClipperLib::ClipType::ctDifference, checkPaths);

    for (int iter = 0; iter < 10; iter++) {
        clipof.Clear();
        clipof.AddPaths(checkPaths, ClipperLib::JoinType::jtSquare, ClipperLib::EndType::etClosedPolygon);
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
                entryPoint = Compute2DPolygonCentroid(ToPath64(lastValidOffset[i]));
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
        const auto checkHelixFit = [&](long testHelixRadiusScaled) {
            clipof.Clear();
            ClipperLib::Path p1;
            p1.push_back(entryPoint);
            clipof.AddPath(p1, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etOpenRound);
            clipof.Execute(clearedPaths, (double)(testHelixRadiusScaled + toolRadiusScaled));
            CleanPolygons(clearedPaths);
            // we got first cleared area - check if it is crossing boundary
            clip.Clear();
            clip.AddPaths(clearedPaths, ClipperLib::PolyType::ptSubject, true);
            clip.AddPaths(boundPaths, ClipperLib::PolyType::ptClip, true);
            ClipperLib::Paths crossing;
            clip.Execute(ClipperLib::ClipType::ctDifference, crossing);

            return crossing.empty();
        };

        if (found) {
            // check that helix fits, and make initial polygon cleared by helix ramp
            if (!checkHelixFit(helixRampMinRadiusScaled)) {
                // min-size helix does not fit
                found = false;
            }
            else {
                // find the largest helix that fits
                // minSize = largest known fit; maxSize = largest possible fit
                long minSize = helixRampMinRadiusScaled;
                long maxSize = helixRampMaxRadiusScaled;
                while (minSize < maxSize) {
                    long testSize = (minSize + maxSize + 1) / 2;  // always testSize > minSize
                    if (checkHelixFit(testSize)) {
                        minSize = testSize;
                    }
                    else {
                        maxSize = testSize - 1;  // always maxSize >= minSize
                    }
                }
                helixRadiusScaled = minSize;
                checkHelixFit(helixRadiusScaled);  // set clearedPaths for final size
                clearedArea.AddClearedPaths(ToPaths64(clearedPaths));
            }
        }

        if (!found) {  // break simetry and try again
            clip.Clear();
            clip.AddPaths(checkPaths, ClipperLib::PolyType::ptSubject, true);
            auto bounds = clip.GetBounds();
            clip.Clear();
            ClipperLib::Path rect;
            rect << ClipperLib::IntPoint(bounds.left, bounds.bottom);
            rect << ClipperLib::IntPoint(bounds.left, (bounds.top + bounds.bottom) / 2);
            rect << ClipperLib::IntPoint((bounds.left + bounds.right) / 2, (bounds.top + bounds.bottom) / 2);
            rect << ClipperLib::IntPoint((bounds.left + bounds.right) / 2, bounds.bottom);
            clip.AddPath(rect, ClipperLib::PolyType::ptSubject, true);
            clip.AddPaths(checkPaths, ClipperLib::PolyType::ptClip, true);
            clip.Execute(ClipperLib::ClipType::ctIntersection, checkPaths);
        }
        if (found) {
            break;
        }
    }

    if (!found) {
        adaptiveOutput.StartPointNotFound = true;
        cerr << "Start point not found!" << endl;
    }
    if (found) {
        // visualize/progress for helix
        clipof.Clear();
        ClipperLib::Path hp;
        hp << entryPoint;
        clipof.AddPath(hp, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etOpenRound);
        ClipperLib::Paths hps;
        clipof.Execute(hps, helixRadiusScaled);
        AddPathsToProgress(progressPaths, ToPaths64(hps));

        toolPos = ClipperLib::IntPoint(entryPoint.X, entryPoint.Y - helixRadiusScaled);
        toolDir = ClipperLib::DoublePoint(1.0, 0.0);
        entryPoint_p64 = ToPoint64(entryPoint);
        toolPos_p64 = ToPoint64(toolPos);
        toolDir_p64 = ToPointD(toolDir);
    }
    return found;
}

//************************************************************
//  IsClearPath - returns true if path is clear from obstacles
//***********************************************************
bool Adaptive2d::IsClearPath(const Path64& tp_p64, ClearedArea& cleared, double safetyClearance)
{
    Perf_IsClearPath.Start();
    ClipperLib::Path tp = ToC1Path(tp_p64);
    ClipperLib::Clipper clip;
    ClipperLib::ClipperOffset clipof;
    clipof.AddPath(tp, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etOpenRound);
    ClipperLib::Paths toolShape;
    clipof.Execute(toolShape, toolRadiusScaled + safetyClearance);
    clip.AddPaths(toolShape, ClipperLib::PolyType::ptSubject, true);
    clip.AddPaths(ToC1Paths(cleared.GetCleared()), ClipperLib::PolyType::ptClip, true);
    ClipperLib::Paths crossing;
    clip.Execute(ClipperLib::ClipType::ctDifference, crossing);
    double collisionArea = 0;
    for (auto& p : crossing) {
        collisionArea += fabs(Area(p));
    }
    Perf_IsClearPath.Stop();
    return collisionArea < 1.0;
}

bool Adaptive2d::IsAllowedToCutTrough(
    const Point64& p1_p64,
    const Point64& p2_p64,
    ClearedArea& cleared,
    const Paths64& toolBoundPaths_p64,
    double areaFactor,
    bool skipBoundsCheck
)
{
    ClipperLib::IntPoint p1 = ToC1Point(p1_p64), p2 = ToC1Point(p2_p64);
    Perf_IsAllowedToCutTrough.Start();

    if (!skipBoundsCheck && !IsPointWithinCutRegion(toolBoundPaths_p64, ToPoint64(p2))) {
        // last point outside boundary - its not clear to cut
        Perf_IsAllowedToCutTrough.Stop();
        return false;
    }
    else if (!skipBoundsCheck && !IsPointWithinCutRegion(toolBoundPaths_p64, ToPoint64(p1))) {
        // first point outside boundary - its not clear to cut
        Perf_IsAllowedToCutTrough.Stop();
        return false;
    }
    else {
        ClipperLib::Clipper clip;
        double distance = sqrt(DistanceSqrd(ToPoint64(p1), ToPoint64(p2)));
        double stepSize = min(0.5 * stepOverScaled, 8 * MIN_STEP_CLIPPER);
        if (distance < stepSize / 2) {  // not significant cut
            Perf_IsAllowedToCutTrough.Stop();
            return true;
        }
        if (distance < stepSize) {  // adjust for numeric instability with small distances
            areaFactor *= 2;
        }

        ClipperLib::IntPoint toolPos1 = p1;
        long steps = long(distance / stepSize) + 1;
        stepSize = distance / steps;
        for (long i = 1; i <= steps; i++) {
            double p = double(i) / steps;
            ClipperLib::IntPoint toolPos2(
                long(p1.X + double(p2.X - p1.X) * p),
                long(p1.Y + double(p2.Y - p1.Y) * p)
            );
            double area = CalcCutArea(ToPoint64(toolPos1), ToPoint64(toolPos2), cleared).first;
            // if we are cutting above optimal -> not clear to cut
            if (area > areaFactor * stepSize * optimalCutAreaPD) {
                Perf_IsAllowedToCutTrough.Stop();
                return false;
            }
            // if tool is outside boundary -> its not clear to cut
            if (!skipBoundsCheck && !IsPointWithinCutRegion(toolBoundPaths_p64, ToPoint64(toolPos2))) {
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
    const Point64& startPoint_p64,
    const Point64& endPoint_p64,
    ClearedArea& clearedArea,
    Path64& output_p64
)
{
    ClipperLib::IntPoint startPoint = ToC1Point(startPoint_p64);
    ClipperLib::IntPoint endPoint = ToC1Point(endPoint_p64);
    ClipperLib::Path output;
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

    vector<pair<ClipperLib::IntPoint, ClipperLib::IntPoint>> queue;
    queue.emplace_back(startPoint, endPoint);
    ClipperLib::Path checkPath;
    double totalLength = 0;
    double directDistance = sqrt(DistanceSqrd(ToPoint64(startPoint), ToPoint64(endPoint)));
    ClipperLib::Paths linkPaths;

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
    Point64 clp;
    Point64 clpDist_p64;
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
        pair<ClipperLib::IntPoint, ClipperLib::IntPoint> pointPair = queue.back();
        queue.pop_back();

        // check for self intersections - if found discard the link path
        for (size_t i = 0; i < linkPaths.size(); i++) {
            if (linkPaths[i].front() != pointPair.first && linkPaths[i].back() != pointPair.first
                && linkPaths[i].front() != pointPair.second && linkPaths[i].back() != pointPair.second
                && IntersectionPoint(
                    ToPoint64(linkPaths[i].front()),
                    ToPoint64(linkPaths[i].back()),
                    ToPoint64(pointPair.first),
                    ToPoint64(pointPair.second),
                    clp
                )) {
                cout << "Unable to resolve tool down linking path (self-intersects)." << endl;
                return false;
            }
        }

        ClipperLib::DoublePoint direction = DirectionV(pointPair.first, pointPair.second);
        checkPath.clear();
        if (pointPair.first == startPoint) {
            checkPath.push_back(ClipperLib::IntPoint(
                pointPair.first.X + offClearance * direction.X,
                pointPair.first.Y + offClearance * direction.Y
            ));
        }
        else {
            checkPath.push_back(pointPair.first);
        }
        if (pointPair.second == endPoint) {
            checkPath.push_back(ClipperLib::IntPoint(
                pointPair.second.X - offClearance * direction.X,
                pointPair.second.Y - offClearance * direction.Y
            ));
        }
        else {
            checkPath.push_back(pointPair.second);
        }

        if (IsClearPath(ToPath64(checkPath), clearedArea, clearance)) {
            totalLength += sqrt(DistanceSqrd(ToPoint64(pointPair.first), ToPoint64(pointPair.second)));
            if (totalLength > keepToolDownDistRatio * directDistance) {
                return false;
            }
            ClipperLib::Path link;
            link.push_back(pointPair.first);
            link.push_back(pointPair.second);
            linkPaths.push_back(link);
        }
        else {
            if (sqrt(DistanceSqrd(ToPoint64(pointPair.first), ToPoint64(pointPair.second))) < 4) {
                // segment became too short but still not clear
                return false;
            }
            ClipperLib::DoublePoint pDir(-direction.Y, direction.X);
            // find mid point
            ClipperLib::IntPoint midPoint(
                0.5 * double(pointPair.first.X + pointPair.second.X),
                0.5 * double(pointPair.first.Y + pointPair.second.Y)
            );
            for (long i = 1;; i++) {
                if (stopProcessing) {
                    return false;
                }
                double offset = i * scanStep;
                ClipperLib::IntPoint checkPoint1(midPoint.X + offset * pDir.X, midPoint.Y + offset * pDir.Y);
                ClipperLib::IntPoint checkPoint2(midPoint.X - offset * pDir.X, midPoint.Y - offset * pDir.Y);

                if (
                    DistancePointToPathsSqrd(clearedArea.GetCleared(), ToPoint64(checkPoint1), clpDist_p64, pindex, sindex, par)
                    < DistancePointToPathsSqrd(clearedArea.GetCleared(), ToPoint64(checkPoint2), clpDist_p64, pindex, sindex, par)
                ) {
                    // exchange points
                    ClipperLib::IntPoint tmp = checkPoint2;
                    checkPoint2 = checkPoint1;
                    checkPoint1 = tmp;
                }

                checkPath.clear();
                checkPath.push_back(checkPoint1);
                if (IsClearPath(ToPath64(checkPath), clearedArea, clearance + 1)) {  // check if point clear
                    queue.emplace_back(pointPair.first, checkPoint1);
                    queue.emplace_back(checkPoint1, pointPair.second);
                    break;
                }
                else {  // check the other side

                    checkPath.clear();
                    checkPath.push_back(checkPoint2);
                    if (IsClearPath(ToPath64(checkPath), clearedArea, clearance + 1)) {
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
    Paths64 lp = ToPaths64(linkPaths);
    ConnectPaths(lp, lp);
    linkPaths = ToC1Paths(lp);
    output = linkPaths[0];
    output_p64 = ToPath64(output);
    return true;
}

bool Adaptive2d::MakeLeadPath(
    bool leadIn,
    const Point64& startPoint_p64,
    const PointD& startDir_p64,
    Point64 beaconPoint_p64,
    ClearedArea& clearedAreaOriginal,
    const Paths64& toolBoundPaths_p64,
    Path64& output_p64,
    AdaptiveOutput& adaptiveOutput
)
{
    ClipperLib::IntPoint startPoint = ToC1Point(startPoint_p64);
    ClipperLib::DoublePoint startDir = ToC1PointD(startDir_p64);
    ClipperLib::IntPoint beaconPoint = ToC1Point(beaconPoint_p64);
    ClipperLib::Paths toolBoundPaths = ToC1Paths(toolBoundPaths_p64);
    ClipperLib::Path output;
    output.push_back(startPoint);
    double stepSize = min(MIN_STEP_CLIPPER * 8, 0.2 * stepOverScaled + 1);

    // make a copy of clearedArea to update as the path progresses (for lead out only)
    ClearedArea clearedArea(toolRadiusScaled);
    clearedArea.SetClearedPaths(clearedAreaOriginal.GetCleared());

    // compute acceptable tool end locations
    ClipperLib::ClipperOffset clipof;
    clipof.Clear();
    ClipperLib::Paths cleared;
    clipof.AddPaths(ToC1Paths(clearedArea.GetCleared()), ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
    clipof.Execute(cleared, -(toolRadiusScaled + stepSize));

    // move the beacon to an acceptable location if necessary
    if (cleared.size() == 0) {
        return false;
    }

    if (getPathNestingLevel(Path64{ToPoint64(beaconPoint)}, ToPaths64(cleared)) % 2 == 0) {
        Point64 clp;  // to store closest point
        size_t clpPathIndex;
        size_t clpSegmentIndex;
        double clpParameter;
        DistancePointToPathsSqrd(ToPaths64(cleared), ToPoint64(beaconPoint), clp, clpPathIndex, clpSegmentIndex, clpParameter);
        beaconPoint = ToC1Point(clp);
    }

    ClipperLib::IntPoint currentPoint = startPoint;
    ClipperLib::DoublePoint targetDir = DirectionV(currentPoint, beaconPoint);

    double distanceToBeacon = sqrt(DistanceSqrd(ToPoint64(startPoint), ToPoint64(beaconPoint)));
    double minExitLength = min(toolRadiusScaled / 5., min(stepOverScaled, distanceToBeacon / 2));
    double maxLength = max(distanceToBeacon * 2, stepSize * 10);
    std::optional<double> clearedStartLen;
    ClipperLib::DoublePoint nextDir = startDir;
    ClipperLib::IntPoint nextPoint
        = ClipperLib::IntPoint(currentPoint.X + nextDir.X * stepSize, currentPoint.Y + nextDir.Y * stepSize);
    ClipperLib::Path checkPath;
    double adaptFactor = 0.4;
    double alfa = std::numbers::pi / 64;
    double pathLen = 0;
    checkPath.push_back(currentPoint);
    for (int i = 0; i < 10000; i++) {
        if (IsAllowedToCutTrough(ToPoint64(currentPoint), ToPoint64(nextPoint), clearedArea, toolBoundPaths_p64)) {
            if (!leadIn) {
                // For lead out paths, update/recompute the cleared area
                checkPath.push_back(nextPoint);
                clearedArea.ExpandCleared(ToPath64(checkPath));
                checkPath.clear();
                checkPath.push_back(nextPoint);

                clipof.Clear();
                clipof.AddPaths(ToC1Paths(clearedArea.GetCleared()), ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
                clipof.Execute(cleared, -(toolRadiusScaled + stepSize));
            }

            output.push_back(nextPoint);
            currentPoint = nextPoint;
            pathLen += stepSize;
            targetDir = DirectionV(currentPoint, beaconPoint);
            nextDir = ClipperLib::DoublePoint(
                nextDir.X + adaptFactor * targetDir.X,
                nextDir.Y + adaptFactor * targetDir.Y
            );
            NormalizeV(nextDir);

            // check if cleared
            if (getPathNestingLevel(Path64{ToPoint64(currentPoint)}, ToPaths64(cleared)) % 2 == 1) {
                if (!clearedStartLen) {
                    clearedStartLen = {pathLen};
                }

                // if the path is long enough, exit with success
                if (pathLen > minExitLength && pathLen - *clearedStartLen > MIN_STEP_CLIPPER) {
                    output_p64 = ToPath64(output);
                    return true;
                }
            }
            else {
                clearedStartLen = {};
            }

            // if traveled too far without getting to a clear area, exit with failure
            if (pathLen > maxLength) {
                if (getPathNestingLevel(Path64{ToPoint64(currentPoint)}, clearedArea.GetCleared()) % 2 == 1) {
                    output_p64 = ToPath64(output);
                    return true;
                }
                else {
                    adaptiveOutput.LeadPathFailed = true;
                    cerr << "MakeLeadPath failed: overtravel without getting to cleared area" << endl;
                    return false;
                }
            }
        }
        else {
            nextDir = rotate(nextDir, leadIn ? -alfa : alfa);
        }
        nextPoint
            = ClipperLib::IntPoint(currentPoint.X + nextDir.X * stepSize, currentPoint.Y + nextDir.Y * stepSize);
    }

    return false;
}

std::optional<TPaths> Adaptive2d::FindLinkPath(
    const std::optional<Point64>& prevPoint_p64,
    const Point64& pathStart_p64,
    const PointD& pathDir_p64,
    ClearedArea& cleared,
    const Paths64& toolBoundPaths_p64,
    AdaptiveOutput& adaptiveOutput
)
{
    Perf_AppendToolPath.Start();
    std::optional<ClipperLib::IntPoint> prevPoint = prevPoint_p64 ? std::optional(ToC1Point(*prevPoint_p64)) : std::nullopt;
    ClipperLib::IntPoint pathStart = ToC1Point(pathStart_p64);
    ClipperLib::DoublePoint pathDir = ToC1PointD(pathDir_p64);
    ClipperLib::Paths toolBoundPaths = ToC1Paths(toolBoundPaths_p64);
    TPaths result;

    ClipperLib::IntPoint endPoint(pathStart);

    // if the link distance is very short, no special linking is required
    double linkDistance = prevPoint ? sqrt(DistanceSqrd(ToPoint64(*prevPoint), ToPoint64(endPoint))) : stepOverScaled;
    if (linkDistance >= NTOL) {
        size_t clpPathIndex;
        size_t clpSegmentIndex;
        double clpParameter;
        Point64 clp;

        double beaconOffset = max(min(stepOverScaled, linkDistance / 2) * 1.5, 8 * MIN_STEP_CLIPPER);

        // plan the lead in, as a reverse-direction lead out
        double eDistToBounds = DistancePointToPathsSqrd(
            toolBoundPaths_p64,
            ToPoint64(endPoint),
            clp,
            clpPathIndex,
            clpSegmentIndex,
            clpParameter
        );

        ClipperLib::DoublePoint revEndDir = {-pathDir.X, -pathDir.Y};

        ClipperLib::DoublePoint endBoundaryDir = GetPathDirectionV(toolBoundPaths_p64[clpPathIndex], clpSegmentIndex);
        if (eDistToBounds > beaconOffset) {
            endBoundaryDir = pathDir;  // if boundary is far away, use beacon to leave the path
        }
        ClipperLib::DoublePoint endBeaconDir = {revEndDir.X - endBoundaryDir.Y, revEndDir.Y + endBoundaryDir.X};
        NormalizeV(endBeaconDir);

        ClipperLib::IntPoint endBeacon(
            endPoint.X + beaconOffset * endBeaconDir.X,
            endPoint.Y + beaconOffset * endBeaconDir.Y
        );

        Path64 leadInPath_p64;
        bool leadInOk = MakeLeadPath(
            true,
            ToPoint64(endPoint),
            ToPointD(revEndDir),
            ToPoint64(endBeacon),
            cleared,
            toolBoundPaths_p64,
            leadInPath_p64,
            adaptiveOutput
        );
        ClipperLib::Path leadInPath = ToC1Path(leadInPath_p64);
        ReversePath(leadInPath);

        if (!leadInOk) {
            Perf_AppendToolPath.Stop();
            return {};
        }

        // Compute linking path
        ClipperLib::Path linkPath;
        MotionType linkType = MotionType::mtCutting;

        if (prevPoint) {
            Path64 linkPath_p64;
            if (ResolveLinkPath(ToPoint64(*prevPoint), ToPoint64(leadInPath.front()), cleared, linkPath_p64)) {
                linkPath = ToC1Path(linkPath_p64);
                linkType = MotionType::mtLinkClear;
                double remainingLeadInExtension = stepOverScaled / 2;
                while (linkPath.size() >= 2 && remainingLeadInExtension > NTOL) {
                    ClipperLib::IntPoint p1 = linkPath.at(linkPath.size() - 2);
                    ClipperLib::IntPoint p2 = linkPath.at(linkPath.size() - 1);
                    double l = sqrt(DistanceSqrd(ToPoint64(p1), ToPoint64(p2)));
                    if (l >= remainingLeadInExtension) {
                        ClipperLib::IntPoint splitPoint(
                            p1.X + (p2.X - p1.X) * (l - remainingLeadInExtension) / l,
                            p1.Y + (p2.Y - p1.Y) * (l - remainingLeadInExtension) / l
                        );
                        linkPath.pop_back();
                        linkPath.push_back(splitPoint);
                        leadInPath.insert(leadInPath.begin(), splitPoint);
                        remainingLeadInExtension = 0;
                        ClipperLib::Path checkPath;
                        checkPath.push_back(p2);
                        checkPath.push_back(splitPoint);
                        if (!IsClearPath(ToPath64(checkPath), cleared, 0)) {
                            remainingLeadInExtension = stepOverScaled / 2;
                        }
                    }
                    else {
                        linkPath.pop_back();
                        leadInPath.insert(leadInPath.begin(), p1);
                        remainingLeadInExtension -= l;
                        if (remainingLeadInExtension < NTOL) {
                            ClipperLib::Path checkPath;
                            checkPath.push_back(p2);
                            checkPath.push_back(p1);
                            if (!IsClearPath(ToPath64(checkPath), cleared, 0)) {
                                remainingLeadInExtension = stepOverScaled / 2;
                            }
                        }
                    }
                }
            }
            else {
                linkType = MotionType::mtLinkNotClear;
                double dist = sqrt(DistanceSqrd(ToPoint64(*prevPoint), ToPoint64(leadInPath.front())));
                if (dist < 2 * stepOverScaled
                    && IsAllowedToCutTrough(
                        Point64(
                            prevPoint->X + (leadInPath.front().X - prevPoint->X) / dist,
                            prevPoint->Y + (leadInPath.front().Y - prevPoint->Y) / dist
                        ),
                        Point64(
                            leadInPath.front().X - (leadInPath.front().X - prevPoint->X) / dist,
                            leadInPath.front().Y - (leadInPath.front().Y - prevPoint->Y) / dist
                        ),
                        cleared,
                        toolBoundPaths_p64
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
        ClipperLib::Paths linkPaths;
        linkPaths.push_back(linkPath);
        linkPaths.push_back(leadInPath);

        if (linkType == MotionType::mtLinkClear) {
            Paths64 linkPaths_p64 = ToPaths64(linkPaths);
            SmoothPaths(linkPaths_p64, 0.1 * stepOverScaled, 1, 4);
            linkPaths = ToC1Paths(linkPaths_p64);
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

std::optional<std::pair<Point64, PointD>> Adaptive2d::AppendToolPath(
    AdaptiveOutput& output,
    const Path64& passToolPath_p64,
    TPaths& linkPath,
    ClearedArea& cleared,
    const Paths64& toolBoundPaths_p64
)
{
    std::optional<std::pair<Point64, PointD>> result;  // new toolPos and toolDir after lead out
    ClipperLib::Path passToolPath = ToC1Path(passToolPath_p64);
    ClipperLib::Paths toolBoundPaths = ToC1Paths(toolBoundPaths_p64);

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
            ClipperLib::IntPoint prevPoint = passToolPath.back();
            ClipperLib::DoublePoint prevDir = GetPathDirectionV(passToolPath_p64, passToolPath_p64.size() - 1);

            size_t clpPathIndex;
            size_t clpSegmentIndex;
            double clpParameter;
            Point64 clp;
            double distToBounds = DistancePointToPathsSqrd(
                toolBoundPaths_p64,
                ToPoint64(prevPoint),
                clp,
                clpPathIndex,
                clpSegmentIndex,
                clpParameter
            );

            ClipperLib::DoublePoint boundaryDir = GetPathDirectionV(toolBoundPaths_p64[clpPathIndex], clpSegmentIndex);
            double beaconOffset
                = max(min(stepOverScaled, PathLength(passToolPath_p64) / 2) * 1.5, 8 * MIN_STEP_CLIPPER);
            if (distToBounds > beaconOffset) {
                boundaryDir = prevDir;  // if boundary is far away, use beacon to leave the path
            }
            ClipperLib::DoublePoint beaconDir = {prevDir.X - boundaryDir.Y, prevDir.Y + boundaryDir.X};
            NormalizeV(beaconDir);

            ClipperLib::IntPoint beacon(
                prevPoint.X + beaconOffset * beaconDir.X,
                prevPoint.Y + beaconOffset * beaconDir.Y
            );

            Path64 leadOutPath_p64;
            bool ok = MakeLeadPath(
                false,
                ToPoint64(prevPoint),
                ToPointD(prevDir),
                ToPoint64(beacon),
                cleared,
                toolBoundPaths_p64,
                leadOutPath_p64,
                output
            );
            ClipperLib::Path leadOutPath = ToC1Path(leadOutPath_p64);

            if (ok && leadOutPath.size() >= 1) {
                // smooth path
                Paths64 linkPaths_p64;
                linkPaths_p64.push_back(ToPath64(leadOutPath));
                SmoothPaths(linkPaths_p64, 0.1 * stepOverScaled, 1, 4);
                leadOutPath = ToC1Path(linkPaths_p64[0]);

                // scale and output
                TPath out;
                out.first = MotionType::mtCutting;
                for (auto& p : leadOutPath) {
                    out.second.push_back({((double)p.X) / scaleFactor, ((double)p.Y) / scaleFactor});
                }
                output.AdaptivePaths.push_back(out);
                cleared.ExpandCleared(ToPath64(leadOutPath));

                ClipperLib::IntPoint p2 = leadOutPath.back();
                ClipperLib::IntPoint p1 = leadOutPath.size() >= 2 ? leadOutPath[leadOutPath.size() - 2]
                                                      : prevPoint;
                ClipperLib::DoublePoint dir = DirectionV(p1, p2);

                result = {{ToPoint64(p2), ToPointD(dir)}};
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

void Adaptive2d::AddPathsToProgress(TPaths& progressPaths, Paths64 paths_p64, MotionType mt)
{
    ClipperLib::Paths paths = ToC1Paths(paths_p64);
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

// performs the intersection of the path (subject) and the area (obj), preserving
// orientation and (closed-path) connectivity
Paths64 PathIntersectArea(ClipperLib::Clipper& clip, Path64& subject_p64, Paths64& obj_p64, bool isClosed = true)
{
    ClipperLib::Path subject = ToC1Path(subject_p64);
    ClipperLib::Paths obj = ToC1Paths(obj_p64);
    if (isClosed) {
        subject.push_back(subject[0]);  // close path explicitly before treating it as open
    }

    // init z-data: p[i].z = 2 * i + 1, and new points are the average of their neighbors
    // this ensures new points have unique z but come between the points they're made from
    for (size_t i = 0; i < subject.size(); i++) {
        subject[i].Z = i * 2 + 1;
    }
    for (ClipperLib::Path& path : obj) {
        for (ClipperLib::IntPoint& p : path) {
            p.Z = 0;
        }
    }
    auto zfill = [](ClipperLib::IntPoint& e1b, ClipperLib::IntPoint& e1t, ClipperLib::IntPoint& e2b, ClipperLib::IntPoint& e2t, ClipperLib::IntPoint& p) {
        if (e1b.Z != 0 && e1t.Z != 0) {
            p.Z = (e1b.Z + e1t.Z) / 2;
        }
        else if (e2b.Z != 0 && e2t.Z != 0) {
            p.Z = (e2b.Z + e2t.Z) / 2;
        }
    };
    clip.ZFillFunction(zfill);

    ClipperLib::PolyTree diffTree;
    ClipperLib::Paths diff;
    clip.Clear();
    clip.AddPath(subject, ClipperLib::PolyType::ptSubject, false);
    clip.AddPaths(obj, ClipperLib::PolyType::ptClip, true);
    clip.Execute(ClipperLib::ClipType::ctIntersection, diffTree);
    clip.ZFillFunction(0);
    OpenPathsFromPolyTree(diffTree, diff);

    // restore orientation
    for (ClipperLib::Path& p : diff) {
        for (size_t i = 0; i < p.size() - 1; i++) {
            if (p[i].Z != 0 && p[i + 1].Z != 0) {
                if (p[i].Z + 1 != p[i + 1].Z && p[i].Z + 2 != p[i + 1].Z) {
                    ReversePath(p);
                }

                break;
            }
        }
    }

    // collect result, joining any path that goes through the end point
    const int zstart = 1;
    const int zend = subject.size() * 2 - 1;
    std::optional<ClipperLib::Path> start, end;
    ClipperLib::Paths result;
    for (ClipperLib::Path& p : diff) {
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
        ClipperLib::Path joined = *end;
        // append points from start, skipping the first, which is a repeat
        for (size_t i = 1; i < start->size(); i++) {
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

    subject_p64 = ToPath64(subject);
    obj_p64 = ToPaths64(obj);
    return ToPaths64(result);
}

struct IterateNextStepOutput
{
    std::optional<double> iterationAngle;
    bool tooManyIterations = false;
    bool failed;
    double area;
    double errorFraction;
    Point64 newToolPos;
    PointD newToolDir;
};

void Adaptive2d::ProcessPolyNode(
    Paths64 boundPaths_p64,
    Paths64 toolBoundPaths_p64,
    Paths64 finishingPaths_p64,
    const Paths64& initialClearedPaths_p64,
    DebugSVGInfo& svgInfo
)
{
    Perf_ProcessPolyNode.Start();
    current_region++;
    cout << "** Processing region: " << current_region << endl;

    // node paths are already constrained to tool boundary path for adaptive path before finishing
    // pass
    Point64 entryPoint;
    long helixRadiusScaled;
    TPaths progressPaths;
    progressPaths.reserve(10000);

    toolBoundPaths_p64 = SimplifyPaths(toolBoundPaths_p64, CLEAN_POLYGONS_TOLERANCE);
    toolBoundPaths_p64 = Union(toolBoundPaths_p64, FillRule::EvenOdd);

    boundPaths_p64 = SimplifyPaths(boundPaths_p64, CLEAN_POLYGONS_TOLERANCE);
    boundPaths_p64 = Union(boundPaths_p64, FillRule::EvenOdd);

    Paths64 tbpMinus = OffsetPaths(toolBoundPaths_p64, -2);  // toolBoundPaths shrunk by some buffer room
    tbpMinus = SimplifyPaths(tbpMinus, CLEAN_POLYGONS_TOLERANCE);
    tbpMinus = Union(tbpMinus, FillRule::EvenOdd);

    AddPathsToProgress(progressPaths, toolBoundPaths_p64, MotionType::mtLinkClear);

    Point64 toolPos;
    PointD toolDir;

    // Initialize cleared area from previously cleared paths
    ClearedArea cleared(toolRadiusScaled);
    cleared.SetClearedPaths(initialClearedPaths_p64);

    long stepScaled = long(MIN_STEP_CLIPPER);

    CheckReportProgress(progressPaths, true);

    Path64 passToolPath_p64;  // to store pass toolpath
    Path64 toClearPath_p64;
    Point64 clp_p64;  // to store closest point
    size_t clpPathIndex;
    size_t clpSegmentIndex;
    double clpParameter;
    vector<PointD> gyro;     // used to average tool direction
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

    AdaptiveOutput output;
    output.clipperScale = double(scaleFactor);
#ifdef DEV_MODE
    clock_t start_clock = clock();
#endif
    ClearedArea clearedBeforePass(toolRadiusScaled);
    clearedBeforePass.SetClearedPaths(cleared.GetCleared());

    PointD lastExpandToolDir_p64 = toolDir;

    //======================================================
    // DO NOT DELETE THIS COMMENT
    // Clipper2 translation in progress: variables above are Clipper2 types; this block
    // converts them back to Clipper1 so that untranslated code below can use them.
    // When moving this block down, check each item — if the C1 variable is no longer
    // used below the new location, it can be deleted.
    ClipperLib::Clipper clip;
    ClipperLib::ClipperOffset clipof;
    ClipperLib::Paths finishingPaths = ToC1Paths(finishingPaths_p64);
    ClipperLib::Paths initialClearedPaths = ToC1Paths(initialClearedPaths_p64);

    //======================================================

    const auto iterateNextStep = [&](const Point64& toolPos_p64,
                                     const PointD& toolDir_p64,
                                     bool warnRotate) {
        const ClipperLib::IntPoint toolPos = ToC1Point(toolPos_p64);
        const ClipperLib::DoublePoint toolDir = ToC1PointD(toolDir_p64);
        IterateNextStepOutput out;
        out.tooManyIterations = false;
        out.failed = false;

        Perf_DistanceToBoundary.Start();

        double distanceToBoundary = sqrt(
            DistancePointToPathsSqrd(toolBoundPaths_p64, ToPoint64(toolPos), clp_p64, clpPathIndex, clpSegmentIndex, clpParameter)
        );
        ClipperLib::DoublePoint boundaryDir = GetPathDirectionV(toolBoundPaths_p64[clpPathIndex], clpSegmentIndex);

        Perf_DistanceToBoundary.Stop();
        double distanceToEngage = sqrt(DistanceSqrd(ToPoint64(toolPos), entryPoint));

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

        //*****************************
        // ANGLE vs AREA ITERATIONS
        //*****************************
        double predictedAngle = averageDV(angleHistory);
        double maxError = AREA_ERROR_FACTOR * optimalCutAreaPD;
        double errorFraction = 1;
        double area = 0;
        bool isConventional = false;
        const double conventionalCutoff = 0.51;  // allow some room for rounding, but otherwise < 50%
        double areaPD = 0;
        interp.clear();
        /******************************/
        Perf_PointIterations.Start();
        int iteration;
        bool pointNotInterp;
        bool foundArea = false;
        ClipperLib::IntPoint newToolPos;
        ClipperLib::DoublePoint newToolDir;
        for (iteration = 0; iteration < MAX_ITERATIONS; iteration++) {
            total_iterations++;
            if (iteration == 0) {
                angle = predictedAngle;
                pointNotInterp = true;
            }
            else if (iteration == 1) {
                angle = interp.MIN_ANGLE;  // max engage
                pointNotInterp = true;
            }
            else if (iteration == 2) {
                if (interp.bothSides()) {
                    angle = interp.interpolateAngle();
                    pointNotInterp = false;
                }
                else {
                    angle = interp.MAX_ANGLE;  // min engage
                    pointNotInterp = true;
                }
            }
            else if (iteration == 3 && !foundArea) {
                // TODO FIXME David, review before PR merge -- is this expansion really needed? Can it at least only be run rarely?
                // Expand cleared area
                cleared.ExpandCleared(toClearPath_p64);
                toClearPath_p64.clear();
                lastExpandToolDir_p64 = ToPointD(toolDir);

                // Find nearby uncleared area in the forward direction
                ClipperLib::Paths clearedArea = ToC1Paths(cleared.GetCleared());

                // 1.5 > sqrt(2) for the constructed triangle to contain possible steps
                double dist = (stepScaled + toolRadiusScaled) * 1.5;
                ClipperLib::Path triangle = {toolPos};
                ClipperLib::DoublePoint leftAngle = rotate(toolDir, -std::numbers::pi / 4);
                ClipperLib::DoublePoint rightAngle = rotate(toolDir, std::numbers::pi / 4);
                triangle.push_back(
                    {(long long)(toolPos.X + rightAngle.X * dist),
                     (long long)(toolPos.Y + rightAngle.Y * dist)}
                );
                triangle.push_back(
                    {(long long)(toolPos.X + leftAngle.X * dist),
                     (long long)(toolPos.Y + leftAngle.Y * dist)}
                );

                clip.Clear();
                clip.AddPath(triangle, ClipperLib::PolyType::ptSubject, true);
                clip.AddPaths(clearedArea, ClipperLib::PolyType::ptClip, true);
                clip.Execute(ClipperLib::ClipType::ctDifference, clearedArea);

                if (clearedArea.size() == 0) {
                    continue;
                }

                // Find the closest point on the boundary, and try stepping towards it
                DistancePointToPathsSqrd(
                    ToPaths64(clearedArea),
                    ToPoint64(toolPos),
                    clp_p64,
                    clpPathIndex,
                    clpSegmentIndex,
                    clpParameter
                );
                double dy = clp_p64.y - toolPos.Y;
                double dx = clp_p64.x - toolPos.X;
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
                pointNotInterp = false;
            }
            angle = interp.clampAngle(angle);

            newToolDir = rotate(toolDir, angle);
            newToolPos = ClipperLib::IntPoint(
                long(toolPos.X + newToolDir.X * stepScaled),
                long(toolPos.Y + newToolDir.Y * stepScaled)
            );

            // Skip iteration if this ClipperLib::IntPoint has already been processed
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
                continue;
            }

            const auto caRet = CalcCutArea(ToPoint64(toolPos), ToPoint64(newToolPos), cleared);
            area = std::get<0>(caRet);
            double conventionalArea = std::get<1>(caRet);
            double fractionConventional = (area == 0) ? 0 : conventionalArea / area;
            isConventional = fractionConventional >= conventionalCutoff;
            if (area > 0) {
                foundArea = true;
            }

            areaPD = area / double(stepScaled);  // area per distance
            double error = areaPD - targetAreaPD;
            errorFraction = abs(error / optimalCutAreaPD);
            interp.addPoint(error, {angle, newToolPos}, pointNotInterp, isConventional);
            if (fabs(error) < maxError && !isConventional) {
                out.iterationAngle = angle;
                break;
            }
            if (iteration == MAX_ITERATIONS - 1) {
                out.tooManyIterations = true;
            }
        }
        Perf_PointIterations.Stop();

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
            while (!IsPointWithinCutRegion(toolBoundPaths_p64, ToPoint64(newToolPos)) && rotateStep < 180) {
                rotateStep++;
                // if new tool pos. outside boundary rotate until back in
                recalcArea = true;
                newToolDir = rotate(newToolDir, rotateIncrement);
                newToolPos = ClipperLib::IntPoint(
                    long(toolPos.X + newToolDir.X * stepScaled),
                    long(toolPos.Y + newToolDir.Y * stepScaled)
                );
            }
            if (rotateStep >= 180) {
#ifdef DEV_MODE
                if (warnRotate) {
                    output.UnexpectedRotateIterations = true;
                    cerr << "Warning: unexpected number of rotate iterations." << endl;
                }
#else
                UNUSED(warnRotate);
#endif
                out.failed = true;
            }

            if (recalcArea) {
                const auto caRet = CalcCutArea(ToPoint64(toolPos), ToPoint64(newToolPos), cleared);
                area = std::get<0>(caRet);
                areaPD = area / double(stepScaled);  // area per distance
                double error = areaPD - targetAreaPD;
                errorFraction = abs(error / optimalCutAreaPD);

                double conventionalArea = std::get<1>(caRet);
                double fractionConventional = area == 0 ? 0 : conventionalArea / area;
                isConventional = fractionConventional >= conventionalCutoff;
            }

            // safety condition
            if (area > stepScaled * optimalCutAreaPD && areaPD > 2 * optimalCutAreaPD) {
                over_cut_count++;
                out.failed = true;
            }
        }

        out.area = area;
        out.failed |= isConventional;
        out.failed |= area < 1;
        out.newToolPos = ToPoint64(newToolPos);
        out.newToolDir = ToPointD(newToolDir);
        out.errorFraction = errorFraction;
        return out;
    };

    const auto initToolDir = [&](const Point64& toolPos_p64, const PointD& baseDir_p64) {
        const ClipperLib::IntPoint toolPos = ToC1Point(toolPos_p64);
        const ClipperLib::DoublePoint baseDir = ToC1PointD(baseDir_p64);
        ClipperLib::DoublePoint testDirs[] = {
            {baseDir.X, baseDir.Y},
            {-baseDir.Y, baseDir.X},
            {-baseDir.X, -baseDir.Y},
            {baseDir.Y, -baseDir.X}
        };
        std::optional<std::pair<PointD, double>> bestDir;
        bool allZero = true;
        for (const auto& testDir : testDirs) {
            const auto itResult = iterateNextStep(ToPoint64(toolPos), ToPointD(testDir), false);
            if (itResult.area != 0) {
                allZero = false;
            }
            if (!itResult.failed) {
                if (!bestDir || itResult.errorFraction < bestDir->second) {
                    bestDir = {itResult.newToolDir, itResult.errorFraction};
                }
            }
        }

        if (bestDir) {
            return std::optional<PointD> {bestDir->first};
        }
        else if (allZero) {
            ClipperLib::Paths clearedArea = ToC1Paths(cleared.GetCleared());
            if (DistancePointToPathsSqrd(ToPaths64(clearedArea), ToPoint64(toolPos), clp_p64, clpPathIndex, clpSegmentIndex, clpParameter)
                < toolRadiusScaled * toolRadiusScaled) {
                ClipperLib::IntPoint p2 = Compute2DPolygonCentroid(ToPath64(clearedArea[clpPathIndex]));
                ClipperLib::DoublePoint dir = DirectionV(toolPos, p2);
                return std::optional<PointD> {ToPointD(dir)};
            }
            return std::optional<PointD> {};
        }
        else {
            return std::optional<PointD> {};
        }
    };

    const auto _getEngagePoint = [&](const std::optional<Point64>& prevPos_p64,
                                     long engagementProtrusion) {
        const std::optional<ClipperLib::IntPoint> prevPos = prevPos_p64 ? std::optional(ToC1Point(*prevPos_p64)) : std::nullopt;
        // engagePoint, engageDir, heuristicCost
        std::vector<std::tuple<ClipperLib::IntPoint, PointD, double>> engagePoints;

        const auto addEngagePoint = [&](const Point64& engagePoint_p64, const PointD& engageDir_p64) {
            const ClipperLib::IntPoint engagePoint = ToC1Point(engagePoint_p64);
            double cost_mm = (prevPos ? sqrt(DistanceSqrd(ToPoint64(*prevPos), ToPoint64(engagePoint))) / scaleFactor : 0);
            engagePoints.emplace_back(
                std::tuple<ClipperLib::IntPoint, PointD, double> {engagePoint, engageDir_p64, cost_mm}
            );
        };

        ClipperLib::Paths clearedArea = ToC1Paths(cleared.GetCleared());

        // offset inward to find places the tool can start
        long engageBuffer = 2;  // smooths out the integer rounding in the two offsets, +toolRadius
                                // and -toolRadius
        ClipperLib::Paths preEngage;
        clipof.Clear();
        clipof.AddPaths(clearedArea, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
        clipof.Execute(preEngage, -(toolRadiusScaled + engageBuffer));

        // offset outward to find places that would protrude outside the cleared area
        ClipperLib::Paths engagePaths;
        clipof.Clear();
        clipof.AddPaths(preEngage, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
        clipof.Execute(engagePaths, engageBuffer + engagementProtrusion);

        // clip engage candidates with tool bounds
        for (ClipperLib::Path& engagePath : engagePaths) {
            // rotate the closed path so it starts with the closest point
            // this is useful because if the path does not get clipped, any point on
            // the path is a valid start location (not just the first) but we want to
            // test against the closest one
            ClipperLib::Path rotated;
            if (!prevPos) {
                rotated = engagePath;
            }
            else {
                int iClosest = 0;
                double dsqClosest = __DBL_MAX__;
                for (size_t i = 0; i < engagePath.size(); i++) {
                    double dsq = DistanceSqrd(ToPoint64(*prevPos), ToPoint64(engagePath[i]));
                    if (dsq < dsqClosest) {
                        dsqClosest = dsq;
                        iClosest = i;
                    }
                }

                for (size_t i = 0; i < engagePath.size(); i++) {
                    rotated.push_back(engagePath[(i + iClosest) % engagePath.size()]);
                }
            }

            // clip with tbpMinus instead of toolBoundPaths to ensure that all
            // resulting points are inside toolBoundPaths, and not rounded to an
            // integer coordinate outside of it
            Path64 rotated_p64 = ToPath64(rotated);
            Paths64 tbpMinus_p64 = tbpMinus;
            ClipperLib::Paths openPaths = ToC1Paths(PathIntersectArea(clip, rotated_p64, tbpMinus_p64));

            for (ClipperLib::Path& open : openPaths) {
                bool added = false;
                double dToGo = 0;  // first step is 0 -- start point
                size_t seg = 0;
                double segD = 0;
                while (!added && seg < open.size() - 1) {
                    // step to next point
                    ClipperLib::IntPoint p;
                    ClipperLib::DoublePoint segDir;
                    do {
                        ClipperLib::IntPoint p1 = open[seg];
                        ClipperLib::IntPoint p2 = open[seg + 1];
                        double segLen = sqrt(DistanceSqrd(ToPoint64(p1), ToPoint64(p2)));
                        segDir = {(p2.X - p1.X) / segLen, (p2.Y - p1.Y) / segLen};
                        if (segLen - segD > dToGo) {
                            // interpolate current segment
                            segD += dToGo;
                            dToGo = 0;
                            double interp = segD / segLen;
                            p = {
                                (long long)(p2.X * interp + p1.X * (1 - interp)),
                                (long long)(p2.Y * interp + p1.Y * (1 - interp))
                            };
                        }
                        else {
                            dToGo -= segLen - segD;
                            segD = 0;
                            seg++;
                            p = p2;  // ensures that we try the endpoint too
                        }
                    } while (dToGo > 0 && seg < open.size() - 1);

                    // Attempt to add the point
                    const auto toolDir = initToolDir(ToPoint64(p), ToPointD(segDir));
                    if (toolDir) {
                        addEngagePoint(ToPoint64(p), *toolDir);
                        added = true;
                    }

                    dToGo = MIN_STEP_CLIPPER;  // all subsequent steps are MIN_STEP_CLIPPER
                }
            }
        }

        // sort engagePoints based on connection cost
        std::sort(
            engagePoints.begin(),
            engagePoints.end(),
            [](std::tuple<ClipperLib::IntPoint, PointD, double> aa,
               std::tuple<ClipperLib::IntPoint, PointD, double> bb) {
                return std::get<double>(aa) < std::get<double>(bb);
            }
        );

        double bestCost = __DBL_MAX__;
        TPaths bestLink;
        ClipperLib::IntPoint bestPos;
        PointD bestDir;

        for (const auto& ep : engagePoints) {
            if (std::get<double>(ep) < bestCost) {
                std::optional<TPaths> link = FindLinkPath(
                    prevPos ? std::optional(ToPoint64(*prevPos)) : std::nullopt,
                    ToPoint64(std::get<ClipperLib::IntPoint>(ep)),
                    std::get<PointD>(ep),
                    cleared,
                    toolBoundPaths_p64,
                    output
                );
                if (!link) {
                    continue;
                }

                double cost_mm = 0;
                std::optional<DPoint> prev = prevPos
                    ? std::optional<DPoint> {{prevPos->X / (double)scaleFactor, prevPos->Y / (double)scaleFactor}}
                    : std::optional<DPoint> {};
                for (TPath tp : *link) {
                    if (tp.first == MotionType::mtLinkNotClear) {
                        cost_mm += 10000;  // prioritize links that don't require retraction
                    }
                    for (size_t i = 0; i < tp.second.size(); i++) {
                        DPoint cur = tp.second[i];
                        if (prev) {
                            double dx = cur.first - prev->first;
                            double dy = cur.second - prev->second;
                            double dist = sqrt(dx * dx + dy * dy);
                            cost_mm += dist;
                        }
                        prev = {cur};
                    }
                }

                if (cost_mm < bestCost) {
                    bestCost = cost_mm;
                    bestLink = *link;
                    bestPos = std::get<ClipperLib::IntPoint>(ep);
                    bestDir = std::get<PointD>(ep);
                }
            }
        }

        if (bestCost < __DBL_MAX__) {
            return std::optional<std::tuple<ClipperLib::IntPoint, PointD, TPaths>> {
                {bestPos, bestDir, bestLink}
            };
        }
        else {
            return std::optional<std::tuple<ClipperLib::IntPoint, PointD, TPaths>> {};
        }
    };

    const auto getEngagePoint = [&](const std::optional<Point64>& prevPos_p64) {
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
        auto result = _getEngagePoint(prevPos_p64, engagementProtrusion);
        if (!result) {
            // consider retry at tiny protrusion?
            // result = _getEngagePoint(prevPos, 4);
        }

        // update cleared area
        if (result) {
            for (const TPath& linkPath : std::get<TPaths>(*result)) {
                if (linkPath.first == MotionType::mtCutting) {
                    ClipperLib::Path p;
                    for (const DPoint& dp : linkPath.second) {
                        p.push_back(
                            {(long long)(dp.first * scaleFactor), (long long)(dp.second * scaleFactor)}
                        );
                    }
                    cleared.ExpandCleared(ToPath64(p));
                }
            }
        }

        Perf_NextEngagePoint.Stop();
        return result;
    };

    std::optional<std::tuple<ClipperLib::IntPoint, PointD, TPaths>> engagePoint = getEngagePoint(std::nullopt);
    TPaths linkPath;
    if (engagePoint) {
        toolPos = ToPoint64(std::get<ClipperLib::IntPoint>(*engagePoint));
        toolDir = std::get<PointD>(*engagePoint);
        linkPath = std::get<TPaths>(*engagePoint);
        entryPoint = linkPath.size() > 0
            ? Point64 {(long long)(linkPath[0].second[0].first * scaleFactor), (long long)(linkPath[0].second[0].second * scaleFactor)}
            : toolPos;
        output.StartPoint
            = DPoint(double(entryPoint.x) / scaleFactor, double(entryPoint.y) / scaleFactor);
    }
    else {
        // Engagement failed; instead helix down
        Point64 entryPoint_p64, toolPos_p64;
        PointD toolDir_pd;
        if (!FindEntryPoint(
                progressPaths,
                toolBoundPaths_p64,
                boundPaths_p64,
                cleared,
                entryPoint_p64,
                toolPos_p64,
                toolDir_pd,
                helixRadiusScaled,
                output
            )) {
            Perf_ProcessPolyNode.Stop();
            results.push_back(output);
            return;
        }
        entryPoint = entryPoint_p64;
        toolPos = toolPos_p64;
        toolDir = toolDir_pd;
        output.StartPoint = DPoint(double(toolPos.x) / scaleFactor, double(toolPos.y) / scaleFactor);
    }

    output.ReturnMotionType = 0;
    output.HelixCenterPoint.first = double(entryPoint.x) / scaleFactor;
    output.HelixCenterPoint.second = double(entryPoint.y) / scaleFactor;

    //*******************************
    // LOOP - PASSES
    //*******************************
    for (long pass = 0; pass < PASSES_LIMIT; pass++) {
        if (stopProcessing) {
            break;
        }

        passToolPath_p64.clear();
        toClearPath_p64.clear();
        angleHistory.clear();
        angleHistory.push_back(0);

        // include linking path in cleared area
        for (TPath lp : linkPath) {
            if (lp.first == MotionType::mtCutting || lp.first == MotionType::mtLinkClear) {
                ClipperLib::Path scaledP;
                for (auto& p : lp.second) {
                    scaledP.push_back(
                        {(long long)(p.first * scaleFactor), (long long)(p.second * scaleFactor)}
                    );
                }
                cleared.ExpandCleared(ToPath64(scaledP));
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

        //*******************************
        // LOOP - POINTS
        //*******************************
        for (long point_index = 0; point_index < POINTS_PER_PASS_LIMIT; point_index++) {
            if (stopProcessing) {
                break;
            }

            total_points++;
            PointD toolDir_p64;
            AverageDirection(gyro, toolDir_p64);
            toolDir = toolDir_p64;

            const auto itResult = iterateNextStep(toolPos, toolDir, true);

            if (itResult.tooManyIterations) {
                total_exceeded++;
            }

            if (!itResult.failed) {  // cut is ok - record it
                if (itResult.iterationAngle) {
                    angleHistory.push_back(*itResult.iterationAngle);
                    if (angleHistory.size() > ANGLE_HISTORY_POINTS) {
                        angleHistory.erase(angleHistory.begin());
                    }
                }

                // if the path has changed direction by more than 45 degrees (such that we consider
                // continuations (>45)+45>90 degrees from the original direction) then we need to
                // update cleared paths
                if (lastExpandToolDir_p64.x * itResult.newToolDir.x
                        + lastExpandToolDir_p64.y * itResult.newToolDir.y
                    < cos(std::numbers::pi / 4)) {
                    cleared.ExpandCleared(toClearPath_p64);
                    toClearPath_p64.clear();
                    lastExpandToolDir_p64 = toolDir;
                }

                if (toClearPath_p64.empty()) {
                    toClearPath_p64.push_back(toolPos);
                }
                toClearPath_p64.push_back(itResult.newToolPos);

                cumulativeCutArea += itResult.area;

                // append to toolpaths
                if (passToolPath_p64.empty()) {
                    passToolPath_p64.push_back(toolPos);
                }
                passToolPath_p64.push_back(itResult.newToolPos);
                perf_total_len += stepScaled;
                toolPos = itResult.newToolPos;

                // append to progress info paths
                if (progressPaths.empty()) {
                    progressPaths.push_back(TPath());
                }
                progressPaths.back().second.emplace_back(
                    double(itResult.newToolPos.x) / scaleFactor,
                    double(itResult.newToolPos.y) / scaleFactor
                );

                // append gyro
                gyro.push_back(itResult.newToolDir);
                gyro.erase(gyro.begin());
                CheckReportProgress(progressPaths);
            }
            else {
                // cout<<"Break: no cut @" << point_index << endl;
                break;
            }
        } /* end of points loop*/

        if (!toClearPath_p64.empty()) {
            cleared.ExpandCleared(toClearPath_p64);
            toClearPath_p64.clear();
        }

        ClipperLib::Paths newlyClearedAreas;
        clip.Clear();
        clip.AddPaths(ToC1Paths(cleared.GetCleared()), ClipperLib::PolyType::ptSubject, true);
        clip.AddPaths(ToC1Paths(clearedBeforePass.GetCleared()), ClipperLib::PolyType::ptClip, true);
        clip.Execute(ClipperLib::ClipType::ctDifference, newlyClearedAreas);
        cumulativeCutArea = 0;
        for (ClipperLib::Path& a : newlyClearedAreas) {
            int nesting = getPathNestingLevel(ToPath64(a), ToPaths64(newlyClearedAreas));
            cumulativeCutArea += (nesting % 2 == 1 ? 1 : -1) * fabs(Area(a));
        }

        if (cumulativeCutArea >= 1) {
            Path64 cleaned_p64;
            CleanPath(passToolPath_p64, cleaned_p64, CLEAN_PATH_TOLERANCE);
            total_output_points += long(cleaned_p64.size());
            auto newPos = AppendToolPath(output, cleaned_p64, linkPath, cleared, toolBoundPaths_p64);
            if (newPos) {
                toolPos = newPos->first;
                toolDir = newPos->second;
            }
            CheckReportProgress(progressPaths);
            bad_engage_count = 0;
        }
        else {
            bad_engage_count++;
        }

        if (bad_engage_count > 10000) {
            output.TooManyFailedEngagements = true;
            cerr << "Break (next valid engage point not found)." << endl;
            break;
        }

        clearedBeforePass.SetClearedPaths(cleared.GetCleared());
        engagePoint = getEngagePoint(toolPos);
        if (engagePoint) {
            toolPos = ToPoint64(std::get<ClipperLib::IntPoint>(*engagePoint));
            toolDir = std::get<PointD>(*engagePoint);
            linkPath = std::get<TPaths>(*engagePoint);
            lastExpandToolDir_p64 = toolDir;
        }
        else {
            // check if there are any uncleared area left
            ClipperLib::Paths remaining;
            for (const auto& p : ToC1Paths(cleared.GetCleared())) {
                if (!p.empty() && IsPointWithinCutRegion(toolBoundPaths_p64, ToPoint64(p.front()))
                    && DistancePointToPathsSqrd(
                           boundPaths_p64,
                           ToPoint64(p.front()),
                           clp_p64,
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

            output.UnclearedAreaRemains = true;
            cerr << "NO ENGAGEMENTS LEFT BUT NOT ALL CELARED!!! " << endl;
            break;
        }
    }

    // sanity check for finishing paths - check the area of finishing cut
    ClipperLib::Paths clearedLocations;
    clipof.Clear();
    clipof.AddPaths(ToC1Paths(cleared.GetCleared()), ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
    clipof.Execute(clearedLocations, long(-toolRadiusScaled));

    ClipperLib::Paths tbpShrink;
    clipof.Clear();
    clipof.AddPaths(ToC1Paths(toolBoundPaths_p64), ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
    clipof.Execute(tbpShrink, long(-stepOverScaled * FINISHING_THICKNESS_SCALE - MIN_STEP_CLIPPER));

    ClipperLib::Paths uncut;
    clip.Clear();
    clip.AddPaths(tbpShrink, ClipperLib::PolyType::ptSubject, true);
    clip.AddPaths(clearedLocations, ClipperLib::PolyType::ptClip, true);
    clip.Execute(ClipperLib::ClipType::ctDifference, uncut);

    if (uncut.size() > 0) {
        output.FailedToSetUpFinishingPass = true;
        cerr << "Warning: some cuts may be above optimal step-over. Please double check the "
                "results."
             << endl
             << "Hint: try to modify accuracy and/or step-over." << endl;
    }


    //**********************************
    //*  FINISHING PASS                *
    //**********************************
    if (finishingProfile) {
        // update tool bound paths to correspond to the finishing paths instead of the interior
        {
            ClipperLib::Paths tbpModified;
            for (const ClipperLib::Path& fp : finishingPaths) {
                clipof.Clear();
                clipof.AddPath(fp, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
                int offset = (getPathNestingLevel(ToPath64(fp), ToPaths64(finishingPaths)) % 2 == 1) ? 3 : -3;
                ClipperLib::Paths out;
                clipof.Execute(out, offset);

                bool orientation = Orientation(fp);
                for (ClipperLib::Path& p : out) {
                    if (Orientation(p) != orientation) {
                        ReversePath(p);
                    }
                    tbpModified.push_back(p);
                }
            }

            toolBoundPaths_p64 = ToPaths64(tbpModified);
        }

        // Split finishingPaths into closed (all Z=1) and open (partial Z=1) paths
        ClipperLib::Paths closedFinishingPaths;
        ClipperLib::Paths openFinishingPaths;

        for (const auto& p : finishingPaths) {
            // Find starting index: first Z=0, or 0 if none found
            size_t startIdx = 0;
            for (size_t i = 0; i < p.size(); i++) {
                if (p[i].Z == 0) {
                    startIdx = i;
                    break;
                }
            }

            // Loop through path starting at startIdx, collecting Z=1 vertices
            ClipperLib::Path currentPath;
            for (size_t offset = 0; offset < p.size(); offset++) {
                size_t i = (startIdx + offset) % p.size();
                const ClipperLib::IntPoint& pt = p[i];

                if (pt.Z == 1) {
                    // Add Z=1 point to current path
                    currentPath.push_back(pt);
                }
                else {
                    // Z=0: if we have accumulated points, save them to open list
                    if (!currentPath.empty()) {
                        openFinishingPaths.push_back(currentPath);
                        currentPath.clear();
                    }
                }
            }

            // After loop completes, check what we accumulated
            if (currentPath.size() == p.size()) {
                // All vertices were Z=1 - add to closed list
                closedFinishingPaths.emplace_back(p);
            }
            else if (currentPath.size() > 0) {
                // Partial Z=1 segment remains - add to open list
                openFinishingPaths.push_back(currentPath);
            }
            else {
                // No Z=1 vertices found
            }
        }


        ClipperLib::Path finShiftedPath;

        // Create offset version of stock boundary to check if finishing passes are within tool radius
        ClipperLib::Paths stockExpandedPaths;
        ClipperLib::Paths stockInputPathsC1 = ToC1Paths(stockInputPaths);
        clipof.Clear();
        clipof.AddPaths(stockInputPathsC1, ClipperLib::JoinType::jtRound, ClipperLib::EndType::etClosedPolygon);
        clipof.Execute(stockExpandedPaths, toolRadiusScaled);

        while (!stopProcessing && (!closedFinishingPaths.empty() || !openFinishingPaths.empty())) {
            Paths64 closedFP_p64 = ToPaths64(closedFinishingPaths);
            Paths64 openFP_p64 = ToPaths64(openFinishingPaths);
            Path64 finShiftedPath_p64;
            bool isClosedPath = PopNextFinishingPass(closedFP_p64, openFP_p64, toolPos, finShiftedPath_p64, stepOverScaled);
            closedFinishingPaths = ToC1Paths(closedFP_p64);
            openFinishingPaths = ToC1Paths(openFP_p64);
            finShiftedPath = ToC1Path(finShiftedPath_p64);

            if (finShiftedPath.empty()) {
                continue;
            }
            // skip finishing passes outside the stock boundary that do not cut any stock
            Path64 finShiftedPathCopy_p64 = ToPath64(finShiftedPath);
            Paths64 stockExpandedPaths_p64 = ToPaths64(stockExpandedPaths);
            ClipperLib::Paths intersection
                = ToC1Paths(PathIntersectArea(clip, finShiftedPathCopy_p64, stockExpandedPaths_p64, isClosedPath));
            if (intersection.empty()) {
                continue;
            }

            progressPaths.push_back(TPath());
            progressPaths.back().first = MotionType::mtCutting;
            // show in progress cb
            for (auto& pt : finShiftedPath) {
                progressPaths.back().second.emplace_back(
                    double(pt.X) / scaleFactor,
                    double(pt.Y) / scaleFactor
                );
            }

            // For closed paths, close the loop by adding the start point at the end
            if (isClosedPath && !finShiftedPath.empty()) {
                progressPaths.back().second.emplace_back(
                    double(finShiftedPath.front().X) / scaleFactor,
                    double(finShiftedPath.front().Y) / scaleFactor
                );
            }

            // Clean path
            Path64 finCleaned_p64;
            CleanPath(ToPath64(finShiftedPath), finCleaned_p64, FINISHING_CLEAN_PATH_TOLERANCE);
            ClipperLib::Path finCleaned = ToC1Path(finCleaned_p64);

            // Ensure correct endpoint after cleaning
            ClipperLib::IntPoint endPoint = isClosedPath ? finCleaned.front() : finShiftedPath.back();
            if (finCleaned.back() != endPoint) {
                // make sure it ends correctly, but don't risk ruining the final direction by making
                // a very short segment
                if (sqrt(DistanceSqrd(ToPoint64(endPoint), ToPoint64(finCleaned.back()))) < FINISHING_CLEAN_PATH_TOLERANCE) {
                    finCleaned.pop_back();
                }
                finCleaned.push_back(endPoint);
            }

            std::optional<TPaths> linkPath = FindLinkPath(
                std::optional(toolPos),
                ToPoint64(finCleaned[0]),
                ToPointD(GetPathDirectionV(ToPath64(finCleaned), 1)),
                cleared,
                toolBoundPaths_p64,
                output
            );
            if (!linkPath) {
                output.FinishingLeadInFailed = true;
                cerr << "Failed to generate lead-in for finishing pass; skipping pass" << endl;
            }
            else {
                auto newPos = AppendToolPath(output, ToPath64(finCleaned), *linkPath, cleared, toolBoundPaths_p64);
                if (newPos) {
                    toolPos = newPos->first;
                    toolDir = newPos->second;
                }
                else {
                    toolPos = ToPoint64(finCleaned.back());
                    toolDir = ToPointD(GetPathDirectionV(ToPath64(finCleaned), finCleaned.size() - 1));
                }

                cleared.ExpandCleared(ToPath64(finCleaned));
                for (TPath lp : *linkPath) {
                    if (lp.first == MotionType::mtCutting) {
                        ClipperLib::Path scaledP;
                        for (auto& p : lp.second) {
                            scaledP.push_back(
                                {(long long)(p.first * scaleFactor),
                                 (long long)(p.second * scaleFactor)}
                            );
                        }
                        cleared.ExpandCleared(ToPath64(scaledP));
                    }
                }
            }
        }

        ClipperLib::Path returnPath;
        returnPath << ToC1Point(toolPos);
        returnPath << ToC1Point(entryPoint);
        output.ReturnMotionType = IsClearPath(ToPath64(returnPath), cleared) ? MotionType::mtLinkClear
                                                                   : MotionType::mtLinkNotClear;
    }

    // dump performance results
    Perf_ProcessPolyNode.Stop();
#ifdef DEV_MODE
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
    cout << "ClipperLib::PolyNode perf:" << perf_total_len / double(scaleFactor) / duration << " mm/sec"
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

    // Calculate newly cleared area (final - initial)
    // First calculate initial cleared area
    double initialClearedArea = 0.0;
    for (const ClipperLib::Path& path : initialClearedPaths) {
        int nesting = getPathNestingLevel(ToPath64(path), ToPaths64(initialClearedPaths));
        initialClearedArea += (nesting % 2 == 1 ? 1 : -1) * fabs(Area(path));
    }

    // Then calculate final cleared area
    const ClipperLib::Paths clearedPaths = ToC1Paths(cleared.GetCleared());
    svgInfo.allFinalClearedPaths.push_back(ToPaths64(clearedPaths));
    double finalClearedArea = 0.0;
    for (const ClipperLib::Path& path : clearedPaths) {
        int nesting = getPathNestingLevel(ToPath64(path), ToPaths64(clearedPaths));
        finalClearedArea += (nesting % 2 == 1 ? 1 : -1) * fabs(Area(path));
    }

    // Convert from scaled units to real units (scaleFactor is applied twice for area)
    // and subtract initial from final to get only newly cleared area
    output.ClearedArea = (finalClearedArea - initialClearedArea) / (scaleFactor * scaleFactor);

    results.push_back(output);
}

}  // namespace AdaptivePath
