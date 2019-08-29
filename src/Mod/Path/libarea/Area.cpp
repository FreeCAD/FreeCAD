// Area.cpp

// Copyright 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "Area.h"
#include "AreaOrderer.h"

#include <map>

double CArea::m_accuracy = 0.01;
double CArea::m_units = 1.0;
bool CArea::m_clipper_simple = false;
double CArea::m_clipper_clean_distance = 0.0;
bool CArea::m_fit_arcs = true;
int CArea::m_min_arc_points = 4;
int CArea::m_max_arc_points = 100;
double CArea::m_single_area_processing_length = 0.0;
double CArea::m_processing_done = 0.0;
bool CArea::m_please_abort = false;
double CArea::m_MakeOffsets_increment = 0.0;
double CArea::m_split_processing_length = 0.0;
bool CArea::m_set_processing_length_in_split = false;
double CArea::m_after_MakeOffsets_length = 0.0;
//static const double PI = 3.1415926535897932;

#define _CAREA_PARAM_DEFINE(_class,_type,_name) \
    _type CArea::get_##_name() {return _class::_name;}\
    void CArea::set_##_name(_type _name) {_class::_name = _name;}

#define CAREA_PARAM_DEFINE(_type,_name) \
    _type CArea::get_##_name() {return m_##_name;}\
    void CArea::set_##_name(_type _name) {m_##_name = _name;}

_CAREA_PARAM_DEFINE(Point,double,tolerance);
CAREA_PARAM_DEFINE(bool,fit_arcs)
CAREA_PARAM_DEFINE(bool,clipper_simple);
CAREA_PARAM_DEFINE(double,clipper_clean_distance);
CAREA_PARAM_DEFINE(double,accuracy);
CAREA_PARAM_DEFINE(double,units);
CAREA_PARAM_DEFINE(short,min_arc_points);
CAREA_PARAM_DEFINE(short,max_arc_points);
CAREA_PARAM_DEFINE(double,clipper_scale);

void CArea::append(const CCurve& curve)
{
	m_curves.push_back(curve);
}

void CArea::move(CCurve&& curve)
{
	m_curves.push_back(std::move(curve));
}

void CArea::FitArcs(){
	for(std::list<CCurve>::iterator It = m_curves.begin(); It != m_curves.end(); It++)
	{
		CCurve& curve = *It;
		curve.FitArcs();
	}
}

Point CArea::NearestPoint(const Point& p)const
{
	double best_dist = 0.0;
	Point best_point = Point(0, 0);
	for(std::list<CCurve>::const_iterator It = m_curves.begin(); It != m_curves.end(); It++)
	{
		const CCurve& curve = *It;
		Point near_point = curve.NearestPoint(p);
		double dist = near_point.dist(p);
		if(It == m_curves.begin() || dist < best_dist)
		{
			best_dist = dist;
			best_point = near_point;
		}
	}
	return best_point;
}

void CArea::ChangeStartToNearest(const Point *point, double min_dist) 
{
	for(std::list<CCurve>::iterator It=m_curves.begin(),ItNext=It; 
            It != m_curves.end(); It=ItNext) 
    {
        ++ItNext;
        if(It->m_vertices.size()<=1)
            m_curves.erase(It);
    }

    if(m_curves.empty()) return;

    std::list<CCurve> curves;
    Point p;
    if(point) p =*point;
    if(min_dist < Point::tolerance) 
        min_dist = Point::tolerance;

    while(m_curves.size()) {
        std::list<CCurve>::iterator It=m_curves.begin();
        std::list<CCurve>::iterator ItBest=It++;
        Point best_point = ItBest->NearestPoint(p);
        double best_dist = p.dist(best_point);
        for(; It != m_curves.end(); ++It)
        {
            const CCurve& curve = *It;
            Point near_point;
            double dist;
            if(min_dist>Point::tolerance && !curve.IsClosed()) {
                double d1 = curve.m_vertices.front().m_p.dist(p);
                double d2 = curve.m_vertices.back().m_p.dist(p);
                if(d1<d2) {
                    dist = d1;
                    near_point = curve.m_vertices.front().m_p;
                }else{
                    dist = d2;
                    near_point = curve.m_vertices.back().m_p;
                }
            }else{
                near_point = curve.NearestPoint(p);
                dist = near_point.dist(p);
            }
            if(dist < best_dist)
            {
                best_dist = dist;
                best_point = near_point;
                ItBest = It;
            }
        }
        if(ItBest->IsClosed()) {
            ItBest->ChangeStart(best_point);
        }else{
            double dfront = ItBest->m_vertices.front().m_p.dist(best_point);
            double dback = ItBest->m_vertices.back().m_p.dist(best_point);
            if(min_dist>Point::tolerance && dfront>min_dist && dback>min_dist) {
                ItBest->Break(best_point);
                m_curves.push_back(*ItBest);
                m_curves.back().ChangeEnd(best_point);
                ItBest->ChangeStart(best_point);
            }else if(dfront>dback)
                ItBest->Reverse();
        }
        curves.splice(curves.end(),m_curves,ItBest);
        p = curves.back().m_vertices.back().m_p;
    }
    m_curves.splice(m_curves.end(),curves);
}


