// Area.h
// Copyright 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.
// repository now moved to github

#ifndef AREA_HEADER
#define AREA_HEADER

#include "Curve.h"
#include "clipper.hpp"

enum PocketMode
{
	SpiralPocketMode,
	ZigZagPocketMode,
	SingleOffsetPocketMode,
	ZigZagThenSingleOffsetPocketMode,
};

struct CAreaPocketParams
{
	double tool_radius;
	double extra_offset;
	double stepover;
	bool from_center;
	PocketMode mode;
	double zig_angle;
	bool only_cut_first_offset;
	CAreaPocketParams(double Tool_radius, double Extra_offset, double Stepover, bool From_center, PocketMode Mode, double Zig_angle)
	{
		tool_radius = Tool_radius;
		extra_offset = Extra_offset;
		stepover = Stepover;
		from_center = From_center;
		mode = Mode;
		zig_angle = Zig_angle;
		only_cut_first_offset = false;
	}
};

class CArea
{
public:
	std::list<CCurve> m_curves;
	static double m_accuracy;
	static double m_units; // 1.0 for mm, 25.4 for inches. All points are multiplied by this before going to the engine
	static bool m_clipper_simple;
	static double m_clipper_clean_distance;
	static bool m_fit_arcs;
    static int m_min_arc_points;
    static int m_max_arc_points;
	static double m_processing_done; // 0.0 to 100.0, set inside MakeOnePocketCurve
	static double m_single_area_processing_length;
	static double m_after_MakeOffsets_length;
	static double m_MakeOffsets_increment;
	static double m_split_processing_length;
	static bool m_set_processing_length_in_split;
	static bool m_please_abort; // the user sets this from another thread, to tell MakeOnePocketCurve to finish with no result.
    static double m_clipper_scale;

	void append(const CCurve& curve);
	void move(CCurve&& curve);
	void Subtract(const CArea& a2);
	void Intersect(const CArea& a2);
	void Union(const CArea& a2);
	static CArea UniteCurves(std::list<CCurve> &curves);
	void Xor(const CArea& a2);
	void Offset(double inwards_value);
    void OffsetWithClipper(double offset, 
                            ClipperLib::JoinType joinType=ClipperLib::jtRound, 
                            ClipperLib::EndType endType=ClipperLib::etOpenRound,
                            double miterLimit = 5.0, 
                            double roundPrecision = 0.0);
	void Thicken(double value);
	void FitArcs();
	unsigned int num_curves(){return static_cast<int>(m_curves.size());}
	Point NearestPoint(const Point& p)const;
	void GetBox(CBox2D &box);
	void Reorder();
	void MakePocketToolpath(std::list<CCurve> &toolpath, const CAreaPocketParams &params)const;
	void SplitAndMakePocketToolpath(std::list<CCurve> &toolpath, const CAreaPocketParams &params)const;
	void MakeOnePocketCurve(std::list<CCurve> &curve_list, const CAreaPocketParams &params)const;
	static bool HolesLinked();
	void Split(std::list<CArea> &m_areas)const;
	double GetArea(bool always_add = false)const;
	void SpanIntersections(const Span& span, std::list<Point> &pts)const; 
	void CurveIntersections(const CCurve& curve, std::list<Point> &pts)const; 
	void InsideCurves(const CCurve& curve, std::list<CCurve> &curves_inside)const;

    void ChangeStartToNearest(const Point *pstart=NULL, double min_dist=1.0);

    //Avoid outside direct accessing static member variable because of Windows DLL issue
#define CAREA_PARAM_DECLARE(_type,_name) \
    static _type get_##_name();\
    static void set_##_name(_type _name);

    CAREA_PARAM_DECLARE(double,tolerance)
    CAREA_PARAM_DECLARE(bool,fit_arcs)
    CAREA_PARAM_DECLARE(bool,clipper_simple)
    CAREA_PARAM_DECLARE(double,clipper_clean_distance)
    CAREA_PARAM_DECLARE(double,accuracy)
    CAREA_PARAM_DECLARE(double,units)
    CAREA_PARAM_DECLARE(short,min_arc_points)
    CAREA_PARAM_DECLARE(short,max_arc_points)
    CAREA_PARAM_DECLARE(double,clipper_scale)

    // Following functions is add to operate on possible open curves
	void PopulateClipper(ClipperLib::Clipper &c, ClipperLib::PolyType type) const;
	void Clip(ClipperLib::ClipType op, 
              const CArea *a,
              ClipperLib::PolyFillType subjFillType = ClipperLib::pftEvenOdd,
              ClipperLib::PolyFillType clipFillType = ClipperLib::pftEvenOdd);
};

enum eOverlapType
{
	eOutside,
	eInside,
	eSiblings,
	eCrossing,
};

eOverlapType GetOverlapType(const CCurve& c1, const CCurve& c2);
eOverlapType GetOverlapType(const CArea& a1, const CArea& a2);
bool IsInside(const Point& p, const CCurve& c);
bool IsInside(const Point& p, const CArea& a);

#endif // #define AREA_HEADER
