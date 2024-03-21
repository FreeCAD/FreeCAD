// Arc.h
// Copyright 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

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
	void GetSegments(void(*callbackfunc)(const double *p), double pixels_per_mm)const;
};