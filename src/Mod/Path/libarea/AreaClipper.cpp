// AreaClipper.cpp

// implements CArea methods using Angus Johnson's "Clipper"

#include "Area.h"
#include "clipper.hpp"
using namespace ClipperLib;

#define TPolygon Path
#define TPolyPolygon Paths

bool CArea::HolesLinked(){ return false; }

//static const double PI = 3.1415926535897932;
static double Clipper4Factor = 10000.0;

class DoubleAreaPoint
{
public:
	double X, Y;

	DoubleAreaPoint(double x, double y){X = x; Y = y;}
	DoubleAreaPoint(const IntPoint& p){X = (double)(p.X) / Clipper4Factor; Y = (double)(p.Y) / Clipper4Factor;}
	IntPoint int_point(){return IntPoint((long64)(X * Clipper4Factor), (long64)(Y * Clipper4Factor));}
};

static std::list<DoubleAreaPoint> pts_for_AddVertex;

static void AddPoint(const DoubleAreaPoint& p)
{
	pts_for_AddVertex.push_back(p);
}

static void AddVertex(const CVertex& vertex, const CVertex* prev_vertex)
{
	if(vertex.m_type == 0 || prev_vertex == NULL)
	{
		AddPoint(DoubleAreaPoint(vertex.m_p.x * CArea::m_units, vertex.m_p.y * CArea::m_units));
	}
	else
	{
		if(vertex.m_p != prev_vertex->m_p)
		{
		double phi,dphi,dx,dy;
		int Segments;
		int i;
		double ang1,ang2,phit;

		dx = (prev_vertex->m_p.x - vertex.m_c.x) * CArea::m_units;
		dy = (prev_vertex->m_p.y - vertex.m_c.y) * CArea::m_units;

		ang1=atan2(dy,dx);
		if (ang1<0) ang1+=2.0*PI;
		dx = (vertex.m_p.x - vertex.m_c.x) * CArea::m_units;
		dy = (vertex.m_p.y - vertex.m_c.y) * CArea::m_units;
		ang2=atan2(dy,dx);
		if (ang2<0) ang2+=2.0*PI;

		if (vertex.m_type == -1)
		{ //clockwise
			if (ang2 > ang1)
				phit=2.0*PI-ang2+ ang1;
			else
				phit=ang1-ang2;
		}
		else
		{ //counter_clockwise
			if (ang1 > ang2)
				phit=-(2.0*PI-ang1+ ang2);
			else
				phit=-(ang2-ang1);
		}

		//what is the delta phi to get an accurancy of aber
		double radius = sqrt(dx*dx + dy*dy);
		dphi=2*acos((radius-CArea::m_accuracy)/radius);

		//set the number of segments
		if (phit > 0)
			Segments=(int)ceil(phit/dphi);
		else
			Segments=(int)ceil(-phit/dphi);

		if (Segments < 1)
			Segments=1;
		if (Segments > 100)
			Segments=100;

		dphi=phit/(Segments);

		double px = prev_vertex->m_p.x * CArea::m_units;
		double py = prev_vertex->m_p.y * CArea::m_units;

		for (i=1; i<=Segments; i++)
		{
			dx = px - vertex.m_c.x * CArea::m_units;
			dy = py - vertex.m_c.y * CArea::m_units;
			phi=atan2(dy,dx);

			double nx = vertex.m_c.x * CArea::m_units + radius * cos(phi-dphi);
			double ny = vertex.m_c.y * CArea::m_units + radius * sin(phi-dphi);

			AddPoint(DoubleAreaPoint(nx, ny));

			px = nx;
			py = ny;
		}
		}
	}
}

static bool IsPolygonClockwise(const TPolygon& p)
{
#if 1
	double area = 0.0;
	unsigned int s = p.size();
	for(unsigned int i = 0; i<s; i++)
	{
		int im1 = i-1;
		if(im1 < 0)im1 += s;

		DoubleAreaPoint pt0(p[im1]);
		DoubleAreaPoint pt1(p[i]);

		area += 0.5 * (pt1.X - pt0.X) * (pt0.Y + pt1.Y);
	}

	return area > 0.0;
#else
	return IsClockwise(p);
#endif
}

