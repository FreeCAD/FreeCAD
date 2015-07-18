// Arc.cpp

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



#include "Arc.h"
#include "Curve.h"

void CArc::SetDirWithPoint(const Point& p)
{
	double angs = atan2(m_s.y - m_c.y, m_s.x - m_c.x);
	double ange = atan2(m_e.y - m_c.y, m_e.x - m_c.x);
	double angp = atan2(p.y - m_c.y, p.x - m_c.x);
	if(ange < angs)ange += 6.2831853071795864;
	if(angp < angs - 0.0000000000001)angp += 6.2831853071795864;
	if(angp > ange + 0.0000000000001)m_dir = false;
	else m_dir = true;
}

double CArc::IncludedAngle()const
{
	double angs = atan2(m_s.y - m_c.y, m_s.x - m_c.x);
	double ange = atan2(m_e.y - m_c.y, m_e.x - m_c.x);
	if(m_dir)
	{
		// make sure ange > angs
		if(ange < angs)ange += 6.2831853071795864;
	}
	else
	{
		// make sure angs > ange
		if(angs < ange)angs += 6.2831853071795864;
	}

	return fabs(ange - angs);
}

bool CArc::AlmostALine()const
{
	Point mid_point = MidParam(0.5);
	if(Line(m_s, m_e - m_s).Dist(mid_point) <= Point::tolerance)
		return true;

	const double max_arc_radius = 1.0 / Point::tolerance;
	double radius = m_c.dist(m_s);
	if (radius > max_arc_radius)
	{
		return true;	// We don't want to produce an arc whose radius is too large.
	}

	return false;
}

Point CArc::MidParam(double param)const {
	/// returns a point which is 0-1 along arc
	if(fabs(param) < 0.00000000000001)return m_s;
	if(fabs(param - 1.0) < 0.00000000000001)return m_e;

	Point p;
	Point v = m_s - m_c;
	v.Rotate(param * IncludedAngle());
	p = v + m_c;

	return p;
}

//segments - number of segments per full revolution!
//d_angle - determines the direction and the ammount of the arc to draw
void CArc::GetSegments(void(*callbackfunc)(const double *p), double pixels_per_mm, bool want_start_point)const
{
	if(m_s == m_e)
		return;

	Point Va = m_s - m_c;
	Point Vb = m_e - m_c;

	double start_angle = atan2(Va.y, Va.x);
	double end_angle = atan2(Vb.y, Vb.x);

	if(m_dir)
	{
		if(start_angle > end_angle)end_angle += 6.28318530717958;
	}
	else
	{
		if(start_angle < end_angle)end_angle -= 6.28318530717958;
	}

	double radius = m_c.dist(m_s);
	double d_angle = end_angle - start_angle;
	int segments = (int)(fabs(pixels_per_mm * radius * d_angle / 6.28318530717958 + 1));

    double theta = d_angle / (double)segments;
	while(theta>1.0){segments*=2;theta = d_angle / (double)segments;}
    double tangetial_factor = tan(theta);
    double radial_factor = 1 - cos(theta);

    double x = radius * cos(start_angle);
    double y = radius * sin(start_angle);

	double pp[3] = {0.0, 0.0, 0.0};

   for(int i = 0; i < segments + 1; i++)
    {
		Point p = m_c + Point(x, y);
		pp[0] = p.x;
		pp[1] = p.y;
		(*callbackfunc)(pp);

        double tx = -y;
        double ty = x;

        x += tx * tangetial_factor;
        y += ty * tangetial_factor;

        double rx = - x;
        double ry = - y;

        x += rx * radial_factor;
        y += ry * radial_factor;
    }
}