void CArea::GetBox(CBox2D &box)
{
	for(std::list<CCurve>::iterator It = m_curves.begin(); It != m_curves.end(); It++)
	{
		CCurve& curve = *It;
		curve.GetBox(box);
	}
}

void CArea::Reorder()
{
	// curves may have been added with wrong directions
	// test all kurves to see which one are outsides and which are insides and 
	// make sure outsides are anti-clockwise and insides are clockwise

	// returns 0, if the curves are OK
	// returns 1, if the curves are overlapping

	CAreaOrderer ao;
	for(std::list<CCurve>::iterator It = m_curves.begin(), ItNext=It; It != m_curves.end(); It=ItNext)
	{
        ++ItNext;
		CCurve& curve = *It;
        if(!It->IsClosed())
            continue;
		ao.Insert(make_shared<CCurve>(curve));
		if(m_set_processing_length_in_split)
		{
			CArea::m_processing_done += (m_split_processing_length / m_curves.size());
		}
        m_curves.erase(It);
	}

    if(ao.m_top_level)
        ao.m_top_level->GetArea(*this);
}

class ZigZag
{
public:
	CCurve zig;
	CCurve zag;
	ZigZag(const CCurve& Zig, const CCurve& Zag):zig(Zig), zag(Zag){}
};

static double stepover_for_pocket = 0.0;
static std::list<ZigZag> zigzag_list_for_zigs;
static std::list<CCurve> *curve_list_for_zigs = NULL;
static bool rightward_for_zigs = true;
static double sin_angle_for_zigs = 0.0;
static double cos_angle_for_zigs = 0.0;
static double sin_minus_angle_for_zigs = 0.0;
static double cos_minus_angle_for_zigs = 0.0;
static double one_over_units = 0.0;

static Point rotated_point(const Point &p)
{
	return Point(p.x * cos_angle_for_zigs - p.y * sin_angle_for_zigs, p.x * sin_angle_for_zigs + p.y * cos_angle_for_zigs);
}
    
static Point unrotated_point(const Point &p)
{
    return Point(p.x * cos_minus_angle_for_zigs - p.y * sin_minus_angle_for_zigs, p.x * sin_minus_angle_for_zigs + p.y * cos_minus_angle_for_zigs);
}

static CVertex rotated_vertex(const CVertex &v)
{
	if(v.m_type)
	{
		return CVertex(v.m_type, rotated_point(v.m_p), rotated_point(v.m_c));
	}
    return CVertex(v.m_type, rotated_point(v.m_p), Point(0, 0));
}

static CVertex unrotated_vertex(const CVertex &v)
{
	if(v.m_type)
	{
		return CVertex(v.m_type, unrotated_point(v.m_p), unrotated_point(v.m_c));
	}
	return CVertex(v.m_type, unrotated_point(v.m_p), Point(0, 0));
}

static void rotate_area(CArea &a)
{
	for(std::list<CCurve>::iterator It = a.m_curves.begin(); It != a.m_curves.end(); It++)
	{
		CCurve& curve = *It;
		for(std::list<CVertex>::iterator CIt = curve.m_vertices.begin(); CIt != curve.m_vertices.end(); CIt++)
		{
			CVertex& vt = *CIt;
			vt = rotated_vertex(vt);
		}
	}
}

void test_y_point(int i, const Point& p, Point& best_p, bool &found, int &best_index, double y, bool left_not_right)
{
	// only consider points at y
	if(fabs(p.y - y) < 0.002 * one_over_units)
	{
		if(found)
		{
			// equal high point
			if(left_not_right)
			{
				// use the furthest left point
				if(p.x < best_p.x)
				{
					best_p = p;
					best_index = i;
				}
			}
			else
			{
				// use the furthest right point
				if(p.x > best_p.x)
				{
					best_p = p;
					best_index = i;
				}
			}
		}
		else
		{
			best_p = p;
			best_index = i;
			found = true;
		}
	}
}