static void MakeLoop(const DoubleAreaPoint &pt0, const DoubleAreaPoint &pt1, const DoubleAreaPoint &pt2, double radius)
{
	Point p0(pt0.X, pt0.Y);
	Point p1(pt1.X, pt1.Y);
	Point p2(pt2.X, pt2.Y);
	Point forward0 = p1 - p0;
	Point right0(forward0.y, -forward0.x);
	right0.normalize();
	Point forward1 = p2 - p1;
	Point right1(forward1.y, -forward1.x);
	right1.normalize();

	int arc_dir = (radius > 0) ? 1 : -1;

	CVertex v0(0, p1 + right0 * radius, Point(0, 0));
	CVertex v1(arc_dir, p1 + right1 * radius, p1);
	CVertex v2(0, p2 + right1 * radius, Point(0, 0));

	double save_units = CArea::m_units;
	CArea::m_units = 1.0;

	AddVertex(v1, &v0);
	AddVertex(v2, &v1);

	CArea::m_units = save_units;
}

static void OffsetWithLoops(const TPolyPolygon &pp, TPolyPolygon &pp_new, double inwards_value)
{
	Clipper c;

	bool inwards = (inwards_value > 0);
	bool reverse = false;
	double radius = -fabs(inwards_value);

	if(inwards)
	{
		// add a large square on the outside, to be removed later
		TPolygon p;
		p.push_back(DoubleAreaPoint(-10000.0, -10000.0).int_point());
		p.push_back(DoubleAreaPoint(-10000.0, 10000.0).int_point());
		p.push_back(DoubleAreaPoint(10000.0, 10000.0).int_point());
		p.push_back(DoubleAreaPoint(10000.0, -10000.0).int_point());
		c.AddPath(p, ptSubject, true);
	}
	else
	{
		reverse = true;
	}

	for(unsigned int i = 0; i < pp.size(); i++)
	{
		const TPolygon& p = pp[i];

		pts_for_AddVertex.clear();

		if(p.size() > 2)
		{
			if(reverse)
			{
				for(unsigned int j = p.size()-1; j > 1; j--)MakeLoop(p[j], p[j-1], p[j-2], radius);
				MakeLoop(p[1], p[0], p[p.size()-1], radius);
				MakeLoop(p[0], p[p.size()-1], p[p.size()-2], radius);
			}
			else
			{
				MakeLoop(p[p.size()-2], p[p.size()-1], p[0], radius);
				MakeLoop(p[p.size()-1], p[0], p[1], radius);
				for(unsigned int j = 2; j < p.size(); j++)MakeLoop(p[j-2], p[j-1], p[j], radius);
			}

			TPolygon loopy_polygon;
			loopy_polygon.reserve(pts_for_AddVertex.size());
			for(std::list<DoubleAreaPoint>::iterator It = pts_for_AddVertex.begin(); It != pts_for_AddVertex.end(); It++)
			{
				loopy_polygon.push_back(It->int_point());
			}
			c.AddPath(loopy_polygon, ptSubject, true);
			pts_for_AddVertex.clear();
		}
	}

	//c.ForceOrientation(false);
	c.Execute(ctUnion, pp_new, pftNonZero, pftNonZero);

	if(inwards)
	{
		// remove the large square
		if(pp_new.size() > 0)
		{
			pp_new.erase(pp_new.begin());
		}
	}
	else
	{
		// reverse all the resulting polygons
		TPolyPolygon copy = pp_new;
		pp_new.clear();
		pp_new.resize(copy.size());
		for(unsigned int i = 0; i < copy.size(); i++)
		{
			const TPolygon& p = copy[i];
			TPolygon p_new;
			p_new.resize(p.size());
			int size_minus_one = p.size() - 1;
			for(unsigned int j = 0; j < p.size(); j++)p_new[j] = p[size_minus_one - j];
			pp_new[i] = p_new;
		}
	}
}

