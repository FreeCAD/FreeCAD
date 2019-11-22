// Point.h

/*==============================
Copyright (c) 2011-2015 Dan Heeks

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
==============================*/


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
	bool operator==(const Point& p)const;
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
