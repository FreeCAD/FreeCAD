// Curve.cpp
// Copyright 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "Curve.h"
#include "Circle.h"
#include "Arc.h"
#include "Area.h"
#include "kurve/geometry.h"

const Point operator*(const double &d, const Point &p){ return p * d;}
double Point::tolerance = 0.001;

//static const double PI = 3.1415926535897932; duplicated in kurve/geometry.h

//This function is moved from header here to solve windows DLL not export
//static variable problem
bool Point::operator==(const Point& p)const{
    return fabs(x-p.x)<tolerance && fabs(y-p.y)<tolerance;
}

double Point::length()const
{
    return sqrt( x*x + y*y );
}

double Point::normalize()
{
	double len = length();
	if(fabs(len)> 0.000000000000001)
		*this = (*this) / len;
	return len;
}

Line::Line(const Point& P0, const Point& V):p0(P0), v(V)
{
}

double Line::Dist(const Point& p)const
{
	Point vn = v;
	vn.normalize();
	double d1 = p0 * vn;
	double d2 = p * vn;
	Point pn = p0 + vn * (d2 - d1);

	return pn.dist(p);
}

CVertex::CVertex(int type, const Point& p, const Point& c, int user_data):m_type(type), m_p(p), m_c(c), m_user_data(user_data)
{
}

CVertex::CVertex(const Point& p, int user_data):m_type(0), m_p(p), m_c(0.0, 0.0), m_user_data(user_data)
{
}

void CCurve::append(const CVertex& vertex)
{
	m_vertices.push_back(vertex);
}

bool CCurve::CheckForArc(const CVertex& prev_vt, std::list<const CVertex*>& might_be_an_arc, CArc &arc_returned)
{
	// this examines the vertices in might_be_an_arc
	// if they do fit an arc, set arc to be the arc that they fit and return true
	// returns true, if arc added
	if(might_be_an_arc.size() < 2)
	    return false;

	// find middle point
	std::size_t num = might_be_an_arc.size();
	std::size_t i = 0;
	const CVertex* mid_vt = NULL;
	std::size_t mid_i = (num-1)/2;
	for(std::list<const CVertex*>::iterator It = might_be_an_arc.begin(); It != might_be_an_arc.end(); It++, i++)
	{
		if(i == mid_i)
		{
			mid_vt = *It;
			break;
		}
	}

	if (mid_vt == NULL)
		return false;

	// create a circle to test
	Point p0(prev_vt.m_p);
	Point p1(mid_vt->m_p);
	Point p2(might_be_an_arc.back()->m_p);
	Circle c(p0, p1, p2);

	const CVertex* current_vt = &prev_vt;
    // It seems that ClipperLib's offset ArcTolerance (same as m_accuracy here)
    // is not exactly what's documented at https://goo.gl/4odfQh. Test shows the
    // maximum arc distance deviate at about 2.2*ArcTolerance units. The maximum
    // deviance seems to always occur at the end of arc.
	double accuracy = CArea::m_accuracy * 2.3 / CArea::m_units;
	for(std::list<const CVertex*>::iterator It = might_be_an_arc.begin(); It != might_be_an_arc.end(); It++)
	{
		const CVertex* vt = *It;

		if(!c.LineIsOn(current_vt->m_p, vt->m_p, accuracy))
			return false;
		current_vt = vt;
	}

	CArc arc;
	arc.m_c = c.m_c;
	arc.m_s = prev_vt.m_p;
	arc.m_e = might_be_an_arc.back()->m_p;
	arc.SetDirWithPoint(might_be_an_arc.front()->m_p);
	arc.m_user_data = might_be_an_arc.back()->m_user_data;

	double angs = atan2(arc.m_s.y - arc.m_c.y, arc.m_s.x - arc.m_c.x);
	double ange = atan2(arc.m_e.y - arc.m_c.y, arc.m_e.x - arc.m_c.x);
	if(arc.m_dir)
	{
		// make sure ange > angs
		if(ange < angs)ange += 6.2831853071795864;
	}
	else
	{
		// make sure angs > ange
		if(angs < ange)angs += 6.2831853071795864;
	}

	if(arc.IncludedAngle() >= 3.15) // We don't want full arcs, so limit to about 180 degrees
	    return false;

	for(std::list<const CVertex*>::iterator It = might_be_an_arc.begin(); It != might_be_an_arc.end(); It++)
	{
		const CVertex* vt = *It;
		double angp = atan2(vt->m_p.y - arc.m_c.y, vt->m_p.x - arc.m_c.x);
		if(arc.m_dir)
		{
			// make sure angp > angs
			if(angp < angs)angp += 6.2831853071795864;
			if(angp > ange)
			    return false;
		}
		else
		{
			// make sure angp > ange
			if(angp < ange)angp += 6.2831853071795864;
			if(angp > angs)
			    return false;
		}
	}

	arc_returned = arc;
	return true;
}

