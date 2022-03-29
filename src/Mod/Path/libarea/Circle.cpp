// Circle.cpp

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


#include "Circle.h"

Circle::Circle(const Point& p0, const Point& p1, const Point& p2)
{
	// from TangentCircles in http://code.google.com/p/heekscad/source/browse/trunk/src/Geom.cpp

	// set default values, in case this fails
	m_radius = 0.0;
	m_c = Point(0, 0);

	double x1 = p0.x;
	double y1 = p0.y;
	double x2 = p1.x;
	double y2 = p1.y;
	double x3 = p2.x;
	double y3 = p2.y;

	double a = 2 * (x1 - x2);
	double b = 2 * (y1 - y2);
	double d = (x1 * x1 + y1 * y1) - (x2 * x2 + y2 * y2);

	double A = 2 * (x1 - x3);
	double B = 2 * (y1 - y3);
	double D = (x1 * x1 + y1 * y1) - (x3 * x3 + y3 * y3);

	double aBmbA = (a*B - b*A); // aB - bA

	// x = k + Kr where
	double k = (B*d - b*D) / aBmbA;

	// y = l + Lr where
	double l = (-A*d + a*D)/ aBmbA;

	double qa = -1;
	double qb = 0.0;
	double qc = k*k + x1*x1 -2*k*x1 + l*l + y1*y1 - 2*l*y1;

	// solve the quadratic equation, r = (-b +- sqrt(b*b - 4*a*c))/(2 * a)
	for(int qs = 0; qs<2; qs++){
		double bb = qb*qb;
		double ac4 = 4*qa*qc;
		if(ac4 <= bb){
			double r = (-qb + ((qs == 0) ? 1 : -1) * sqrt(bb - ac4))/(2 * qa);
			double x = k;
			double y = l;

			// set the circle
			if(r >= 0.0){
				m_c = Point(x, y);
				m_radius = r;
			}
		}
	}
}

bool Circle::PointIsOn(const Point& p, double accuracy)
{
	double rp = p.dist(m_c);
	bool on = fabs(m_radius - rp) < accuracy;
	return on;
}

bool Circle::LineIsOn(const Point& p0, const Point& p1, double accuracy)
{
	// checks the points are on the arc, to the given accuracy, and the mid point of the line.

	if(!PointIsOn(p0, accuracy))
	    return false;
	if(!PointIsOn(p1, accuracy))
	    return false;

	Point mid = Point((p0 + p1)/2);
	if(!PointIsOn(mid, accuracy))
	    return false;

	return true;
}