static void MakeObround(const Point &pt0, const CVertex &vt1, double radius)
{
	Span span(pt0, vt1);
	Point forward0 = span.GetVector(0.0);
	Point forward1 = span.GetVector(1.0);
	Point right0(forward0.y, -forward0.x);
	Point right1(forward1.y, -forward1.x);
	right0.normalize();
	right1.normalize();

	CVertex v0(pt0 + right0 * radius);
	CVertex v1(vt1.m_type, vt1.m_p + right1 * radius, vt1.m_c);
	CVertex v2(1, vt1.m_p + right1 * -radius, vt1.m_p);
	CVertex v3(-vt1.m_type, pt0 + right0 * -radius, vt1.m_c);
	CVertex v4(1, pt0 + right0 * radius, pt0);

	double save_units = CArea::m_units;
	CArea::m_units = 1.0;

	AddVertex(v0, NULL);
	AddVertex(v1, &v0);
	AddVertex(v2, &v1);
	AddVertex(v3, &v2);
	AddVertex(v4, &v3);

	CArea::m_units = save_units;
}

static void OffsetSpansWithObrounds(const CArea& area, TPolyPolygon &pp_new, double radius)
{
	Clipper c;


	for(std::list<CCurve>::const_iterator It = area.m_curves.begin(); It != area.m_curves.end(); It++)
	{
		pts_for_AddVertex.clear();
		const CCurve& curve = *It;
		const CVertex* prev_vertex = NULL;
		for(std::list<CVertex>::const_iterator It2 = curve.m_vertices.begin(); It2 != curve.m_vertices.end(); It2++)
		{
			const CVertex& vertex = *It2;
			if(prev_vertex)
			{
				MakeObround(prev_vertex->m_p, vertex, radius);

				TPolygon loopy_polygon;
				loopy_polygon.reserve(pts_for_AddVertex.size());
				for(std::list<DoubleAreaPoint>::iterator It = pts_for_AddVertex.begin(); It != pts_for_AddVertex.end(); It++)
				{
					loopy_polygon.push_back(It->int_point());
				}
				c.AddPath(loopy_polygon, ptSubject, true);
				pts_for_AddVertex.clear();
			}
			prev_vertex = &vertex;
		}
	}

	pp_new.clear();
	c.Execute(ctUnion, pp_new, pftNonZero, pftNonZero);

	// reverse all the resulting polygons
	TPolyPolygon copy = pp_new;
	pp_new.clear();
	pp_new.resize(copy.size());
	for(unsigned int i = 0; i < copy.size(); i++)
	{
		const TPolygon& p = copy[i];
		TPolygon p_new;
		p_new.resize(p.size());
		int size_minus_one = p.size() - 1;
		for(unsigned int j = 0; j < p.size(); j++)p_new[j] = p[size_minus_one - j];
		pp_new[i] = p_new;
	}
}

static void MakePolyPoly( const CArea& area, TPolyPolygon &pp, bool reverse = true ){
	pp.clear();

	for(std::list<CCurve>::const_iterator It = area.m_curves.begin(); It != area.m_curves.end(); It++)
	{
		pts_for_AddVertex.clear();
		const CCurve& curve = *It;
		const CVertex* prev_vertex = NULL;
		for(std::list<CVertex>::const_iterator It2 = curve.m_vertices.begin(); It2 != curve.m_vertices.end(); It2++)
		{
			const CVertex& vertex = *It2;
			if(prev_vertex)AddVertex(vertex, prev_vertex);
			prev_vertex = &vertex;
		}

		TPolygon p;
		p.resize(pts_for_AddVertex.size());
		if(reverse)
		{
			unsigned int i = pts_for_AddVertex.size() - 1;// clipper wants them the opposite way to CArea
			for(std::list<DoubleAreaPoint>::iterator It = pts_for_AddVertex.begin(); It != pts_for_AddVertex.end(); It++, i--)
			{
				p[i] = It->int_point();
			}
		}
		else
		{
			unsigned int i = 0;
			for(std::list<DoubleAreaPoint>::iterator It = pts_for_AddVertex.begin(); It != pts_for_AddVertex.end(); It++, i++)
			{
				p[i] = It->int_point();
			}
		}

		pp.push_back(p);
	}
}

