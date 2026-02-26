// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once


#include "MillMotion.h"
#include "EndMill.h"
#include "linmath.h"
#include "MillPathLine.h"

namespace MillSim
{

enum MotionType
{
    MTVertical = 0,
    MTHorizontal,
    MTCurved
};

bool IsVerticalMotion(MillMotion* m1, MillMotion* m2);


class MillPathSegment
{
public:
    /// <summary>
    /// Create a mill path segment primitive
    /// </summary>
    /// <param name="endmill">Mill object</param>
    /// <param name="from">Start point</param>
    /// <param name="to">End point</param>
    MillPathSegment(EndMill* endmill, MillMotion* from, MillMotion* to);
    virtual ~MillPathSegment();


    virtual void AppendPathPoints(std::vector<MillPathPosition>& pointsBuffer);
    virtual void render(int substep);
    virtual void GetHeadPosition(vec3 headPos);
    static float SetQuality(float quality, float maxStockDimension);  // 1 minimum, 10 maximum

public:
    EndMill* endmill = nullptr;
    bool isMultyPart;
    int numSimSteps;
    int indexInArray;
    int segmentIndex;


protected:
    mat4x4 mShearMat;
    Shape mShape;
    float mXYDistance;
    float mXYZDistance;
    float mZDistance;
    float mXYAngle;
    float mStartAngRad;
    float mStepAngRad;
    float mStepDistance = 0;
    float mSweepAng;
    float mRadius = 0;
    float mArcDir = 0;
    bool mSmallRad = false;
    int mStepNumber = 0;

    static float mSmallRadStep;
    static float mResolution;

    vec3 mDiff;
    vec3 mStepLength = {0};
    vec3 mCenter = {0};
    vec3 mStartPos;
    vec3 mHeadPos = {0};
    MotionType mMotionType;
};
}  // namespace MillSim