void CCurve::AddArcOrLines(bool check_for_arc, std::list<CVertex> &new_vertices, std::list<const CVertex*>& might_be_an_arc, CArc &arc, bool &arc_found, bool &arc_added)
{
	if(check_for_arc && CheckForArc(new_vertices.back(), might_be_an_arc, arc))
	{
		arc_found = true;
	}
	else
	{
		if(arc_found)
		{
			if(arc.AlmostALine())
			{
				new_vertices.emplace_back(arc.m_e, arc.m_user_data);
			}
			else
			{
				new_vertices.emplace_back(arc.m_dir ? 1:-1, arc.m_e, arc.m_c, arc.m_user_data);
			}

			arc_added = true;
			arc_found = false;
			const CVertex* back_vt = might_be_an_arc.back();
			might_be_an_arc.clear();
			if(check_for_arc)might_be_an_arc.push_back(back_vt);
		}
		else
		{
			const CVertex* back_vt = might_be_an_arc.back();
			if(check_for_arc)might_be_an_arc.pop_back();
			for(std::list<const CVertex*>::iterator It = might_be_an_arc.begin(); It != might_be_an_arc.end(); It++)
			{
				const CVertex* v = *It;
				if(It != might_be_an_arc.begin() || (new_vertices.size() == 0) || (new_vertices.back().m_p != v->m_p))
				{
					new_vertices.push_back(*v);
				}
			}
			might_be_an_arc.clear();
			if(check_for_arc)might_be_an_arc.push_back(back_vt);
		}
	}
}

void CCurve::FitArcs(bool retry)
{
	std::list<CVertex> new_vertices;

	std::list<const CVertex*> might_be_an_arc;
	CArc arc;
	bool arc_found = false;
	bool arc_added = false;
	int i = 0;
	for(std::list<CVertex>::iterator It = m_vertices.begin(); It != m_vertices.end(); It++, i++)
	{
		CVertex& vt = *It;
		if(vt.m_type || i == 0)
		{
			if (i != 0)
			{
				AddArcOrLines(false, new_vertices, might_be_an_arc, arc, arc_found, arc_added);
			}
			new_vertices.push_back(vt);
		}
		else
		{
			might_be_an_arc.push_back(&vt);

			if(might_be_an_arc.size() == 1)
			{
			}
			else
			{
				AddArcOrLines(true, new_vertices, might_be_an_arc, arc, arc_found, arc_added);
			}
		}
	}

	if(might_be_an_arc.size() > 0) {
        // check if the last edge can form an arc with the starting edge
        if(!retry && 
           m_vertices.size()>2 && 
           m_vertices.begin()->m_type==0 && 
           IsClosed()) 
        {
	        std::list<const CVertex*> tmp;
            auto it = m_vertices.begin();
            tmp.push_back(&(*it++));

            // this condition check is to skip the situation when both the
            // starting and ending has already been fitted with some arc
            if(!arc_found || it->m_type==0) {
                tmp.push_back(&(*it));
                CArc tmpArc;
                auto itEnd = m_vertices.end();
                --itEnd;
                --itEnd;
                if(CheckForArc(*itEnd,tmp,tmpArc)) {
                    if(arc_found) {
                        // this means the last edge has already been fitted with
                        // some arc, so we move the first edge to the end
                        
                        // Must pop first, because this is a closed curve,
                        // meaning the last point must be equal to the first
                        // point.
                        m_vertices.pop_front();
                        m_vertices.push_back(m_vertices.front());
                    }else{
                        m_vertices.push_front(CVertex(new_vertices.back().m_p));
                        m_vertices.pop_back();
                    }
                    FitArcs(true);
                    return;
                }
            }
        }
        AddArcOrLines(false, new_vertices, might_be_an_arc, arc, arc_found, arc_added);
    }

	if(arc_added)
	{
        for(auto *v : might_be_an_arc)
            new_vertices.push_back(*v);
        m_vertices.swap(new_vertices);
	}
}

void CCurve::UnFitArcs()
{
	std::list<Point> new_pts;

	const CVertex* prev_vertex = NULL;
	for(std::list<CVertex>::const_iterator It2 = m_vertices.begin(); It2 != m_vertices.end(); It2++)
	{
		const CVertex& vertex = *It2;
		if(vertex.m_type == 0 || prev_vertex == NULL)
		{
			new_pts.push_back(vertex.m_p * CArea::m_units);
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

				//what is the delta phi to get an accuracy of aber
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

					new_pts.emplace_back(nx, ny);

					px = nx;
					py = ny;
				}
			}
		}
		prev_vertex = &vertex;
	}

	m_vertices.clear();

	for(std::list<Point>::iterator It = new_pts.begin(); It != new_pts.end(); It++)
	{
		Point &pt = *It;
		CVertex vertex(0, pt / CArea::m_units, Point(0.0, 0.0));
		m_vertices.push_back(vertex);
	}
}

