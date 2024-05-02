/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef __end_mill_h__
#define __end_mill_h__

#include "SimShapes.h"

#define PROFILE_BUFFER_POINTS(npoints) ((npoints) * 2 - 1)
#define PROFILE_BUFFER_SIZE(npoints) (PROFILE_BUFFER_POINTS(npoints) * 2)
#define MILL_HEIGHT 10

namespace MillSim
{
	class EndMill
	{
	public:
		float* mProfPoints = nullptr;
		float mRadius;
		int mNPoints = 0;
		int mNSlices;
		int mToolId = -1;
		//unsigned int mPathDisplayId;
		//unsigned int mHToolDisplayId;
		//unsigned int mToolDisplayId;

		Shape mPathShape;
		Shape mHToolShape;
		Shape mToolShape;

	public:
		EndMill(int toolid, float diameter, int nslices);
		EndMill(const float* toolProfile, int numPoints, int toolid, float diameter, int nslices);
		virtual ~EndMill();
		void GenerateDisplayLists();
		unsigned int GenerateArcSegmentDL(float radius, float angleRad, float zShift, Shape* retShape);

	protected:
		void MirrorPointBuffer();

	private:
		bool mHandleAllocation = false;
	};
}

#endif
