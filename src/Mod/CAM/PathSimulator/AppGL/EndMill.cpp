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

EndMill::EndMill(int toolid, float diameter)
{
    radius = diameter / 2;
    toolId = toolid;
}

EndMill::EndMill(const std::vector<float>& toolProfile, int toolid, float diameter)
    : EndMill(toolid, diameter)
{
    profilePoints = nullptr;
    mHandleAllocation = false;

    int srcBuffSize = toolProfile.size();
    nPoints = srcBuffSize / 2;
    if (nPoints < 2) {
        return;
    }

    // make sure last point is at 0,0 else, add it
    bool missingCenterPoint = fabs(toolProfile[nPoints * 2 - 2]) > 0.0001F;
    if (missingCenterPoint) {
        nPoints++;
    }

    int buffSize = PROFILE_BUFFER_SIZE(nPoints);
    profilePoints = new float[buffSize];
    if (profilePoints == nullptr) {
        return;
    }

    // copy profile points
    mHandleAllocation = true;
    for (int i = 0; i < srcBuffSize; i++) {
        profilePoints[i] = toolProfile[i] + 0.01F;  // add some width to reduce simulation artifacts
    }
    if (missingCenterPoint) {
        profilePoints[srcBuffSize] = profilePoints[srcBuffSize + 1] = 0.0F;
    }

    MirrorPointBuffer();
}

EndMill::~EndMill()
{
    toolShape.FreeResources();
    halfToolShape.FreeResources();
    pathShape.FreeResources();
    if (mHandleAllocation) {
        delete[] profilePoints;
    }
}

void EndMill::GenerateDisplayLists(float quality)
{
    // calculate number of slices based on quality.
    int nslices = 16;
    if (quality < 3) {
        nslices = 4;
    }
    else if (quality < 7) {
        nslices = 8;
    }

    // full tool
    toolShape.RotateProfile(profilePoints, nPoints, 0,  nslices, false);

    // half tool
    halfToolShape.RotateProfile(profilePoints, nPoints, 0,  nslices / 2, true);

    // unit path
    int nFullPoints = PROFILE_BUFFER_POINTS(nPoints);
    pathShape.ExtrudeProfileLinear(profilePoints, nFullPoints, 0, 1, 0, 0, true, false);
}

unsigned int
EndMill::GenerateArcSegmentDL(float radius, float angleRad, float zShift, Shape* retShape)
{
    int nFullPoints = PROFILE_BUFFER_POINTS(nPoints);
    retShape->ExtrudeProfileRadial(profilePoints,
                                   nFullPoints,
                                   radius,
                                   angleRad,
                                   zShift,
                                   true,
                                   true);
    return 0;
}

void EndMill::MirrorPointBuffer()
{
    int endpoint = PROFILE_BUFFER_POINTS(nPoints) - 1;
    for (int i = 0, j = endpoint * 2; i < (nPoints - 1) * 2; i += 2, j -= 2) {
        profilePoints[j] = -profilePoints[i];
        profilePoints[j + 1] = profilePoints[i + 1];
    }
}