static void SetFromResult( CCurve& curve, const TPolygon& p, bool reverse = true )
{
	for(unsigned int j = 0; j < p.size(); j++)
	{
		const IntPoint &pt = p[j];
		DoubleAreaPoint dp(pt);
		CVertex vertex(0, Point(dp.X / CArea::m_units, dp.Y / CArea::m_units), Point(0.0, 0.0));
		if(reverse)curve.m_vertices.push_front(vertex);
		else curve.m_vertices.push_back(vertex);
	}
	// make a copy of the first point at the end
	if(reverse)curve.m_vertices.push_front(curve.m_vertices.back());
	else curve.m_vertices.push_back(curve.m_vertices.front());

	if(CArea::m_fit_arcs)curve.FitArcs();
}

static void SetFromResult( CArea& area, const TPolyPolygon& pp, bool reverse = true )
{
	// delete existing geometry
	area.m_curves.clear();

	for(unsigned int i = 0; i < pp.size(); i++)
	{
		const TPolygon& p = pp[i];

		area.m_curves.push_back(CCurve());
		CCurve &curve = area.m_curves.back();
		SetFromResult(curve, p, reverse);
    }
}

void CArea::Subtract(const CArea& a2)
{
	Clipper c;
	TPolyPolygon pp1, pp2;
	MakePolyPoly(*this, pp1);
	MakePolyPoly(a2, pp2);
	c.AddPaths(pp1, ptSubject, true);
	c.AddPaths(pp2, ptClip, true);
	TPolyPolygon solution;
	c.Execute(ctDifference, solution);
	SetFromResult(*this, solution);
}

void CArea::Intersect(const CArea& a2)
{
	Clipper c;
	TPolyPolygon pp1, pp2;
	MakePolyPoly(*this, pp1);
	MakePolyPoly(a2, pp2);
	c.AddPaths(pp1, ptSubject, true);
	c.AddPaths(pp2, ptClip, true);
	TPolyPolygon solution;
	c.Execute(ctIntersection, solution);
	SetFromResult(*this, solution);
}

void CArea::Union(const CArea& a2)
{
	Clipper c;
	TPolyPolygon pp1, pp2;
	MakePolyPoly(*this, pp1);
	MakePolyPoly(a2, pp2);
	c.AddPaths(pp1, ptSubject, true);
	c.AddPaths(pp2, ptClip, true);
	TPolyPolygon solution;
	c.Execute(ctUnion, solution);
	SetFromResult(*this, solution);
}

void CArea::Xor(const CArea& a2)
{
	Clipper c;
	TPolyPolygon pp1, pp2;
	MakePolyPoly(*this, pp1);
	MakePolyPoly(a2, pp2);
	c.AddPaths(pp1, ptSubject, true);
	c.AddPaths(pp2, ptClip, true);
	TPolyPolygon solution;
	c.Execute(ctXor, solution);
	SetFromResult(*this, solution);
}

void CArea::Offset(double inwards_value)
{
	TPolyPolygon pp, pp2;
	MakePolyPoly(*this, pp, false);
	OffsetWithLoops(pp, pp2, inwards_value * m_units);
	SetFromResult(*this, pp2, false);
	this->Reorder();
}

void CArea::Thicken(double value)
{
	TPolyPolygon pp;
	OffsetSpansWithObrounds(*this, pp, value * m_units);
	SetFromResult(*this, pp, false);
	this->Reorder();
}

void UnFitArcs(CCurve &curve)
{
	pts_for_AddVertex.clear();
	const CVertex* prev_vertex = NULL;
	for(std::list<CVertex>::const_iterator It2 = curve.m_vertices.begin(); It2 != curve.m_vertices.end(); It2++)
	{
		const CVertex& vertex = *It2;
		AddVertex(vertex, prev_vertex);
		prev_vertex = &vertex;
	}

	curve.m_vertices.clear();

	for(std::list<DoubleAreaPoint>::iterator It = pts_for_AddVertex.begin(); It != pts_for_AddVertex.end(); It++)
	{
		DoubleAreaPoint &pt = *It;
		CVertex vertex(0, Point(pt.X / CArea::m_units, pt.Y / CArea::m_units), Point(0.0, 0.0));
		curve.m_vertices.push_back(vertex);
	}
}