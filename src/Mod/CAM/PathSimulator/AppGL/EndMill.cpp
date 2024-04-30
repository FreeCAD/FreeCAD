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

#include "EndMill.h"
#include "OpenGlWrapper.h"
#include "SimShapes.h"

using namespace MillSim;

EndMill::EndMill(int toolid, float diameter, int nslices)
{
	mRadius = diameter / 2;
	mNSlices = nslices;
	mToolId = toolid;
	//mToolDisplayId = mHToolDisplayId = mPathDisplayId = 0;
}

EndMill::~EndMill()
{
	mToolShape.FreeResources();
	mHToolShape.FreeResources();
	mPathShape.FreeResources();
}

void EndMill::GenerateDisplayLists()
{
	// full tool
	mToolShape.RotateProfile(mProfPoints, mNPoints, 0, 0, mNSlices, false);

	// half tool
	mHToolShape.RotateProfile(mProfPoints, mNPoints, 0, 0, mNSlices / 2, true);

	// unit path
	int nFullPoints = PROFILE_BUFFER_POINTS(mNPoints);
	mPathShape.ExtrudeProfileLinear(mProfPoints, nFullPoints, 0, 1, 0, 0, true, false);
}

unsigned int EndMill::GenerateArcSegmentDL(float radius, float angleRad, float zShift, Shape *retShape)
{
	int nFullPoints = PROFILE_BUFFER_POINTS(mNPoints);
	retShape->ExtrudeProfileRadial(mProfPoints, PROFILE_BUFFER_POINTS(mNPoints), radius, angleRad, zShift, true, true);
	return 0;
}

void EndMill::MirrorPointBuffer()
{
	int endpoint = PROFILE_BUFFER_POINTS(mNPoints) - 1;
	for (int i = 0, j = endpoint * 2; i < (mNPoints - 1) * 2; i += 2, j -= 2)
	{
		mProfPoints[j] = -mProfPoints[i];
		mProfPoints[j + 1] = mProfPoints[i + 1];
	}
}
