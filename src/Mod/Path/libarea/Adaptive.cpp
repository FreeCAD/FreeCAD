/**************************************************************************
*   Copyright (c) Kresimir Tusek         (kresimir.tusek@gmail.com) 2018  *
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
#include <chrono>
#include <thread>

namespace ClipperLib {
	 void TranslatePath(const Path& input, Path& output, IntPoint delta);
}

namespace AdaptivePath {
	using namespace ClipperLib;
	using namespace std;
	#define SAME_POINT_TOL_SQRD_SCALED 16.0
	#define UNUSED(expr) (void)(expr)
	/*********************************************
	 * Utils - inline
	 ***********************************************/

	inline double DistanceSqrd(const IntPoint& pt1, const IntPoint& pt2)
	{
	  double Dx = ((double)pt1.X - pt2.X);
	  double dy = ((double)pt1.Y - pt2.Y);
	  return (Dx*Dx + dy*dy);
	}

	inline bool SetSegmentLength(const IntPoint& pt1, IntPoint& pt2, double new_length)
	{
	  double Dx = ((double)pt2.X - pt1.X);
	  double dy = ((double)pt2.Y - pt1.Y);
	  double l=sqrt(Dx*Dx + dy*dy);
	  if(l>0.0) {
	  	pt2.X = long( pt1.X + new_length * Dx/l);
	  	pt2.Y = long(pt1.Y + new_length * dy/l);	
		return true;
	  }
	  return false;
	}

	int getPathNestingLevel(const Path & path, const Paths & paths) {
		int nesting = 0;
		for(const auto & other : paths) {
			if(path.size() >0 && PointInPolygon(path.front(),other)!=0) nesting++;
		}
		return nesting;
	}
	void apendDirectChildPaths(Paths & outPaths, const Path &path, const Paths &paths ) {
		int nesting = getPathNestingLevel(path,paths);
		for(const auto & other : paths) {
			if(path.size()>0 && other.size()>0 && PointInPolygon(other.front(),path)!=0) {
				if(getPathNestingLevel(other,paths)==nesting+1) outPaths.push_back(other);
			}
		}
	}


	inline bool HasAnyPath(const Paths &paths) {
		for(Paths::size_type i=0;i<paths.size();i++) {
			if(paths[i].size()>0) return true;
		}
		return false;
	}

	inline double averageDV(const vector<double> & vec) {
		double s=0;
		std::size_t size = vec.size();
		if(size==0) return 0;
		for(std::size_t i=0;i<size;i++) s+=vec[i];
		return s/double(size);
	}

	inline DoublePoint rotate(const DoublePoint &in, double rad) {
	   double c =cos(rad);
       double s =sin(rad);
	   return DoublePoint(c*in.X-s*in.Y,s*in.X + c*in.Y);
	}

	// calculates path length for open path
	inline double PathLength(const Path & path) {
		double len=0;
		if(path.size()<2) return len;
		for(size_t i=1;i<path.size();i++) {
			len+=sqrt(DistanceSqrd(path[i-1],path[i]));
		}
		return len;
	}

	inline double PointSideOfLine(const IntPoint& p1, const IntPoint& p2,const IntPoint& pt) {
		return double((pt.X - p1.X)*(p2.Y-p1.Y) - (pt.Y - p2.Y)*(p2.X-p1.X));
	}

	inline double Angle3Points(const DoublePoint & p1,const DoublePoint& p2, const DoublePoint& p3) {
		  double t1= atan2(p1.Y-p2.Y,p1.X-p2.X);
    	  double t2=atan2(p3.Y-p2.Y,p3.X-p2.X);
		  double  a = fabs( t2 - t1 );
    	  return min(a,2*M_PI-a);
	}

	/*********************************************
	 * Utils
	 ***********************************************/


	void AverageDirection(const vector<DoublePoint> &unityVectors, DoublePoint& output) {
		std::size_t size=unityVectors.size();
		output.X =0;
		output.Y=0;
		// sum vectors
		for(std::size_t i=0;i<size;i++) {
			DoublePoint v= unityVectors[i];
			output.X += v.X;
			output.Y += v.Y;
		}
		// normalize
		double magnitude = sqrt(output.X*output.X + output.Y*output.Y);
		output.X/=magnitude;
		output.Y/=magnitude;
	}

	 double DistancePointToLineSegSquared(const IntPoint& p1, const IntPoint& p2,const IntPoint& pt,
	 					IntPoint &closestPoint, double & ptParameter, bool clamp=true) {
		double D21X=double(p2.X-p1.X);
		double D21Y=double(p2.Y-p1.Y);
		double DP1X=double(pt.X-p1.X);
		double DP1Y=double(pt.Y-p1.Y);
		double lsegLenSqr = D21X*D21X + D21Y*D21Y;
		if (lsegLenSqr==0) { // segment is zero length, return point to point distance
			closestPoint=p1;
			ptParameter=0;
			return DP1X*DP1X+DP1Y*DP1Y;
		}
		double parameter = DP1X*D21X + DP1Y*D21Y;
		if(clamp) {
			// clamp the parameter
			if(parameter<0) parameter=0;
			else if(parameter>lsegLenSqr) parameter=lsegLenSqr;
		}
		// point on line at parameter
		ptParameter=parameter/lsegLenSqr;
		closestPoint.X = long(p1.X + ptParameter*D21X);
		closestPoint.Y = long(p1.Y + ptParameter*D21Y);
		// calculate distance from point on line to pt
		double DX=double(pt.X-closestPoint.X);
		double DY=double(pt.Y-closestPoint.Y);
		return DX*DX+DY*DY; // return distance squared
	}

	// joins collinear segments (within the tolerance)
	void CleanPath(const Path &inp, Path &outp, double tolerance) {
		bool first=true;
		outp.clear();
		for(const auto & pt : inp) {
			if(first) {
				first=false;
				outp.push_back(pt);
			} else {
				if(outp.size()>2) {
					IntPoint clp; // to hold closest point
					double ptPar;
					double distSqrd = DistancePointToLineSegSquared(outp[outp.size()-2],outp[outp.size()-1],pt,clp,ptPar,false);
					if(sqrt(distSqrd)<tolerance) {
						outp.pop_back();
						outp.push_back(pt);
					} else {
						outp.push_back(pt);
					}
				} else if(sqrt(DistanceSqrd(outp[outp.size()-1],pt))<tolerance) {
						outp.pop_back();
						outp.push_back(pt);
				} else {
					outp.push_back(pt);
				}
			}
		}
	}

	double DistancePointToPathsSqrd(const Paths &paths, const IntPoint & pt, IntPoint &closestPointOnPath,
									size_t & clpPathIndex,
									size_t & clpSegmentIndex,
									double & clpParameter) {
		double minDistSq=__DBL_MAX__;
		IntPoint clp;
		// iterate though paths
		for(Path::size_type i=0;i<paths.size();i++) {
			const Path * path = &paths[i];
			Path::size_type size=path->size();
			// iterate through segments
			for(Path::size_type j=0;j<size;j++) {
				double ptPar;
				double distSq=DistancePointToLineSegSquared(path->at(j>0 ? j-1 : size-1),path->at(j),pt,clp,ptPar);
				if(distSq<minDistSq) {
					clpPathIndex = i;
					clpSegmentIndex = j;
					clpParameter = ptPar;
					closestPointOnPath=clp;
					minDistSq=distSq;
				}
			}
		}
		return minDistSq;
	}

	bool Circle2CircleIntersect(const IntPoint & c1, const IntPoint &c2, double radius, pair<DoublePoint,DoublePoint> & intersections ) {
		double DX = double(c2.X - c1.X);
		double DY = double(c2.Y - c1.Y);
		double d = sqrt(DX*DX+DY*DY);
		if(d<NTOL) return false; // same center
		if(d>=radius) return false; // do not intersect, or intersect in one point (this case not relevant here)
		double a_2 = sqrt(4*radius*radius-d*d)/2.0;
		intersections.first = DoublePoint(0.5*(c1.X+c2.X)-DY*a_2/d, 0.5*(c1.Y+c2.Y)+DX*a_2/d);
		intersections.second = DoublePoint(0.5*(c1.X+c2.X)+DY*a_2/d, 0.5*(c1.Y+c2.Y)-DX*a_2/d);
		return true;
	}


	bool Line2CircleIntersect(const IntPoint &c, double radius,const IntPoint &p1, const IntPoint &p2, vector<DoublePoint> & result, bool clamp=true)
	{
		// if more intersections returned, first is closer to p1
		//to do: box  check for performance
		 double dx=double(p2.X-p1.X);
    	 double dy=double(p2.Y-p1.Y);
    	 double lcx = double(p1.X - c.X);
    	 double lcy = double(p1.Y - c.Y);
    	 double a=dx*dx+dy*dy;
    	 double b=2*dx*lcx+2*dy*lcy;
    	 double C=lcx*lcx+lcy*lcy-radius*radius;
    	 double sq = b*b-4*a*C;
    	 if (sq<0) return false; // no solution
    	 sq=sqrt(sq);
    	 double t1=(-b-sq)/(2*a);
    	 double t2=(-b+sq)/(2*a);
		 result.clear();
    	if(clamp) {
        	if (t1>=0.0 && t1<=1.0) result.push_back(DoublePoint(p1.X + t1*dx, p1.Y + t1*dy));
        	if (t2>=0.0 && t2<=1.0) result.push_back(DoublePoint(p1.X + t2*dx, p1.Y + t2*dy));
		} else {
			result.push_back(DoublePoint(p1.X + t2*dx, p1.Y + t2*dy));
			result.push_back(DoublePoint(p1.X + t2*dx, p1.Y + t2*dy));
		}
		return result.size()>0;
	}

	// calculate center point of polygon
	IntPoint Compute2DPolygonCentroid(const Path &vertices)
	{
	    DoublePoint centroid(0,0);
	    double signedArea = 0.0;
	    double x0 = 0.0; // Current vertex X
	    double y0 = 0.0; // Current vertex Y
	    double x1 = 0.0; // Next vertex X
	    double y1 = 0.0; // Next vertex Y
	    double a = 0.0;  // Partial signed area

	    // For all vertices
	    size_t i=0;
		Path::size_type size = vertices.size();
	    for (i=0; i<size; ++i)
	    {
	        x0 = double(vertices[i].X);
	        y0 = double(vertices[i].Y);
	        x1 = double(vertices[(i+1) % size].X);
	        y1 = double(vertices[(i+1) % size].Y);
	        a = x0*y1 - x1*y0;
	        signedArea += a;
	        centroid.X += (x0 + x1)*a;
	        centroid.Y += (y0 + y1)*a;
	    }

	    signedArea *= 0.5;
	    centroid.X /= (6.0*signedArea);
	    centroid.Y /= (6.0*signedArea);
    	return IntPoint(long(centroid.X),long(centroid.Y));
	}

	// point must be within first path (boundary) and must not be within all other paths (holes)
	bool IsPointWithinCutRegion(const Paths & toolBoundPaths,const IntPoint & point) {
		for(size_t i=0; i<toolBoundPaths.size();i++) {
			int pip=PointInPolygon(point, toolBoundPaths[i]);
			if(i==0 && pip==0 ) return false; // is outside or on boundary
			if(i>0 && pip!=0) return false; // is inside hole
		}
		return true;
	}

	/* finds intersection of line segment with line segment */
	bool IntersectionPoint(const IntPoint & s1p1,
						const IntPoint & s1p2,
						const IntPoint & s2p1,
						const IntPoint & s2p2,
						IntPoint & intersection) {
		// todo: bounds check for perfomance
		double S1DX = double(s1p2.X - s1p1.X);
		double S1DY = double(s1p2.Y - s1p1.Y);
		double S2DX = double(s2p2.X - s2p1.X);
		double S2DY = double(s2p2.Y - s2p1.Y);
		double d=S1DY*S2DX - S2DY*S1DX;
		if(fabs(d)<NTOL) return false; // lines are parallel

		double LPDX = double(s1p1.X - s2p1.X);
		double LPDY = double(s1p1.Y - s2p1.Y);
		double p1d = S2DY*LPDX - S2DX*LPDY;
		double p2d = S1DY*LPDX - S1DX*LPDY;
		if((d<0) && (
			p1d<d || p1d>0 || p2d<d || p2d>0
		)) return false ; // intersection not within segment1
		if((d>0) && (
			p1d<0 || p1d>d || p2d<0 || p2d>d
		)) return true; // intersection not within segment2
		double t=p1d/d;
		intersection=IntPoint(long(s1p1.X + S1DX*t), long(s1p1.Y + S1DY*t));
		return true;
	}

	/* finds one/first intersection of line segment with paths */
	bool IntersectionPoint(const Paths & paths,const IntPoint & p1, const IntPoint & p2, IntPoint & intersection) {
		for(size_t i=0; i< paths.size(); i++) {
			const Path *path = &paths[i];
			size_t size=path->size();
			if(size<2) continue;
			for(size_t j=0;j<size;j++) {
				// todo: box check for perfomance
				const IntPoint * pp1 = &path->at(j>0?j-1:size-1);
				const IntPoint * pp2 = &path->at(j);
				double LDY = double(p2.Y - p1.Y);
				double LDX = double(p2.X - p1.X);
				double PDX = double(pp2->X - pp1->X);
				double PDY = double(pp2->Y - pp1->Y);
				double d=LDY*PDX - PDY*LDX;
				if(fabs(d)<NTOL) continue; // lines are parallel

				double LPDX = double(p1.X - pp1->X);
				double LPDY = double(p1.Y - pp1->Y);
				double p1d = PDY*LPDX - PDX*LPDY;
				double p2d = LDY*LPDX - LDX*LPDY;
				if((d<0) && (
					p1d<d || p1d>0 || p2d<d || p2d>0
				)) continue; // intersection not within segment
				if((d>0) && (
					p1d<0 || p1d>d || p2d<0 || p2d>d
				)) continue; // intersection not within segment
				double t=p1d/d;
				intersection=IntPoint(long(p1.X + LDX*t), long(p1.Y + LDY*t));
				return true;
			}
		}
		return false;
	}


	// finds the section (sub-path) of the one path between points that are closest to p1 and p2, if that distance is lower than distanceLmit
	bool FindPathBetweenClosestPoints(const Paths & paths,IntPoint p1,IntPoint p2, double distanceLmit, Path & res) {

		size_t clpPathIndex;
		IntPoint clp1;
		size_t clpSegmentIndex1;
		double clpParameter1;

		IntPoint clp2;
		size_t clpSegmentIndex2;
		double clpParameter2;

		double limitSqrd = distanceLmit*distanceLmit;

		double distSqrd=DistancePointToPathsSqrd(paths,p1,clp1,clpPathIndex,clpSegmentIndex1,clpParameter1);

		if(distSqrd>limitSqrd) return false; // too far
		Path closestPath = paths.at(clpPathIndex);
		Paths closestPaths; closestPaths.push_back(closestPath); // limit to the path where clp is found

		// find second point
		distSqrd=DistancePointToPathsSqrd(closestPaths,p2,clp2,clpPathIndex,clpSegmentIndex2,clpParameter2);

		if(distSqrd>limitSqrd) {
			// second point too far, try other way around
			distSqrd=DistancePointToPathsSqrd(paths,p2,clp1,clpPathIndex,clpSegmentIndex1,clpParameter1);
			if(distSqrd>limitSqrd) return false; // still too far
			closestPath = paths.at(clpPathIndex);
			closestPaths.clear();
			closestPaths.push_back(closestPath);
			distSqrd=DistancePointToPathsSqrd(closestPaths,p1,clp2,clpPathIndex,clpSegmentIndex2,clpParameter2);
			if(distSqrd>limitSqrd) return false; // still too far
		}

		// result in reverse direction
		Path rev_result;
		rev_result << clp1;
		long minIndex = long(clpSegmentIndex2);
		if(minIndex >= long(clpSegmentIndex1-1)) minIndex -= long(closestPath.size());
		for(long i=long(clpSegmentIndex1)-1;i>=minIndex;i--) {
			long index=i;
			if(index<0) index+= long(closestPath.size());
			if(sqrt(DistanceSqrd(rev_result.back(),closestPath.at(index)))>NTOL) rev_result << closestPath.at(index);
		}
		if(sqrt(DistanceSqrd(rev_result.back(),clp2)) >NTOL) rev_result << clp2;

		// result in forward direction
		Path fwd_result;
		fwd_result << clp1;
		size_t maxIndex = clpSegmentIndex2;
		if(maxIndex <= clpSegmentIndex1) maxIndex = closestPath.size() + clpSegmentIndex2;
		for(size_t i=clpSegmentIndex1;i<maxIndex;i++) {
			size_t index=i;
			if(index>=closestPath.size()) index-= closestPath.size();
			if(sqrt(DistanceSqrd(fwd_result.back(),closestPath.at(index)))>NTOL) fwd_result << closestPath.at(index);
		}
		if(sqrt(DistanceSqrd(fwd_result.back(),clp2))>NTOL) fwd_result << clp2;

		res = (PathLength(rev_result) < PathLength(fwd_result)) ? rev_result: fwd_result; // take shortest
		return res.size()>1;
	}


		// finds interim points not neccesarily on the same keeptooldown linking path
	bool FindClosestClearPoints(const Paths & paths,IntPoint p1,IntPoint p2, double distanceLmit, IntPoint &interim1,IntPoint &interim2) {
		size_t clpPathIndex;
		size_t clpSegmentIndex1;
		double clpParameter1;
		size_t clpSegmentIndex2;
		double clpParameter2;
		double limitSqrd = distanceLmit*distanceLmit;
		double distSqrd=DistancePointToPathsSqrd(paths,p1,interim1,clpPathIndex,clpSegmentIndex1,clpParameter1);
		if(distSqrd>limitSqrd) return false; // too far
		// find second point
		distSqrd=DistancePointToPathsSqrd(paths,p2,interim2,clpPathIndex,clpSegmentIndex2,clpParameter2);
		if(distSqrd>limitSqrd) return false; // still too far
		return true;
	}

	bool PopPathWithClosestPoint(Paths & paths	 /*closest path is removed from collection */
			,IntPoint p1, Path & result) {

		if(paths.size()==0) return false;

		double minDistSqrd=__DBL_MAX__;
		size_t closestPathIndex=0;
		long closestPointIndex=0;
		for(size_t pathIndex=0;pathIndex<paths.size();pathIndex++) {
			Path & path = paths.at(pathIndex);
			for(size_t i=0;i<path.size();i++) {
				double dist=DistanceSqrd(p1,path.at(i));
				if(dist<minDistSqrd) {
					minDistSqrd=dist;
					closestPathIndex=pathIndex;
					closestPointIndex=long(i);
				}
			}
		}

		result.clear();
		// make new path starting with that point
		Path & closestPath = paths.at(closestPathIndex);
		for(size_t i=0;i<closestPath.size();i++) {
			long index=closestPointIndex+long(i);
			if(index>=long(closestPath.size())) index-=long(closestPath.size());
			result.push_back(closestPath.at(index));
		}
		// remove the closest path
		paths.erase(paths.begin()+closestPathIndex);
		return true;
	}

	void DeduplicatePaths(const Paths & inputs,Paths & outputs) {
		outputs.clear();
		for(const auto & new_pth :inputs) {
			bool duplicate=false;
			// aff all points of new path exist on some of the old paths, path is considered duplicate
			for(const auto & old_pth  :outputs) {
				bool all_points_exists=true;
				for(const auto pt1:new_pth) {
					bool pointExists=false;
					for(const auto pt2:old_pth) {
						if(DistanceSqrd(pt1,pt2)<SAME_POINT_TOL_SQRD_SCALED) {
							pointExists=true;
							break;
						}
					}
					if(!pointExists) {
						 all_points_exists=false;
						 break;
					}
				}
				if(all_points_exists) {
					duplicate=true;
					break;
				}
			}

			if(!duplicate && new_pth.size()>0) {
				outputs.push_back(new_pth);
			}
		}
	}

	void ConnectPaths(Paths input,Paths & output) {
		// remove duplicate paths
		output.clear();
		bool newPath = true;
		Path joined;
		while(input.size()>0) {
			if(newPath) {
				if(joined.size()>0) output.push_back(joined);
				joined.clear();
				for(auto pt:input.front()) {
					joined.push_back(pt);
				}
				input.erase(input.begin());
				newPath=false;
			}
			bool anyMatch=false;
			for(size_t i=0;i<input.size();i++) {
				Path & n= input.at(i);
				if(DistanceSqrd(n.front(),joined.back())<SAME_POINT_TOL_SQRD_SCALED) {
					for(auto pt:n) joined.push_back(pt);
					input.erase(input.begin()+i);
					anyMatch=true;
					break;
				} else if(DistanceSqrd(n.back(),joined.back())<SAME_POINT_TOL_SQRD_SCALED) {
					ReversePath(n);
					for(auto pt:n) joined.push_back(pt);
					input.erase(input.begin()+i);
					anyMatch=true;
					break;
				}  else if(DistanceSqrd(n.front(),joined.front())<SAME_POINT_TOL_SQRD_SCALED) {
					for(auto pt:n) joined.insert(joined.begin(),pt);
					input.erase(input.begin()+i);
					anyMatch=true;
					break;
				} else if(DistanceSqrd(n.back(),joined.front())<SAME_POINT_TOL_SQRD_SCALED) {
					ReversePath(n);
					for(auto pt:n) joined.insert(joined.begin(),pt);
					input.erase(input.begin()+i);
					anyMatch=true;
					break;
				}
			}
			if(!anyMatch) newPath = true;
		}
		if(joined.size()>0) output.push_back(joined);
	}


	// helper class for measuring performance
	class PerfCounter {
		public:
			PerfCounter(string p_name) {
				name = p_name;
				count =0;
			}
			void Start() {
				start_ticks=clock();
			}
			void Stop() {
				total_ticks+=clock()-start_ticks;
				count++;
			}
			void DumpResults() {
				double total_time=double(total_ticks)/CLOCKS_PER_SEC;
				cout<<"Perf: " << name.c_str() << " total_time: " <<  total_time  << " sec, call_count:" << count << " per_call:" << double(total_time/count) << endl;
			}
		private:
			string name;
			clock_t start_ticks;
			clock_t total_ticks;
			size_t count;
	};

	PerfCounter Perf_ProcessPolyNode("ProcessPolyNode");
	PerfCounter Perf_CalcCutArea("CalcCutArea");
	PerfCounter Perf_NextEngagePoint("NextEngagePoint");
	PerfCounter Perf_PointIterations("PointIterations");
	PerfCounter Perf_ExpandCleared("ExpandCleared");
	PerfCounter Perf_DistanceToBoundary("DistanceToBoundary");

	/*****************************************
	 * Linear Interpolation - area vs angle
	 * ***************************************/
	class Interpolation {
		public:
			const double MIN_ANGLE = -M_PI/4;
			const double MAX_ANGLE = M_PI/4;

			void clear() {
				angles.clear();
				areas.clear();
			}
			// adds point keeping the incremental order of areas for interpolation to work correctly
			void addPoint(double area, double angle) {
				std::size_t size = areas.size();
				if(size==0 || area > areas[size-1] + NTOL) { // first point or largest area point
					areas.push_back(area);
					angles.push_back(angle);
					return;
				}

				for(std::size_t i=0;i<size;i++) {
					if(area<areas[i] - NTOL && (i==0 || area > areas[i-1] + NTOL)) {
						areas.insert(areas.begin() + i,area);
						angles.insert(angles.begin() + i,angle);
					}
				}
			}

			double interpolateAngle(double targetArea) {
				std::size_t size = areas.size();
				if(size<2 || targetArea>areas[size-1]) return MIN_ANGLE; //max engage angle - convinient value to initially measure cut area
				if(targetArea<areas[0]) return MAX_ANGLE; // min engage angle

				for(size_t i=1;i<size;i++) {
					// find 2 subsequent points where target area is between
					if(areas[i-1]<=targetArea && areas[i]>targetArea) {
						// linear interpolation
						double af = (targetArea-areas[i-1])/(areas[i] - areas[i-1]);
						double a = angles[i-1]  + af*(angles[i] - angles[i-1]);
						return a;
					}
				}
				return MIN_ANGLE;
			}

			double clampAngle(double angle) {
				if(angle<MIN_ANGLE) return MIN_ANGLE;
				if(angle>MAX_ANGLE) return MAX_ANGLE;
				return angle;
			}

			double getRandomAngle() {
				return MIN_ANGLE + (MAX_ANGLE-MIN_ANGLE)*double(rand())/double(RAND_MAX);
			}
			size_t getPointCount() {
				return areas.size();
			}

		private:
			vector<double> angles;
			vector<double> areas;

	};

	/****************************************
	 * Engage Point
	 ***************************************/
	class EngagePoint {
		public:
			struct EngageState {
				size_t currentPathIndex;
				size_t currentSegmentIndex;
				double segmentPos =0;
				double totalDistance=0;
				double currentPathLength=0;
				int passes=0;

				double metric; // engage point metric

				bool operator < (const EngageState& other) const
    			{
     				   return (metric < other.metric);
    			}
			};
			EngagePoint(const Paths & p_toolBoundPaths) {
				SetPaths(p_toolBoundPaths);

				state.currentPathIndex=0;
				state.currentSegmentIndex=0;
				state.segmentPos =0;
				state.totalDistance=0;
				calculateCurrentPathLength();
			}

		void SetPaths(const Paths & paths) {
			toolBoundPaths = paths;
			state.currentPathIndex=0;
			state.currentSegmentIndex=0;
			state.segmentPos =0;
			state.totalDistance=0;
			state.passes =0;
			calculateCurrentPathLength();
		}

		EngageState GetState() {
			return state;
		}

		void SetState(const EngageState &new_state ) {
			state=new_state;
		}

		void ResetPasses() {
			state.passes=0;
		}
		void moveToClosestPoint(const IntPoint &pt,double step) {

				Path result;
				IntPoint current=pt;
				// chain paths according to distance in between
				Paths toChain = toolBoundPaths;
				toolBoundPaths.clear();
				// if(toChain.size()>0) {
				// 	toolBoundPaths.push_back(toChain.front());
				// 	toChain.erase(toChain.begin());
				// }
				while(PopPathWithClosestPoint(toChain,current,result)) {
					toolBoundPaths.push_back(result);
					if(result.size()>0) current = result.back();
				}


				double minDistSq = __DBL_MAX__;
				size_t minPathIndex = state.currentPathIndex;
				size_t minSegmentIndex = state.currentSegmentIndex;
				double minSegmentPos = state.segmentPos;
				state.totalDistance=0;
				for(;;) {
					while(moveForward(step)) {
						double distSqrd = DistanceSqrd(pt,getCurrentPoint());
						if(distSqrd<minDistSq) {
							//cout << sqrt(minDistSq) << endl;
							minDistSq = distSqrd;
							minPathIndex = state.currentPathIndex;
							minSegmentIndex = state.currentSegmentIndex;
							minSegmentPos = state.segmentPos;
						}
					}
					if(!nextPath()) break;
				}
				state.currentPathIndex=minPathIndex;
				state.currentSegmentIndex=minSegmentIndex;
				state.segmentPos=minSegmentPos ;
				calculateCurrentPathLength();
				ResetPasses();
		}
		bool nextEngagePoint(Adaptive2d*parent,  const Paths & cleared, double step, double minCutArea, double maxCutArea, int maxPases=2) {
			//cout << "nextEngagePoint called step: " << step << endl;
			Perf_NextEngagePoint.Start();
			double prevArea = 0; // we want to make sure that we catch the point where the area is on raising slope
			//IntPoint initialPoint = getCurrentPoint();
			IntPoint initialPoint(-1000000000,-1000000000);
			for(;;) {
				if(!moveForward(step))	 {
					if(!nextPath()) {
						state.passes++;
						if(state.passes>=maxPases) {
							Perf_NextEngagePoint.Stop();
							return false; // nothin more to cut
						}
					 prevArea=0;
					}
				}
				IntPoint cpt = getCurrentPoint();
				double area=parent->CalcCutArea(clip,initialPoint,cpt,cleared);
				//cout << "engage scan path: " << currentPathIndex << " distance:" << totalDistance << " area:" << area << " areaPD:" << area/step << " min:" << minCutArea << " max:" << maxCutArea << endl;
				if(area>minCutArea && area<maxCutArea && area>prevArea) {
					Perf_NextEngagePoint.Stop();
					return true;
				}
				prevArea=area;
			}
		}
			IntPoint getCurrentPoint() {
				const Path * pth = &toolBoundPaths.at(state.currentPathIndex);
				const IntPoint * p1=&pth->at(state.currentSegmentIndex>0?state.currentSegmentIndex-1:pth->size()-1);
				const IntPoint * p2=&pth->at(state.currentSegmentIndex);
				double segLength =sqrt(DistanceSqrd(*p1,*p2));
				return IntPoint(long(p1->X + state.segmentPos*double(p2->X-p1->X)/segLength),long(p1->Y + state.segmentPos*double(p2->Y-p1->Y)/segLength));
			}

			DoublePoint getCurrentDir() {
				const Path * pth = &toolBoundPaths.at(state.currentPathIndex);
				const IntPoint * p1=&pth->at(state.currentSegmentIndex>0?state.currentSegmentIndex-1:pth->size()-1);
				const IntPoint * p2=&pth->at(state.currentSegmentIndex);
				double segLength =sqrt(DistanceSqrd(*p1,*p2));
				return DoublePoint(double(p2->X-p1->X)/segLength,double(p2->Y-p1->Y)/segLength);
			}

			bool moveForward(double distance) {
				const Path * pth = &toolBoundPaths.at(state.currentPathIndex);
				if(distance<NTOL) throw std::invalid_argument( "distance must be positive" );
				state.totalDistance+=distance;
				double segmentLength =  currentSegmentLength();
				while(state.segmentPos+distance>segmentLength) {
					state.currentSegmentIndex++;
					if(state.currentSegmentIndex>=pth->size()) {
						state.currentSegmentIndex=0;
					}
					distance=distance-(segmentLength-state.segmentPos);
					state.segmentPos =0;
					segmentLength =currentSegmentLength();
				}
				state.segmentPos+=distance;
				return state.totalDistance<=1.2 * state.currentPathLength;
			}

			bool nextPath() {
				state.currentPathIndex++;
				state.currentSegmentIndex=0;
				state.segmentPos =0;
				state.totalDistance=0;
				if(state.currentPathIndex>=toolBoundPaths.size()) {
					state.currentPathIndex =0;
					calculateCurrentPathLength();
					return false;
				}
				calculateCurrentPathLength();
				//cout << "nextPath:" << currentPathIndex << endl;
				return true;
			}




		private:
			Paths toolBoundPaths;
			EngageState state;
			Clipper clip;
			void calculateCurrentPathLength() {
				const Path * pth = &toolBoundPaths.at(state.currentPathIndex);
				size_t size=pth->size();
				state.currentPathLength=0;
				for(size_t i=0;i<size;i++) {
					const IntPoint * p1=&pth->at(i>0?i-1:size-1);
					const IntPoint * p2=&pth->at(i);
					state.currentPathLength += sqrt(DistanceSqrd(*p1,*p2));
				}
			}

			double currentSegmentLength() {
				const Path * pth = &toolBoundPaths.at(state.currentPathIndex);
				const IntPoint * p1=&pth->at(state.currentSegmentIndex>0?state.currentSegmentIndex-1:pth->size()-1);
				const IntPoint * p2=&pth->at(state.currentSegmentIndex);
				return sqrt(DistanceSqrd(*p1,*p2));
			}

	};


	/****************************************
	// Adaptive2d - constructor
	*****************************************/
	Adaptive2d::Adaptive2d() {
	}

	double Adaptive2d::CalcCutArea(Clipper & clip,const IntPoint &c1, const IntPoint &c2, const Paths &cleared_paths, bool preventConvetional) {
		Perf_CalcCutArea.Start();

		double dist = DistanceSqrd(c1,c2);
		if(dist<NTOL) return 0;

		/// new alg
		double rsqrd=toolRadiusScaled*toolRadiusScaled;
    	double area =0;
		Paths interPaths;
		IntPoint clp; // to hold closest point
		vector<DoublePoint> inters; // to hold intersection results

		for(const Path &path : cleared_paths) {
			size_t size = path.size();
			size_t curPtIndex = 0;
			bool found=false;
			// step 1: we find the starting point on the cleared path that is outside new tool shape (c2)
			for(size_t i=0;i<size;i++) {
				if(DistanceSqrd(path[curPtIndex],c2)>rsqrd) {
					found = true;
					break;
				}
				curPtIndex++; if(curPtIndex>=size) curPtIndex=0;
			}
			if(!found) continue; // try anohter path

			// step 2: iterate throuh path from starting point and find the part of the path inside the c2
			size_t prevPtIndex = curPtIndex;
			Path *interPath=NULL;
			bool prev_inside=false;
			const IntPoint *p1=&path[prevPtIndex];

			for(size_t i=0;i<size;i++) {
				curPtIndex++; if(curPtIndex>=size) curPtIndex=0;
				const IntPoint *p2=&path[curPtIndex];
				if(!prev_inside) { // prev state: outside, find first point inside C2
					// TODO:BBOX check here	maybe
					double par;
					if(DistancePointToLineSegSquared(*p1,*p2,c2,clp,par)<=rsqrd) {  // current segment inside, start
						prev_inside=true;
						interPaths.push_back(Path());
						if(interPaths.size()>1) break;  // we will use poly clipping alg. if there are more intersecting paths
						interPath=&interPaths.back();
						// current segment inside c2, prev point outside, find intersection:
						if(Line2CircleIntersect(c2,toolRadiusScaled,*p1,*p2,inters)) {
							interPath->push_back(IntPoint(long(inters[0].X),long(inters[0].Y)));
							if(inters.size()>1) {
								interPath->push_back(IntPoint(long(inters[1].X),long(inters[1].Y)));
								prev_inside=false;
							} else {
								interPath->push_back(IntPoint(*p2));
							}
						} else { // no intersection - must be edge case, add p2
							//prev_inside=false;
							interPath->push_back(IntPoint(*p2));
						}
					}
				}
				else if (interPath!=NULL) { // state: inside
					if( (DistanceSqrd(c2,*p2) <= rsqrd)) {  // next point still inside, add it and continue, no state change
						interPath->push_back(IntPoint(*p2));
					} else { // prev point inside, current point outside, find instersection
						if(Line2CircleIntersect(c2,toolRadiusScaled,*p1,*p2,inters)) {
							if(inters.size()>1) {
								interPath->push_back(IntPoint(long(inters[1].X),long(inters[1].Y)));
							} else {
								interPath->push_back(IntPoint(long(inters[0].X),long(inters[0].Y)));
							}
						}
						prev_inside=false;
					}
				}
				prevPtIndex = curPtIndex;
				p1 = p2;

			}
			if(interPaths.size()>1) break; // we will use poly clipping alg. if there are more intersecting paths with the tool (rare case)
		}

		if(interPaths.size()==1 &&  interPaths.front().size()>1 ) {
				Path *interPath=&interPaths.front();
				// interPath - now contains the part of cleared path inside the C2
				size_t ipc2_size =interPath->size();
				const IntPoint &fpc2=interPath->front(); // first point
				const IntPoint &lpc2=interPath->back(); // last point
				// path length
				double interPathLen=0;
				for(size_t j=1;j<ipc2_size;j++)
					interPathLen+=sqrt(DistanceSqrd(interPath->at(j-1),interPath->at(j)));

				Paths inPaths;
				inPaths.reserve(200);
				inPaths.push_back(*interPath);
				Path pthToSubtract ;
				pthToSubtract.push_back(fpc2);

				double fi1 = atan2(fpc2.Y-c2.Y,fpc2.X-c2.X);
				double fi2 = atan2(lpc2.Y-c2.Y,lpc2.X-c2.X);
				double minFi=fi1;
				double maxFi=fi2;
				if(maxFi<minFi) maxFi += 2*M_PI;

				if(preventConvetional && interPathLen>=RESOLUTION_FACTOR) {
					// detect conventional mode cut - we want only climb mode
						IntPoint midPoint(long(c2.X + toolRadiusScaled*cos(0.5*(maxFi+minFi))),long(c2.Y + toolRadiusScaled*sin(0.5*(maxFi+minFi))));
						if(PointSideOfLine(c1,c2,midPoint)<0) {
									area = __DBL_MAX__;
									Perf_CalcCutArea.Stop();
									// #ifdef DEV_MODE
									// 	cout << "Break: @(" << double(c2.X)/scaleFactor << "," << double(c2.Y)/scaleFactor  << ") conventional mode" << endl;
									// #endif
									return area;
						}
				}

				double scanDistance = 2.5*toolRadiusScaled;
				// stepping through path discretized to stepDistance
				double stepDistance=min(double(RESOLUTION_FACTOR),interPathLen/24)+1;
				//cout << stepDistance << endl;
				const IntPoint * prevPt=&interPath->front();
				double distance=0;
				for(size_t j=1;j<ipc2_size;j++) {
					const IntPoint * cpt =&interPath->at(j);
					double segLen = sqrt(DistanceSqrd(*cpt,*prevPt));
					if(segLen<NTOL) continue; // skip point - segment too short
					for(double pos_unclamped=0.0;pos_unclamped<segLen+stepDistance;pos_unclamped+=stepDistance) {
						double pos=pos_unclamped;
						if(pos>segLen) {
							distance+=stepDistance-(pos-segLen);
							pos=segLen; // make sure we get exact end point
						} else {
							distance+=stepDistance;
						}
						double dx=double(cpt->X-prevPt->X);
						double dy=double(cpt->Y-prevPt->Y);
						IntPoint segPoint(long(prevPt->X + dx*pos/segLen),long( prevPt->Y + dy*pos/segLen));
						IntPoint scanPoint(long(c2.X + scanDistance*cos(minFi + distance*(maxFi-minFi)/interPathLen)),
									long(c2.Y + scanDistance*sin(minFi + distance*(maxFi-minFi)/interPathLen)));

						IntPoint intersC2(segPoint.X,segPoint.Y);
						IntPoint intersC1(segPoint.X,segPoint.Y);

						// there should be intersection with C2
						if(Line2CircleIntersect(c2,toolRadiusScaled,segPoint,scanPoint,inters)) {
							if(inters.size()>1) {										
								intersC2.X = long(inters[1].X);
								intersC2.Y = long(inters[1].Y);
							} else {
								intersC2.X = long(inters[0].X);
								intersC2.Y = long(inters[0].Y);
							}
						} else {
							pthToSubtract.push_back(segPoint);
						}

						if(Line2CircleIntersect(c1,toolRadiusScaled,segPoint,scanPoint,inters)) {
								if(inters.size()>1) {											
									intersC1.X = long(inters[1].X);
									intersC1.Y = long(inters[1].Y);
								} else {
									intersC1.X = long(inters[0].X);
									intersC1.Y = long(inters[0].Y);
								}
							if(DistanceSqrd(segPoint,intersC2)<DistanceSqrd(segPoint,intersC1)) {
								pthToSubtract.push_back(intersC2);
							} else {
								pthToSubtract.push_back(intersC1);
							}
						} else { // add the segpoint if no intersection with C1
							pthToSubtract.push_back(segPoint);
						}
					}
					prevPt = cpt;
				}

				pthToSubtract.push_back(lpc2); // add last point
				pthToSubtract.push_back(c2);

				double segArea =Area(pthToSubtract);
				double A=(maxFi-minFi)*rsqrd/2; // sector area
				area+=A- fabs(segArea);

			} else  if(interPaths.size()>1)
			{
				// old way of calculating cut area based on polygon slipping
				// used in case when there are multiple intersections of tool with cleared poly (very rare case, but important)
				// 1. find differene beween old and new tool shape
				Path oldTool;
				Path newTool;
				TranslatePath(toolGeometry,oldTool,c1);
				TranslatePath(toolGeometry,newTool,c2);
				clip.Clear();
				clip.AddPath(newTool, PolyType::ptSubject, true);
				clip.AddPath(oldTool, PolyType::ptClip, true);
				Paths toolDiff;
				clip.Execute(ClipType::ctDifference,toolDiff);

				// 2. difference to cleared
				clip.Clear();
				clip.AddPaths(toolDiff,PolyType::ptSubject, true);
				clip.AddPaths(cleared_paths,PolyType::ptClip, true);
				Paths cutAreaPoly;
				clip.Execute(ClipType::ctDifference, cutAreaPoly);

				// calculate resulting area
				area=0;
				for(Path &path : cutAreaPoly) {
					area +=fabs(Area(path));
				}
			}
		//  cout<< "PolyArea:" << areaSum << " new area:" << area << endl;
		Perf_CalcCutArea.Stop();
		return area;
	}

	void Adaptive2d::ApplyStockToLeave(Paths &inputPaths) {
		ClipperOffset clipof;
		if(stockToLeave>NTOL) {
			clipof.Clear();
			clipof.AddPaths(inputPaths,JoinType::jtRound,EndType::etClosedPolygon);
			if(opType==OperationType::otClearingOutside || opType==OperationType::otProfilingOutside)
				clipof.Execute(inputPaths,stockToLeave*scaleFactor);
			else
				clipof.Execute(inputPaths,-stockToLeave*scaleFactor);
		} else {
			// fix for clipper glitches
			clipof.Clear();
			clipof.AddPaths(inputPaths,JoinType::jtRound,EndType::etClosedPolygon);
			clipof.Execute(inputPaths,-1);
			clipof.Clear();
			clipof.AddPaths(inputPaths,JoinType::jtRound,EndType::etClosedPolygon);
			clipof.Execute(inputPaths,1);
		}
	}

	/****************************************
	// Adaptive2d - Execute
	*****************************************/
 	std::list<AdaptiveOutput> Adaptive2d::Execute(const DPaths &stockPaths, const DPaths &paths,  std::function<bool(TPaths)> progressCallbackFn) {
		//**********************************
		// Initializations
		// **********************************

		// a keep the tolerance in workable range
		if(tolerance<0.01) tolerance=0.01;
		if(tolerance>0.2) tolerance=0.2;

		scaleFactor = RESOLUTION_FACTOR/tolerance;
		toolRadiusScaled = long(toolDiameter*scaleFactor/2);
		bbox_size =long(toolDiameter*scaleFactor);
		progressCallback = &progressCallbackFn;
		lastProgressTime=clock();
		stopProcessing=false;

		if(helixRampDiameter>toolDiameter) helixRampDiameter = toolDiameter;
		if(helixRampDiameter<toolDiameter/8) helixRampDiameter = toolDiameter/8;

		helixRampRadiusScaled=long(helixRampDiameter*scaleFactor/2);
		finishPassOffsetScaled=long(tolerance*scaleFactor/2)+1;


		ClipperOffset clipof;
		Clipper clip;

		// generate tool shape
		clipof.Clear();
		Path p;
		p << IntPoint(0,0);
		clipof.AddPath(p,JoinType::jtRound,EndType::etOpenRound);
		Paths toolGeometryPaths;
		clipof.Execute(toolGeometryPaths,toolRadiusScaled);
		toolGeometry=toolGeometryPaths[0];
		//calculate referece area
		Path slotCut;
		TranslatePath(toolGeometryPaths[0],slotCut,IntPoint(toolRadiusScaled/2,0));
		clip.Clear();
		clip.AddPath(toolGeometryPaths[0],PolyType::ptSubject,true);
		clip.AddPath(slotCut,PolyType::ptClip,true);
		Paths crossing;
		clip.Execute(ClipType::ctDifference,crossing);
		referenceCutArea = fabs(Area(crossing[0]));
		optimalCutAreaPD =2 * stepOverFactor * referenceCutArea/toolRadiusScaled;
		#ifdef DEV_MODE
			cout<< "optimalCutAreaPD:" << optimalCutAreaPD  << " scaleFactor:" << scaleFactor << " toolRadiusScaled:" << toolRadiusScaled << " helixRampRadiusScaled:" << helixRampRadiusScaled << endl;
		#endif

		// **********************
		// Convert input paths to clipper
		//************************
		Paths converted;
		for(size_t i=0;i<paths.size();i++) {
			Path cpth;
			for(size_t j=0;j<paths[i].size();j++) {
				std::pair<double,double> pt = paths[i][j];
				cpth.push_back(IntPoint(long(pt.first*scaleFactor),long(pt.second*scaleFactor)));
			}
			converted.push_back(cpth);
		}

		DeduplicatePaths(converted,inputPaths);
		ConnectPaths(inputPaths,inputPaths);
		SimplifyPolygons(inputPaths);
		ApplyStockToLeave(inputPaths);

		//************************
		// convert stock paths
		// *************************
		stockInputPaths.clear();
		for(size_t i=0;i<stockPaths.size();i++) {
			Path cpth;
			for(size_t j=0;j<stockPaths[i].size();j++) {
				std::pair<double,double> pt = stockPaths[i][j];
				cpth.push_back(IntPoint(long(pt.first*scaleFactor),long(pt.second*scaleFactor)));
			}
			stockInputPaths.push_back(cpth);
		}

		SimplifyPolygons(stockInputPaths);

		// *******************************
		//	Resolve hierarchy and run processing
		// ********************************

		if(opType==OperationType::otClearingInside || opType==OperationType::otClearingOutside) {

				// prepare stock boundary overshooted paths
				clipof.Clear();
				clipof.AddPaths(stockInputPaths,JoinType::jtSquare,EndType::etClosedPolygon);
				double overshootDistance =4*toolRadiusScaled + stockToLeave*scaleFactor ;
				if(forceInsideOut) overshootDistance=0;
				Paths stockOvershoot;
				clipof.Execute(stockOvershoot,overshootDistance);
				ReversePaths(stockOvershoot);

				if(opType==OperationType::otClearingOutside) {
					// add stock paths, with overshooting
					for(auto p : stockOvershoot) inputPaths.push_back(p);
				} else if(opType==OperationType::otClearingInside)  {
					// potential TODO: check if there are open paths, and try to close it through overshooted stock boundary
				}

				clipof.Clear();
				clipof.AddPaths(inputPaths,JoinType::jtRound,EndType::etClosedPolygon);
				Paths paths;
				clipof.Execute(paths,-toolRadiusScaled-finishPassOffsetScaled);
				for(const auto & current : paths) {
					int nesting = getPathNestingLevel(current, paths);
					//cout<< " nesting:" << nesting << " limit:" << polyTreeNestingLimit <<  endl;
					if(nesting%2!=0 && (polyTreeNestingLimit==0 || nesting<=polyTreeNestingLimit)) {
						Paths toolBoundPaths;
						toolBoundPaths.push_back(current);
						if(polyTreeNestingLimit != nesting) {
							apendDirectChildPaths(toolBoundPaths,current,paths);
						}

						// calc bounding paths - i.e. area that must be cleared inside
						// it's not the same as input paths due to filtering (nesting logic)
						Paths boundPaths;
						clipof.Clear();
						clipof.AddPaths(toolBoundPaths,JoinType::jtRound,EndType::etClosedPolygon);
						clipof.Execute(boundPaths,toolRadiusScaled+finishPassOffsetScaled);
						ProcessPolyNode(boundPaths,toolBoundPaths);
					}
				}
		}

		if(opType==OperationType::otProfilingInside || opType==OperationType::otProfilingOutside) {
				double offset = opType==OperationType::otProfilingInside  ? -2*(helixRampRadiusScaled+toolRadiusScaled)-RESOLUTION_FACTOR : 2*(helixRampRadiusScaled+toolRadiusScaled) + RESOLUTION_FACTOR;
				for(const auto & current : inputPaths) {
					int nesting = getPathNestingLevel(current,inputPaths);
					//cout<< " nesting:" << nesting << " limit:" << polyTreeNestingLimit << " processHoles:" << processHoles << endl;
					if(nesting%2!=0 && (polyTreeNestingLimit==0 || nesting<=polyTreeNestingLimit)) {
						Paths profilePaths;
						profilePaths.push_back(current);
						if(polyTreeNestingLimit != nesting) {
							apendDirectChildPaths(profilePaths,current,inputPaths);
						}
						for(size_t i=0;i<profilePaths.size();i++) {
									double efOffset= i==0 ? offset : -offset;
									clipof.Clear();
									clipof.AddPath(profilePaths[i],JoinType::jtSquare,EndType::etClosedPolygon);
									Paths off1;
									clipof.Execute(off1,efOffset);
									// make poly between original path and ofset path
									Paths boundPaths;
									clip.Clear();
									if(efOffset<0) {
										clip.AddPath(profilePaths[i],PolyType::ptSubject,true);
										clip.AddPaths(off1,PolyType::ptClip,true);
									} else {
										clip.AddPaths(off1,PolyType::ptSubject,true);
										clip.AddPath(profilePaths[i],PolyType::ptClip,true);
									}
									clip.Execute(ClipType::ctDifference,boundPaths,PolyFillType::pftEvenOdd);

									Paths toolBoundPaths;
									clipof.Clear();
									clipof.AddPaths(boundPaths,JoinType::jtRound,EndType::etClosedPolygon);
									clipof.Execute(toolBoundPaths,-toolRadiusScaled-finishPassOffsetScaled);
									ProcessPolyNode(boundPaths,toolBoundPaths);
						}
					}
				}
		}
		//cout<<" Adaptive2d::Execute finish" << endl;
		return results;
	}

	bool Adaptive2d::FindEntryPoint(TPaths &progressPaths, const Paths & toolBoundPaths,const Paths &boundPaths,
				Paths &cleared /*output-initial cleard area by helix*/,
				IntPoint &entryPoint /*output*/,
				IntPoint & toolPos, DoublePoint & toolDir) {
		Paths incOffset;
		Paths lastValidOffset;
		Clipper clip;
		ClipperOffset clipof;
		bool found= false;

		Paths checkPaths = toolBoundPaths;
		for(int iter=0;iter<10;iter++) {
			clipof.Clear();
			clipof.AddPaths(checkPaths,JoinType::jtSquare,EndType::etClosedPolygon);
			double step = RESOLUTION_FACTOR;
			double currentDelta=-1;
			clipof.Execute(incOffset,currentDelta);
			while(incOffset.size()>0) {
				clipof.Execute(incOffset,currentDelta);
				if(incOffset.size()>0) lastValidOffset=incOffset;
				currentDelta-=step;
			}
			for(size_t i=0;i<lastValidOffset.size();i++) {
				if(lastValidOffset[i].size()>0) {
					entryPoint = Compute2DPolygonCentroid(lastValidOffset[i]);
					//DrawPath(lastValidOffset[i],22);
					found=true;
					break;
				}
			}
			// check if the start point is in any of the holes
			// this may happen in case when toolBoundPaths are simetric (boundary + holes)
			// we need to break simetry and try again
			for(size_t j=0;j<checkPaths.size();j++) {
				int pip = PointInPolygon(entryPoint,checkPaths[j]);
				if((j==0 && pip==0) || (j>0 && pip!=0)  ) {
					found=false;
					break;
				}
			}
			// check if helix fits
			if(found) {
				// make initial polygon cleard by helix ramp
				clipof.Clear();
				Path p1;
				p1.push_back(entryPoint);
				clipof.AddPath(p1,JoinType::jtRound,EndType::etOpenRound);
				clipof.Execute(cleared,helixRampRadiusScaled+toolRadiusScaled);
				CleanPolygons(cleared);
				// we got first cleared area - check if it is crossing boundary
				clip.Clear();
				clip.AddPaths(cleared,PolyType::ptSubject,true);
				clip.AddPaths(boundPaths,PolyType::ptClip,true);
				Paths crossing;
				clip.Execute(ClipType::ctDifference,crossing);
				if(crossing.size()>0) {
					//cerr<<"Helix does not fit to the cutting area"<<endl;
					found=false;
				}
			}

			if(!found) { // break simetry and try again
				clip.Clear();
				clip.AddPaths(checkPaths,PolyType::ptSubject, true);
				auto bounds=clip.GetBounds();
				clip.Clear();
				Path rect;
				rect <<  IntPoint(bounds.left,bounds.bottom);
				rect << IntPoint(bounds.left, (bounds.top+bounds.bottom)/2);
				rect << IntPoint((bounds.left + bounds.right)/2, (bounds.top+bounds.bottom)/2);
				rect << IntPoint((bounds.left + bounds.right)/2, bounds.bottom);
				clip.AddPath(rect,PolyType::ptSubject, true);
				clip.AddPaths(checkPaths,PolyType::ptClip,true);
				clip.Execute(ClipType::ctIntersection,checkPaths);
			}
			if(found) break;
		}
		//DrawCircle(entryPoint,scaleFactor,10);
		if(!found) cerr<<"Start point not found!"<<endl;
		if(found) {
				// visualize/progress for helix
			clipof.Clear();
			Path hp;
			hp << entryPoint;
			clipof.AddPath(hp,JoinType::jtRound,EndType::etOpenRound);
			Paths hps;
			clipof.Execute(hps,helixRampRadiusScaled);
			AddPathsToProgress(progressPaths,hps);


			toolPos = IntPoint(entryPoint.X,entryPoint.Y - helixRampRadiusScaled);
			toolDir = DoublePoint(1.0,0.0);

		}
		return found;
	}
	bool Adaptive2d::FindEntryPointOutside(TPaths &progressPaths, const Paths & toolBoundPaths,const Paths &boundPaths,
				Paths &cleared /*output-initial cleard area by helix*/,
				IntPoint &entryPoint /*output*/,
				IntPoint & toolPos, DoublePoint & toolDir) {

	    UNUSED(progressPaths);  // to silence compiler warning
		UNUSED(boundPaths); // to silence compiler warning

		Clipper clip;
		ClipperOffset clipof;
		// check if boundary shape to cut is outside the stock
		// clip.AddPaths(boundPaths,PolyType::ptSubject, true);
		// clip.AddPaths(stockInputPaths,PolyType::ptClip, true);
		// Paths outsidePaths;
		// clip.Execute(ClipType::ctDifference,outsidePaths);
		for(const auto & pth : toolBoundPaths) {
			for(size_t i=0;i<pth.size();i++) {
				IntPoint checkPoint = pth[i];
				IntPoint lastPoint = i>0 ? pth[i-1] : pth.back();
				// if point is outside the stock
				if(PointInPolygon(checkPoint,stockInputPaths.front())==0) {

					clipof.Clear();
					clipof.AddPaths(stockInputPaths,JoinType::jtSquare,EndType::etClosedPolygon);
					clipof.Execute(cleared,1000*toolRadiusScaled);

					clip.Clear();
					clip.AddPaths(cleared,PolyType::ptSubject, true);
					clip.AddPaths(stockInputPaths,PolyType::ptClip, true);
					clip.Execute(ClipType::ctDifference,cleared);
					CleanPolygons(cleared);
					SimplifyPolygons(cleared);
					//AddPathsToProgress(progressPaths,cleared);
					entryPoint=checkPoint;
					toolPos = entryPoint;
					// find tool dir
					double len=sqrt(DistanceSqrd(lastPoint,checkPoint));
					toolDir = DoublePoint((checkPoint.X - lastPoint.X)/len,(checkPoint.Y - lastPoint.Y)/len);
					return true;
				}
			}
		}
		//AddPathsToProgress(progressPaths,outsidePaths);
		return false;
	}

	/**
	 * returns true if path is clear from obstacles
	*/
	bool  Adaptive2d::IsClearPath(const Path &tp,const Paths & cleared, double safetyDistanceScaled) {
		Clipper clip;
		ClipperOffset clipof;
		clipof.AddPath(tp,JoinType::jtRound,EndType::etOpenRound);
		Paths toolShape;
		clipof.Execute(toolShape,toolRadiusScaled+safetyDistanceScaled);
		clip.AddPaths(toolShape,PolyType::ptSubject,true);
		clip.AddPaths(cleared,PolyType::ptClip,true);
		Paths crossing;
		clip.Execute(ClipType::ctDifference,crossing);
		double collisionArea =0;
		for(auto &p : crossing) {
			collisionArea += fabs(Area(p));
		}
		return collisionArea < optimalCutAreaPD*RESOLUTION_FACTOR/10+1;
	}

	bool Adaptive2d::IsAllowedToCutTrough(const IntPoint &p1,const IntPoint &p2,const Paths & cleared, const Paths & toolBoundPaths, double areaFactor, bool skipBoundsCheck) {
		double stepSize=2*RESOLUTION_FACTOR;
		Clipper clip;
		size_t clpPathIndex;
		size_t clpSegmentIndex;
		double clpParameter;
		IntPoint clp;
		double distance = sqrt(DistanceSqrd(p1,p2));
		if(!skipBoundsCheck && !IsPointWithinCutRegion(toolBoundPaths,p1) // check with some tolerance
			&& sqrt(DistancePointToPathsSqrd(toolBoundPaths,p1,clp,clpPathIndex,clpSegmentIndex,clpParameter))>RESOLUTION_FACTOR
			) {
			// first point outside boundary - its not clear to cut
			return false;
		} else for(double d=stepSize;d<distance;d+=stepSize) {
			IntPoint toolPos1(long(p1.X + double(p2.X-p1.X)*(d-stepSize)/distance),long(p1.Y + double(p2.Y-p1.Y)*(d-stepSize)/distance));
			IntPoint toolPos2(long(p1.X + double(p2.X-p1.X)*d/distance),long(p1.Y + double(p2.Y-p1.Y)*d/distance));
			double area = CalcCutArea(clip,toolPos1,toolPos2, cleared,false);
			// if we are cutting above optimal -> not clear to cut
			if(area>areaFactor*stepSize*optimalCutAreaPD) return false;

			//if tool is outside boundary -> its not clear to cut
			if(!skipBoundsCheck && !IsPointWithinCutRegion(toolBoundPaths,toolPos2)
				&& sqrt(DistancePointToPathsSqrd(toolBoundPaths,toolPos2,clp,clpPathIndex,clpSegmentIndex,clpParameter))>RESOLUTION_FACTOR
				) {
				return false;
			}
		}
		return true;
	}

	void Adaptive2d::AppendToolPath(TPaths &progressPaths,AdaptiveOutput & output,const Path & passToolPath,const Paths & cleared, const Paths & toolBoundPaths) {
		if(passToolPath.size()<1) return;

		UNUSED(progressPaths); // to silence compiler warning,var is occasionally used in dev. for debugging

		IntPoint nextPoint(passToolPath[0]);

		// if there is a previous path - need to resolve linking move to new path
		if(output.AdaptivePaths.size()>0 && output.AdaptivePaths.back().second.size()>0) {
			auto & lastTPath = output.AdaptivePaths.back();
			auto & lastTPoint = lastTPath.second.back();
			IntPoint lastPoint(long(lastTPoint.first*scaleFactor),long(lastTPoint.second*scaleFactor));

			//first we try to cut through the linking move for short distances
			bool linkFound = false;
			double linkDistance = sqrt(DistanceSqrd(lastPoint,nextPoint));
			if(linkDistance<toolRadiusScaled && IsAllowedToCutTrough(lastPoint,nextPoint,cleared,toolBoundPaths)) {
					//cout << "cleared through" << endl;
					TPath linkPath;
					linkPath.first = MotionType::mtCutting;
					linkPath.second.push_back(DPoint(double(lastPoint.X)/scaleFactor,double(lastPoint.Y)/scaleFactor));
					linkPath.second.push_back(DPoint(double(nextPoint.X)/scaleFactor,double(nextPoint.Y)/scaleFactor));
					output.AdaptivePaths.push_back(linkPath);
					linkFound=true;
			}
			if(!linkFound) {
				IntPoint lastInterimPoint =lastPoint;
				IntPoint nextInterimPoint =nextPoint;
				// find the closes cleared path within stepover distance, this will be keepToolDownPath if stright line is not possible
				Path keepToolDownLinkPath;
				ClipperOffset clipof;
				clipof.AddPaths(cleared,JoinType::jtRound,EndType::etClosedPolygon);
				Paths clearedOff;
				double offset=stepOverFactor*toolRadiusScaled;
				bool keepDownLinkExists = false;
				bool keepDownLinkTooLong = true;
				for(int i=1;i<10;i+=2) {
					clipof.Execute(clearedOff,-toolRadiusScaled-offset/i);
					// AddPathsToProgress(progressPaths,clearedOff, MotionType::mtLinkClear);
					// CheckReportProgress(progressPaths,true);
					// usleep(100000);
					keepDownLinkExists = FindPathBetweenClosestPoints(clearedOff,lastPoint,nextPoint,toolRadiusScaled + 2*offset/i ,keepToolDownLinkPath);
					if(keepDownLinkExists) {
						lastInterimPoint=keepToolDownLinkPath.front();
						nextInterimPoint=keepToolDownLinkPath.back();
						if(!IsAllowedToCutTrough(lastPoint,lastInterimPoint,cleared,toolBoundPaths)) {
							//cout << "not IsAllowedToCutTrough1" << endl;
							keepDownLinkExists=false;
						}
						if(!IsAllowedToCutTrough(nextPoint,nextInterimPoint,cleared,toolBoundPaths)) {
							//cout << "not IsAllowedToCutTrough2" << endl;
							keepDownLinkExists=false;
						}
						if(!IsClearPath(keepToolDownLinkPath,cleared,stepOverFactor*toolRadiusScaled/2)) {
							//cout << "not IsClearPath keepToolDownLinkPath" << endl;
							keepDownLinkExists=false;
						}
						// is inefficient to link through interim points?
						if(sqrt(DistanceSqrd(lastPoint,lastInterimPoint))+sqrt(DistanceSqrd(nextInterimPoint,nextPoint))
							> sqrt(DistanceSqrd(lastPoint,nextPoint))) keepDownLinkExists=false;
					}
					keepDownLinkTooLong = (PathLength(keepToolDownLinkPath) > linkDistance*keepToolDownDistRatio) && (linkDistance>4*toolRadiusScaled) ;
					if(keepDownLinkExists) break;
				}
				if(keepDownLinkExists)  {
					// check direct link
					Path tp;
					tp << lastInterimPoint;
					tp << nextInterimPoint;

					bool directLinkInterimLinkClear = IsClearPath(tp,cleared,stepOverFactor*toolRadiusScaled/2); // cleared direct line between interim points?
					if(directLinkInterimLinkClear) { // if direct link is ok
						// add disengage moves
						TPath linkPath1;
						linkPath1.first = MotionType::mtCutting;
						linkPath1.second.push_back(lastTPoint);
						linkPath1.second.push_back(DPoint(double(lastInterimPoint.X)/scaleFactor,double(lastInterimPoint.Y)/scaleFactor));
						output.AdaptivePaths.push_back(linkPath1);

						// add linking move
						TPath linkPath2;
						linkPath2.first = MotionType::mtLinkClear;
						linkPath2.second.push_back(DPoint(double(lastInterimPoint.X)/scaleFactor,double(lastInterimPoint.Y)/scaleFactor));
						linkPath2.second.push_back(DPoint(double(nextInterimPoint.X)/scaleFactor,double(nextInterimPoint.Y)/scaleFactor));
						output.AdaptivePaths.push_back(linkPath2);

						// add engage move
						TPath linkPath3;
						linkPath3.first = MotionType::mtCutting;
						linkPath3.second.push_back(DPoint(double(nextInterimPoint.X)/scaleFactor,double(nextInterimPoint.Y)/scaleFactor));
						linkPath3.second.push_back(DPoint(double(nextPoint.X)/scaleFactor,double(nextPoint.Y)/scaleFactor));
						output.AdaptivePaths.push_back(linkPath3);
						linkFound=true;
					}
					if(!linkFound && keepDownLinkTooLong) { // if keeptooldown link too long make link through interim points with tool raise
						// add disengage moves
						TPath linkPath1;
						linkPath1.first = MotionType::mtCutting;
						linkPath1.second.push_back(lastTPoint);
						linkPath1.second.push_back(DPoint(double(lastInterimPoint.X)/scaleFactor,double(lastInterimPoint.Y)/scaleFactor));
						output.AdaptivePaths.push_back(linkPath1);

						// add linking move at clear height
						TPath linkPath2;
						linkPath2.first = MotionType::mtLinkNotClear;
						linkPath2.second.push_back(DPoint(double(lastInterimPoint.X)/scaleFactor,double(lastInterimPoint.Y)/scaleFactor));
						linkPath2.second.push_back(DPoint(double(nextInterimPoint.X)/scaleFactor,double(nextInterimPoint.Y)/scaleFactor));
						output.AdaptivePaths.push_back(linkPath2);

						// add engage move
						TPath linkPath3;
						linkPath3.first = MotionType::mtCutting;
						linkPath3.second.push_back(DPoint(double(nextInterimPoint.X)/scaleFactor,double(nextInterimPoint.Y)/scaleFactor));
						linkPath3.second.push_back(DPoint(double(nextPoint.X)/scaleFactor,double(nextPoint.Y)/scaleFactor));
						output.AdaptivePaths.push_back(linkPath3);
						linkFound=true;
					}
					if(!linkFound) { // if direct link over interim points not clear and keepToolDownLinkPath not too long - make it through keep tool down link
						// add disengage move
						TPath linkPath1;
						linkPath1.first = MotionType::mtCutting;
						linkPath1.second.push_back(DPoint(double(lastPoint.X)/scaleFactor,double(lastPoint.Y)/scaleFactor));
						linkPath1.second.push_back(DPoint(double(lastInterimPoint.X)/scaleFactor,double(lastInterimPoint.Y)/scaleFactor));
						output.AdaptivePaths.push_back(linkPath1);

						// add linking path
						TPath linkPath2;
						linkPath2.first = MotionType::mtLinkClear;
						for(auto & p : keepToolDownLinkPath) {
							linkPath2.second.push_back(DPoint(double(p.X)/scaleFactor,double(p.Y)/scaleFactor));
						}
						output.AdaptivePaths.push_back(linkPath2);

						// add engage move
						TPath linkPath3;
						linkPath3.first = MotionType::mtCutting;
						linkPath3.second.push_back(DPoint(double(nextInterimPoint.X)/scaleFactor,double(nextInterimPoint.Y)/scaleFactor));
						linkPath3.second.push_back(DPoint(double(nextPoint.X)/scaleFactor,double(nextPoint.Y)/scaleFactor));
						output.AdaptivePaths.push_back(linkPath3);
						linkFound= true;
					}
				}
			}
			if(!linkFound) { // keeptool down link not found, try lift through (any) clear interim points
				ClipperOffset clipof;
				clipof.AddPaths(cleared,JoinType::jtRound,EndType::etClosedPolygon);
				Paths clearedOff;
				double offset=stepOverFactor*toolRadiusScaled;

				// see if link through interim points is possible,
				clipof.Execute(clearedOff,-toolRadiusScaled-offset);
				IntPoint interim1;
				IntPoint interim2;
				if(FindClosestClearPoints(clearedOff,lastPoint,nextPoint,toolRadiusScaled + 2*offset,interim1,interim2)) {
						if(IsAllowedToCutTrough(lastPoint,interim1,cleared,toolBoundPaths)
							&& IsAllowedToCutTrough(nextPoint,interim2,cleared,toolBoundPaths)) {

						// add disengage moves
						TPath linkPath1;
						linkPath1.first = MotionType::mtCutting;
						linkPath1.second.push_back(lastTPoint);
						linkPath1.second.push_back(DPoint(double(interim1.X)/scaleFactor,double(interim1.Y)/scaleFactor));
						output.AdaptivePaths.push_back(linkPath1);

						// add linking move
						TPath linkPath2;
						linkPath2.first = MotionType::mtLinkNotClear;
						linkPath2.second.push_back(DPoint(double(interim1.X)/scaleFactor,double(interim1.Y)/scaleFactor));
						linkPath2.second.push_back(DPoint(double(interim2.X)/scaleFactor,double(interim2.Y)/scaleFactor));
						output.AdaptivePaths.push_back(linkPath2);

						// add engage move
						TPath linkPath3;
						linkPath3.first = MotionType::mtCutting;
						linkPath3.second.push_back(DPoint(double(interim2.X)/scaleFactor,double(interim2.Y)/scaleFactor));
						linkPath3.second.push_back(DPoint(double(nextPoint.X)/scaleFactor,double(nextPoint.Y)/scaleFactor));
						output.AdaptivePaths.push_back(linkPath3);
						linkFound=true;
						}

				}
			}
			if(!linkFound) { // nothing clear so far - check direct link with no interim points - either this is clear or we need to raise the tool
					Path tp;
					tp << lastPoint;
					tp << nextPoint;
					MotionType mt = IsClearPath(tp,cleared) ? MotionType::mtLinkClear : MotionType::mtLinkNotClear;

					// make cutting move through small clear links
					if(mt==MotionType::mtLinkClear && linkDistance<toolRadiusScaled) mt=MotionType::mtCutting;

					TPath linkPath;
					linkPath.first = mt;
					linkPath.second.push_back(DPoint(double(lastPoint.X)/scaleFactor,double(lastPoint.Y)/scaleFactor));
					linkPath.second.push_back(DPoint(double(nextPoint.X)/scaleFactor,double(nextPoint.Y)/scaleFactor));
					output.AdaptivePaths.push_back(linkPath);
			}
		}
		TPath cutPath;
		cutPath.first =MotionType::mtCutting;
		for(const auto &p : passToolPath) {
			DPoint nextT;
			nextT.first = double(p.X)/scaleFactor;
			nextT.second = double(p.Y)/scaleFactor;
			cutPath.second.push_back(nextT);
		}

		// if(close) {
		// 	DPoint nextT;
		// 	nextT.first = double(passToolPath[0].X)/scaleFactor;
		// 	nextT.second = double(passToolPath[0].Y)/scaleFactor;
		// 	cutPath.second.push_back(nextT);
		// }
		if(cutPath.second.size()>0) output.AdaptivePaths.push_back(cutPath);
	}

	void Adaptive2d::CheckReportProgress(TPaths &progressPaths, bool force) {
		if(!force && (clock()-lastProgressTime<PROGRESS_TICKS)) return; // not yet
		lastProgressTime=clock();
		if(progressPaths.size()==0) return;
		if(progressCallback)
		if((*progressCallback)(progressPaths)) stopProcessing=true; // call python function, if returns true signal stop processing
		// clean the paths - keep the last point
		if(progressPaths.back().second.size()==0) return;
		TPath * lastPath = &progressPaths.back();
		DPoint *lastPoint =&lastPath->second.back();
		DPoint next(lastPoint->first,lastPoint->second);
		while(progressPaths.size()>1) progressPaths.pop_back();
		while(progressPaths.front().second.size()>0) progressPaths.front().second.pop_back();
		progressPaths.front().first = MotionType::mtCutting;
		progressPaths.front().second.push_back(next);
	}

	void Adaptive2d::AddPathsToProgress(TPaths &progressPaths,Paths paths, MotionType mt) {
		for(const auto & pth :paths) {
			if(pth.size()>0) {
				progressPaths.push_back(TPath());
				progressPaths.back().first = mt;
				for(const auto pt: pth)
					progressPaths.back().second.push_back(DPoint(double(pt.X)/scaleFactor,double(pt.Y)/scaleFactor));
				progressPaths.back().second.push_back(DPoint(double(pth.front().X)/scaleFactor,double(pth.front().Y)/scaleFactor));
			}
		}
	}

	void Adaptive2d::AddPathToProgress(TPaths &progressPaths,const Path pth, MotionType mt) {
		if(pth.size()>0) {
				progressPaths.push_back(TPath());
				progressPaths.back().first = mt;
				for(const auto pt: pth)
					progressPaths.back().second.push_back(DPoint(double(pt.X)/scaleFactor,double(pt.Y)/scaleFactor));
		}
	}


	void Adaptive2d::ProcessPolyNode(Paths boundPaths, Paths toolBoundPaths) {
		//cout << " Adaptive2d::ProcessPolyNode" << endl;
		Perf_ProcessPolyNode.Start();
		// node paths are already constrained to tool boundary path for adaptive path before finishing pass
		Clipper clip;
		ClipperOffset clipof;

		IntPoint entryPoint;
		TPaths progressPaths;
		progressPaths.reserve(10000);

		// AddPathsToProgress(progressPaths,boundPaths, MotionType::mtLinkClear);
		// CheckReportProgress(progressPaths, true);
		// std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		CleanPolygons(toolBoundPaths);
		SimplifyPolygons(toolBoundPaths);

		CleanPolygons(boundPaths);
		SimplifyPolygons(boundPaths);


		AddPathsToProgress(progressPaths,toolBoundPaths, MotionType::mtLinkClear);

		IntPoint toolPos;
		DoublePoint toolDir;
		Paths cleared;
		bool outsideEntry = false;
		bool firstEngagePoint=true;
		Paths engageBounds = toolBoundPaths;


		if(!forceInsideOut && FindEntryPointOutside(progressPaths, toolBoundPaths, boundPaths, cleared, entryPoint, toolPos,toolDir)) {
			if( Orientation(engageBounds[0]) == false) ReversePath(engageBounds[0]);
			//engageBounds.erase(engageBounds.begin());
			// add initial offset of cleared area to engage paths
			Paths outsideEngage;
			clipof.Clear();
			clipof.AddPaths(stockInputPaths,JoinType::jtRound,EndType::etClosedPolygon);
			clipof.Execute(outsideEngage,toolRadiusScaled - stepOverFactor*toolRadiusScaled);
			CleanPolygons(outsideEngage);
			ReversePaths(outsideEngage);
			for(auto p:outsideEngage) engageBounds.push_back(p);
			outsideEntry=true;
		} else {
			if(!FindEntryPoint(progressPaths, toolBoundPaths, boundPaths, cleared, entryPoint, toolPos,toolDir)) return;
		}

		EngagePoint engage(engageBounds); // engage point stepping instance


		if(outsideEntry) {
			engage.moveToClosestPoint(toolPos,2*RESOLUTION_FACTOR);
			engage.moveForward(RESOLUTION_FACTOR);
			toolPos = engage.getCurrentPoint();
			toolDir = engage.getCurrentDir();
			entryPoint=toolPos;
		}

		//cout << "Entry point:" << double(entryPoint.X)/scaleFactor << "," << double(entryPoint.Y)/scaleFactor << endl;

		AdaptiveOutput output;
		output.HelixCenterPoint.first = double(entryPoint.X)/scaleFactor;
		output.HelixCenterPoint.second =double(entryPoint.Y)/scaleFactor;

		long stepScaled = long(RESOLUTION_FACTOR);
		IntPoint engagePoint;


		IntPoint newToolPos;
		DoublePoint newToolDir;

		CheckReportProgress(progressPaths, true);

		IntPoint startPoint = toolPos;
		output.StartPoint =DPoint(double(startPoint.X)/scaleFactor,double(startPoint.Y)/scaleFactor);


		Path passToolPath; // to store pass toolpath
		Path toClearPath; // to clear toolpath
		IntPoint clp; // to store closest point
		vector<DoublePoint> gyro; // used to average tool direction
		vector<double> angleHistory; // use to predict deflection angle
		double angle = M_PI;
		engagePoint = toolPos;
		Interpolation interp; // interpolation instance

		long total_iterations =0;
		long total_points =0;
		long total_exceeded=0;
		long total_output_points=0;
		long over_cut_count =0;
		long bad_engage_count=0;
		unclearLinkingMoveCount=0;
		//long engage_no_cut_count=0;
		double prevDistFromStart =0;
		bool prevDistTrend = false;

		double perf_total_len=0;
		#ifdef DEV_MODE
		clock_t start_clock=clock();
		#endif
		Paths clearedBeforePass;
		/*******************************
		 * LOOP - PASSES
		 *******************************/
		for(long pass=0;pass<PASSES_LIMIT;pass++) {
			if(stopProcessing) break;
			//cout<<"Pass:"<< pass << endl;
			passToolPath.clear();
			toClearPath.clear();
			angleHistory.clear();

			// append a new path to progress info paths
			if(progressPaths.size()==0) {						
				progressPaths.push_back(TPath());
			} else {
				// append new path if previous not empty
				if(progressPaths.back().second.size()>0)
						progressPaths.push_back(TPath());
			}

			angle = M_PI/4; // initial pass angle
			bool reachedBoundary = false;
			double cumulativeCutArea=0;
			// init gyro
			gyro.clear();
			for(int i=0;i<DIRECTION_SMOOTHING_BUFLEN;i++) gyro.push_back(toolDir);

			size_t clpPathIndex;
			size_t clpSegmentIndex;
			double clpParamter;
			clearedBeforePass = cleared;
			/*******************************
			 * LOOP - POINTS
			 *******************************/
			for(long point_index=0;point_index<POINTS_PER_PASS_LIMIT;point_index++) {
				if(stopProcessing) break;
				//cout<<"Pass:"<< pass << " Point:" << point_index << endl;
				total_points++;
				AverageDirection(gyro, toolDir);
				Perf_DistanceToBoundary.Start();

				//double distanceToBoundary = __DBL_MAX__;
				double distanceToBoundary = sqrt(DistancePointToPathsSqrd(toolBoundPaths, toolPos, clp, clpPathIndex, clpSegmentIndex,clpParamter));
				// double range = 2*toolRadiusScaled*stepOverFactor;
				// if(IntersectionPoint(toolBoundPaths,toolPos,IntPoint(toolPos.X + range* toolDir.X,toolPos.Y + range* toolDir.Y), clp)) {
				// 	distanceToBoundary=sqrt(DistanceSqrd(toolPos,clp));
				// }
				Perf_DistanceToBoundary.Stop();
				double distanceToEngage = sqrt(DistanceSqrd(toolPos,engagePoint));
				//double relDistToBoundary = distanceToBoundary/toolRadiusScaled ;

				double	targetAreaPD =  optimalCutAreaPD; //*(0.8*(1-exp(-4*distanceToBoundary/toolRadiusScaled)) + 0.2);

				// set the step size
				double slowDownDistance = max(double(toolRadiusScaled)/4,RESOLUTION_FACTOR*8);
				if(distanceToBoundary<slowDownDistance || distanceToEngage<slowDownDistance) {
					stepScaled = long(RESOLUTION_FACTOR);
				} else if(fabs(angle)>1e-5) {
					stepScaled = long(RESOLUTION_FACTOR/fabs(angle));
				} else {
					stepScaled = long(RESOLUTION_FACTOR*4);
				}



				// clamp the step size - for stability
				if(stepScaled>min(long(toolRadiusScaled/4), long(RESOLUTION_FACTOR*8)))
					stepScaled=min(long(toolRadiusScaled/4), long(RESOLUTION_FACTOR*8));
				if(stepScaled<RESOLUTION_FACTOR) stepScaled=long(RESOLUTION_FACTOR);

				//stepScaled=RESOLUTION_FACTOR;

				/************************************
				 * ANGLE vs AREA ITERATIONS
				 *********************************/
				double predictedAngle = averageDV(angleHistory);
				double maxError = AREA_ERROR_FACTOR * optimalCutAreaPD;
				double area=0;
				double areaPD=0;
				interp.clear();
				/******************************/
				Perf_PointIterations.Start();
				int iteration;
				double prev_error=__DBL_MAX__;
				for(iteration=0;iteration<MAX_ITERATIONS;iteration++) {
					total_iterations++;
					if(iteration==0) angle=predictedAngle;
					else if(iteration==1) angle=interp.MIN_ANGLE; // max engage
					else if(iteration==3) angle=interp.MAX_ANGLE; // min engage
					else if(interp.getPointCount()<2) angle=interp.getRandomAngle();
					else angle=interp.interpolateAngle(targetAreaPD);
					angle=interp.clampAngle(angle);

					newToolDir = rotate(toolDir,angle);
					newToolPos = IntPoint(long(toolPos.X + newToolDir.X * stepScaled), long(toolPos.Y + newToolDir.Y * stepScaled));

					area = CalcCutArea(clip, toolPos,newToolPos, cleared);
					areaPD = area/double(stepScaled); // area per distance
					interp.addPoint(areaPD,angle);
					double error=areaPD-targetAreaPD;
				//	cout << " iter:" << iteration << " angle:" << angle << " area:" << areaPD << " target:" << targetAreaPD << " error:" << error << " max:"<< maxError << endl;
					if (fabs(error) < maxError) {
						angleHistory.push_back(angle);
						if(angleHistory.size() > ANGLE_HISTORY_POINTS)
							angleHistory.erase(angleHistory.begin());
						break;
					}
					if(iteration>5 && fabs(error-prev_error)<0.001) break;
					if(iteration==MAX_ITERATIONS-1) total_exceeded++;
					prev_error = error;
				}
				Perf_PointIterations.Stop();
				/************************************************
				 * CHECK AND RECORD NEW TOOL POS
				 * **********************************************/
				if(!IsPointWithinCutRegion(toolBoundPaths,newToolPos)) {
						reachedBoundary=true;
						// we reached end of cutting area
						IntPoint boundaryPoint;
						if(IntersectionPoint(toolBoundPaths,toolPos,newToolPos, boundaryPoint)) {
							newToolPos=boundaryPoint;
							area = CalcCutArea(clip,toolPos,newToolPos,cleared);
							double dist = sqrt(DistanceSqrd(toolPos, newToolPos));
							if(dist>NTOL)
								areaPD = area/double(dist); // area per distance
							else {
								areaPD=0;
								area=0;
							}

						} else {
							newToolPos=toolPos;
							area=0;
							areaPD=0;
						}
				}

				if(area>stepScaled*optimalCutAreaPD && areaPD>2*optimalCutAreaPD) { // safety condition
					over_cut_count++;
					#ifdef DEV_MODE
					cout<<"Break: over cut @"  << point_index  << "(" << double(toolPos.X)/scaleFactor << ","<< double(toolPos.Y)/scaleFactor  << ")"
								<< " iter:" << iteration << " @bound:" << reachedBoundary << endl;
					#endif
					// ClearScreenFn();
					// DrawCircle(toolPos,toolRadiusScaled,0);
					// DrawCircle(newToolPos,toolRadiusScaled,1);
					// DrawPaths(cleared,22);
					break;
				}



				// update cleared paths when trend of distance from start point changes sign (starts to get closer, or start to get farther)
				double distFromStart = sqrt(DistanceSqrd(toolPos,startPoint));
				bool distanceTrend = distFromStart > prevDistFromStart ? true : false;

				if(distanceTrend!=prevDistTrend || toClearPath.size()>10) {
						Perf_ExpandCleared.Start();
						// expand cleared
						clipof.Clear();
						clipof.AddPath(toClearPath,JoinType::jtRound,EndType::etOpenRound);
						Paths toolCoverPoly;
						clipof.Execute(toolCoverPoly,toolRadiusScaled+1);
						clip.Clear();
						clip.AddPaths(cleared,PolyType::ptSubject,true);
						clip.AddPaths(toolCoverPoly,PolyType::ptClip,true);
						clip.Execute(ClipType::ctUnion,cleared);
						CleanPolygons(cleared);
						toClearPath.clear();
						Perf_ExpandCleared.Stop();
				}
				prevDistTrend = distanceTrend;
				prevDistFromStart = distFromStart;


				if(area>0) { // cut is ok - record it
					if(toClearPath.size()==0) toClearPath.push_back(toolPos);
					toClearPath.push_back(newToolPos);

					cumulativeCutArea+=area;

					// append to toolpaths
					if(passToolPath.size()==0) {
						// in outside entry first successfull cut defines the helix center and start point
						if(output.AdaptivePaths.size()==0 && outsideEntry) {
								entryPoint=toolPos;
								output.HelixCenterPoint.first = double(entryPoint.X)/scaleFactor;
								output.HelixCenterPoint.second =double(entryPoint.Y)/scaleFactor;
								output.StartPoint =DPoint(double(entryPoint.X)/scaleFactor,double(entryPoint.Y)/scaleFactor);
						}
						passToolPath.push_back(toolPos);
					}
					passToolPath.push_back(newToolPos);
					perf_total_len+=stepScaled;
					toolPos=newToolPos;

					// append to progress info paths
					if(progressPaths.size()==0) progressPaths.push_back(TPath());
					progressPaths.back().second.push_back(DPoint(double(newToolPos.X)/scaleFactor,double(newToolPos.Y)/scaleFactor));

					// apend gyro
					gyro.push_back(newToolDir);
					gyro.erase(gyro.begin());
					CheckReportProgress(progressPaths);
				} else {
					#ifdef DEV_MODE
						// if(point_index==0) {
						// 	engage_no_cut_count++;
						// 	cout<<"Break:no cut #" << engage_no_cut_count << ", bad engage, pass:" << pass << " over_cut_count:" << over_cut_count << endl;
						// }
					#endif
					//cerr<<"Break: no cut @" << point_index << endl;
					break;
				}
				if(reachedBoundary)
					break;
			} /* end of points loop*/

			if(toClearPath.size()>0) {
				// expand cleared
				Perf_ExpandCleared.Start();
				clipof.Clear();
				clipof.AddPath(toClearPath,JoinType::jtRound,EndType::etOpenRound);
				Paths toolCoverPoly;
				clipof.Execute(toolCoverPoly,toolRadiusScaled+1);
				clip.Clear();
				clip.AddPaths(cleared,PolyType::ptSubject,true);
				clip.AddPaths(toolCoverPoly,PolyType::ptClip,true);
				clip.Execute(ClipType::ctUnion,cleared);
				CleanPolygons(cleared);
				toClearPath.clear();
				Perf_ExpandCleared.Stop();
			}
			if(cumulativeCutArea>tolerance*MIN_CUT_AREA_FACTOR*stepScaled*stepOverFactor*referenceCutArea) {
				Path cleaned;
				CleanPath(passToolPath,cleaned,CLEAN_PATH_TOLERANCE);
				total_output_points+=long(cleaned.size());
				AppendToolPath(progressPaths, output,cleaned,clearedBeforePass,toolBoundPaths);
				CheckReportProgress(progressPaths);
				bad_engage_count=0;
				//engage.ResetPasses();
				//firstEngagePoint=true;
			} else {
				bad_engage_count++;
			}
			/*****NEXT ENGAGE POINT******/
			if(firstEngagePoint) {
				engage.moveToClosestPoint(newToolPos,stepScaled+1);
				firstEngagePoint=false;
			} else {
				// check which is better to find next cut from closest point or to continue from current
				double moveDistance = ENGAGE_SCAN_DISTANCE_FACTOR * RESOLUTION_FACTOR * 8;
				if(!engage.nextEngagePoint(this, cleared,moveDistance,
					ENGAGE_AREA_THR_FACTOR*optimalCutAreaPD*RESOLUTION_FACTOR,
					4*referenceCutArea*stepOverFactor)) {
					break;
				}
			}
			toolPos = engage.getCurrentPoint();
			toolDir = engage.getCurrentDir();
		}
		/**********************************/
		/*  FINISHING PASS                */
		/**********************************/

		Paths finishingPaths;
		clipof.Clear();
		clipof.AddPaths(boundPaths,JoinType::jtRound,EndType::etClosedPolygon);
		clipof.Execute(finishingPaths,-toolRadiusScaled);
		IntPoint lastPoint = toolPos;
		Path finShiftedPath;

		bool allCutsAllowed=true;
		while(!stopProcessing && PopPathWithClosestPoint(finishingPaths,lastPoint,finShiftedPath)) {
			if(finShiftedPath.empty()) continue;
			// skip finishing passes outside the stock boundary - make no sense to cut where is no material
			bool allPointsOutside=true;
			IntPoint p1 = finShiftedPath.front();
			for(const auto & pt : finShiftedPath) {

				// midpoint
				if(IsPointWithinCutRegion(stockInputPaths,IntPoint((p1.X+pt.X)/2,(p1.Y+pt.Y)/2))) {
					allPointsOutside=false;
					break;
				}
				//current point
				if(IsPointWithinCutRegion(stockInputPaths,pt)) {
					allPointsOutside=false;
					break;
				}

				p1=pt;
			}
			if(allPointsOutside) continue;

			// shift the path so that is starts with the closest point to current tool pos

			progressPaths.push_back(TPath());
			// show in progress cb
			for(auto & pt:finShiftedPath) {
				progressPaths.back().second.push_back(DPoint(double(pt.X)/scaleFactor,double(pt.Y)/scaleFactor));
			}

			if(!finShiftedPath.empty()) finShiftedPath << finShiftedPath.front(); // make sure its closed

			//safety check for finishing paths - check the area of finishing cut
			for(size_t i=1;i<finShiftedPath.size();i++) {
				if(!IsAllowedToCutTrough(finShiftedPath.at(i-1),finShiftedPath.at(i),cleared,toolBoundPaths,2.0,true)) allCutsAllowed=false;
			}

			Path finCleaned;
			CleanPath(finShiftedPath,finCleaned,FINISHING_CLEAN_PATH_TOLERANCE);


			AppendToolPath(progressPaths,output,finCleaned,clearedBeforePass,toolBoundPaths);
			//expand cleared for finishing path
			clipof.Clear();
			clipof.AddPath(finCleaned,JoinType::jtRound,EndType::etOpenRound);
			Paths toolCoverPoly;
			clipof.Execute(toolCoverPoly,toolRadiusScaled+1);

			clip.Clear();
			clip.AddPaths(cleared,PolyType::ptSubject,true);
			clip.AddPaths(toolCoverPoly,PolyType::ptClip,true);
			clip.Execute(ClipType::ctUnion,cleared);
			CleanPolygons(cleared);
			if(!finCleaned.empty()) {
				lastPoint.X = finCleaned.back().X;
				lastPoint.Y = finCleaned.back().Y;
			}
		}

		Path returnPath;
		returnPath << lastPoint;
		returnPath << entryPoint;
		output.ReturnMotionType = IsClearPath(returnPath,cleared) ? MotionType::mtLinkClear : MotionType::mtLinkNotClear;

		// dump performance results
		#ifdef DEV_MODE
			Perf_ProcessPolyNode.Stop();
			Perf_ProcessPolyNode.DumpResults();
			Perf_PointIterations.DumpResults();
			Perf_CalcCutArea.DumpResults();
			Perf_NextEngagePoint.DumpResults();
			Perf_ExpandCleared.DumpResults();
			Perf_DistanceToBoundary.DumpResults();
		#endif
		CheckReportProgress(progressPaths, true);
		#ifdef DEV_MODE
			double duration=((double)(clock()-start_clock))/CLOCKS_PER_SEC;
			cout<<"PolyNode perf:"<< perf_total_len/double(scaleFactor)/duration << " mm/sec"
				<< " processed_points:" << total_points
				<< " output_points:" << total_output_points
				<< " total_iterations:" << total_iterations
				<< " iter_per_point:" << (double(total_iterations)/((double(total_points)+0.001)))
				<< " total_exceeded:" << total_exceeded  <<  " (" << 100 * double(total_exceeded)/double(total_points) << "%)"
				<< " linking moves:" << unclearLinkingMoveCount
				<< endl;
		#endif

		// make sure invalid paths are not used
		if(!allCutsAllowed) {
			cerr << "INVALID CUTS DETECTED! Please try to modify accuracy and/or step-over." << endl;
		}

		results.push_back(output);
	}

}