Point CCurve::NearestPoint(const Point& p)const
{
	double best_dist = 0.0;
	Point best_point = Point(0, 0);
	bool best_point_valid = false;
	Point prev_p = Point(0, 0);
	bool prev_p_valid = false;
	bool first_span = true;
	for(std::list<CVertex>::const_iterator It = m_vertices.begin(); It != m_vertices.end(); It++)
	{
		const CVertex& vertex = *It;
		if(prev_p_valid)
		{
			Point near_point = Span(prev_p, vertex, first_span).NearestPoint(p);
			first_span = false;
			double dist = near_point.dist(p);
			if(!best_point_valid || dist < best_dist)
			{
				best_dist = dist;
				best_point = near_point;
				best_point_valid = true;
			}
		}
		prev_p = vertex.m_p;
		prev_p_valid = true;
	}
	return best_point;
}

Point CCurve::NearestPoint(const CCurve& c, double *d)const
{
	double best_dist = 0.0;
	Point best_point = Point(0, 0);
	bool best_point_valid = false;
	Point prev_p = Point(0, 0);
	bool prev_p_valid = false;
	bool first_span = true;
	for(std::list<CVertex>::const_iterator It = c.m_vertices.begin(); It != c.m_vertices.end(); It++)
	{
		const CVertex& vertex = *It;
		if(prev_p_valid)
		{
			double dist;
			Point near_point = NearestPoint(Span(prev_p, vertex, first_span), &dist);
			first_span = false;
			if(!best_point_valid || dist < best_dist)
			{
				best_dist = dist;
				best_point = near_point;
				best_point_valid = true;
			}
		}
		prev_p = vertex.m_p;
		prev_p_valid = true;
	}
	if(d)*d = best_dist;
	return best_point;
}

void CCurve::GetBox(CBox2D &box)
{
	Point prev_p = Point(0, 0);
	bool prev_p_valid = false;
	for(std::list<CVertex>::iterator It = m_vertices.begin(); It != m_vertices.end(); It++)
	{
		CVertex& vertex = *It;
		if(prev_p_valid)
		{
			Span(prev_p, vertex).GetBox(box);
		}
		prev_p = vertex.m_p;
		prev_p_valid = true;
	}
}

void CCurve::Reverse()
{
	std::list<CVertex> new_vertices;

	CVertex* prev_v = NULL;

	for(std::list<CVertex>::reverse_iterator It = m_vertices.rbegin(); It != m_vertices.rend(); It++)
	{
		CVertex &v = *It;
		int type = 0;
		Point cp(0.0, 0.0);
		if(prev_v)
		{
			type = -prev_v->m_type;
			cp = prev_v->m_c;
		}
		CVertex new_v(type, v.m_p, cp);
		new_vertices.push_back(new_v);
		prev_v = &v;
	}

	m_vertices.swap(new_vertices);
}

double CCurve::GetArea()const
{
	double area = 0.0;
	Point prev_p = Point(0, 0);
	bool prev_p_valid = false;
	for(std::list<CVertex>::const_iterator It = m_vertices.begin(); It != m_vertices.end(); It++)
	{
		const CVertex& vertex = *It;
		if(prev_p_valid)
		{
			area += Span(prev_p, vertex).GetArea();
		}
		prev_p = vertex.m_p;
		prev_p_valid = true;
	}
	return area;
}

bool CCurve::IsClosed()const
{
	if(m_vertices.size() == 0)
	    return false;
	return m_vertices.front().m_p == m_vertices.back().m_p;
}

void CCurve::ChangeStart(const Point &p) {
	CCurve new_curve;

	bool started = false;
	bool finished = false;
	int start_span = 0;
	bool closed = IsClosed();

	for(int i = 0; i < (closed ? 2:1); i++)
	{
		const Point *prev_p = NULL;

		int span_index = 0;
		for(std::list<CVertex>::const_iterator VIt = m_vertices.begin(); VIt != m_vertices.end() && !finished; VIt++)
		{
			const CVertex& vertex = *VIt;

			if(prev_p)
			{
				Span span(*prev_p, vertex);
				if(span.On(p))
				{
					if(started)
					{
						if(p == *prev_p || span_index != start_span)
						{
							new_curve.m_vertices.push_back(vertex);
						}
						else
						{
							if(p == vertex.m_p)new_curve.m_vertices.push_back(vertex);
							else
							{
								CVertex v(vertex);
								v.m_p = p;
								new_curve.m_vertices.push_back(v);
							}
							finished = true;
						}
					}
					else
					{
						new_curve.m_vertices.emplace_back(p);
						started = true;
						start_span = span_index;
						if(p != vertex.m_p)new_curve.m_vertices.push_back(vertex);
					}
				}
				else
				{
					if(started)
					{
						new_curve.m_vertices.push_back(vertex);
					}
				}
				span_index++;
			}
			prev_p = &(vertex.m_p);
		}
	}

	if(started)
	{
		m_vertices.swap(new_curve.m_vertices);
	}
}

