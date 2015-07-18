// Arc.h
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

#include "Point.h"

class CArc{
public:
	Point m_s;
	Point m_e;
	Point m_c;
	bool m_dir; // true - anti-clockwise, false - clockwise
	int m_user_data;

	CArc():m_dir(true), m_user_data(0){}
	CArc(const Point& s, const Point& e, const Point& c, bool dir, int user_data):m_s(s), m_e(e), m_c(c), m_dir(dir), m_user_data(user_data){}

	void SetDirWithPoint(const Point& p); // set m_dir, such that this point lies between m_s and m_e
	double IncludedAngle()const; // always > 0
	bool AlmostALine()const;
	Point MidParam(double param)const;
	void GetSegments(void(*callbackfunc)(const double *p), double pixels_per_mm, bool want_start_point = true)const;
};