static void make_zig_curve(const CCurve& input_curve, double y0, double y)
{
	CCurve curve(input_curve);

	if(rightward_for_zigs)
	{
		if(curve.IsClockwise())
			curve.Reverse();
	}
	else
	{
		if(!curve.IsClockwise())
			curve.Reverse();
	}

    // find a high point to start looking from
	Point top_left;
	int top_left_index = 0;
	bool top_left_found = false;
	Point top_right;
	int top_right_index = 0;
	bool top_right_found = false;
	Point bottom_left;
	int bottom_left_index = 0;
	bool bottom_left_found = false;

	int i =0;
	for(std::list<CVertex>::const_iterator VIt = curve.m_vertices.begin(); VIt != curve.m_vertices.end(); VIt++, i++)
	{
		const CVertex& vertex = *VIt;

		test_y_point(i, vertex.m_p, top_right, top_right_found, top_right_index, y, !rightward_for_zigs);
		test_y_point(i, vertex.m_p, top_left, top_left_found, top_left_index, y, rightward_for_zigs);
		test_y_point(i, vertex.m_p, bottom_left, bottom_left_found, bottom_left_index, y0, rightward_for_zigs);
	}

	int start_index = 0;
	int end_index = 0;
	int zag_end_index = 0;

	if(bottom_left_found)start_index = bottom_left_index;
	else if(top_left_found)start_index = top_left_index;

	if(top_right_found)
	{
		end_index = top_right_index;
		zag_end_index = top_left_index;
	}
	else
	{
		end_index = bottom_left_index;
		zag_end_index =  bottom_left_index;
	}
	if(end_index <= start_index)end_index += (i-1);
	if(zag_end_index <= start_index)zag_end_index += (i-1);

    CCurve zig, zag;
    
    bool zig_started = false;
    bool zig_finished = false;
    bool zag_finished = false;
    
	int v_index = 0;
	for(int i = 0; i < 2; i++)
	{
		// process the curve twice because we don't know where it will start
		if(zag_finished)
			break;
		for(std::list<CVertex>::const_iterator VIt = curve.m_vertices.begin(); VIt != curve.m_vertices.end(); VIt++)
		{
			if(i == 1 && VIt == curve.m_vertices.begin())
			{
				continue;
			}

			const CVertex& vertex = *VIt;

			if(zig_finished)
			{
				zag.m_vertices.push_back(unrotated_vertex(vertex));
				if(v_index == zag_end_index)
				{
					zag_finished = true;
					break;
				}
			}
			else if(zig_started)
			{
				zig.m_vertices.push_back(unrotated_vertex(vertex));
				if(v_index == end_index)
				{
					zig_finished = true;
					if(v_index == zag_end_index)
					{
						zag_finished = true;
						break;
					}
					zag.m_vertices.push_back(unrotated_vertex(vertex));
				}
			}
			else
			{
				if(v_index == start_index)
				{
					zig.m_vertices.push_back(unrotated_vertex(vertex));
					zig_started = true;
				}
			}
			v_index++;
		}
	}
        
    if(zig_finished)
		zigzag_list_for_zigs.push_back(ZigZag(zig, zag));
}

void make_zig(const CArea &a, double y0, double y)
{
	for(std::list<CCurve>::const_iterator It = a.m_curves.begin(); It != a.m_curves.end(); It++)
	{
		const CCurve &curve = *It;
		make_zig_curve(curve, y0, y);
	}
}
        
std::list< std::list<ZigZag> > reorder_zig_list_list;
        