void CCurve::Break(const Point &p) {
	// inserts a point, if it lies on the curve
	const Point *prev_p = NULL;

	for(std::list<CVertex>::iterator VIt = m_vertices.begin(); VIt != m_vertices.end(); VIt++)
	{
		CVertex& vertex = *VIt;

		if(p == vertex.m_p)break; // point is already on a vertex

		if(prev_p)
		{
			Span span(*prev_p, vertex);
			if(span.On(p))
			{
				CVertex v(vertex);
				v.m_p = p;
				m_vertices.insert(VIt, v);
				break;
			}
		}
		prev_p = &(vertex.m_p);
	}
}

void CCurve::ExtractSeparateCurves(const std::list<Point> &ordered_points, std::list<CCurve> &separate_curves)const
{
	// returns separate curves for this curve split at points
	// the points must be in order along this curve, already, and lie on this curve
	const Point *prev_p = NULL;

	if(ordered_points.size() == 0)
	{
		separate_curves.push_back(*this);
		return;
	}

	CCurve current_curve;

	std::list<Point>::const_iterator PIt = ordered_points.begin();
	Point point = *PIt;

	for(std::list<CVertex>::const_iterator VIt = m_vertices.begin(); VIt != m_vertices.end(); VIt++)
	{
		const CVertex& vertex = *VIt;
		if(prev_p)// not the first vertex
		{
			Span span(*prev_p, vertex);
			while((PIt != ordered_points.end()) && span.On(point))
			{
				CVertex v(vertex);
				v.m_p = point;
				current_curve.m_vertices.push_back(v);
				if(current_curve.m_vertices.size() > 1)// don't add single point curves
					separate_curves.push_back(current_curve); // add the curve
				current_curve = CCurve();// make a new curve
				current_curve.m_vertices.push_back(v); // add it's first point
				PIt++;
				if(PIt != ordered_points.end())point = *PIt; // increment the point
			}

			// add the end of span
			if(current_curve.m_vertices.back().m_p != vertex.m_p)
				current_curve.m_vertices.push_back(vertex);
		}
		if((current_curve.m_vertices.size() == 0) || (current_curve.m_vertices.back().m_p != vertex.m_p))
		{
			// very first vertex, start the current curve
			current_curve.m_vertices.push_back(vertex);
		}
		prev_p = &(vertex.m_p);
	}

	// add whatever is left
	if(current_curve.m_vertices.size() > 1)// don't add single point curves
		separate_curves.push_back(current_curve); // add the curve
}



void CCurve::RemoveTinySpans() {
	CCurve new_curve;

	std::list<CVertex>::const_iterator VIt = m_vertices.begin(); 
	new_curve.m_vertices.push_back(*VIt);
	VIt++;

	for(; VIt != m_vertices.end(); VIt++)
	{
		const CVertex& vertex = *VIt;

		if(vertex.m_type != 0 || new_curve.m_vertices.back().m_p.dist(vertex.m_p) > Point::tolerance)
		{
			new_curve.m_vertices.push_back(vertex);
		}
	}
    m_vertices.swap(new_curve.m_vertices);
}

void CCurve::ChangeEnd(const Point &p) {
	// changes the end position of the Kurve, doesn't keep closed kurves closed
	CCurve new_curve;

	const Point *prev_p = NULL;

	for(std::list<CVertex>::const_iterator VIt = m_vertices.begin(); VIt != m_vertices.end(); VIt++)
	{
		const CVertex& vertex = *VIt;

		if(prev_p)
		{
			Span span(*prev_p, vertex);
			if(span.On(p))
			{
				CVertex v(vertex);
				v.m_p = p;
				new_curve.m_vertices.push_back(v);
				break;
			}
			else
			{
				if(p != vertex.m_p)new_curve.m_vertices.push_back(vertex);
			}
		}
		else
		{
			new_curve.m_vertices.push_back(vertex);
		}
		prev_p = &(vertex.m_p);
	}

	m_vertices.swap(new_curve.m_vertices);
}

