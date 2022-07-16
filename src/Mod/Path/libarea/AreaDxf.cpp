// AreaDxf.cpp
// Copyright (c) 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "AreaDxf.h"
#include "Area.h"

AreaDxfRead::AreaDxfRead(CArea* area, const char* filepath):CDxfRead(filepath), m_area(area){}

void AreaDxfRead::StartCurveIfNecessary(const double* s)
{
	Point ps(s);
	if((m_area->m_curves.size() == 0) || (m_area->m_curves.back().m_vertices.size() == 0) || (m_area->m_curves.back().m_vertices.back().m_p != ps))
	{
		// start a new curve
		m_area->m_curves.emplace_back();
		m_area->m_curves.back().m_vertices.push_back(ps);
	}
}

void AreaDxfRead::OnReadLine(const double* s, const double* e, bool /*hidden*/)
{
	StartCurveIfNecessary(s);
	m_area->m_curves.back().m_vertices.push_back(Point(e));
}

void AreaDxfRead::OnReadArc(const double* s, const double* e, const double* c, bool dir, bool /*hidden*/)
{
	StartCurveIfNecessary(s);
	m_area->m_curves.back().m_vertices.emplace_back(dir?1:0, Point(e), Point(c));
}
