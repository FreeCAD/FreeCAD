// AreaOrderer.h
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#pragma once
#include <list>
#include <set>

class CArea;
class CCurve;

class CAreaOrderer;

class CInnerCurves
{
	CInnerCurves* m_pOuter;
	const CCurve* m_curve; // always empty if top level
	std::set<CInnerCurves*> m_inner_curves;
	CArea *m_unite_area; // new curves made by uniting are stored here

public:
	static CAreaOrderer* area_orderer;
	CInnerCurves(CInnerCurves* pOuter, const CCurve* curve);
	~CInnerCurves();

	void Insert(const CCurve* pcurve);
	void GetArea(CArea &area, bool outside = true, bool use_curve = true)const;
	void Unite(const CInnerCurves* c);
};

class CAreaOrderer
{
public:
	CInnerCurves* m_top_level;

	CAreaOrderer();

	void Insert(CCurve* pcurve);
	CArea ResultArea()const;
};