Point CCurve::NearestPoint(const Span& p, double *d)const
{
	double best_dist = 0.0;
	Point best_point = Point(0, 0);
	bool best_point_valid = false;
	Point prev_p = Point(0, 0);
	bool prev_p_valid = false;
	bool first_span = true;
	for(std::list<CVertex>::const_iterator It = m_vertices.begin(); It != m_vertices.end(); It++)
	{
		const CVertex& vertex = *It;
		if(prev_p_valid)
		{
			double dist;
			Point near_point = Span(prev_p, vertex, first_span).NearestPoint(p, &dist);
			first_span = false;
			if(!best_point_valid || dist < best_dist)
			{
				best_dist = dist;
				best_point = near_point;
				best_point_valid = true;
			}
		}
		prev_p = vertex.m_p;
		prev_p_valid = true;
	}
	if(d)*d = best_dist;
	return best_point;
}

static geoff_geometry::Kurve MakeKurve(const CCurve& curve)
{
	geoff_geometry::Kurve k;
	for(std::list<CVertex>::const_iterator It = curve.m_vertices.begin(); It != curve.m_vertices.end(); It++)
	{
		const CVertex& v = *It;
		k.Add(geoff_geometry::spVertex(v.m_type, geoff_geometry::Point(v.m_p.x, v.m_p.y), geoff_geometry::Point(v.m_c.x, v.m_c.y)));
	}
	return k;
}

static CCurve MakeCCurve(const geoff_geometry::Kurve& k)
{
	CCurve c;
	int n = k.nSpans();
	for(int i = 0; i<= n; i++)
	{
		geoff_geometry::spVertex spv;
		k.Get(i, spv);
		c.append(CVertex(spv.type, Point(spv.p.x, spv.p.y), Point(spv.pc.x, spv.pc.y)));
	}
	return c;
}

static geoff_geometry::Span MakeSpan(const Span& span)
{
	return geoff_geometry::Span(span.m_v.m_type, geoff_geometry::Point(span.m_p.x, span.m_p.y), geoff_geometry::Point(span.m_v.m_p.x, span.m_v.m_p.y), geoff_geometry::Point(span.m_v.m_c.x, span.m_v.m_c.y));
}

bool CCurve::Offset(double leftwards_value)
{
	// use the kurve code donated by Geoff Hawkesford, to offset the curve as an open curve
	// returns true for success, false for failure
	bool success = true;

	CCurve save_curve = *this;

	try
	{
		geoff_geometry::Kurve k = MakeKurve(*this);
		geoff_geometry::Kurve kOffset;
		int ret = 0;
		k.OffsetMethod1(kOffset, fabs(leftwards_value), (leftwards_value > 0) ? 1:-1, 1, ret);
		success = (ret == 0);
		if(success)*this = MakeCCurve(kOffset);
	}
	catch(...)
	{
		success = false;
	}

	if(!success)
	{
		if(this->IsClosed())
		{
			double inwards_offset = leftwards_value;
			bool cw = false;
			if(this->IsClockwise())
			{
				inwards_offset = -inwards_offset;
				cw = true;
			}
			CArea a;
			a.append(*this);
			a.Offset(inwards_offset);
			if(a.m_curves.size() == 1)
			{
				Span* start_span = NULL;
				if(this->m_vertices.size() > 1)
				{
					std::list<CVertex>::iterator It = m_vertices.begin();
					CVertex &v0 = *It;
					It++;
					CVertex &v1 = *It;
					start_span = new Span(v0.m_p, v1, true);
				}
				*this = a.m_curves.front();
				if(this->IsClockwise() != cw)this->Reverse();
				if(start_span)
				{
					Point forward = start_span->GetVector(0.0);
					Point left(-forward.y, forward.x);
					Point offset_start = start_span->m_p + left * leftwards_value;
					this->ChangeStart(this->NearestPoint(offset_start));
					delete start_span;
				}
				success = true;
			}
		}
	}

	return success;
}

void CCurve::GetSpans(std::list<Span> &spans)const
{
	const Point *prev_p = NULL;
	for(std::list<CVertex>::const_iterator It = m_vertices.begin(); It != m_vertices.end(); It++)
	{
		const CVertex& vertex = *It;
		if(prev_p)
		{
			spans.emplace_back(*prev_p, vertex);
		}
		prev_p = &(vertex.m_p);
	}
}