void add_reorder_zig(ZigZag &zigzag)
{
    // look in existing lists

	// see if the zag is part of an existing zig
	if(zigzag.zag.m_vertices.size() > 1)
	{
		const Point& zag_e = zigzag.zag.m_vertices.front().m_p;
		bool zag_removed = false;
		for(std::list< std::list<ZigZag> >::iterator It = reorder_zig_list_list.begin(); It != reorder_zig_list_list.end() && !zag_removed; It++)
		{
			std::list<ZigZag> &zigzag_list = *It;
			for(std::list<ZigZag>::iterator It2 = zigzag_list.begin(); It2 != zigzag_list.end() && !zag_removed; It2++)
			{
				const ZigZag& z = *It2;
				for(std::list<CVertex>::const_iterator It3 = z.zig.m_vertices.begin(); It3 != z.zig.m_vertices.end() && !zag_removed; It3++)
				{
					const CVertex &v = *It3;
					if((fabs(zag_e.x - v.m_p.x) < (0.002 * one_over_units)) && (fabs(zag_e.y - v.m_p.y) < (0.002 * one_over_units)))
					{
						// remove zag from zigzag
						zigzag.zag.m_vertices.clear();
						zag_removed = true;
					}
				}
			}
		}
	}

	// see if the zigzag can join the end of an existing list
	const Point& zig_s = zigzag.zig.m_vertices.front().m_p;
	for(std::list< std::list<ZigZag> >::iterator It = reorder_zig_list_list.begin(); It != reorder_zig_list_list.end(); It++)
	{
		std::list<ZigZag> &zigzag_list = *It;
		const ZigZag& last_zigzag = zigzag_list.back();
        const Point& e = last_zigzag.zig.m_vertices.back().m_p;
        if((fabs(zig_s.x - e.x) < (0.002 * one_over_units)) && (fabs(zig_s.y - e.y) < (0.002 * one_over_units)))
		{
            zigzag_list.push_back(zigzag);
			return;
		}
	}
        
    // else add a new list
    std::list<ZigZag> zigzag_list;
    zigzag_list.push_back(zigzag);
    reorder_zig_list_list.push_back(zigzag_list);
}

void reorder_zigs()
{
	for(std::list<ZigZag>::iterator It = zigzag_list_for_zigs.begin(); It != zigzag_list_for_zigs.end(); It++)
	{
		ZigZag &zigzag = *It;
        add_reorder_zig(zigzag);
	}
        
	zigzag_list_for_zigs.clear();

	for(std::list< std::list<ZigZag> >::iterator It = reorder_zig_list_list.begin(); It != reorder_zig_list_list.end(); It++)
	{
		std::list<ZigZag> &zigzag_list = *It;
		if(zigzag_list.size() == 0)continue;

		curve_list_for_zigs->push_back(CCurve());
		for(std::list<ZigZag>::const_iterator It = zigzag_list.begin(); It != zigzag_list.end();)
		{
			const ZigZag &zigzag = *It;
			for(std::list<CVertex>::const_iterator It2 = zigzag.zig.m_vertices.begin(); It2 != zigzag.zig.m_vertices.end(); It2++)
			{
				if(It2 == zigzag.zig.m_vertices.begin() && It != zigzag_list.begin())continue; // only add the first vertex if doing the first zig
				const CVertex &v = *It2;
				curve_list_for_zigs->back().m_vertices.push_back(v);
			}

			It++;
			if(It == zigzag_list.end())
			{
				for(std::list<CVertex>::const_iterator It2 = zigzag.zag.m_vertices.begin(); It2 != zigzag.zag.m_vertices.end(); It2++)
				{
					if(It2 == zigzag.zag.m_vertices.begin())continue; // don't add the first vertex of the zag
					const CVertex &v = *It2;
					curve_list_for_zigs->back().m_vertices.push_back(v);
				}
			}
		}
	}
	reorder_zig_list_list.clear();
}

static void zigzag(const CArea &input_a)
{
	if(input_a.m_curves.size() == 0)
	{
		CArea::m_processing_done += CArea::m_single_area_processing_length;
		return;
	}
    
    one_over_units = 1 / CArea::m_units;
    
	CArea a(input_a);
    rotate_area(a);
    
    CBox2D b;
	a.GetBox(b);
    
    double x0 = b.MinX() - 1.0;
    double x1 = b.MaxX() + 1.0;

    double height = b.MaxY() - b.MinY();
    int num_steps = int(height / stepover_for_pocket + 1);
    double y = b.MinY();// + 0.1 * one_over_units;
    Point null_point(0, 0);
	rightward_for_zigs = true;

	if(CArea::m_please_abort)return;

	double step_percent_increment = 0.8 * CArea::m_single_area_processing_length / num_steps;

	for(int i = 0; i<num_steps; i++)
	{
		double y0 = y;
		y = y + stepover_for_pocket;
		Point p0(x0, y0);
		Point p1(x0, y);
		Point p2(x1, y);
		Point p3(x1, y0);
		CCurve c;
		c.m_vertices.push_back(CVertex(0, p0, null_point, 0));
		c.m_vertices.push_back(CVertex(0, p1, null_point, 0));
		c.m_vertices.push_back(CVertex(0, p2, null_point, 1));
		c.m_vertices.push_back(CVertex(0, p3, null_point, 0));
		c.m_vertices.push_back(CVertex(0, p0, null_point, 1));
		CArea a2;
		a2.m_curves.push_back(c);
		a2.Intersect(a);
		make_zig(a2, y0, y);
		rightward_for_zigs = !rightward_for_zigs;
		if(CArea::m_please_abort)return;
		CArea::m_processing_done += step_percent_increment;
	}

	reorder_zigs();
	CArea::m_processing_done += 0.2 * CArea::m_single_area_processing_length;
}

