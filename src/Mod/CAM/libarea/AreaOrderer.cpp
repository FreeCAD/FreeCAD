// AreaOrderer.cpp

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


#include "AreaOrderer.h"
#include <memory>
#include "Area.h"

using namespace std;

CAreaOrderer* CInnerCurves::area_orderer = NULL;

CInnerCurves::CInnerCurves(shared_ptr<CInnerCurves> pOuter, shared_ptr<CCurve> curve)
:m_pOuter(pOuter)
,m_curve(curve)
{
}

CInnerCurves::~CInnerCurves()
{
}

void CInnerCurves::Insert(shared_ptr<CCurve> pcurve)
{
	std::list<shared_ptr<CInnerCurves> > outside_of_these;
	std::list<shared_ptr<CInnerCurves> > crossing_these;

	// check all inner curves
    for(shared_ptr<CInnerCurves> c : m_inner_curves) {

		switch(GetOverlapType(*pcurve, *(c->m_curve)))
		{
		case eOutside:
			outside_of_these.push_back(c);
			break;

		case eInside:
			// insert in this inner curve
			c->Insert(pcurve);
			return;

		case eSiblings:
			break;

		case eCrossing:
			crossing_these.push_back(c);
			break;
		}
	}

	// add as a new inner
	shared_ptr<CInnerCurves> new_item(new CInnerCurves(shared_from_this(), pcurve));
	this->m_inner_curves.insert(new_item);

    for(const shared_ptr<CInnerCurves>& c : outside_of_these) {
		// move items
		c->m_pOuter = new_item;
		new_item->m_inner_curves.insert(c);
		this->m_inner_curves.erase(c);
	}

    for(const shared_ptr<CInnerCurves>& c : crossing_these) {
		// unite these
		new_item->Unite(c);
		this->m_inner_curves.erase(c);
	}
}

void CInnerCurves::GetArea(CArea &area, bool outside, bool use_curve)
{
	if(use_curve && m_curve)
	{
		area.m_curves.push_back(*m_curve);
		outside = !outside;
	}

	std::list<shared_ptr<CInnerCurves> > do_after;

	for(shared_ptr<CInnerCurves> c: m_inner_curves) {
		area.m_curves.push_back(*c->m_curve);
		if(!outside)area.m_curves.back().Reverse();

		if(outside)c->GetArea(area, !outside, false);
		else do_after.push_back(c);
	}

	for(shared_ptr<CInnerCurves> c : do_after)
		c->GetArea(area, !outside, false);
}

void CInnerCurves::Unite(shared_ptr<CInnerCurves> c)
{
	// unite all the curves in c, with this one
	shared_ptr<CArea> new_area(new CArea());
	new_area->m_curves.push_back(*m_curve);
    m_unite_area = new_area;

	CArea a2;
	c->GetArea(a2);

	m_unite_area->Union(a2);
	m_unite_area->Reorder();
	for(std::list<CCurve>::iterator It = m_unite_area->m_curves.begin(); It != m_unite_area->m_curves.end(); It++)
	{
		CCurve &curve = *It;
		if(It == m_unite_area->m_curves.begin())
			m_curve = make_shared<CCurve>(curve);
		else
		{
			if(curve.IsClockwise())curve.Reverse();
			Insert(std::make_shared<CCurve>(curve));
		}
	}
}

CAreaOrderer::CAreaOrderer()
    :m_top_level(make_shared<CInnerCurves>())
{
}

void CAreaOrderer::Insert(shared_ptr<CCurve> pcurve)
{
	CInnerCurves::area_orderer = this;

	// make them all anti-clockwise as they come in
	if(pcurve->IsClockwise())pcurve->Reverse();

	m_top_level->Insert(pcurve);
}

CArea CAreaOrderer::ResultArea()const
{
	CArea a;

	if(m_top_level)
	{
		m_top_level->GetArea(a);
	}

	return a;
}