void CCurve::OffsetForward(double forwards_value, bool refit_arcs)
{
	// for drag-knife compensation

	// replace arcs with lines
	UnFitArcs();

	std::list<Span> spans;
	GetSpans(spans);

	m_vertices.clear();

	// shift all the spans
	for(std::list<Span>::iterator It = spans.begin(); It != spans.end(); It++)
	{
		Span &span = *It;
		Point v = span.GetVector(0.0);
		v.normalize();
		Point shift = v * forwards_value;
		span.m_p = span.m_p + shift;
		span.m_v.m_p = span.m_v.m_p + shift;
	}

	// loop through the shifted spans
	for(std::list<Span>::iterator It = spans.begin(); It != spans.end();)
	{
		Span &span = *It;
		Point v = span.GetVector(0.0);
		v.normalize();

		// add the span
		if(It == spans.begin())m_vertices.push_back(span.m_p);
		m_vertices.push_back(span.m_v.m_p);

		It++;
		if(It != spans.end())
		{
			Span &next_span = *It;
			Point nv = next_span.GetVector(0.0);
			nv.normalize();
			double sin_angle = v ^ nv;
			bool sharp_corner = ( fabs(sin_angle) > 0.5 ); // angle > 30 degrees

			if(sharp_corner)
			{
				// add an arc to the start of the next span
				int arc_type = ((sin_angle > 0) ? 1 : (-1));
				Point centre = span.m_v.m_p - v * forwards_value;
				m_vertices.emplace_back(arc_type, next_span.m_p, centre);
			}
		}
	}

	if(refit_arcs)
		FitArcs(); // find the arcs again
	else
		UnFitArcs(); // convert those little arcs added to lines
}

double CCurve::Perim()const
{
	const Point *prev_p = NULL;
	double perim = 0.0;
	for(std::list<CVertex>::const_iterator It = m_vertices.begin(); It != m_vertices.end(); It++)
	{
		const CVertex& vertex = *It;
		if(prev_p)
		{
			Span span(*prev_p, vertex);
			perim += span.Length();
		}
		prev_p = &(vertex.m_p);
	}

	return perim;
}

Point CCurve::PerimToPoint(double perim)const
{
	if(m_vertices.size() == 0)
	    return Point(0, 0);

	const Point *prev_p = NULL;
	double kperim = 0.0;
	for(std::list<CVertex>::const_iterator It = m_vertices.begin(); It != m_vertices.end(); It++)
	{
		const CVertex& vertex = *It;
		if(prev_p)
		{
			Span span(*prev_p, vertex);
			double length = span.Length();
			if(perim < kperim + length)
			{
				Point p = span.MidPerim(perim - kperim);
				return p;
			}
			kperim += length;
		}
		prev_p = &(vertex.m_p);
	}

	return m_vertices.back().m_p;
}

double CCurve::PointToPerim(const Point& p)const
{
	double best_dist = 0.0;
	double perim_at_best_dist = 0.0;
	//Point best_point = Point(0, 0);
	bool best_dist_found = false;

	double perim = 0.0;

	const Point *prev_p = NULL;
	bool first_span = true;
	for(std::list<CVertex>::const_iterator It = m_vertices.begin(); It != m_vertices.end(); It++)
	{
		const CVertex& vertex = *It;
		if(prev_p)
		{
			Span span(*prev_p, vertex, first_span);
			Point near_point = span.NearestPoint(p);
			first_span = false;
			double dist = near_point.dist(p);
			if(!best_dist_found || dist < best_dist)
			{
				best_dist = dist;
				Span span_to_point(*prev_p, CVertex(span.m_v.m_type, near_point, span.m_v.m_c));
				perim_at_best_dist = perim + span_to_point.Length();
				best_dist_found = true;
			}
			perim += span.Length();
		}
		prev_p = &(vertex.m_p);
	}
	return perim_at_best_dist;
}

void CCurve::operator+=(const CCurve& curve)
{
	for(std::list<CVertex>::const_iterator It = curve.m_vertices.begin(); It != curve.m_vertices.end(); It++)
	{
		const CVertex &vt = *It;
		if(It == curve.m_vertices.begin())
		{
			if((m_vertices.size() == 0) || (It->m_p != m_vertices.back().m_p))
			{
				m_vertices.emplace_back(It->m_p);
			}
		}
		else
		{
			m_vertices.push_back(vt);
		}
	}
}

void CCurve::CurveIntersections(const CCurve& c, std::list<Point> &pts)const
{
	CArea a;
	a.append(*this);
	a.CurveIntersections(c, pts);
}

void CCurve::SpanIntersections(const Span& s, std::list<Point> &pts)const
{
	std::list<Span> spans;
	GetSpans(spans);
	for(std::list<Span>::iterator It = spans.begin(); It != spans.end(); It++)
	{
		Span& span = *It;
		std::list<Point> pts2;
		span.Intersect(s, pts2);
		for(std::list<Point>::iterator It = pts2.begin(); It != pts2.end(); It++)
		{
			Point &pt = *It;
			if(pts.size() == 0)
			{
				pts.push_back(pt);
			}
			else
			{
				if(pt != pts.back())pts.push_back(pt);
			}
		}
	}
}

