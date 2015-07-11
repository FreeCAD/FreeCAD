// Point.h

// Copyright 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#pragma once

#include <cmath>
#include "kurve/geometry.h"

class Point{
public:
	// can be a position, or a vector
	double x, y;

	Point():x(0.0), y(0.0){}
	Point(double X, double Y):x(X), y(Y){}
	Point(const double* p):x(p[0]), y(p[1]){}
	Point(const Point& p0, const Point& p1):x(p1.x - p0.x), y(p1.y - p0.y){} // vector from p0 to p1

	static double tolerance;

	const Point operator+(const Point& p)const{return Point(x + p.x, y + p.y);}
	const Point operator-(const Point& p)const{return Point(x - p.x, y - p.y);}
	const Point operator*(double d)const{return Point(x * d, y * d);}
	const Point operator/(double d)const{return Point(x / d, y / d);}
	bool operator==(const Point& p)const{return fabs(x-p.x)<tolerance && fabs(y-p.y)<tolerance;}
	bool operator!=(const Point &p)const{ return !(*this == p);}
	double dist(const Point &p)const{double dx = p.x - x; double dy = p.y - y; return sqrt(dx*dx + dy*dy);}
    double length()const;
    double normalize();
	double operator*(const Point &p)const{return (x * p.x + y * p.y);}// dot product
	double operator^(const Point &p)const{return (x * p.y - y * p.x);}// cross product m0.m1.sin a = v0 ^ v1
	Point operator~(void)const{return Point(-y, x);}// perp to left
	Point operator-(void)const{return Point(-x, -y);}// v1 = -v0;  (unary minus)
	void Rotate(double cosa, double sina){// rotate vector by angle
		double temp = -y * sina + x * cosa;
		y = x * sina + cosa * y;
		x = temp;
	}	
	void Rotate(double angle){if(fabs(angle) < 1.0e-09)return; Rotate(cos(angle), sin(angle));}
	void Transform(const geoff_geometry::Matrix &m)
	{
		geoff_geometry::Point p(x,y);
		p = p.Transform(m);
		x = p.x;
		y = p.y;
	}
};

const Point operator*(const double &d, const Point &p);
