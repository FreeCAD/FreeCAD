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

#include "EndMillBall.h"
#include "SimShapes.h"
#include "GlUtils.h"
#include <math.h>
#include <stdlib.h>

using namespace MillSim;

EndMillBall::EndMillBall(int toolid, float diameter, int nslices, int nSections, float flatRadius):
	EndMill(toolid, diameter, nslices)
{
	mNPoints = nSections + 2;
	if (flatRadius < 0.0001)
		flatRadius = 0;
	else
		mNPoints++;
	mProfPoints = (float*)malloc(PROFILE_BUFFER_SIZE(mNPoints) * sizeof(float));
	if (mProfPoints == nullptr)
		return;

	float r2 = mRadius - flatRadius;
	if (r2 < 0.0001)
		r2 = 0.0001f;
	float astep =  (float)(PI / (2 * nSections));
	int idx = 0;
	SET_DUAL(mProfPoints, idx, mRadius, MILL_HEIGHT);
	SET_DUAL(mProfPoints, idx, mRadius, r2);
	float ang = astep;
	for (int i = 1; i <nSections; i++, ang += astep)
		SET_DUAL(mProfPoints, idx, flatRadius + r2 * cosf(ang), r2 - r2 * sinf(ang));
	SET_DUAL(mProfPoints, idx, flatRadius, 0);
	if (flatRadius > 0)
		SET_DUAL(mProfPoints, idx, 0, 0);
	MirrorPointBuffer();
	//GenerateDisplayLists();
}

EndMillBall::~EndMillBall()
{
	if (mProfPoints != nullptr)
		free(mProfPoints);
}