const Point Span::null_point = Point(0, 0);
const CVertex Span::null_vertex = CVertex(Point(0, 0));

Span::Span():m_start_span(false), m_p(null_point), m_v(null_vertex){}

Point Span::NearestPointNotOnSpan(const Point& p)const
{
	if(m_v.m_type == 0)
	{
		Point Vs = (m_v.m_p - m_p);
		Vs.normalize();
		double dp = (p - m_p) * Vs;		
		return (Vs * dp) + m_p;
	}
	else
	{
		double radius = m_p.dist(m_v.m_c);
		double r = p.dist(m_v.m_c);
		if(r < Point::tolerance)
		    return m_p;
		Point vc = (m_v.m_c - p);
		return p + vc * ((r - radius) / r);
	}
}

Point Span::NearestPoint(const Point& p)const
{
	Point np = NearestPointNotOnSpan(p);
	double t = Parameter(np);
	if(t >= 0.0 && t <= 1.0)
	    return np;

	double d1 = p.dist(this->m_p);
	double d2 = p.dist(this->m_v.m_p);

	if(d1 < d2)
	    return this->m_p;
	else return m_v.m_p;
}

Point Span::MidPerim(double d)const {
	/// returns a point which is 0-d along span
	Point p;
	if(m_v.m_type == 0) {
		Point vs = m_v.m_p - m_p;
		vs.normalize();
		p = vs * d + m_p;
	}
	else {
		Point v = m_p - m_v.m_c;
		double radius = v.length();
		v.Rotate(d * m_v.m_type / radius);
		p = v + m_v.m_c;
	}
	return p;
}

Point Span::MidParam(double param)const {
	/// returns a point which is 0-1 along span
	if(fabs(param) < 0.00000000000001)
	    return m_p;
	if(fabs(param - 1.0) < 0.00000000000001)
	    return m_v.m_p;

	Point p;
	if(m_v.m_type == 0) {
		Point vs = m_v.m_p - m_p;
		p = vs * param + m_p;
	}
	else {
		Point v = m_p - m_v.m_c;
		v.Rotate(param * IncludedAngle());
		p = v + m_v.m_c;
	}
	return p;
}

Point Span::NearestPointToSpan(const Span& p, double &d)const
{
	Point midpoint = MidParam(0.5);
	Point np = p.NearestPoint(m_p);
	Point best_point = m_p;
	double dist = np.dist(m_p);
	if(p.m_start_span)dist -= (CArea::m_accuracy * 2); // give start of curve most priority
	Point npm = p.NearestPoint(midpoint);
	double dm = npm.dist(midpoint) - CArea::m_accuracy; // lie about midpoint distance to give midpoints priority
	if(dm < dist){dist = dm; best_point = midpoint;}
	Point np2 = p.NearestPoint(m_v.m_p);
	double dp2 = np2.dist(m_v.m_p);
	if(dp2 < dist){dist = dp2; best_point = m_v.m_p;}
	d = dist;
	return best_point;
}

Point Span::NearestPoint(const Span& p, double *d)const
{
	double best_dist;
	Point best_point = this->NearestPointToSpan(p, best_dist);

	// try the other way round too
	double best_dist2;
	Point best_point2 = p.NearestPointToSpan(*this, best_dist2);
	if(best_dist2 < best_dist)
	{
		best_point = NearestPoint(best_point2);
		best_dist = best_dist2;
	}

	if(d)*d = best_dist;
	return best_point;
}

static int GetQuadrant(const Point& v){
	// 0 = [+,+], 1 = [-,+], 2 = [-,-], 3 = [+,-]
	if(v.x > 0)
	{
		if(v.y > 0)
			return 0;
		return 3;
	}
	if(v.y > 0)
		return 1;
	return 2;
}

static Point QuadrantEndPoint(int i)
{
	if(i >3)i-=4;
	switch(i)
	{
	case 0:
		return Point(0.0,1.0);
	case 1:
		return Point(-1.0,0.0);
	case 2:
		return Point(0.0,-1.0);
	default:
		return Point(1.0,0.0);
	}
}

