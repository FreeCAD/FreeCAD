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

#include "EndMill.h"
#include "MillMotion.h"
#include "MillPathLine.h"
#include "SimShapes.h"
#include "linmath.h"

namespace CAMSimulator
{

enum MotionType
{
    MTVertical = 0,
    MTHorizontal,
    MTCurved
};

class MillPathSegment
{
public:
    /// <summary>
    /// Create a mill path segment primitive
    /// </summary>
    /// <param name="endmill">Mill object</param>
    /// <param name="from">Start point</param>
    /// <param name="to">End point</param>
    /// <param name="frame">Frame both points are given in; places the segment in the world</param>
    MillPathSegment(
        const EndMill& endmill,
        const MillMotion& from,
        const MillMotion& to,
        const mat4x4 frame
    );
    virtual ~MillPathSegment();

    virtual void AppendPathPoints(std::vector<MillPathPosition>& pointsBuffer);
    virtual void render(int substep);
    virtual void GetHeadPosition(vec3 headPos);
    void GetToolRotation(mat4x4 rot) const;
    void SetMinSimSteps(int steps);
    float Length() const;
    void SetStepNumber(int step)
    {
        mStepNumber = step;
    }
    static float SetQuality(float quality, float maxStockDimension);  // 1 minimum, 10 maximum

public:
    const EndMill* endmill = nullptr;
    bool isMultyPart;
    bool isCutting = true;  // false for a move that changes the tool axis between frames
    int frameFrom = 0;      // frame of the motion the segment starts from
    int frameTo = 0;        // frame the segment is given and drawn in
    float feed = 0;         // mm/s, 0 when not known
    bool isRapid = false;
    float duration = 0;    // seconds the machine takes for the segment
    float startTime = 0;   // seconds from program start to the segment's start
    int firstStep = 0;     // simulation step the segment starts at
    int op = -1;           // operation the segment's move belongs to
    float turn = 0;        // degrees between the two poses of an index segment; 0 for a normal move
    float axisTurn = 0;    // rotary travel over the index summed over the axes, degrees, if known
    float rotaryTime = 0;  // seconds of the segment's duration spent turning the rotaries
    bool index = false;    // rotaries turn between frames; the tool cuts nothing new
    bool continuous = false;  // program turns the rotaries while cutting (XYZA moves and similar)
    std::vector<float> angFrom, angTo;  // rotary positions at the segment ends, degrees
    float machineLength = 0;            // continuous move: machine X, Y, Z travel
    float rotaryTravel = 0;             // continuous move: largest rotary travel, degrees
    vec3 machineFrom = {0, 0, 0};       // continuous move: end points in machine coordinates
    vec3 machineTo = {0, 0, 0};
    int numSimSteps;
    int indexInArray = -1;
    int segmentIndex = -1;

protected:
    mat4x4 mShearMat;
    mat4x4 mFrame;
    mat4x4 mFrameRot;
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

}  // namespace CAMSimulator
