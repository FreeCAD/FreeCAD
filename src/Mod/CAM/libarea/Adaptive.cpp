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
#include <cmath>
#include <cstring>
#include <ctime>
#include <algorithm>

namespace ClipperLib
{
void TranslatePath(const Path &input, Path &output, IntPoint delta);
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

inline double DistanceSqrd(const IntPoint &pt1, const IntPoint &pt2)
{
	double Dx = double(pt1.X - pt2.X);
	double dy = double(pt1.Y - pt2.Y);
	return (Dx * Dx + dy * dy);
}

inline bool SetSegmentLength(const IntPoint &pt1, IntPoint &pt2, double new_length)
{
	double Dx = double(pt2.X - pt1.X);
	double dy = double(pt2.Y - pt1.Y);
	double l = sqrt(Dx * Dx + dy * dy);
	if (l > 0.0)
	{
		pt2.X = long(pt1.X + new_length * Dx / l);
		pt2.Y = long(pt1.Y + new_length * dy / l);
		return true;
	}
	return false;
}

inline bool HasAnyPath(const Paths &paths)
{
	for (Paths::size_type i = 0; i < paths.size(); i++)
	{
		if (!paths[i].empty())
			return true;
	}
	return false;
}

inline double averageDV(const vector<double> &vec)
{
	double s = 0;
	std::size_t size = vec.size();
	if (size == 0)
		return 0;
	for (std::size_t i = 0; i < size; i++)
		s += vec[i];
	return s / double(size);
}

inline DoublePoint rotate(const DoublePoint &in, double rad)
{
	double c = cos(rad);
	double s = sin(rad);
	return DoublePoint(c * in.X - s * in.Y, s * in.X + c * in.Y);
}

// calculates path length for open path
inline double PathLength(const Path &path)
{
	double len = 0;
	if (path.size() < 2)
		return len;
	for (size_t i = 1; i < path.size(); i++)
	{
		len += sqrt(DistanceSqrd(path[i - 1], path[i]));
	}
	return len;
}

inline double PointSideOfLine(const IntPoint &p1, const IntPoint &p2, const IntPoint &pt)
{
	return double((pt.X - p1.X) * (p2.Y - p1.Y) - (pt.Y - p2.Y) * (p2.X - p1.X));
}

inline double Angle3Points(const DoublePoint &p1, const DoublePoint &p2, const DoublePoint &p3)
{
	double t1 = atan2(p2.Y - p1.Y, p2.X - p1.X);
	double t2 = atan2(p3.Y - p2.Y, p3.X - p2.X);
	double a = fabs(t2 - t1);
	return min(a, 2 * M_PI - a);
}

inline DoublePoint DirectionV(const IntPoint &pt1, const IntPoint &pt2)
{
	double DX = double(pt2.X - pt1.X);
	double DY = double(pt2.Y - pt1.Y);
	double l = sqrt(DX * DX + DY * DY);
	if (l < NTOL)
		return DoublePoint(0, 0);
	return DoublePoint(DX / l, DY / l);
}

inline void NormalizeV(DoublePoint &pt)
{
	double len = sqrt(pt.X * pt.X + pt.Y * pt.Y);
	if (len > NTOL)
	{
		pt.X /= len;
		pt.Y /= len;
	}
}

inline DoublePoint GetPathDirectionV(const Path &pth, size_t pointIndex)
{
	if (pth.size() < 2)
		return DoublePoint(0, 0);
	const IntPoint &p1 = pth.at(pointIndex > 0 ? pointIndex - 1 : pth.size() - 1);
	const IntPoint &p2 = pth.at(pointIndex);
	return DirectionV(p1, p2);
}

// Returns true if points 'a' and 'b' are coincident or nearly so.
bool isClose(const IntPoint &a, const IntPoint &b) {
	return abs(a.X - b.X) <= 1 && abs(a.Y - b.Y) <= 1;
}

// Remove coincident and almost-coincident points from Paths.
void filterCloseValues(Paths &ppg) {
	for (auto& pth : ppg) {
		while (true) {
			auto i = std::adjacent_find(pth.begin(), pth.end(), isClose);
			if (i == pth.end())
				break;
			pth.erase(i);
		}
		// adjacent_find doesn't compare first with last element, so
		// do that manually.
		while (pth.size() > 1 && isClose(pth.front(), pth.back()))
			pth.pop_back();
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
	BoundBox(const IntPoint &p1)
	{
		minX = p1.X;
		maxX = p1.X;
		minY = p1.Y;
		maxY = p1.Y;
	}

	void SetFirstPoint(const IntPoint &p1)
	{
		minX = p1.X;
		maxX = p1.X;
		minY = p1.Y;
		maxY = p1.Y;
	}

	// generic: subsequent points
	void AddPoint(const IntPoint &pt)
	{
		minX = min(pt.X, minX);
		maxX = max(pt.X, maxX);
		minY = min(pt.Y, minY);
		maxY = max(pt.Y, maxY);
	}

	// line segment: two points
	BoundBox(const IntPoint &p1, const IntPoint &p2)
	{
		if (p1.X < p2.X)
		{
			minX = p1.X;
			maxX = p2.X;
		}
		else
		{
			minX = p2.X;
			maxX = p1.X;
		}
		if (p1.Y < p2.Y)
		{
			minY = p1.Y;
			maxY = p2.Y;
		}
		else
		{
			minY = p2.Y;
			maxY = p1.Y;
		}
	}

	// for circle: center and radius
	BoundBox(const IntPoint &center, long radius)
	{
		minX = center.X - radius;
		maxX = center.X + radius;
		minY = center.Y - radius;
		maxY = center.Y + radius;
	}

	// bounds check - intersection
	inline bool CollidesWith(const BoundBox &bb2)
	{
		return minX <= bb2.maxX && maxX >= bb2.minX && minY <= bb2.maxY && maxY >= bb2.minY;
	}

	// bounds check -  contains
	inline bool Contains(const BoundBox &bb2)
	{
		return minX <= bb2.minX && maxX >= bb2.maxX && minY <= bb2.minY && maxY >= bb2.maxY;
	}

	ClipperLib::cInt minX;
	ClipperLib::cInt maxX;
	ClipperLib::cInt minY;
	ClipperLib::cInt maxY;
};

std::ostream &operator<<(std::ostream &s, const BoundBox &p)
{
	s << "(" << p.minX << "," << p.minY << ") - (" << p.maxX << "," << p.maxY << ")";
	return s;
}

int getPathNestingLevel(const Path &path, const Paths &paths)
{
	int nesting = 0;
	for (const auto &other : paths)
	{
		if (!path.empty() && PointInPolygon(path.front(), other) != 0)
			nesting++;
	}
	return nesting;
}

void appendDirectChildPaths(Paths &outPaths, const Path &path, const Paths &paths)
{
	int nesting = getPathNestingLevel(path, paths);
	for (const auto &other : paths)
	{
		if (!path.empty() && !other.empty() && PointInPolygon(other.front(), path) != 0)
		{
			if (getPathNestingLevel(other, paths) == nesting + 1)
				outPaths.push_back(other);
		}
	}
}

void AverageDirection(const vector<DoublePoint> &unityVectors, DoublePoint &output)
{
	std::size_t size = unityVectors.size();
	output.X = 0;
	output.Y = 0;
	// sum vectors
	for (std::size_t i = 0; i < size; i++)
	{
		DoublePoint v = unityVectors[i];
		output.X += v.X;
		output.Y += v.Y;
	}
	// normalize
	double magnitude = sqrt(output.X * output.X + output.Y * output.Y);
	output.X /= magnitude;
	output.Y /= magnitude;
}

double DistancePointToLineSegSquared(const IntPoint &p1, const IntPoint &p2, const IntPoint &pt,
									 IntPoint &closestPoint, double &ptParameter, bool clamp = true)
{
	double D21X = double(p2.X - p1.X);
	double D21Y = double(p2.Y - p1.Y);
	double DP1X = double(pt.X - p1.X);
	double DP1Y = double(pt.Y - p1.Y);
	double lsegLenSqr = D21X * D21X + D21Y * D21Y;
	if (lsegLenSqr == 0)
	{ // segment is zero length, return point to point distance
		closestPoint = p1;
		ptParameter = 0;
		return DP1X * DP1X + DP1Y * DP1Y;
	}
	double parameter = DP1X * D21X + DP1Y * D21Y;
	if (clamp)
	{
		// clamp the parameter
		if (parameter < 0)
			parameter = 0;
		else if (parameter > lsegLenSqr)
			parameter = lsegLenSqr;
	}
	// point on line at parameter
	ptParameter = parameter / lsegLenSqr;
	closestPoint.X = long(p1.X + ptParameter * D21X);
	closestPoint.Y = long(p1.Y + ptParameter * D21Y);
	// calculate distance from point on line to pt
	double DX = double(pt.X - closestPoint.X);
	double DY = double(pt.Y - closestPoint.Y);
	return DX * DX + DY * DY; // return distance squared
}

void ScaleUpPaths(Paths &paths,long scaleFactor) {
	for(auto &pth:paths) {
		for(auto &pt:pth) {
			pt.X*=scaleFactor;
			pt.Y*=scaleFactor;
		}
	}
}

void ScaleDownPaths(Paths &paths,long scaleFactor) {
	for(auto &pth:paths) {
		for(auto &pt:pth) {
			pt.X/=scaleFactor;
			pt.Y/=scaleFactor;
		}
	}
}




double DistancePointToPathsSqrd(const Paths &paths, const IntPoint &pt, IntPoint &closestPointOnPath,
								size_t &clpPathIndex,
								size_t &clpSegmentIndex,
								double &clpParameter)
{
	double minDistSq = __DBL_MAX__;
	IntPoint clp;
	// iterate though paths
	for (Path::size_type i = 0; i < paths.size(); i++)
	{
		const Path *path = &paths[i];
		Path::size_type size = path->size();
		// iterate through segments
		for (Path::size_type j = 0; j < size; j++)
		{
			double ptPar;
			double distSq = DistancePointToLineSegSquared(path->at(j > 0 ? j - 1 : size - 1), path->at(j), pt, clp, ptPar);
			if (distSq < minDistSq)
			{
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
void CleanPath(const Path &inp, Path &outpt, double tolerance)
{
	if (inp.size() < 3)
	{
		outpt = inp;
		return;
	}
	outpt.clear();
	Path tmp;
	CleanPolygon(inp, tmp, tolerance);
	long size=long(tmp.size());

	// CleanPolygon will have empty result if all points are collinear,
	// 	need to add first and last point to the output
	if(size<=2) {
		outpt.push_back(inp.front());
		outpt.push_back(inp.back());
		return;
	}

	// restore starting point
	double clpPar =  0;
	size_t clpSegmentIndex=0;
	size_t clpPathIndex=0;
	Paths tmpPaths;
	tmpPaths.push_back(tmp);
	IntPoint clp;
	// find point on cleaned poly that is closest to original starting point
	DistancePointToPathsSqrd(tmpPaths,inp.front(),clp,clpPathIndex,clpSegmentIndex,clpPar);


	// if closes point is not one of the polygon points, add it as separate first point
	if(DistanceSqrd(clp,tmp.at(clpSegmentIndex)) > 0 &&
		DistanceSqrd(clp,tmp.at(clpSegmentIndex>0 ? clpSegmentIndex-1 : size-1)) > 0) outpt.push_back(clp);

	// add remaining points starting from closest
	long index;
	for (long i = 0; i < size; i++)
	{
		index = static_cast<long>(clpSegmentIndex + i);
		if (index >= size) index -= size;
		outpt.push_back(tmp.at(index));
	}


	if(DistanceSqrd(outpt.front(),inp.front()) > SAME_POINT_TOL_SQRD_SCALED)
		outpt.insert(outpt.begin(), inp.front());

	if(DistanceSqrd(outpt.back(),inp.back()) > SAME_POINT_TOL_SQRD_SCALED)
		outpt.push_back( inp.back());

}

bool Circle2CircleIntersect(const IntPoint &c1, const IntPoint &c2, double radius, pair<DoublePoint, DoublePoint> &intersections)
{
	double DX = double(c2.X - c1.X);
	double DY = double(c2.Y - c1.Y);
	double d = sqrt(DX * DX + DY * DY);
	if (d < NTOL)
		return false; // same center
	if (d >= radius)
		return false; // do not intersect, or intersect in one point (this case not relevant here)
	double a_2 = sqrt(4 * radius * radius - d * d) / 2.0;
	intersections.first = DoublePoint(0.5 * (c1.X + c2.X) - DY * a_2 / d, 0.5 * (c1.Y + c2.Y) + DX * a_2 / d);
	intersections.second = DoublePoint(0.5 * (c1.X + c2.X) + DY * a_2 / d, 0.5 * (c1.Y + c2.Y) - DX * a_2 / d);
	return true;
}

bool Line2CircleIntersect(const IntPoint &c, double radius, const IntPoint &p1, const IntPoint &p2, vector<DoublePoint> &result, bool clamp = true)
{
	// if more intersections returned, first is closer to p1

	//box  check for performance
	if (clamp)
	{
		BoundBox cBB(c, (ClipperLib::cInt)radius + 1); // circle bound box
		BoundBox sBB(p1, p2);
		if (!sBB.CollidesWith(cBB))
			return false;
	}

	double dx = double(p2.X - p1.X);
	double dy = double(p2.Y - p1.Y);
	double lcx = double(p1.X - c.X);
	double lcy = double(p1.Y - c.Y);
	double a = dx * dx + dy * dy;
	double b = 2 * dx * lcx + 2 * dy * lcy;
	double C = lcx * lcx + lcy * lcy - radius * radius;
	double sq = b * b - 4 * a * C;
	if (sq < 0)
		return false; // no solution
	sq = sqrt(sq);
	double t1 = (-b - sq) / (2 * a);
	double t2 = (-b + sq) / (2 * a);
	result.clear();
	if (clamp)
	{
		if (t1 >= 0.0 && t1 <= 1.0)
			result.emplace_back(p1.X + t1 * dx, p1.Y + t1 * dy);
		if (t2 >= 0.0 && t2 <= 1.0)
			result.emplace_back(p1.X + t2 * dx, p1.Y + t2 * dy);
	}
	else
	{
		result.emplace_back(p1.X + t2 * dx, p1.Y + t2 * dy);
		result.emplace_back(p1.X + t2 * dx, p1.Y + t2 * dy);
	}
	return !result.empty();
}

// calculate center point of polygon
IntPoint Compute2DPolygonCentroid(const Path &vertices)
{
	DoublePoint centroid(0, 0);
	double signedArea = 0.0;
	double x0 = 0.0; // Current vertex X
	double y0 = 0.0; // Current vertex Y
	double x1 = 0.0; // Next vertex X
	double y1 = 0.0; // Next vertex Y
	double a = 0.0;  // Partial signed area

	// For all vertices
	size_t i = 0;
	Path::size_type size = vertices.size();
	for (i = 0; i < size; ++i)
	{
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
bool IsPointWithinCutRegion(const Paths &toolBoundPaths, const IntPoint &point)
{
	for (size_t i = 0; i < toolBoundPaths.size(); i++)
	{
		int pip = PointInPolygon(point, toolBoundPaths[i]);
		if (i == 0 && pip == 0)
			return false; // is outside or on boundary
		if (i > 0 && pip != 0)
			return false; // is inside hole
	}
	return true;
}

/* finds intersection of line segment with line segment */
bool IntersectionPoint(const IntPoint &s1p1,
					   const IntPoint &s1p2,
					   const IntPoint &s2p1,
					   const IntPoint &s2p2,
					   IntPoint &intersection)
{
	double S1DX = double(s1p2.X - s1p1.X);
	double S1DY = double(s1p2.Y - s1p1.Y);
	double S2DX = double(s2p2.X - s2p1.X);
	double S2DY = double(s2p2.Y - s2p1.Y);
	double d = S1DY * S2DX - S2DY * S1DX;
	if (fabs(d) < NTOL)
		return false; // lines are parallel

	double LPDX = double(s1p1.X - s2p1.X);
	double LPDY = double(s1p1.Y - s2p1.Y);
	double p1d = S2DY * LPDX - S2DX * LPDY;
	double p2d = S1DY * LPDX - S1DX * LPDY;
	if ((d < 0) && (p1d < d || p1d > 0 || p2d < d || p2d > 0))
		return false; // intersection not within segment1
	if ((d > 0) && (p1d < 0 || p1d > d || p2d < 0 || p2d > d))
		return false; // intersection not within segment2
	double t = p1d / d;
	intersection = IntPoint(long(s1p1.X + S1DX * t), long(s1p1.Y + S1DY * t));
	return true;
}

/* finds one/first intersection of line segment with paths */
bool IntersectionPoint(const Paths &paths, const IntPoint &p1, const IntPoint &p2, IntPoint &intersection)
{
	BoundBox segBB(p1, p2);
	for (size_t i = 0; i < paths.size(); i++)
	{
		const Path *path = &paths[i];
		size_t size = path->size();
		if (size < 2)
			continue;
		BoundBox pathBB(path->front());
		for (size_t j = 0; j < size; j++)
		{

			const IntPoint *pp2 = &path->at(j);

			// box check for performance
			pathBB.AddPoint(*pp2);
			if (!pathBB.CollidesWith(segBB))
				continue;

			const IntPoint *pp1 = &path->at(j > 0 ? j - 1 : size - 1);
			double LDY = double(p2.Y - p1.Y);
			double LDX = double(p2.X - p1.X);
			double PDX = double(pp2->X - pp1->X);
			double PDY = double(pp2->Y - pp1->Y);
			double d = LDY * PDX - PDY * LDX;
			if (fabs(d) < NTOL)
				continue; // lines are parallel

			double LPDX = double(p1.X - pp1->X);
			double LPDY = double(p1.Y - pp1->Y);
			double p1d = PDY * LPDX - PDX * LPDY;
			double p2d = LDY * LPDX - LDX * LPDY;
			if ((d < 0) && (p1d < d || p1d > 0 || p2d < d || p2d > 0))
				continue; // intersection not within segment
			if ((d > 0) && (p1d < 0 || p1d > d || p2d < 0 || p2d > d))
				continue; // intersection not within segment
			double t = p1d / d;
			intersection = IntPoint(long(p1.X + LDX * t), long(p1.Y + LDY * t));
			return true;
		}
	}
	return false;
}

void SmoothPaths(Paths &paths, double stepSize, long pointCount, long iterations)
{
	Paths output;
	output.resize(paths.size());
	const long scale=1000;
	const double stepScaled = stepSize*scale;

	ScaleUpPaths(paths,scale);
	vector<pair<size_t /*path index*/, IntPoint>> points;
	for (size_t i = 0; i < paths.size(); i++)
	{
		for (const auto &pt : paths[i])
		{
			if (points.empty())
			{
				points.emplace_back(i, pt);
				continue;
			}
			const auto back=points.back();
			const IntPoint & lastPt = back.second;


			const double l = sqrt(DistanceSqrd(lastPt, pt));

			if (l < 0.5*stepScaled )
			{
				if(points.size()>1) points.pop_back();
				points.emplace_back(i, pt);
				continue;
			}
			size_t lastPathIndex = back.first;
			const long steps = max(long(l / stepScaled),1L);
			const long left=pointCount*iterations*2;
			const long right =steps-pointCount*iterations*2;
			for (long idx = 0; idx <= steps; idx++)
			{
				if(idx>left && idx<right) {
					idx=right;
					continue;
				}
				const double p = double(idx) / steps;
				const IntPoint ptx(long(lastPt.X + double(pt.X - lastPt.X) * p),
							 long(lastPt.Y + double(pt.Y - lastPt.Y) * p));

				if(idx==0 && DistanceSqrd(back.second,ptx)<scale && points.size()>1) points.pop_back();

				if (p < 0.5)
					points.emplace_back(lastPathIndex, ptx);
				else
					points.emplace_back(i, ptx);
			}
		}
	}
	if (points.empty())
		return;
	const long size=long(points.size());
	for(long iter=0;iter<iterations; iter++) {
		for (long i = 1; i < size-1; i++)
		{
			IntPoint &cp = points[i].second;
			IntPoint avgPoint(cp);
			long cnt = 1;

			long ptsToAverage = pointCount;
			if (i <= ptsToAverage)
			 	ptsToAverage = max(i-1,0L);
			else if(i + ptsToAverage >= size-1)
			 	ptsToAverage = size-1-i;
			for (long j = i-ptsToAverage; j <= i+ptsToAverage; j++)
			{
				if (j == i) continue;
				long index=j;
				if(index<0)	index=0;
				if(index>=size) index=size-1;
				IntPoint &p = points[index].second;
				avgPoint.X += p.X ;
				avgPoint.Y += p.Y ;
				cnt++;
			}
			cp.X=avgPoint.X/cnt;
			cp.Y=avgPoint.Y/cnt;
		}
	}

	for (const auto &pr:points)
	{
		output[pr.first].push_back(pr.second);
	}
	for (size_t i = 0; i < paths.size(); i++)
	{
		CleanPath(output[i], paths[i], 1.4*scale);
	}
	ScaleDownPaths(paths,scale);
}

bool PopPathWithClosestPoint(Paths &paths /*closest path is removed from collection and shifted to start with closest point */
							 ,
							 IntPoint p1, Path &result)
{

	if (paths.empty())
		return false;

	double minDistSqrd = __DBL_MAX__;
	size_t closestPathIndex = 0;
	long closestPointIndex = 0;
	for (size_t pathIndex = 0; pathIndex < paths.size(); pathIndex++)
	{
		Path &path = paths.at(pathIndex);
		for (size_t i = 0; i < path.size(); i++)
		{
			double dist = DistanceSqrd(p1, path.at(i));
			if (dist < minDistSqrd)
			{
				minDistSqrd = dist;
				closestPathIndex = pathIndex;
				closestPointIndex = long(i);
			}
		}
	}

	result.clear();
	// make new path starting with that point
	Path &closestPath = paths.at(closestPathIndex);
	for (size_t i = 0; i < closestPath.size(); i++)
	{
		long index = closestPointIndex + long(i);
		if (index >= long(closestPath.size()))
			index -= long(closestPath.size());
		result.push_back(closestPath.at(index));
	}
	// remove the closest path
	paths.erase(paths.begin() + closestPathIndex);
	return true;
}

void DeduplicatePaths(const Paths &inputs, Paths &outputs)
{
	outputs.clear();
	for (const auto &new_pth : inputs)
	{
		bool duplicate = false;
		// if all points of new path exist on some of the old paths, path is considered duplicate
		for (const auto &old_pth : outputs)
		{
			bool all_points_exists = true;
			for (const auto pt1 : new_pth)
			{
				bool pointExists = false;
				for (const auto pt2 : old_pth)
				{
					if (DistanceSqrd(pt1, pt2) < SAME_POINT_TOL_SQRD_SCALED)
					{
						pointExists = true;
						break;
					}
				}
				if (!pointExists)
				{
					all_points_exists = false;
					break;
				}
			}
			if (all_points_exists)
			{
				duplicate = true;
				break;
			}
		}

		if (!duplicate && !new_pth.empty())
		{
			outputs.push_back(new_pth);
		}
	}
}

void ConnectPaths(Paths input, Paths &output)
{
	output.clear();
	bool newPath = true;
	Path joined;
	while (!input.empty())
	{
		if (newPath)
		{
			if (!joined.empty())
				output.push_back(joined);
			joined.clear();
			for (auto pt : input.front())
			{
				joined.push_back(pt);
			}
			input.erase(input.begin());
			newPath = false;
		}
		bool anyMatch = false;
		for (size_t i = 0; i < input.size(); i++)
		{
			Path &n = input.at(i);
			if (DistanceSqrd(n.front(), joined.back()) < SAME_POINT_TOL_SQRD_SCALED)
			{
				for (auto pt : n)
					joined.push_back(pt);
				input.erase(input.begin() + i);
				anyMatch = true;
				break;
			}
			else if (DistanceSqrd(n.back(), joined.back()) < SAME_POINT_TOL_SQRD_SCALED)
			{
				ReversePath(n);
				for (auto pt : n)
					joined.push_back(pt);
				input.erase(input.begin() + i);
				anyMatch = true;
				break;
			}
			else if (DistanceSqrd(n.front(), joined.front()) < SAME_POINT_TOL_SQRD_SCALED)
			{
				for (auto pt : n)
					joined.insert(joined.begin(), pt);
				input.erase(input.begin() + i);
				anyMatch = true;
				break;
			}
			else if (DistanceSqrd(n.back(), joined.front()) < SAME_POINT_TOL_SQRD_SCALED)
			{
				ReversePath(n);
				for (auto pt : n)
					joined.insert(joined.begin(), pt);
				input.erase(input.begin() + i);
				anyMatch = true;
				break;
			}
		}
		if (!anyMatch)
			newPath = true;
	}
	if (!joined.empty())
		output.push_back(joined);
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
		if (running)
		{
			cerr << "PerfCounter already running:" << name << endl;
		}
		running = true;
#endif
	}
	inline void Stop()
	{
#ifdef DEV_MODE
		if (!running)
		{
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
		cout << "Perf: " << name.c_str() << " total_time: " << total_time << " sec, call_count:" << count << " per_call:" << double(total_time / count) << endl;
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
PerfCounter Perf_CalcCutAreaClip("CalcCutAreaClip");
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

	void SetClearedPaths(const Paths &paths)
	{
		clearedPaths = paths;
		bboxPathsInvalid = true;
		bboxClippedInvalid = true;
	}
	void ExpandCleared(const Path toClearToolPath)
	{
		if (toClearToolPath.empty())
			return;
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

	// gets the path sections inside the ext. tool bounding box
	Paths &GetBoundedClearedPaths(const IntPoint &toolPos)
	{
		BoundBox toolBB(toolPos, toolRadiusScaled);
		if (!bboxPathsInvalid && clearedBBPathsInFocus.Contains(toolBB))
		{
			return clearedBoundedPaths;
		}
		ClipperLib::cInt delta = focusBBFactor1 * toolRadiusScaled;
		clearedBBPathsInFocus.SetFirstPoint(IntPoint(toolPos.X - delta, toolPos.Y - delta));
		clearedBBPathsInFocus.AddPoint(IntPoint(toolPos.X + delta, toolPos.Y + delta));

		BoundBox bb(toolPos, focusBBFactor2 * toolRadiusScaled);
		clearedBoundedPaths.clear();
		for (const auto &pth : clearedPaths)
		{
			if (pth.size() < 2)
				continue;
			Path bPath;
			size_t size = pth.size();
			for (size_t i = 0; i < size + 1; i++)
			{
				IntPoint last = (i > 0 ? pth[i - 1] : pth.back());
				IntPoint next = i < size ? pth[i] : pth.front();
				BoundBox ptbox(last, next);
				if (ptbox.CollidesWith(bb))
				{
					if (bPath.empty() || bPath.back() != last)
						bPath.push_back(last);
					bPath.push_back(next);
				}
				else
				{
					if (!bPath.empty())
					{
						clearedBoundedPaths.push_back(bPath);
						bPath.clear();
					}
				}
			}
			if (!bPath.empty())
			{
				clearedBoundedPaths.push_back(bPath);
				bPath.clear();
			}
		}
		bboxPathsInvalid = false;
		return clearedBoundedPaths;
	}

	// get cleared area/poly bounded to toolbox
	Paths &GetBoundedClearedAreaClipped(const IntPoint &toolPos)
	{
		BoundBox toolBB(toolPos, toolRadiusScaled);
		if (!bboxClippedInvalid && clearedBBClippedInFocus.Contains(toolBB))
		{
			return clearedBoundedClipped;
		}
		ClipperLib::cInt delta = focusBBFactor1 * toolRadiusScaled;
		clearedBBClippedInFocus.SetFirstPoint(IntPoint(toolPos.X - delta, toolPos.Y - delta));
		clearedBBClippedInFocus.AddPoint(IntPoint(toolPos.X + delta, toolPos.Y + delta));

		// a little larger area is bounded than checked
		ClipperLib::cInt delta2 = focusBBFactor2 * toolRadiusScaled;
		Path bbPath;
		bbPath.push_back(IntPoint(toolPos.X - delta2, toolPos.Y - delta2));
		bbPath.push_back(IntPoint(toolPos.X + delta2, toolPos.Y - delta2));
		bbPath.push_back(IntPoint(toolPos.X + delta2, toolPos.Y + delta2));
		bbPath.push_back(IntPoint(toolPos.X - delta2, toolPos.Y + delta2));
		clip.Clear();
		clip.AddPath(bbPath, PolyType::ptSubject, true);
		clip.AddPaths(clearedPaths, PolyType::ptClip, true);
		clip.Execute(ClipType::ctIntersection, clearedBoundedClipped);
		bboxClippedInvalid = false;
		return clearedBoundedClipped;
	}

	// get full cleared area
	Paths &GetCleared()
	{
		return clearedPaths;
	}

  private:
	Clipper clip;
	ClipperOffset clipof;
	Paths clearedPaths;
	Paths clearedBoundedClipped;
	Paths clearedBoundedPaths;

	ClipperLib::cInt toolRadiusScaled;
	BoundBox clearedBBClippedInFocus;
	BoundBox clearedBBPathsInFocus;

	bool bboxClippedInvalid = false;
	bool bboxPathsInvalid = false;
	// size of the focus BB
	const ClipperLib::cInt focusBBFactor1 = 8;
	const ClipperLib::cInt focusBBFactor2 = 9;
};

//***************************************
// Linear Interpolation - area vs angle
//***************************************
class Interpolation
{
  public:
	const double MIN_ANGLE = -M_PI / 4;
	const double MAX_ANGLE = M_PI / 4;

	void clear()
	{
		angles.clear();
		areas.clear();
	}
	// adds point keeping the incremental order of areas for interpolation to work correctly
	void addPoint(double area, double angle)
	{
		std::size_t size = areas.size();
		if (size == 0 || area > areas[size - 1] + NTOL)
		{ // first point or largest area point
			areas.push_back(area);
			angles.push_back(angle);
			return;
		}

		for (std::size_t i = 0; i < size; i++)
		{
			if (area < areas[i] - NTOL && (i == 0 || area > areas[i - 1] + NTOL))
			{
				areas.insert(areas.begin() + i, area);
				angles.insert(angles.begin() + i, angle);
			}
		}
	}

	double interpolateAngle(double targetArea)
	{
		std::size_t size = areas.size();
		if (size < 2 || targetArea > areas[size - 1])
			return MIN_ANGLE; //max engage angle - convenient value to initially measure cut area
		if (targetArea < areas[0])
			return MAX_ANGLE; // min engage angle

		for (size_t i = 1; i < size; i++)
		{
			// find 2 subsequent points where target area is between
			if (areas[i - 1] <= targetArea && areas[i] > targetArea)
			{
				// linear interpolation
				double af = (targetArea - areas[i - 1]) / (areas[i] - areas[i - 1]);
				double a = angles[i - 1] + af * (angles[i] - angles[i - 1]);
				return a;
			}
		}
		return MIN_ANGLE;
	}

	double clampAngle(double angle)
	{
		if (angle < MIN_ANGLE)
			return MIN_ANGLE;
		if (angle > MAX_ANGLE)
			return MAX_ANGLE;
		return angle;
	}

	double getRandomAngle()
	{
		return MIN_ANGLE + (MAX_ANGLE - MIN_ANGLE) * double(rand()) / double(RAND_MAX);
	}
	size_t getPointCount()
	{
		return areas.size();
	}

  private:
	vector<double> angles;
	vector<double> areas;
};

//***************************************
// Engage Point
//***************************************

class EngagePoint
{
  public:
	struct EngageState
	{
		size_t currentPathIndex = 0;
		size_t currentSegmentIndex = 0;
		double segmentPos = 0;
		double totalDistance = 0;
		double currentPathLength = 0;
		int passes = 0;

		double metric = 0; // engage point metric

		bool operator<(const EngageState &other) const
		{
			return (metric < other.metric);
		}
	};
	EngagePoint(const Paths &p_toolBoundPaths)
	{
		SetPaths(p_toolBoundPaths);

		state.currentPathIndex = 0;
		state.currentSegmentIndex = 0;
		state.segmentPos = 0;
		state.totalDistance = 0;
		calculateCurrentPathLength();
	}

	void SetPaths(const Paths &paths)
	{
		toolBoundPaths = paths;
		state.currentPathIndex = 0;
		state.currentSegmentIndex = 0;
		state.segmentPos = 0;
		state.totalDistance = 0;
		state.passes = 0;
		calculateCurrentPathLength();
	}

	EngageState GetState()
	{
		return state;
	}

	void SetState(const EngageState &new_state)
	{
		state = new_state;
	}

	void ResetPasses()
	{
		state.passes = 0;
	}
	void moveToClosestPoint(const IntPoint &pt, double step)
	{

		Path result;
		IntPoint current = pt;
		// chain paths according to distance in between
		Paths toChain = toolBoundPaths;
		toolBoundPaths.clear();
		// if(toChain.size()>0) {
		// 	toolBoundPaths.push_back(toChain.front());
		// 	toChain.erase(toChain.begin());
		// }
		while (PopPathWithClosestPoint(toChain, current, result))
		{
			toolBoundPaths.push_back(result);
			if (!result.empty())
				current = result.back();
		}

		double minDistSq = __DBL_MAX__;
		size_t minPathIndex = state.currentPathIndex;
		size_t minSegmentIndex = state.currentSegmentIndex;
		double minSegmentPos = state.segmentPos;
		state.totalDistance = 0;
		for (;;)
		{
			while (moveForward(step))
			{
				double distSqrd = DistanceSqrd(pt, getCurrentPoint());
				if (distSqrd < minDistSq)
				{
					minDistSq = distSqrd;
					minPathIndex = state.currentPathIndex;
					minSegmentIndex = state.currentSegmentIndex;
					minSegmentPos = state.segmentPos;
				}
			}
			if (!nextPath())
				break;
		}
		state.currentPathIndex = minPathIndex;
		state.currentSegmentIndex = minSegmentIndex;
		state.segmentPos = minSegmentPos;
		calculateCurrentPathLength();
		ResetPasses();
	}
	bool nextEngagePoint(Adaptive2d *parent, ClearedArea &clearedArea, double step, double minCutArea, double maxCutArea, int maxPases = 2)
	{
		Perf_NextEngagePoint.Start();
		double prevArea = 0; // we want to make sure that we catch the point where the area is on raising slope
		IntPoint initialPoint(-1000000000, -1000000000);
		for (;;)
		{
			if (!moveForward(step))
			{
				if (!nextPath())
				{
					state.passes++;
					if (state.passes >= maxPases)
					{
						Perf_NextEngagePoint.Stop();
						return false; // nothing more to cut
					}
					prevArea = 0;
				}
			}
			IntPoint cpt = getCurrentPoint();
			double area = parent->CalcCutArea(clip, initialPoint, cpt, clearedArea);
			if (area > minCutArea && area < maxCutArea && area > prevArea)
			{
				Perf_NextEngagePoint.Stop();
				return true;
			}
			prevArea = area;
		}
	}
	IntPoint getCurrentPoint()
	{
		const Path *pth = &toolBoundPaths.at(state.currentPathIndex);
		const IntPoint *p1 = &pth->at(state.currentSegmentIndex > 0 ? state.currentSegmentIndex - 1 : pth->size() - 1);
		const IntPoint *p2 = &pth->at(state.currentSegmentIndex);
		double segLength = sqrt(DistanceSqrd(*p1, *p2));
		return IntPoint(long(p1->X + state.segmentPos * double(p2->X - p1->X) / segLength), long(p1->Y + state.segmentPos * double(p2->Y - p1->Y) / segLength));
	}

	DoublePoint getCurrentDir()
	{
		const Path *pth = &toolBoundPaths.at(state.currentPathIndex);
		const IntPoint *p1 = &pth->at(state.currentSegmentIndex > 0 ? state.currentSegmentIndex - 1 : pth->size() - 1);
		const IntPoint *p2 = &pth->at(state.currentSegmentIndex);
		double segLength = sqrt(DistanceSqrd(*p1, *p2));
		return DoublePoint(double(p2->X - p1->X) / segLength, double(p2->Y - p1->Y) / segLength);
	}

	bool moveForward(double distance)
	{
		const Path *pth = &toolBoundPaths.at(state.currentPathIndex);
		if (distance < NTOL)
			throw std::invalid_argument("distance must be positive");
		state.totalDistance += distance;
		double segmentLength = currentSegmentLength();
		while (state.segmentPos + distance > segmentLength)
		{
			state.currentSegmentIndex++;
			if (state.currentSegmentIndex >= pth->size())
			{
				state.currentSegmentIndex = 0;
			}
			distance = distance - (segmentLength - state.segmentPos);
			state.segmentPos = 0;
			segmentLength = currentSegmentLength();
		}
		state.segmentPos += distance;
		return state.totalDistance <= 1.2 * state.currentPathLength;
	}

	bool nextPath()
	{
		state.currentPathIndex++;
		state.currentSegmentIndex = 0;
		state.segmentPos = 0;
		state.totalDistance = 0;
		if (state.currentPathIndex >= toolBoundPaths.size())
		{
			state.currentPathIndex = 0;
			calculateCurrentPathLength();
			return false;
		}
		calculateCurrentPathLength();
		return true;
	}

  private:
	Paths toolBoundPaths;
	EngageState state;
	Clipper clip;
	void calculateCurrentPathLength()
	{
		const Path *pth = &toolBoundPaths.at(state.currentPathIndex);
		size_t size = pth->size();
		state.currentPathLength = 0;
		for (size_t i = 0; i < size; i++)
		{
			const IntPoint *p1 = &pth->at(i > 0 ? i - 1 : size - 1);
			const IntPoint *p2 = &pth->at(i);
			state.currentPathLength += sqrt(DistanceSqrd(*p1, *p2));
		}
	}

	double currentSegmentLength()
	{
		const Path *pth = &toolBoundPaths.at(state.currentPathIndex);
		const IntPoint *p1 = &pth->at(state.currentSegmentIndex > 0 ? state.currentSegmentIndex - 1 : pth->size() - 1);
		const IntPoint *p2 = &pth->at(state.currentSegmentIndex);
		return sqrt(DistanceSqrd(*p1, *p2));
	}
};

//***************************************
// Adaptive2d main class - implementation
//***************************************

Adaptive2d::Adaptive2d()
{
}

double Adaptive2d::CalcCutArea(Clipper &clip, const IntPoint &c1, const IntPoint &c2, ClearedArea &clearedArea, bool preventConventional)
{

	double dist = DistanceSqrd(c1, c2);
	if (dist < NTOL)
		return 0;

	Perf_CalcCutAreaCirc.Start();

	/// new alg
	double rsqrd = toolRadiusScaled * toolRadiusScaled;
	double area = 0;
	Paths interPaths;
	IntPoint clp;				// to hold closest point
	vector<DoublePoint> inters; // to hold intersection results
	BoundBox c2BB(c2, toolRadiusScaled);
	BoundBox c1BB(c1, toolRadiusScaled);
	Paths &clearedBounded = clearedArea.GetBoundedClearedAreaClipped(c2);
	for (const Path &path : clearedBounded)
	{
		size_t size = path.size();
		if (size == 0)
			continue;

		//** bound box check
		// construct bound box for path
		BoundBox pathBB(path.front());
		for (const auto &pt : path)
			pathBB.AddPoint(pt);
		if (!c2BB.CollidesWith(c2))
			continue; // this path cannot colide with tool
		//** end of BB check

		size_t curPtIndex = 0;
		bool found = false;
		// step 1: we find the starting point on the cleared path that is outside new tool shape (c2)
		for (size_t i = 0; i < size; i++)
		{
			if (DistanceSqrd(path[curPtIndex], c2) > rsqrd)
			{
				found = true;
				break;
			}
			curPtIndex++;
			if (curPtIndex >= size)
				curPtIndex = 0;
		}
		if (!found)
			continue; // try another path

		// step 2: iterate through path from starting point and find the part of the path inside the c2
		size_t prevPtIndex = curPtIndex;
		Path *interPath = NULL;
		bool prev_inside = false;
		const IntPoint *p1 = &path[prevPtIndex];
		double par; // to hold parameter output
		for (size_t i = 0; i < size; i++)
		{
			curPtIndex++;
			if (curPtIndex >= size)
				curPtIndex = 0;
			const IntPoint *p2 = &path[curPtIndex];
			BoundBox segBB(*p1, *p2);
			if (!prev_inside)
			{ // prev state: outside, find first point inside C2
				if (segBB.CollidesWith(c2BB) &&
					DistancePointToLineSegSquared(*p1, *p2, c2, clp, par) <= rsqrd)
				{ // current segment inside, start
					prev_inside = true;
					interPaths.push_back(Path());
					if (interPaths.size() > 1)
						break; // we will use poly clipping alg. if there are more intersecting paths
					interPath = &interPaths.back();
					// current segment inside c2, prev point outside, find intersection:
					if (Line2CircleIntersect(c2, toolRadiusScaled, *p1, *p2, inters))
					{
						interPath->push_back(IntPoint(long(inters[0].X), long(inters[0].Y)));
						if (inters.size() > 1)
						{
							interPath->push_back(IntPoint(long(inters[1].X), long(inters[1].Y)));
							prev_inside = false;
						}
						else
						{
							interPath->push_back(IntPoint(*p2));
						}
					}
					else
					{ // no intersection - must be edge case, add p2
						interPath->push_back(IntPoint(*p2));
					}
				}
			}
			else if (interPath != NULL)
			{ // state: inside
				if ((DistanceSqrd(c2, *p2) <= rsqrd))
				{ // next point still inside, add it and continue, no state change
					interPath->push_back(IntPoint(*p2));
				}
				else
				{ // prev point inside, current point outside, find intersection
					if (Line2CircleIntersect(c2, toolRadiusScaled, *p1, *p2, inters))
					{
						if (inters.size() > 1)
						{
							interPath->push_back(IntPoint(long(inters[1].X), long(inters[1].Y)));
						}
						else
						{
							interPath->push_back(IntPoint(long(inters[0].X), long(inters[0].Y)));
						}
					}
					prev_inside = false;
				}
			}
			prevPtIndex = curPtIndex;
			p1 = p2;
		}
		if (interPaths.size() > 1)
			break; // we will use poly clipping alg. if there are more intersecting paths with the tool (rare case)
	}
	Perf_CalcCutAreaCirc.Stop();
	if (interPaths.size() == 1 && interPaths.front().size() > 1)
	{
		Perf_CalcCutAreaCirc.Start();
		Path *interPath = &interPaths.front();
		// interPath - now contains the part of cleared path inside the C2
		size_t ipc2_size = interPath->size();
		const IntPoint &fpc2 = interPath->front(); // first point
		const IntPoint &lpc2 = interPath->back();  // last point
		// path length
		double interPathLen = 0;
		for (size_t j = 1; j < ipc2_size; j++)
			interPathLen += sqrt(DistanceSqrd(interPath->at(j - 1), interPath->at(j)));

		Paths inPaths;
		inPaths.reserve(200);
		inPaths.push_back(*interPath);
		Path pthToSubtract;
		pthToSubtract.push_back(fpc2);

		double fi1 = atan2(fpc2.Y - c2.Y, fpc2.X - c2.X);
		double fi2 = atan2(lpc2.Y - c2.Y, lpc2.X - c2.X);
		double minFi = fi1;
		double maxFi = fi2;
		if (maxFi < minFi)
			maxFi += 2 * M_PI;

		if (preventConventional && interPathLen >= RESOLUTION_FACTOR)
		{
			// detect conventional mode cut - we want only climb mode
			IntPoint midPoint(long(c2.X + toolRadiusScaled * cos(0.5 * (maxFi + minFi))), long(c2.Y + toolRadiusScaled * sin(0.5 * (maxFi + minFi))));
			if (PointSideOfLine(c1, c2, midPoint) < 0)
			{
				area = __DBL_MAX__;
				Perf_CalcCutAreaCirc.Stop();
				// #ifdef DEV_MODE
				// 	cout << "Break: @(" << double(c2.X)/scaleFactor << "," << double(c2.Y)/scaleFactor  << ") conventional mode" << endl;
				// #endif
				return area;
			}
		}

		double scanDistance = 2.5 * toolRadiusScaled;
		// stepping through path discretized to stepDistance
		double stepDistance = min(double(RESOLUTION_FACTOR), interPathLen / 24) + 1;
		const IntPoint *prevPt = &interPath->front();
		double distance = 0;
		for (size_t j = 1; j < ipc2_size; j++)
		{
			const IntPoint *cpt = &interPath->at(j);
			double segLen = sqrt(DistanceSqrd(*cpt, *prevPt));
			if (segLen < NTOL)
				continue; // skip point - segment too short
			for (double pos_unclamped = 0.0; pos_unclamped < segLen + stepDistance; pos_unclamped += stepDistance)
			{
				double pos = pos_unclamped;
				if (pos > segLen)
				{
					distance += stepDistance - (pos - segLen);
					pos = segLen; // make sure we get exact end point
				}
				else
				{
					distance += stepDistance;
				}
				double dx = double(cpt->X - prevPt->X);
				double dy = double(cpt->Y - prevPt->Y);
				IntPoint segPoint(long(prevPt->X + dx * pos / segLen), long(prevPt->Y + dy * pos / segLen));
				IntPoint scanPoint(long(c2.X + scanDistance * cos(minFi + distance * (maxFi - minFi) / interPathLen)),
								   long(c2.Y + scanDistance * sin(minFi + distance * (maxFi - minFi) / interPathLen)));

				IntPoint intersC2(segPoint.X, segPoint.Y);
				IntPoint intersC1(segPoint.X, segPoint.Y);

				// there should be intersection with C2
				if (Line2CircleIntersect(c2, toolRadiusScaled, segPoint, scanPoint, inters))
				{
					if (inters.size() > 1)
					{
						intersC2.X = long(inters[1].X);
						intersC2.Y = long(inters[1].Y);
					}
					else
					{
						intersC2.X = long(inters[0].X);
						intersC2.Y = long(inters[0].Y);
					}
				}
				else
				{
					pthToSubtract.push_back(segPoint);
				}

				if (Line2CircleIntersect(c1, toolRadiusScaled, segPoint, scanPoint, inters))
				{
					if (inters.size() > 1)
					{
						intersC1.X = long(inters[1].X);
						intersC1.Y = long(inters[1].Y);
					}
					else
					{
						intersC1.X = long(inters[0].X);
						intersC1.Y = long(inters[0].Y);
					}
					if (DistanceSqrd(segPoint, intersC2) < DistanceSqrd(segPoint, intersC1))
					{
						pthToSubtract.push_back(intersC2);
					}
					else
					{
						pthToSubtract.push_back(intersC1);
					}
				}
				else
				{ // add the segpoint if no intersection with C1
					pthToSubtract.push_back(segPoint);
				}
			}
			prevPt = cpt;
		}

		pthToSubtract.push_back(lpc2); // add last point
		pthToSubtract.push_back(c2);

		double segArea = Area(pthToSubtract);
		double A = (maxFi - minFi) * rsqrd / 2; // sector area
		area += A - fabs(segArea);
		Perf_CalcCutAreaCirc.Stop();
	}
	else if (interPaths.size() > 1)
	{
		Perf_CalcCutAreaClip.Start();
		// old way of calculating cut area based on polygon clipping
		// used in case when there are multiple intersections of tool with cleared poly (very rare case, but important)
		// 1. find difference between old and new tool shape
		Path oldTool;
		Path newTool;
		TranslatePath(toolGeometry, oldTool, c1);
		TranslatePath(toolGeometry, newTool, c2);
		clip.Clear();
		clip.AddPath(newTool, PolyType::ptSubject, true);
		clip.AddPath(oldTool, PolyType::ptClip, true);
		Paths toolDiff;
		clip.Execute(ClipType::ctDifference, toolDiff);

		// 2. difference to cleared
		clip.Clear();
		clip.AddPaths(toolDiff, PolyType::ptSubject, true);
		clip.AddPaths(clearedBounded, PolyType::ptClip, true);
		Paths cutAreaPoly;
		clip.Execute(ClipType::ctDifference, cutAreaPoly);

		// calculate resulting area
		area = 0;
		for (Path &path : cutAreaPoly)
		{
			area += fabs(Area(path));
		}
		Perf_CalcCutAreaClip.Stop();
	}
	return area;
}

void Adaptive2d::ApplyStockToLeave(Paths &inputPaths)
{
	ClipperOffset clipof;
	if (stockToLeave > NTOL)
	{
		clipof.Clear();
		clipof.AddPaths(inputPaths, JoinType::jtRound, EndType::etClosedPolygon);
		if (opType == OperationType::otClearingOutside || opType == OperationType::otProfilingOutside)
			clipof.Execute(inputPaths, stockToLeave * scaleFactor);
		else
			clipof.Execute(inputPaths, -stockToLeave * scaleFactor);
	}
	else
	{
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

std::list<AdaptiveOutput> Adaptive2d::Execute(const DPaths &stockPaths, const DPaths &paths, std::function<bool(TPaths)> progressCallbackFn)
{
	//**********************************
	// Initializations
	//**********************************

	// keep the tolerance in workable range
	if (tolerance < 0.01)
		tolerance = 0.01;
	if (tolerance > 0.2)
		tolerance = 0.2;

	scaleFactor = RESOLUTION_FACTOR / tolerance;
	long maxScaleFactor = toolDiameter<1.0 ? 10000: 1000;

	if (stepOverFactor * toolDiameter < 1.0)
		scaleFactor *= 1.0 / (stepOverFactor * toolDiameter);


	if (scaleFactor > maxScaleFactor )
		scaleFactor = maxScaleFactor;
	//scaleFactor = round(scaleFactor);

	current_region=0;
	cout << "Tool Diameter: " << toolDiameter << endl;
	cout << "Accuracy: " << round(10000.0/scaleFactor)/10 << " um" << endl;
	cout << flush;

	toolRadiusScaled = long(toolDiameter * scaleFactor / 2);
	stepOverScaled = toolRadiusScaled * stepOverFactor;
	progressCallback = &progressCallbackFn;
	lastProgressTime = clock();
	stopProcessing = false;

	if(helixRampDiameter<NTOL)
		helixRampDiameter=0.75*toolDiameter;
	if (helixRampDiameter > toolDiameter)
		helixRampDiameter = toolDiameter;
	if (helixRampDiameter < toolDiameter / 8)
		helixRampDiameter = toolDiameter / 8;

	helixRampRadiusScaled = long(helixRampDiameter * scaleFactor / 2);
	if(finishingProfile)
		finishPassOffsetScaled = long(stepOverScaled / 10);

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
	//calculate reference area
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
	cout << "optimalCutAreaPD:" << optimalCutAreaPD << " scaleFactor:" << scaleFactor << " toolRadiusScaled:" << toolRadiusScaled << " helixRampRadiusScaled:" << helixRampRadiusScaled << endl;
#endif
	//******************************
	// Convert input paths to clipper
	//******************************
	Paths converted;
	for (size_t i = 0; i < paths.size(); i++)
	{
		Path cpth;
		for (size_t j = 0; j < paths[i].size(); j++)
		{
			std::pair<double, double> pt = paths[i][j];
			cpth.push_back(IntPoint(long(pt.first * scaleFactor), long(pt.second * scaleFactor)));
		}
		Path cpth2;
		CleanPath(cpth,cpth2,FINISHING_CLEAN_PATH_TOLERANCE);
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
	for (size_t i = 0; i < stockPaths.size(); i++)
	{
		Path cpth;
		for (size_t j = 0; j < stockPaths[i].size(); j++)
		{
			std::pair<double, double> pt = stockPaths[i][j];
			cpth.push_back(IntPoint(long(pt.first * scaleFactor), long(pt.second * scaleFactor)));
		}

		stockInputPaths.push_back(cpth);
	}

	SimplifyPolygons(stockInputPaths);
	//CleanPolygons(stockInputPaths,0.707);

	//***************************************
	//	Resolve hierarchy and run processing
	//***************************************
	double cornerRoundingOffset = 0.15 * toolRadiusScaled / 2;
	if (opType == OperationType::otClearingInside || opType == OperationType::otClearingOutside)
	{

		// prepare stock boundary overshooted paths
		clipof.Clear();
		clipof.AddPaths(stockInputPaths, JoinType::jtSquare, EndType::etClosedPolygon);
		double overshootDistance = 4 * toolRadiusScaled + stockToLeave * scaleFactor;
		if (forceInsideOut)
			overshootDistance = 0;
		Paths stockOvershoot;
		clipof.Execute(stockOvershoot, overshootDistance);
		ReversePaths(stockOvershoot);

		if (opType == OperationType::otClearingOutside)
		{
			// add stock paths, with overshooting
			for (const auto& p : stockOvershoot)
				inputPaths.push_back(p);
		}
		else if (opType == OperationType::otClearingInside)
		{
			// potential TODO: check if there are open paths, and try to close it through overshooted stock boundary
		}

		clipof.Clear();
		clipof.AddPaths(inputPaths, JoinType::jtRound, EndType::etClosedPolygon);
		Paths paths;
		clipof.Execute(paths, -toolRadiusScaled - finishPassOffsetScaled - cornerRoundingOffset);
		for (const auto &current : paths)
		{
			int nesting = getPathNestingLevel(current, paths);
			if (nesting % 2 != 0 && (polyTreeNestingLimit == 0 || nesting <= polyTreeNestingLimit))
			{
				Paths toolBoundPaths;
				toolBoundPaths.push_back(current);
				if (polyTreeNestingLimit != nesting)
				{
					appendDirectChildPaths(toolBoundPaths, current, paths);
				}

				// offset back outwards - corner rounding
				clipof.Clear();
				clipof.AddPaths(toolBoundPaths, JoinType::jtRound, EndType::etClosedPolygon);
				clipof.Execute(toolBoundPaths, cornerRoundingOffset);

				// restore original bound paths
				// bounding paths - i.e. area that must be cleared inside
				// it's not the same as input paths due to filtering (nesting logic) and corner rounding
				Paths boundPaths;
				clipof.Clear();
				clipof.AddPaths(toolBoundPaths, JoinType::jtRound, EndType::etClosedPolygon);
				clipof.Execute(boundPaths, toolRadiusScaled + finishPassOffsetScaled);
				ProcessPolyNode(boundPaths, toolBoundPaths);
			}
		}
	}

	if (opType == OperationType::otProfilingInside || opType == OperationType::otProfilingOutside)
	{
		double offset = opType == OperationType::otProfilingInside ? -2 * (helixRampRadiusScaled + toolRadiusScaled) - RESOLUTION_FACTOR : 2 * (helixRampRadiusScaled + toolRadiusScaled) + RESOLUTION_FACTOR;
		for (const auto &current : inputPaths)
		{
			int nesting = getPathNestingLevel(current, inputPaths);
			if (nesting % 2 != 0 && (polyTreeNestingLimit == 0 || nesting <= polyTreeNestingLimit))
			{
				Paths profilePaths;
				profilePaths.push_back(current);
				if (polyTreeNestingLimit != nesting)
				{
					appendDirectChildPaths(profilePaths, current, inputPaths);
				}
				for (size_t i = 0; i < profilePaths.size(); i++)
				{
					double efOffset = i == 0 ? offset : -offset;
					clipof.Clear();
					clipof.AddPath(profilePaths[i], JoinType::jtSquare, EndType::etClosedPolygon);
					Paths off1;
					clipof.Execute(off1, efOffset);
					// make poly between original path and offset path
					Paths boundPaths;
					clip.Clear();
					if (efOffset < 0)
					{
						clip.AddPath(profilePaths[i], PolyType::ptSubject, true);
						clip.AddPaths(off1, PolyType::ptClip, true);
					}
					else
					{
						clip.AddPaths(off1, PolyType::ptSubject, true);
						clip.AddPath(profilePaths[i], PolyType::ptClip, true);
					}
					clip.Execute(ClipType::ctDifference, boundPaths, PolyFillType::pftEvenOdd);

					/** tool bounds */
					Paths toolBoundPaths;
					clipof.Clear();
					clipof.AddPaths(boundPaths, JoinType::jtRound, EndType::etClosedPolygon);
					clipof.Execute(toolBoundPaths, -toolRadiusScaled - finishPassOffsetScaled - cornerRoundingOffset);

					/** offset back outwards - corner rounding */
					clipof.Clear();
					clipof.AddPaths(toolBoundPaths, JoinType::jtRound, EndType::etClosedPolygon);
					clipof.Execute(toolBoundPaths, cornerRoundingOffset);

					// restore original bound paths
					// bounding paths - i.e. area that must be cleared inside
					// it's not the same as above due to corner rounding
					clipof.Clear();
					clipof.AddPaths(toolBoundPaths, JoinType::jtRound, EndType::etClosedPolygon);
					clipof.Execute(boundPaths, toolRadiusScaled + finishPassOffsetScaled);

					ProcessPolyNode(boundPaths, toolBoundPaths);
				}
			}
		}
	}
	return results;
}

bool Adaptive2d::FindEntryPoint(TPaths &progressPaths, const Paths &toolBoundPaths, const Paths &boundPaths,
								ClearedArea &clearedArea /*output-initial cleared area by helix*/,
								IntPoint &entryPoint /*output*/,
								IntPoint &toolPos, DoublePoint &toolDir)
{
	Paths incOffset;
	Paths lastValidOffset;
	Clipper clip;
	ClipperOffset clipof;
	bool found = false;
	Paths clearedPaths;
	Paths checkPaths = toolBoundPaths;
	for (int iter = 0; iter < 10; iter++)
	{
		clipof.Clear();
		clipof.AddPaths(checkPaths, JoinType::jtSquare, EndType::etClosedPolygon);
		double step = RESOLUTION_FACTOR;
		double currentDelta = -1;
		clipof.Execute(incOffset, currentDelta);
		while (!incOffset.empty())
		{
			clipof.Execute(incOffset, currentDelta);
			if (!incOffset.empty())
				lastValidOffset = incOffset;
			currentDelta -= step;
		}
		for (size_t i = 0; i < lastValidOffset.size(); i++)
		{
			if (!lastValidOffset[i].empty())
			{
				entryPoint = Compute2DPolygonCentroid(lastValidOffset[i]);
				found = true;
				break;
			}
		}
		// check if the start point is in any of the holes
		// this may happen in case when toolBoundPaths are symmetric (boundary + holes)
		// we need to break simetry and try again
		for (size_t j = 0; j < checkPaths.size(); j++)
		{
			int pip = PointInPolygon(entryPoint, checkPaths[j]);
			if ((j == 0 && pip == 0) || (j > 0 && pip != 0))
			{
				found = false;
				break;
			}
		}
		// check if helix fits
		if (found)
		{
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
			if (!crossing.empty())
			{
				// helix does not fit to the cutting area
				found = false;
			}
			else
			{
				clearedArea.SetClearedPaths(clearedPaths);
			}
		}

		if (!found)
		{ // break simetry and try again
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
		if (found)
			break;
	}

	if (!found)
		cerr << "Start point not found!" << endl;
	if (found)
	{
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
bool Adaptive2d::FindEntryPointOutside(TPaths &progressPaths, const Paths &toolBoundPaths, const Paths &boundPaths,
									   ClearedArea &clearedArea /*output-initial cleared area by helix*/,
									   IntPoint &entryPoint /*output*/,
									   IntPoint &toolPos, DoublePoint &toolDir)
{

	UNUSED(progressPaths); // to silence compiler warning
	UNUSED(boundPaths);	// to silence compiler warning

	Clipper clip;
	ClipperOffset clipof;
	Paths clearedPaths;
	// check if boundary shape to cut is outside the stock
	for (const auto &pth : toolBoundPaths)
	{
		for (size_t i = 0; i < pth.size(); i++)
		{
			IntPoint checkPoint = pth[i];
			IntPoint lastPoint = i > 0 ? pth[i - 1] : pth.back();
			// if point is outside the stock
			if (PointInPolygon(checkPoint, stockInputPaths.front()) == 0)
			{

				clipof.Clear();
				clipof.AddPaths(stockInputPaths, JoinType::jtSquare, EndType::etClosedPolygon);
				clipof.Execute(clearedPaths, 1000 * toolRadiusScaled);

				clip.Clear();
				clip.AddPaths(clearedPaths, PolyType::ptSubject, true);
				clip.AddPaths(stockInputPaths, PolyType::ptClip, true);
				clip.Execute(ClipType::ctDifference, clearedPaths);
				CleanPolygons(clearedPaths);
				SimplifyPolygons(clearedPaths);
				clearedArea.SetClearedPaths(clearedPaths);
				entryPoint = checkPoint;
				toolPos = entryPoint;
				// find tool dir
				double len = sqrt(DistanceSqrd(lastPoint, checkPoint));
				toolDir = DoublePoint((checkPoint.X - lastPoint.X) / len, (checkPoint.Y - lastPoint.Y) / len);
				return true;
			}
		}
	}
	return false;
}

//************************************************************
//  IsClearPath - returns true if path is clear from obstacles
//***********************************************************
bool Adaptive2d::IsClearPath(const Path &tp, ClearedArea &cleared, double safetyClearance)
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
	for (auto &p : crossing)
	{
		collisionArea += fabs(Area(p));
	}
	Perf_IsClearPath.Stop();
	return collisionArea < 1.0;
}

bool Adaptive2d::IsAllowedToCutTrough(const IntPoint &p1, const IntPoint &p2, ClearedArea &cleared, const Paths &toolBoundPaths, double areaFactor, bool skipBoundsCheck)
{
	Perf_IsAllowedToCutTrough.Start();

	if (!skipBoundsCheck && !IsPointWithinCutRegion(toolBoundPaths, p2))
	{
		// last point outside boundary - its not clear to cut
		Perf_IsAllowedToCutTrough.Stop();
		return false;
	}
	else if (!skipBoundsCheck && !IsPointWithinCutRegion(toolBoundPaths, p1))
	{
		// first point outside boundary - its not clear to cut
		Perf_IsAllowedToCutTrough.Stop();
		return false;
	}
	else
	{
		Clipper clip;
		double distance = sqrt(DistanceSqrd(p1, p2));
		double stepSize = min(0.5 * stepOverScaled, 8 * RESOLUTION_FACTOR);
		if (distance < stepSize / 2)
		{ // not significant cut
			Perf_IsAllowedToCutTrough.Stop();
			return true;
		}
		if (distance < stepSize)
		{ // adjust for numeric instability with small distances
			areaFactor *= 2;
		}

		IntPoint toolPos1 = p1;
		long steps = long(distance / stepSize) + 1;
		stepSize = distance / steps;
		for (long i = 1; i <= steps; i++)
		{
			double p = double(i) / steps;
			IntPoint toolPos2(long(p1.X + double(p2.X - p1.X) * p), long(p1.Y + double(p2.Y - p1.Y) * p));
			double area = CalcCutArea(clip, toolPos1, toolPos2, cleared, false);
			// if we are cutting above optimal -> not clear to cut
			if (area > areaFactor * stepSize * optimalCutAreaPD)
			{
				Perf_IsAllowedToCutTrough.Stop();
				return false;
			}
			//if tool is outside boundary -> its not clear to cut
			if (!skipBoundsCheck && !IsPointWithinCutRegion(toolBoundPaths, toolPos2))
			{
				Perf_IsAllowedToCutTrough.Stop();
				return false;
			}
			toolPos1 = toolPos2;
		}
	}
	Perf_IsAllowedToCutTrough.Stop();
	return true;
}

bool Adaptive2d::ResolveLinkPath(const IntPoint &startPoint, const IntPoint &endPoint, ClearedArea &clearedArea, Path &output)
{
	vector<pair<IntPoint, IntPoint>> queue;
	queue.emplace_back(startPoint, endPoint);
	Path checkPath;
	double totalLength = 0;
	double directDistance = sqrt(DistanceSqrd(startPoint, endPoint));
	Paths linkPaths;

	double scanStep = 2 * RESOLUTION_FACTOR;
	if (scanStep > scaleFactor * 0.1)
		scanStep = scaleFactor * 0.1;
	if (scanStep < scaleFactor * 0.01)
		scanStep = scaleFactor * 0.01;
	long limit = 10000;

	double clearance = stepOverScaled;
	double offClearance = 2 * stepOverScaled;
	if (offClearance > directDistance / 2)
	{
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

	while (!queue.empty())
	{
		if (stopProcessing)
			return false;
		if (clock() > time_out)
		{
			cout << "Unable to resolve tool down linking path (limit reached)." << endl;
			return false;
		}

		cnt++;
		if (cnt > limit)
		{
			cout << "Unable to resolve tool down linking path @(" << endPoint.X / scaleFactor << "," << endPoint.Y / scaleFactor << ") (" << limit << " points limit reached)." << endl;
			return false;
		}
		pair<IntPoint, IntPoint> pointPair = queue.back();
		queue.pop_back();

		// check for self intersections - if found discard the link path
		for (size_t i = 0; i < linkPaths.size(); i++)
		{
			if (linkPaths[i].front() != pointPair.first && linkPaths[i].back() != pointPair.first && linkPaths[i].front() != pointPair.second && linkPaths[i].back() != pointPair.second && IntersectionPoint(linkPaths[i].front(), linkPaths[i].back(), pointPair.first, pointPair.second, clp))
			{
				cout << "Unable to resolve tool down linking path (self-intersects)." << endl;
				return false;
			}
		}

		DoublePoint direction = DirectionV(pointPair.first, pointPair.second);
		checkPath.clear();
		if (pointPair.first == startPoint)
		{
			checkPath.push_back(IntPoint(pointPair.first.X + offClearance * direction.X, pointPair.first.Y + offClearance * direction.Y));
		}
		else
		{
			checkPath.push_back(pointPair.first);
		}
		if (pointPair.second == endPoint)
		{
			checkPath.push_back(IntPoint(pointPair.second.X - offClearance * direction.X, pointPair.second.Y - offClearance * direction.Y));
		}
		else
		{
			checkPath.push_back(pointPair.second);
		}

		if (IsClearPath(checkPath, clearedArea, clearance))
		{
			totalLength += sqrt(DistanceSqrd(pointPair.first, pointPair.second));
			if (totalLength > keepToolDownDistRatio * directDistance)
			{
				return false;
			}
			Path link;
			link.push_back(pointPair.first);
			link.push_back(pointPair.second);
			linkPaths.push_back(link);
		}
		else
		{
			if (sqrt(DistanceSqrd(pointPair.first, pointPair.second)) < 4)
			{
				//segment became too short but still not clear
				return false;
			}
			DoublePoint pDir(-direction.Y, direction.X);
			// find mid point
			IntPoint midPoint(0.5 * double(pointPair.first.X + pointPair.second.X), 0.5 * double(pointPair.first.Y + pointPair.second.Y));
			for (long i = 1;; i++)
			{
				if (stopProcessing)
					return false;
				double offset = i * scanStep;
				IntPoint checkPoint1(midPoint.X + offset * pDir.X, midPoint.Y + offset * pDir.Y);
				IntPoint checkPoint2(midPoint.X - offset * pDir.X, midPoint.Y - offset * pDir.Y);

				if (DistancePointToPathsSqrd(clearedArea.GetCleared(), checkPoint1, clp, pindex, sindex, par) <
					DistancePointToPathsSqrd(clearedArea.GetCleared(), checkPoint2, clp, pindex, sindex, par))
				{
					// exchange points
					IntPoint tmp = checkPoint2;
					checkPoint2 = checkPoint1;
					checkPoint1 = tmp;
				}

				checkPath.clear();
				checkPath.push_back(checkPoint1);
				if (IsClearPath(checkPath, clearedArea, clearance + 1))
				{ // check if point clear
					queue.emplace_back(pointPair.first, checkPoint1);
					queue.emplace_back(checkPoint1, pointPair.second);
					break;
				}
				else
				{ // check the other side

					checkPath.clear();
					checkPath.push_back(checkPoint2);
					if (IsClearPath(checkPath, clearedArea, clearance + 1))
					{
						queue.emplace_back(pointPair.first, checkPoint2);
						queue.emplace_back(checkPoint2, pointPair.second);
						break;
					}
				}
				if (offset > keepToolDownDistRatio * directDistance)
					return false; // can't find keep tool down link
			}
		}
	}
	if (linkPaths.empty())
		return false;
	ConnectPaths(linkPaths, linkPaths);
	output = linkPaths[0];
	return true;
}

bool Adaptive2d::MakeLeadPath(bool leadIn, const IntPoint &startPoint, const DoublePoint &startDir, const IntPoint &beaconPoint,
							  ClearedArea &clearedArea, const Paths &toolBoundPaths, Path &output)
{
	IntPoint currentPoint = startPoint;
	DoublePoint targetDir = DirectionV(currentPoint, beaconPoint);
	double distanceToBeacon = sqrt(DistanceSqrd(startPoint, beaconPoint));
	double stepSize = 0.2 * stepOverScaled + 1;
	double maxPathLen = stepOverScaled;
	DoublePoint nextDir = startDir;
	IntPoint nextPoint = IntPoint(currentPoint.X + nextDir.X * stepSize, currentPoint.Y + nextDir.Y * stepSize);
	Path checkPath;
	double adaptFactor = 0.4;
	double alfa = M_PI / 64;
	double pathLen = 0;
	checkPath.push_back(nextPoint);
	for (int i = 0; i < 10000; i++)
	{
		if (IsAllowedToCutTrough(IntPoint(currentPoint.X + RESOLUTION_FACTOR * nextDir.X, currentPoint.Y + RESOLUTION_FACTOR * nextDir.Y), nextPoint, clearedArea, toolBoundPaths))
		{
			if (output.empty())
				output.push_back(currentPoint);
			output.push_back(nextPoint);
			currentPoint = nextPoint;
			pathLen += stepSize;
			targetDir = DirectionV(currentPoint, beaconPoint);
			nextDir = DoublePoint(nextDir.X + adaptFactor * targetDir.X, nextDir.Y + adaptFactor * targetDir.Y);
			NormalizeV(nextDir);
			if (pathLen > maxPathLen)
				break;
			if (pathLen > distanceToBeacon / 2)
				break;
		}
		else
		{
			nextDir = rotate(nextDir, leadIn ? -alfa : alfa);
		}
		nextPoint = IntPoint(currentPoint.X + nextDir.X * stepSize, currentPoint.Y + nextDir.Y * stepSize);
	}
	if (output.empty())
		output.push_back(startPoint);
	return true;
}
void Adaptive2d::AppendToolPath(TPaths &progressPaths, AdaptiveOutput &output,
								const Path &passToolPath, ClearedArea &clearedBefore,
								ClearedArea &clearedAfter, const Paths &toolBoundPaths)
{
	if (passToolPath.size() < 2)
		return;
	Perf_AppendToolPath.Start();
	UNUSED(progressPaths); // to silence compiler warning,var is occasionally used in dev. for debugging

	IntPoint endPoint(passToolPath[0]);
	// if there is a previous path - need to resolve linking move to new path
	if (!output.AdaptivePaths.empty() && output.AdaptivePaths.back().second.size() > 1)
	{
		auto &lastTPath = output.AdaptivePaths.back();

		auto &lastPrevTPoint = lastTPath.second.at(lastTPath.second.size() - 2);
		auto &lastTPoint = lastTPath.second.back();

		IntPoint startPrevPoint(long(lastPrevTPoint.first * scaleFactor), long(lastPrevTPoint.second * scaleFactor));
		IntPoint startPoint(long(lastTPoint.first * scaleFactor), long(lastTPoint.second * scaleFactor));

		ClipperOffset clipof;
		//first we try to cut through the linking move for short distances
		bool linkFound = false;
		double linkDistance = sqrt(DistanceSqrd(startPoint, endPoint));
		if (linkDistance < NTOL)
			linkFound = true;

		if (!linkFound)
		{
			size_t clpPathIndex;
			size_t clpSegmentIndex;
			double clpParameter;
			IntPoint clp;

			double beaconOffset = stepOverScaled;
			if (beaconOffset > linkDistance)
			{
				beaconOffset = linkDistance;
			}

			double pathLen = PathLength(passToolPath);
			if (beaconOffset > pathLen / 2)
			{
				beaconOffset = pathLen / 2;
			}
			if (beaconOffset > linkDistance / 2)
			{
				beaconOffset = linkDistance / 2;
			}

			DistancePointToPathsSqrd(toolBoundPaths, startPoint, clp, clpPathIndex, clpSegmentIndex, clpParameter);
			DoublePoint startDir = GetPathDirectionV(toolBoundPaths[clpPathIndex], clpSegmentIndex);

			DistancePointToPathsSqrd(toolBoundPaths, endPoint, clp, clpPathIndex, clpSegmentIndex, clpParameter);
			DoublePoint endDir = GetPathDirectionV(toolBoundPaths[clpPathIndex], clpSegmentIndex);

			IntPoint startBeacon(startPoint.X - beaconOffset * (startDir.Y - startDir.X), startPoint.Y + beaconOffset * (startDir.X + startDir.Y));
			IntPoint endBeacon(endPoint.X - beaconOffset * (endDir.X + endDir.Y), endPoint.Y + beaconOffset * (endDir.X - endDir.Y));
			Path leadOutPath;
			MakeLeadPath(false, startPoint, startDir, startBeacon, clearedBefore, toolBoundPaths, leadOutPath);

			Path leadInPath;
			MakeLeadPath(true, endPoint, DoublePoint(-endDir.X, -endDir.Y), endBeacon, clearedBefore, toolBoundPaths, leadInPath);
			ReversePath(leadInPath);

			Path linkPath;
			MotionType linkType = MotionType::mtCutting;

			// this is not needed:
			//clearedBefore.ExpandCleared(leadInPath);
			//clearedBefore.ExpandCleared(leadOutPath);

			if (ResolveLinkPath(leadOutPath.back(), leadInPath.front(), clearedBefore, linkPath))
			{
				linkType = MotionType::mtLinkClear;
				double remainingLeadInExtension = stepOverScaled / 2;
				while (linkPath.size() >= 2 && remainingLeadInExtension > NTOL)
				{
					IntPoint p1 = linkPath.at(linkPath.size() - 2);
					IntPoint p2 = linkPath.at(linkPath.size() - 1);
					double l = sqrt(DistanceSqrd(p1, p2));
					if (l >= remainingLeadInExtension)
					{
						IntPoint splitPoint(p1.X + (p2.X - p1.X) * (l - remainingLeadInExtension) / l, p1.Y + (p2.Y - p1.Y) * (l - remainingLeadInExtension) / l);
						linkPath.pop_back();
						linkPath.push_back(splitPoint);
						leadInPath.insert(leadInPath.begin(), splitPoint);
						remainingLeadInExtension = 0;
						Path checkPath;
						checkPath.push_back(p2);
						checkPath.push_back(splitPoint);
						if (!IsClearPath(checkPath, clearedBefore, 0))
						{
							remainingLeadInExtension = stepOverScaled / 2;
						}
					}
					else
					{
						linkPath.pop_back();
						leadInPath.insert(leadInPath.begin(), p1);
						remainingLeadInExtension -= l;
						if (remainingLeadInExtension < NTOL)
						{
							Path checkPath;
							checkPath.push_back(p2);
							checkPath.push_back(p1);
							if (!IsClearPath(checkPath, clearedBefore, 0))
							{
								remainingLeadInExtension = stepOverScaled / 2;
							}
						}
					}
				}
			}
			else
			{
				linkType = MotionType::mtLinkNotClear;
				double dist = sqrt(DistanceSqrd(leadOutPath.back(), leadInPath.front()));
				if (dist < 2 * stepOverScaled && IsAllowedToCutTrough(
													 IntPoint(leadOutPath.back().X + (leadInPath.front().X - leadOutPath.back().X) / dist, leadOutPath.back().Y + (leadInPath.front().Y - leadOutPath.back().Y) / dist),
													 IntPoint(leadInPath.front().X - (leadInPath.front().X - leadOutPath.back().X) / dist, leadInPath.front().Y - (leadInPath.front().Y - leadOutPath.back().Y) / dist),
													 clearedBefore, toolBoundPaths))
					linkType = MotionType::mtCutting;
				// add direct linking move at clear height

				linkPath.clear();
				linkPath.push_back(leadOutPath.back());
				linkPath.push_back(leadInPath.front());
			}

			/* paths smoothing*/
			Paths linkPaths;
			linkPaths.push_back(leadOutPath);
			linkPaths.push_back(linkPath);
			linkPaths.push_back(leadInPath);

			if (linkType == MotionType::mtLinkClear)
			{
				SmoothPaths(linkPaths, 0.1*stepOverScaled,1,4);
			}

			leadOutPath = linkPaths[0];
			linkPath = linkPaths[1];
			leadInPath = linkPaths[2];

			// add lead-out move
			TPath linkPath1;
			linkPath1.first = MotionType::mtCutting;
			for (const auto &pt : leadOutPath)
			{
				linkPath1.second.emplace_back(double(pt.X) / scaleFactor, double(pt.Y) / scaleFactor);
			}
			output.AdaptivePaths.push_back(linkPath1);

			// add linking path
			TPath linkPath2;
			linkPath2.first = linkType;
			for (const auto &pt : linkPath)
			{
				linkPath2.second.emplace_back(double(pt.X) / scaleFactor, double(pt.Y) / scaleFactor);
			}
			output.AdaptivePaths.push_back(linkPath2);

			// add lead-in move
			TPath linkPath3;
			linkPath3.first = MotionType::mtCutting;
			for (const auto &pt : leadInPath)
			{
				linkPath3.second.emplace_back(double(pt.X) / scaleFactor, double(pt.Y) / scaleFactor);
			}

			output.AdaptivePaths.push_back(linkPath3);

			clearedAfter.ExpandCleared(leadInPath);
			clearedAfter.ExpandCleared(leadOutPath);

			linkFound = true;
		}
		if (!linkFound)
		{ // nothing clear so far - check direct link with no interim points - either this is clear or we need to raise the tool
			Path tp;
			tp << startPoint;
			tp << endPoint;
			MotionType mt = IsClearPath(tp, clearedBefore) ? MotionType::mtLinkClear : MotionType::mtLinkNotClear;

			// make cutting move through small clear links
			if (mt == MotionType::mtLinkClear && linkDistance < toolRadiusScaled)
			{
				mt = MotionType::mtCutting;
				clearedAfter.ExpandCleared(tp);
			}

			TPath linkPath;
			linkPath.first = mt;
			linkPath.second.emplace_back(double(startPoint.X) / scaleFactor, double(startPoint.Y) / scaleFactor);
			linkPath.second.emplace_back(double(endPoint.X) / scaleFactor, double(endPoint.Y) / scaleFactor);
			output.AdaptivePaths.push_back(linkPath);
		}
	}
	TPath cutPath;
	cutPath.first = MotionType::mtCutting;
	for (const auto &p : passToolPath)
	{
		DPoint nextT;
		nextT.first = double(p.X) / scaleFactor;
		nextT.second = double(p.Y) / scaleFactor;
		cutPath.second.push_back(nextT);
	}

	if (!cutPath.second.empty())
		output.AdaptivePaths.push_back(cutPath);
	Perf_AppendToolPath.Stop();
}

void Adaptive2d::CheckReportProgress(TPaths &progressPaths, bool force)
{
	if (!force && (clock() - lastProgressTime < PROGRESS_TICKS))
		return; // not yet
	lastProgressTime = clock();
	if (progressPaths.empty())
		return;
	if (progressCallback)
		if ((*progressCallback)(progressPaths))
			stopProcessing = true; // call python function, if returns true signal stop processing
	// clean the paths - keep the last point
	if (progressPaths.back().second.empty())
		return;
	TPath *lastPath = &progressPaths.back();
	DPoint *lastPoint = &lastPath->second.back();
	DPoint next(lastPoint->first, lastPoint->second);
	while (progressPaths.size() > 1)
		progressPaths.pop_back();
	while (!progressPaths.front().second.empty())
		progressPaths.front().second.pop_back();
	progressPaths.front().first = MotionType::mtCutting;
	progressPaths.front().second.push_back(next);
}

void Adaptive2d::AddPathsToProgress(TPaths &progressPaths, Paths paths, MotionType mt)
{
	for (const auto &pth : paths)
	{
		if (!pth.empty())
		{
			progressPaths.push_back(TPath());
			progressPaths.back().first = mt;
			for (const auto pt : pth)
				progressPaths.back().second.emplace_back(double(pt.X) / scaleFactor, double(pt.Y) / scaleFactor);
			progressPaths.back().second.emplace_back(double(pth.front().X) / scaleFactor, double(pth.front().Y) / scaleFactor);
		}
	}
}

void Adaptive2d::AddPathToProgress(TPaths &progressPaths, const Path pth, MotionType mt)
{
	if (!pth.empty())
	{
		progressPaths.push_back(TPath());
		progressPaths.back().first = mt;
		for (const auto pt : pth)
			progressPaths.back().second.emplace_back(double(pt.X) / scaleFactor, double(pt.Y) / scaleFactor);
	}
}

void Adaptive2d::ProcessPolyNode(Paths boundPaths, Paths toolBoundPaths)
{
	Perf_ProcessPolyNode.Start();
	current_region++;
	cout << "** Processing region: " << current_region << endl;

	// node paths are already constrained to tool boundary path for adaptive path before finishing pass
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
	ClearedArea cleared(toolRadiusScaled);
	bool outsideEntry = false;
	bool firstEngagePoint = true;
	Paths engageBounds = toolBoundPaths;

	if (!forceInsideOut && FindEntryPointOutside(progressPaths, toolBoundPaths, boundPaths, cleared, entryPoint, toolPos, toolDir))
	{
		if (!Orientation(engageBounds[0]))
			ReversePath(engageBounds[0]);
		// add initial offset of cleared area to engage paths
		Paths outsideEngage;
		clipof.Clear();
		clipof.AddPaths(stockInputPaths, JoinType::jtRound, EndType::etClosedPolygon);
		clipof.Execute(outsideEngage, toolRadiusScaled - stepOverFactor * toolRadiusScaled);
		CleanPolygons(outsideEngage);
		ReversePaths(outsideEngage);
		for (const auto& p : outsideEngage)
			engageBounds.push_back(p);
		outsideEntry = true;
	}
	else
	{
		if (!FindEntryPoint(progressPaths, toolBoundPaths, boundPaths, cleared, entryPoint, toolPos, toolDir))
		{
			Perf_ProcessPolyNode.Stop();
			return;
		}
	}

	EngagePoint engage(engageBounds); // engage point stepping instance

	if (outsideEntry)
	{
		engage.moveToClosestPoint(toolPos, 2 * RESOLUTION_FACTOR);
		engage.moveForward(RESOLUTION_FACTOR);
		toolPos = engage.getCurrentPoint();
		toolDir = engage.getCurrentDir();
		entryPoint = toolPos;
	}

	//cout << "Entry point:" << double(entryPoint.X)/scaleFactor << "," << double(entryPoint.Y)/scaleFactor << endl;

	AdaptiveOutput output;
	output.HelixCenterPoint.first = double(entryPoint.X) / scaleFactor;
	output.HelixCenterPoint.second = double(entryPoint.Y) / scaleFactor;

	long stepScaled = long(RESOLUTION_FACTOR);
	IntPoint engagePoint;

	IntPoint newToolPos;
	DoublePoint newToolDir;

	CheckReportProgress(progressPaths, true);

	IntPoint startPoint = toolPos;
	output.StartPoint = DPoint(double(startPoint.X) / scaleFactor, double(startPoint.Y) / scaleFactor);

	Path passToolPath; // to store pass toolpath
	Path toClearPath;
	IntPoint clp;				 // to store closest point
	vector<DoublePoint> gyro;	// used to average tool direction
	vector<double> angleHistory; // use to predict deflection angle
	double angle = M_PI;
	engagePoint = toolPos;
	Interpolation interp; // interpolation instance

	long total_iterations = 0;
	long total_points = 0;
	long total_exceeded = 0;
	long total_output_points = 0;
	long over_cut_count = 0;
	long bad_engage_count = 0;
	double prevDistFromStart = 0;
	double refinement_factor = 1;
	bool prevDistTrend = false;

	double perf_total_len = 0;
#ifdef DEV_MODE
	clock_t start_clock = clock();
#endif
	ClearedArea clearedBeforePass(toolRadiusScaled);
	clearedBeforePass.SetClearedPaths(cleared.GetCleared());

	//*******************************
	// LOOP - PASSES
	//*******************************
	for (long pass = 0; pass < PASSES_LIMIT; pass++)
	{
		if (stopProcessing)
			break;

		passToolPath.clear();
		toClearPath.clear();
		angleHistory.clear();

		// append a new path to progress info paths
		if (progressPaths.empty())
		{
			progressPaths.push_back(TPath());
		}
		else
		{
			// append new path if previous not empty
			if (!progressPaths.back().second.empty())
				progressPaths.push_back(TPath());
		}

		angle = M_PI / 4; // initial pass angle
		bool recalcArea = false;
		double cumulativeCutArea = 0;
		// init gyro
		gyro.clear();
		for (int i = 0; i < DIRECTION_SMOOTHING_BUFLEN; i++)
			gyro.push_back(toolDir);

		size_t clpPathIndex;
		size_t clpSegmentIndex;
		double clpParameter;
		double passLength = 0;
		double noCutDistance=0;
		clearedBeforePass.SetClearedPaths(cleared.GetCleared());
		//*******************************
		// LOOP - POINTS
		//*******************************
		for (long point_index = 0; point_index < POINTS_PER_PASS_LIMIT; point_index++)
		{
			if (stopProcessing)
				break;

			total_points++;
			AverageDirection(gyro, toolDir);
			Perf_DistanceToBoundary.Start();

			double distanceToBoundary = sqrt(DistancePointToPathsSqrd(toolBoundPaths, toolPos, clp, clpPathIndex, clpSegmentIndex, clpParameter));
			DoublePoint boundaryDir = GetPathDirectionV(toolBoundPaths[clpPathIndex], clpSegmentIndex);
			double distBoundaryPointToEngage = sqrt(DistanceSqrd(clp, engagePoint));

			Perf_DistanceToBoundary.Stop();
			double distanceToEngage = sqrt(DistanceSqrd(toolPos, engagePoint));

			double targetAreaPD = optimalCutAreaPD;

			// set the step size
			double slowDownDistance = max(double(toolRadiusScaled) / 4, RESOLUTION_FACTOR * 8);
			if (distanceToBoundary < slowDownDistance || distanceToEngage < slowDownDistance)
			{
				stepScaled = long(RESOLUTION_FACTOR);
			}
			else if (fabs(angle) > NTOL)
			{
				stepScaled = long(RESOLUTION_FACTOR / fabs(angle));
			}
			else
			{
				stepScaled = long(RESOLUTION_FACTOR * 4);
			}

			// clamp the step size - for stability
			if (stepScaled > min(long(toolRadiusScaled / 4), long(RESOLUTION_FACTOR * 8)))
				stepScaled = min(long(toolRadiusScaled / 4), long(RESOLUTION_FACTOR * 8));
			if (stepScaled < RESOLUTION_FACTOR)
				stepScaled = long(RESOLUTION_FACTOR);

			//*****************************
			// ANGLE vs AREA ITERATIONS
			//*****************************
			double predictedAngle = averageDV(angleHistory);
			double maxError = AREA_ERROR_FACTOR * optimalCutAreaPD;
			double area = 0;
			double areaPD = 0;
			interp.clear();
			/******************************/
			Perf_PointIterations.Start();
			int iteration;
			double prev_error = __DBL_MAX__;
			for (iteration = 0; iteration < MAX_ITERATIONS; iteration++)
			{
				total_iterations++;
				if (iteration == 0)
					angle = predictedAngle;
				else if (iteration == 1)
					angle = interp.MIN_ANGLE; // max engage
				else if (iteration == 3)
					angle = interp.MAX_ANGLE; // min engage
				else if (interp.getPointCount() < 2)
					angle = interp.getRandomAngle();
				else
					angle = interp.interpolateAngle(targetAreaPD);
				angle = interp.clampAngle(angle);

				newToolDir = rotate(toolDir, angle);
				newToolPos = IntPoint(long(toolPos.X + newToolDir.X * stepScaled), long(toolPos.Y + newToolDir.Y * stepScaled));

				area = CalcCutArea(clip, toolPos, newToolPos, cleared);

				areaPD = area / double(stepScaled); // area per distance
				interp.addPoint(areaPD, angle);
				double error = areaPD - targetAreaPD;
				//	cout << " iter:" << iteration << " angle:" << angle << " area:" << areaPD << " target:" << targetAreaPD << " error:" << error << " max:"<< maxError << endl;
				if (fabs(error) < maxError)
				{
					angleHistory.push_back(angle);
					if (angleHistory.size() > ANGLE_HISTORY_POINTS)
						angleHistory.erase(angleHistory.begin());
					break;
				}
				if (iteration > 5 && fabs(error - prev_error) < 0.001)
					break;
				if (iteration == MAX_ITERATIONS - 1)
					total_exceeded++;
				prev_error = error;
			}
			Perf_PointIterations.Stop();

			recalcArea = false;
			// approach end boundary tangentially
			double relDistToBoundary = 4 * distanceToBoundary / stepOverScaled;
			if (relDistToBoundary <= 1.0 && passLength > 2 * stepOverFactor && distanceToEngage > 2 * stepOverScaled && distBoundaryPointToEngage > 2 * stepOverScaled)
			{
				double wb = 1 - relDistToBoundary;
				newToolDir = DoublePoint(newToolDir.X + wb * boundaryDir.X, newToolDir.Y + wb * boundaryDir.Y);
				NormalizeV(newToolDir);
				newToolPos = IntPoint(long(toolPos.X + newToolDir.X * stepScaled), long(toolPos.Y + newToolDir.Y * stepScaled));
				recalcArea = true;
			}

			//**********************************************
			// CHECK AND RECORD NEW TOOL POS
			//**********************************************
			long rotateStep = 0;
			while (!IsPointWithinCutRegion(toolBoundPaths, newToolPos) && rotateStep < 180)
			{
				rotateStep++;
				// if new tool pos. outside boundary rotate until back in
				recalcArea = true;
				newToolDir = rotate(newToolDir, M_PI / 90);
				newToolPos = IntPoint(long(toolPos.X + newToolDir.X * stepScaled), long(toolPos.Y + newToolDir.Y * stepScaled));
			}
			if (rotateStep >= 180)
			{
				#ifdef DEV_MODE
					cerr << "Warning: unexpected number of rotate iterations." << endl;
				#endif
				break;
			}

			if (recalcArea)
			{
				area = CalcCutArea(clip, toolPos, newToolPos, cleared);
			}

			// safety condition
			if (area > stepScaled * optimalCutAreaPD && areaPD > 2 * optimalCutAreaPD)
			{
				over_cut_count++;
				break;
			}

			// update cleared paths when trend of distance from start point changes sign (starts to get closer, or start to get farther)
			double distFromStart = sqrt(DistanceSqrd(toolPos, startPoint));
			bool distanceTrend = distFromStart > prevDistFromStart ? true : false;

			if (distanceTrend != prevDistTrend)
			{
				cleared.ExpandCleared(toClearPath);
				toClearPath.clear();
			}
			prevDistTrend = distanceTrend;
			prevDistFromStart = distFromStart;

			if (area > 0.5 * MIN_CUT_AREA_FACTOR * optimalCutAreaPD * RESOLUTION_FACTOR)
			{ // cut is ok - record it
				noCutDistance=0;
				if (toClearPath.empty())
					toClearPath.push_back(toolPos);
				toClearPath.push_back(newToolPos);

				cumulativeCutArea += area;

				// append to toolpaths
				if (passToolPath.empty())
				{
					// in outside entry first successful cut defines the "helix center" and start point
					// in this case helix diameter is 0 (straight line downwards)
					if (output.AdaptivePaths.empty() && outsideEntry)
					{
						entryPoint = toolPos;
						output.HelixCenterPoint.first = double(entryPoint.X) / scaleFactor;
						output.HelixCenterPoint.second = double(entryPoint.Y) / scaleFactor;
						output.StartPoint = DPoint(double(entryPoint.X) / scaleFactor, double(entryPoint.Y) / scaleFactor);
					}
					passToolPath.push_back(toolPos);
				}
				passToolPath.push_back(newToolPos);
				perf_total_len += stepScaled;
				passLength += stepScaled;
				toolPos = newToolPos;

				// append to progress info paths
				if (progressPaths.empty())
					progressPaths.push_back(TPath());
				progressPaths.back().second.emplace_back(double(newToolPos.X) / scaleFactor, double(newToolPos.Y) / scaleFactor);

				// append gyro
				gyro.push_back(newToolDir);
				gyro.erase(gyro.begin());
				CheckReportProgress(progressPaths);
			}
			else
			{
#ifdef DEV_MODE
				// if(point_index==0) {
				// 	engage_no_cut_count++;
				// 	cout<<"Break:no cut #" << engage_no_cut_count << ", bad engage, pass:" << pass << " over_cut_count:" << over_cut_count << endl;
				// }
#endif
				//cout<<"Break: no cut @" << point_index << endl;
				if(noCutDistance>stepOverScaled) break;
				noCutDistance+=stepScaled;
			}
		} /* end of points loop*/

		if (!toClearPath.empty())
		{
			cleared.ExpandCleared(toClearPath);
			toClearPath.clear();
		}
		if (cumulativeCutArea > MIN_CUT_AREA_FACTOR * optimalCutAreaPD * RESOLUTION_FACTOR)
		{
			Path cleaned;
			CleanPath(passToolPath, cleaned, CLEAN_PATH_TOLERANCE);
			total_output_points += long(cleaned.size());
			AppendToolPath(progressPaths, output, cleaned, clearedBeforePass, cleared, toolBoundPaths);
			CheckReportProgress(progressPaths);
			bad_engage_count = 0;
			engage.ResetPasses();
		}
		else
		{
			bad_engage_count++;
		}

		if (bad_engage_count > 10000)
		{
			cerr << "Break (next valid engage point not found)." << endl;
			break;
		}

		/*****NEXT ENGAGE POINT******/
		if (firstEngagePoint)
		{
			engage.moveToClosestPoint(newToolPos, stepScaled + 1);
			firstEngagePoint = false;
		}
		else
		{
			double moveDistance = ENGAGE_SCAN_DISTANCE_FACTOR * stepOverScaled * refinement_factor;

			if (!engage.nextEngagePoint(this, cleared, moveDistance,
										ENGAGE_AREA_THR_FACTOR * optimalCutAreaPD * RESOLUTION_FACTOR,
										4 * referenceCutArea * stepOverFactor))
			{
				// check if there are any uncleared area left
				Paths remaining;
				for (const auto &p : cleared.GetCleared())
				{
					if (!p.empty() && IsPointWithinCutRegion(toolBoundPaths, p.front()) && DistancePointToPathsSqrd(boundPaths, p.front(), clp, clpPathIndex, clpSegmentIndex, clpParameter) > 4 * toolRadiusScaled * toolRadiusScaled)
					{
						remaining.push_back(p);
					}
				};
				if (remaining.empty())
				{
					cout << "All cleared." << endl;
					break;
				}
				else
				{
					cout << "Clearing " << remaining.size() << " remaining internal path(s)." << endl;
				}

				// try to find new engage point along the remaining
				clipof.Clear();
				clipof.AddPaths(remaining, JoinType::jtRound, EndType::etClosedPolygon);
				clipof.Execute(remaining, toolRadiusScaled - 0.5 * stepOverScaled);

				ReversePaths(remaining);
				engage.SetPaths(remaining);
				engage.moveToClosestPoint(newToolPos, stepScaled + 1);
				if (!engage.nextEngagePoint(this, cleared, moveDistance,
											ENGAGE_AREA_THR_FACTOR * optimalCutAreaPD * RESOLUTION_FACTOR,
											4 * referenceCutArea * stepOverFactor))
					break;
			}
		}
		toolPos = engage.getCurrentPoint();
		toolDir = engage.getCurrentDir();
	}

	//**********************************
	//*  FINISHING PASS                *
	//**********************************
	if(finishingProfile) {
		Paths finishingPaths;
		clipof.Clear();
		clipof.AddPaths(boundPaths, JoinType::jtRound, EndType::etClosedPolygon);
		clipof.Execute(finishingPaths, -toolRadiusScaled);

		clipof.Clear();
		clipof.AddPaths(finishingPaths, JoinType::jtRound, EndType::etClosedPolygon);
		clipof.Execute(toolBoundPaths, -1);

		IntPoint lastPoint = toolPos;
		Path finShiftedPath;

		bool allCutsAllowed = true;
		while(!stopProcessing && PopPathWithClosestPoint(finishingPaths, lastPoint, finShiftedPath)) {
			if(finShiftedPath.empty())
				continue;
			// skip finishing passes outside the stock boundary - no sense to cut where is no material
			bool allPointsOutside = true;
			IntPoint p1 = finShiftedPath.front();
			for(const auto& pt : finShiftedPath) {

				// midpoint
				if(IsPointWithinCutRegion(stockInputPaths, IntPoint((p1.X + pt.X) / 2, (p1.Y + pt.Y) / 2))) {
					allPointsOutside = false;
					break;
				}
				//current point
				if(IsPointWithinCutRegion(stockInputPaths, pt)) {
					allPointsOutside = false;
					break;
				}

				p1 = pt;
			}
			if(allPointsOutside)
				continue;

			progressPaths.push_back(TPath());
			// show in progress cb
			for(auto& pt : finShiftedPath) {
				progressPaths.back().second.emplace_back(double(pt.X) / scaleFactor, double(pt.Y) / scaleFactor);
			}

			if(!finShiftedPath.empty())
				finShiftedPath << finShiftedPath.front(); // make sure its closed

			Path finCleaned;
			CleanPath(finShiftedPath, finCleaned, FINISHING_CLEAN_PATH_TOLERANCE);

			// sanity check for finishing paths - check the area of finishing cut
			for(size_t i = 1; i < finCleaned.size(); i++) {
				if(!IsAllowedToCutTrough(finCleaned.at(i - 1), finCleaned.at(i), cleared, toolBoundPaths, 2.0, true)) {
					allCutsAllowed = false;
				}
			}

			// make sure it's closed
			finCleaned.push_back(finCleaned.front());
			AppendToolPath(progressPaths, output, finCleaned, cleared, cleared, toolBoundPaths);

			cleared.ExpandCleared(finCleaned);

			if(!finCleaned.empty()) {
				lastPoint.X = finCleaned.back().X;
				lastPoint.Y = finCleaned.back().Y;
			}
		}

		Path returnPath;
		returnPath << lastPoint;
		returnPath << entryPoint;
		output.ReturnMotionType = IsClearPath(returnPath, cleared) ? MotionType::mtLinkClear : MotionType::mtLinkNotClear;

		// dump performance results
#ifdef DEV_MODE
		Perf_ProcessPolyNode.Stop();
		Perf_ProcessPolyNode.DumpResults();
		Perf_PointIterations.DumpResults();
		Perf_CalcCutAreaCirc.DumpResults();
		Perf_CalcCutAreaClip.DumpResults();
		Perf_NextEngagePoint.DumpResults();
		Perf_ExpandCleared.DumpResults();
		Perf_DistanceToBoundary.DumpResults();
		Perf_AppendToolPath.DumpResults();
		Perf_IsAllowedToCutTrough.DumpResults();
		Perf_IsClearPath.DumpResults();
#endif
		CheckReportProgress(progressPaths, true);
#ifdef DEV_MODE
		double duration = ((double) (clock() - start_clock)) / CLOCKS_PER_SEC;
		cout << "PolyNode perf:" << perf_total_len / double(scaleFactor) / duration << " mm/sec"
			<< " processed_points:" << total_points
			<< " output_points:" << total_output_points
			<< " total_iterations:" << total_iterations
			<< " iter_per_point:" << (double(total_iterations) / ((double(total_points) + 0.001)))
			<< " total_exceeded:" << total_exceeded << " (" << 100 * double(total_exceeded) / double(total_points) << "%)"
			<< endl;
#else
		(void)total_output_points;
		(void)over_cut_count;
		(void)total_exceeded;
		(void)total_points;
		(void)total_iterations;
		(void)perf_total_len;
#endif

		// warn about invalid paths being detected
		if(!allCutsAllowed) {
			cerr << "Warning: some cuts may be above optimal step-over. Please double check the results." << endl
				<< "Hint: try to modify accuracy and/or step-over." << endl;
		}
	}
	results.push_back(output);
}

} // namespace AdaptivePath