void Span::GetBox(CBox2D &box)
{
	box.Insert(m_p);
	box.Insert(m_v.m_p);

	if(this->m_v.m_type)
	{
		// arc, add quadrant points
		Point vs = m_p - m_v.m_c;
		Point ve = m_v.m_p - m_v.m_c;
		int qs = GetQuadrant(vs);
		int qe = GetQuadrant(ve);
		if(m_v.m_type == -1)
		{
			// swap qs and qe
			int t=qs;
			qs = qe;
			qe = t;
		}

		if(qe<qs)qe = qe + 4;

		double rad = m_v.m_p.dist(m_v.m_c);

		for(int i = qs; i<qe; i++)
		{
			box.Insert(m_v.m_c + QuadrantEndPoint(i) * rad);
		}
	}
}

double IncludedAngle(const Point& v0, const Point& v1, int dir) {
	// returns the absolute included angle between 2 vectors in the direction of dir ( 1=acw  -1=cw)
	double inc_ang = v0 * v1;
	if(inc_ang > 1. - 1.0e-10)
	    return 0;
	if(inc_ang < -1. + 1.0e-10)
		inc_ang = PI;  
	else {									// dot product,   v1 . v2  =  cos ang
		if(inc_ang > 1.0) inc_ang = 1.0;
		inc_ang = acos(inc_ang);									// 0 to pi radians

		if(dir * (v0 ^ v1) < 0) inc_ang = 2 * PI - inc_ang ;		// cp
	}
	return dir * inc_ang;
}

double Span::IncludedAngle()const
{
	if(m_v.m_type)
	{
		Point vs = ~(m_p - m_v.m_c);
		Point ve = ~(m_v.m_p - m_v.m_c);
		if(m_v.m_type == -1)
		{
			vs = -vs;
			ve = -ve;
		}
		vs.normalize();
		ve.normalize();

		return ::IncludedAngle(vs, ve, m_v.m_type);
	}

	return 0.0;
}

double Span::GetArea()const
{
	if(m_v.m_type)
	{
		double angle = IncludedAngle();
		double radius = m_p.dist(m_v.m_c);
		return ( 0.5 * ((m_v.m_c.x - m_p.x) * (m_v.m_c.y + m_p.y) - (m_v.m_c.x - m_v.m_p.x) * (m_v.m_c.y + m_v.m_p.y) - angle * radius * radius));
	}

	return 0.5 * (m_v.m_p.x - m_p.x) * (m_p.y + m_v.m_p.y);
}

double Span::Parameter(const Point& p)const
{
	double t;
	if(m_v.m_type == 0) {
		Point v0 = p - m_p;
		Point vs = m_v.m_p - m_p;
		double length = vs.length();
		vs.normalize();
		t = vs * v0;
		t = t / length;
	}
	else
	{
		// true if p lies on arc span sp (p must be on circle of span)
		Point vs = ~(m_p - m_v.m_c);
		Point v = ~(p - m_v.m_c);
		vs.normalize();
		v.normalize();
		if(m_v.m_type == -1){
			vs = -vs;
			v = -v;
		}
		double ang = ::IncludedAngle(vs, v, m_v.m_type);
		double angle = IncludedAngle();
		t = ang / angle;
	}
	return t;
}

bool Span::On(const Point& p, double* t)const
{
	if(p != NearestPoint(p))
	    return false;
	if(t)*t = Parameter(p);
	return true;
}

double Span::Length()const
{
	if(m_v.m_type) {
		double radius = m_p.dist(m_v.m_c);
		return fabs(IncludedAngle()) * radius;
	}

	return m_p.dist(m_v.m_p);
}

Point Span::GetVector(double fraction)const
{
	/// returns the direction vector at point which is 0-1 along span
	if(m_v.m_type == 0){
		Point v(m_p, m_v.m_p);
		v.normalize();
		return v;
	}

	Point p= MidParam(fraction);
	Point v(m_v.m_c, p);
	v.normalize();
	if(m_v.m_type == 1)
	{
		return Point(-v.y, v.x);
	}
	else
	{
		return Point(v.y, -v.x);
	}
}

void Span::Intersect(const Span& s, std::list<Point> &pts)const
{
	// finds all the intersection points between two spans and puts them in the given list
	geoff_geometry::Point pInt1, pInt2;
	double t[4];
	int num_int = MakeSpan(*this).Intof(MakeSpan(s), pInt1, pInt2, t);
	if(num_int > 0)pts.emplace_back(pInt1.x, pInt1.y);
	if(num_int > 1)pts.emplace_back(pInt2.x, pInt2.y);
}

void tangential_arc(const Point &p0, const Point &p1, const Point &v0, Point &c, int &dir)
{
	geoff_geometry::Point gp0(p0.x, p0.y);
	geoff_geometry::Point gp1(p1.x, p1.y);
	geoff_geometry::Vector2d gv0(v0.x, v0.y);
	geoff_geometry::Point gc;
	geoff_geometry::tangential_arc(gp0, gp1, gv0, gc, dir);
	c = Point(gc.x, gc.y);
}
