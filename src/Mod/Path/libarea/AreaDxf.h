// AreaDxf.h
// Copyright (c) 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#pragma once

#include "dxf.h"

class CSketch;
class CArea;
class CCurve;

class AreaDxfRead : public CDxfRead{
    void StartCurveIfNecessary(const Base::Vector3d& startPoint) const;

public:
	CArea* m_area;
	AreaDxfRead(CArea* area, const char* filepath);

	// AreaDxfRead's virtual functions
	void OnReadLine(const Base::Vector3d& start, const Base::Vector3d& end, bool /*hidden*/) override;
	void OnReadArc(const Base::Vector3d& start, const Base::Vector3d& end, const Base::Vector3d& center, bool dir, bool /*hidden*/) override;
};
