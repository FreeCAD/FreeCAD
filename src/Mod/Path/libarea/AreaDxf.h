// AreaDxf.h
// Copyright (c) 2011, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#pragma once

#include "dxf.h"

class CSketch;
class CArea;
class CCurve;

class AreaDxfRead : public CDxfRead{
	void StartCurveIfNecessary(const double* s);

public:
	CArea* m_area;
	AreaDxfRead(CArea* area, const char* filepath);

	// AreaDxfRead's virtual functions
	void OnReadLine(const double* s, const double* e, bool /*hidden*/) override;
	void OnReadArc(const double* s, const double* e, const double* c, bool dir, bool /*hidden*/) override;
};
