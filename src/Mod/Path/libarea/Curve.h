// Curve.h
// Copyright 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#pragma once

#include <vector>
#include <list>
#include <math.h>
#include "Point.h"
#include "Box2D.h"

class Line{
public:
	Point p0;
	Point v;

	// constructors
	Line(const Point& P0, const Point& V);

	double Dist(const Point& p)const;
};

class CArc;

class CVertex
{
public:
	int m_type; // 0 - line ( or start point ), 1 - anti-clockwise arc, -1 - clockwise arc
	Point m_p; // end point
	Point m_c; // centre point in absolute coordinates
	int m_user_data;

	CVertex():m_type(0), m_p(Point(0, 0)), m_c(Point(0,0)), m_user_data(0){}
	CVertex(int type, const Point& p, const Point& c, int user_data = 0);
	CVertex(const Point& p, int user_data = 0);
};

class Span
{
	Point NearestPointNotOnSpan(const Point& p)const;
	double Parameter(const Point& p)const;
	Point NearestPointToSpan(const Span& p, double &d)const;

	static const Point null_point;
	static const CVertex null_vertex;

public:
	bool m_start_span;
	Point m_p;
	CVertex m_v;
	Span();
	Span(const Point& p, const CVertex& v, bool start_span = false):m_start_span(start_span), m_p(p), m_v(v){}
	Point NearestPoint(const Point& p)const;
	Point NearestPoint(const Span& p, double *d = NULL)const;
	void GetBox(CBox2D &box);
	double IncludedAngle()const;
	double GetArea()const;
	bool On(const Point& p, double* t = NULL)const;
	Point MidPerim(double d)const;
	Point MidParam(double param)const;
	double Length()const;
	Point GetVector(double fraction)const;
	void Intersect(const Span& s, std::list<Point> &pts)const; // finds all the intersection points between two spans
};

class CCurve
{
	// a closed curve, please make sure you add an end point, the same as the start point

protected:
	void AddArcOrLines(bool check_for_arc, std::list<CVertex> &new_vertices, std::list<const CVertex*>& might_be_an_arc, CArc &arc, bool &arc_found, bool &arc_added);
	bool CheckForArc(const CVertex& prev_vt, std::list<const CVertex*>& might_be_an_arc, CArc &arc);

public:
	std::list<CVertex> m_vertices;
	void append(const CVertex& vertex);

	void FitArcs();
	void UnFitArcs();
	Point NearestPoint(const Point& p)const;
	Point NearestPoint(const CCurve& p, double *d = NULL)const;
	Point NearestPoint(const Span& p, double *d = NULL)const;
	void GetBox(CBox2D &box);
	void Reverse();
	double GetArea()const;
	bool IsClockwise()const{return GetArea()>0;}
	bool IsClosed()const;
	void ChangeStart(const Point &p);
	void ChangeEnd(const Point &p);
	bool Offset(double leftwards_value);
	void OffsetForward(double forwards_value, bool refit_arcs = true); // for drag-knife compensation
	void Break(const Point &p);
	void ExtractSeparateCurves(const std::list<Point> &ordered_points, std::list<CCurve> &separate_curves)const;
	double Perim()const;
	Point PerimToPoint(double perim)const;
	double PointToPerim(const Point& p)const;
	void GetSpans(std::list<Span> &spans)const;
	void RemoveTinySpans();
	void operator+=(const CCurve& p);
	void SpanIntersections(const Span& s, std::list<Point> &pts)const;
	void CurveIntersections(const CCurve& c, std::list<Point> &pts)const;
};

void tangential_arc(const Point &p0, const Point &p1, const Point &v0, Point &c, int &dir);
