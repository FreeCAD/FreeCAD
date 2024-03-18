// AreaDxf.cpp
// Copyright (c) 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "AreaDxf.h"
#include "Area.h"

AreaDxfRead::AreaDxfRead(CArea* area, const char* filepath):CDxfRead(filepath), m_area(area){}

void AreaDxfRead::StartCurveIfNecessary(const Base::Vector3d& startPoint) const
{
	Point ps(startPoint.x, startPoint.y);
	if(m_area->m_curves.empty() || m_area->m_curves.back().m_vertices.empty() || m_area->m_curves.back().m_vertices.back().m_p != ps)
	{
		// start a new curve
		m_area->m_curves.emplace_back();
		m_area->m_curves.back().m_vertices.emplace_back(ps);
	}
}

void AreaDxfRead::OnReadLine(const Base::Vector3d& start, const Base::Vector3d& end, bool /*hidden*/)
{
	StartCurveIfNecessary(start);
	m_area->m_curves.back().m_vertices.emplace_back(Point(end.x, end.y));
}

void AreaDxfRead::OnReadArc(const Base::Vector3d& start,
                            const Base::Vector3d& end,
                            const Base::Vector3d& center,
                            bool dir,
                            bool /*hidden*/)
{
	StartCurveIfNecessary(start);
    m_area->m_curves.back().m_vertices.emplace_back(dir ? 1 : 0,
                                                    Point(end.x, end.y),
                                                    Point(center.x, center.y));
}