void CArea::SplitAndMakePocketToolpath(std::list<CCurve> &curve_list, const CAreaPocketParams &params)const
{
	CArea::m_processing_done = 0.0;

	double save_units = CArea::m_units;
	CArea::m_units = 1.0;
	std::list<CArea> areas;
	m_split_processing_length = 50.0; // jump to 50 percent after split
	m_set_processing_length_in_split = true;
	Split(areas);
	m_set_processing_length_in_split = false;
	CArea::m_processing_done = m_split_processing_length;
	CArea::m_units = save_units;

	if(areas.size() == 0)return;

	double single_area_length = 50.0 / areas.size();

	for(std::list<CArea>::iterator It = areas.begin(); It != areas.end(); It++)
	{
		CArea::m_single_area_processing_length = single_area_length;
		CArea &ar = *It;
		ar.MakePocketToolpath(curve_list, params);
	}
}

void CArea::MakePocketToolpath(std::list<CCurve> &curve_list, const CAreaPocketParams &params)const
{
	double radians_angle = params.zig_angle * PI / 180;
	sin_angle_for_zigs = sin(-radians_angle);
	cos_angle_for_zigs = cos(-radians_angle);
	sin_minus_angle_for_zigs = sin(radians_angle);
	cos_minus_angle_for_zigs = cos(radians_angle);
	stepover_for_pocket = params.stepover;

	CArea a_offset = *this;
	double current_offset = params.tool_radius + params.extra_offset;

	a_offset.Offset(current_offset);

	if(params.mode == ZigZagPocketMode || params.mode == ZigZagThenSingleOffsetPocketMode)
	{
		curve_list_for_zigs = &curve_list;
		zigzag(a_offset);
	}
	else if(params.mode == SpiralPocketMode)
	{
		std::list<CArea> m_areas;
		a_offset.Split(m_areas);
		if(CArea::m_please_abort)return;
		if(m_areas.size() == 0)
		{
			CArea::m_processing_done += CArea::m_single_area_processing_length;
			return;
		}

		CArea::m_single_area_processing_length /= m_areas.size();

		for(std::list<CArea>::iterator It = m_areas.begin(); It != m_areas.end(); It++)
		{
			CArea &a2 = *It;
			a2.MakeOnePocketCurve(curve_list, params);
		}
	}

	if(params.mode == SingleOffsetPocketMode || params.mode == ZigZagThenSingleOffsetPocketMode)
	{
		// add the single offset too
		for(std::list<CCurve>::iterator It = a_offset.m_curves.begin(); It != a_offset.m_curves.end(); It++)
		{
			CCurve& curve = *It;
			curve_list.push_back(curve);
		}
	}
}

void CArea::Split(std::list<CArea> &m_areas)const
{
	if(HolesLinked())
	{
		for(std::list<CCurve>::const_iterator It = m_curves.begin(); It != m_curves.end(); It++)
		{
			const CCurve& curve = *It;
			m_areas.push_back(CArea());
			m_areas.back().m_curves.push_back(curve);
		}
	}
	else
	{
		CArea a = *this;
		a.Reorder();

		if(CArea::m_please_abort)return;

		for(std::list<CCurve>::const_iterator It = a.m_curves.begin(); It != a.m_curves.end(); It++)
		{
			const CCurve& curve = *It;
			if(curve.IsClockwise())
			{
				if(m_areas.size() > 0)
					m_areas.back().m_curves.push_back(curve);
			}
			else
			{
				m_areas.push_back(CArea());
				m_areas.back().m_curves.push_back(curve);
			}
		}
	}
}

double CArea::GetArea(bool always_add)const
{
	// returns the area of the area
	double area = 0.0;
	for(std::list<CCurve>::const_iterator It = m_curves.begin(); It != m_curves.end(); It++)
	{
		const CCurve& curve = *It;
		double a = curve.GetArea();
		if(always_add)area += fabs(a);
		else area += a;
	}
	return area;
}

eOverlapType GetOverlapType(const CCurve& c1, const CCurve& c2)
{
	CArea a1;
	a1.m_curves.push_back(c1);
	CArea a2;
	a2.m_curves.push_back(c2);

	return GetOverlapType(a1, a2);
}

eOverlapType GetOverlapType(const CArea& a1, const CArea& a2)
{
	CArea A1(a1);

	A1.Subtract(a2);
	if(A1.m_curves.size() == 0)
	{
		return eInside;
	}

	CArea A2(a2);
	A2.Subtract(a1);
	if(A2.m_curves.size() == 0)
	{
		return eOutside;
	}

	A1 = a1;
	A1.Intersect(a2);
	if(A1.m_curves.size() == 0)
	{
		return eSiblings;
	}

	return eCrossing;
}

bool IsInside(const Point& p, const CCurve& c)
{
	CArea a;
	a.m_curves.push_back(c);
	return IsInside(p, a);
}

bool IsInside(const Point& p, const CArea& a)
{
	CArea a2;
	CCurve c;
	c.m_vertices.push_back(CVertex(Point(p.x - 0.01, p.y - 0.01)));
	c.m_vertices.push_back(CVertex(Point(p.x + 0.01, p.y - 0.01)));
	c.m_vertices.push_back(CVertex(Point(p.x + 0.01, p.y + 0.01)));
	c.m_vertices.push_back(CVertex(Point(p.x - 0.01, p.y + 0.01)));
	c.m_vertices.push_back(CVertex(Point(p.x - 0.01, p.y - 0.01)));
	a2.m_curves.push_back(c);
	a2.Intersect(a);
	if(fabs(a2.GetArea()) < 0.0004)return false;
	return true;
}

void CArea::SpanIntersections(const Span& span, std::list<Point> &pts)const
{
	// this returns all the intersections of this area with the given span, ordered along the span

	// get all points where this area's curves intersect the span
	std::list<Point> pts2;
	for(std::list<CCurve>::const_iterator It = m_curves.begin(); It != m_curves.end(); It++)
	{
		const CCurve &c = *It;
		c.SpanIntersections(span, pts2);
	}

	// order them along the span
	std::multimap<double, Point> ordered_points;
	for(std::list<Point>::iterator It = pts2.begin(); It != pts2.end(); It++)
	{
		Point &p = *It;
		double t;
		if(span.On(p, &t))
		{
			ordered_points.insert(std::make_pair(t, p));
		}
	}

	// add them to the given list of points
	for(std::multimap<double, Point>::iterator It = ordered_points.begin(); It != ordered_points.end(); It++)
	{
		Point p = It->second;
		pts.push_back(p);
	}
}

void CArea::CurveIntersections(const CCurve& curve, std::list<Point> &pts)const
{
	// this returns all the intersections of this area with the given curve, ordered along the curve
	std::list<Span> spans;
	curve.GetSpans(spans);
	for(std::list<Span>::iterator It = spans.begin(); It != spans.end(); It++)
	{
		Span& span = *It;
		std::list<Point> pts2;
		SpanIntersections(span, pts2);
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

class ThickLine
{
public:
	CArea m_area;
	CCurve m_curve;

	ThickLine(const CCurve& curve)
	{
		m_curve = curve;
		m_area.append(curve);
		m_area.Thicken(0.001);
	}
};

void CArea::InsideCurves(const CCurve& curve, std::list<CCurve> &curves_inside)const
{
	//1. find the intersectionpoints between these two curves.
	std::list<Point> pts;
	CurveIntersections(curve, pts);

	//2.separate curve2 in multiple curves between these intersections.
	std::list<CCurve> separate_curves;
	curve.ExtractSeparateCurves(pts, separate_curves);

	//3. if the midpoint of a separate curve lies in a1, then we return it.
	for(std::list<CCurve>::iterator It = separate_curves.begin(); It != separate_curves.end(); It++)
	{
		CCurve &curve = *It;
		double length = curve.Perim();
		Point mid_point = curve.PerimToPoint(length * 0.5);
		if(IsInside(mid_point, *this))curves_inside.push_back(curve);
	}
}
