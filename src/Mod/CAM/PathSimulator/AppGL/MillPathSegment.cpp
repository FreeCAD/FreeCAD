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

#include "MillPathSegment.h"

#include "GlUtils.h"
#include <cmath>
#include <iostream>
#include <numbers>

constexpr auto pi = std::numbers::pi_v<float>;

#define N_MILL_SLICES 8
#define MAX_SEG_DEG (pi / 2.0f)   // 90 deg
#define MIN_SEG_DEG (pi / 90.0f)  // 2 deg
#define SWEEP_ARC_PAD 1.05f
#define PX 0
#define PY 1
#define PZ 2

namespace CAMSimulator
{

bool IsVerticalMotion(const MillMotion& m1, const MillMotion& m2)
{
    return (m1.z != m2.z && EQ_FLOAT(m1.x, m2.x) && EQ_FLOAT(m1.y, m2.y));
}

bool IsArcMotion(const MillMotion& m)
{
    if (m.cmd != eRotateCCW && m.cmd != eRotateCW) {
        return false;
    }
    return fabs(m.i) > EPSILON || fabs(m.j) > EPSILON;
}

float MillPathSegment::mResolution = 1;
float MillPathSegment::mSmallRadStep = (pi / 8);

MillPathSegment::MillPathSegment(
    const EndMill& _endmill,
    const MillMotion& from,
    const MillMotion& to,
    const mat4x4 frame
)
{
    mat4x4_identity(mShearMat);
    mat4x4_dup(mFrame, frame);
    FrameRotation(mFrameRot, frame);
    MotionPosToVec(mStartPos, from);
    MotionPosToVec(mDiff, to);
    vec3_sub(mDiff, mDiff, mStartPos);
    mXYDistance = sqrtf(mDiff[PX] * mDiff[PX] + mDiff[PY] * mDiff[PY]);
    mZDistance = fabsf(mDiff[PY]);
    mXYZDistance = sqrtf(mXYDistance * mXYDistance + mDiff[PZ] * mDiff[PZ]);
    mXYAngle = atan2f(mDiff[PY], mDiff[PX]);
    endmill = &_endmill;
    mStartAngRad = mStepAngRad = 0;
    if (IsArcMotion(to)) {
        mMotionType = MTCurved;
        mRadius = sqrtf(to.j * to.j + to.i * to.i);
        mSmallRad = mRadius <= endmill->radius;

        if (mSmallRad) {
            mStepAngRad = mSmallRadStep;
        }
        else {
            mStepAngRad = asinf(mResolution / mRadius);
            mStepAngRad = std::min(std::fmax(mStepAngRad, MAX_SEG_DEG), MIN_SEG_DEG);
        }

        MotionPosToVec(mCenter, from);
        mCenter[PX] += to.i;
        mCenter[PY] += to.j;
        mArcDir = to.cmd == eRotateCCW ? -1.f : 1.f;
        mStartAngRad = atan2f(mCenter[PX] - from.x, from.y - mCenter[PY]);
        float endAng = atan2f(mCenter[PX] - to.x, to.y - mCenter[PY]);
        mSweepAng = (mStartAngRad - endAng) * mArcDir;
        if (mSweepAng < EPSILON) {
            mSweepAng += pi * 2;
        }
        numSimSteps = (int)(mSweepAng / mStepAngRad) + 1;
        mStepAngRad = mArcDir * mSweepAng / numSimSteps;
        // A tight arc is cut with the tool's own shapes (see render). It must not copy them into
        // mShape: that would share their buffers, which the destructor then frees.
        if (!mSmallRad) {
            endmill->GenerateArcSegmentDL(
                mRadius,
                mStepAngRad * SWEEP_ARC_PAD,
                mDiff[PZ] / numSimSteps,
                &mShape
            );
        }

        isMultyPart = true;
    }
    else {
        numSimSteps = (int)(mXYZDistance / mResolution);
        if (numSimSteps == 0) {
            numSimSteps = 1;
        }
        isMultyPart = false;
        mStepDistance = mXYDistance / numSimSteps;
        mStepLength[PX] = mDiff[PX];
        mStepLength[PY] = mDiff[PY];
        mStepLength[PZ] = mDiff[PZ];
        vec3_scale(mStepLength, mStepLength, 1.f / (float)numSimSteps);

        if (IsVerticalMotion(from, to)) {
            mMotionType = MTVertical;
        }
        else {
            mMotionType = MTHorizontal;
            mShearMat[0][2] = mDiff[PZ] / mXYDistance;
        }
    }

    assert(numSimSteps >= 0);
}

MillPathSegment::~MillPathSegment()
{
    mShape.FreeResources();
}

void MillPathSegment::AppendPathPoints(std::vector<MillPathPosition>& pointsBuffer)
{
    MillPathPosition mpPos;
    if (mMotionType == MTCurved) {
        float ang = mStartAngRad;
        float z = mStartPos[PZ];
        float zStep = mDiff[PZ] / numSimSteps;
        for (int i = 1; i <= numSimSteps; i++) {
            ang -= mStepAngRad;
            z += zStep;
            vec3 world;
            FramePosToWorld(
                world,
                mFrame,
                mCenter[PX] - sinf(ang) * mRadius,
                mCenter[PY] + cosf(ang) * mRadius,
                z
            );
            mpPos.X = world[PX];
            mpPos.Y = world[PY];
            mpPos.Z = world[PZ];
            mpPos.SegmentId = segmentIndex;
            pointsBuffer.push_back(mpPos);
        }
    }
    else {
        vec3 world;
        FramePosToWorld(
            world,
            mFrame,
            mStartPos[PX] + mDiff[PX],
            mStartPos[PY] + mDiff[PY],
            mStartPos[PZ] + mDiff[PZ]
        );
        mpPos.X = world[PX];
        mpPos.Y = world[PY];
        mpPos.Z = world[PZ];
        mpPos.SegmentId = segmentIndex;
        pointsBuffer.push_back(mpPos);
    }
}

void MillPathSegment::render(int step)
{
    mStepNumber = step;
    mat4x4 mat, mat2, rmat;
    mat4x4_dup(mat, mFrame);
    mat4x4_dup(rmat, mFrameRot);
    if (mMotionType == MTCurved && mSmallRad) {
        // Tight arc (radius not larger than the tool's): sweep the tool along the chord from the
        // previous step to this one, like a straight move, and draw the whole tool only at the
        // arc start. Drawing the whole tool at every step left dimples between the steps.
        const float a0 = mStartAngRad - (step - 1) * mStepAngRad;
        const float a1 = a0 - mStepAngRad;
        const float dz = mDiff[PZ] / numSimSteps;
        const float z0 = mCenter[PZ] + dz * (step - 1);
        const float x0 = mCenter[PX] - sinf(a0) * mRadius;
        const float y0 = mCenter[PY] + cosf(a0) * mRadius;
        const float dx = mCenter[PX] - sinf(a1) * mRadius - x0;
        const float dy = mCenter[PY] + cosf(a1) * mRadius - y0;
        const float len = sqrtf(dx * dx + dy * dy);
        mat4x4_translate_in_place(mat, x0, y0, z0);
        if (step == 1 || len < EPSILON) {
            endmill->toolShape.Render(mat, rmat);
        }
        if (len >= EPSILON) {
            const float ang = atan2f(dy, dx);
            mat4x4_rotate_Z(mat, mat, ang);
            mat4x4_rotate_Z(rmat, rmat, ang);
            mat4x4 shear;
            mat4x4_identity(shear);
            shear[0][2] = dz / len;
            mat4x4_mul(mat2, mat, shear);
            mat4x4_scale_aniso(mat2, mat2, len, 1, 1);
            endmill->pathShape.Render(mat2, rmat);
            mat4x4_translate_in_place(mat, len, 0, dz);
            endmill->halfToolShape.Render(mat, rmat);
        }
    }
    else if (mMotionType == MTCurved) {
        mat4x4_translate_in_place(
            mat,
            mCenter[PX],
            mCenter[PY],
            mCenter[PZ] + mDiff[PZ] * (step - 1) / numSimSteps
        );
        mat4x4_rotate_Z(mat, mat, mStartAngRad - (step - 1) * mStepAngRad);
        mat4x4_rotate_Z(rmat, rmat, mStartAngRad - (step - 1) * mStepAngRad);
        mShape.Render(mat, rmat);
        // the last piece ends in the whole tool
        if (step == numSimSteps) {
            mat4x4_rotate_Z(mat, mat, -mStepAngRad);
            mat4x4_rotate_Z(rmat, rmat, -mStepAngRad);
            mat4x4_translate_in_place(mat, 0, mRadius, mDiff[PZ] / numSimSteps);
            endmill->toolShape.Render(mat, rmat);
        }
    }
    else {
        if (mMotionType == MTVertical) {
            // the tool at the lowest point of its travel so far
            const int lowest = mStepLength[PZ] > 0 ? 0 : step;
            mat4x4_translate_in_place(
                mat,
                mStartPos[PX],
                mStartPos[PY],
                mStartPos[PZ] + lowest * mStepLength[PZ]
            );
            endmill->toolShape.Render(mat, rmat);
        }
        else {
            // The end tool goes where the tool has got to, at the depth reached so far: placing it
            // at the move's final depth cut too deep at the end of a ramp.
            float renderDist = step * mStepDistance;
            mat4x4_translate_in_place_v(mat, mStartPos);
            mat4x4_rotate_Z(mat, mat, mXYAngle);
            mat4x4_rotate_Z(rmat, rmat, mXYAngle);
            mat4x4_dup(mat2, mat);
            if (mDiff[PZ] != 0.0) {
                mat4x4_mul(mat2, mat2, mShearMat);
            }
            mat4x4_scale_aniso(mat2, mat2, renderDist, 1, 1);
            endmill->pathShape.Render(mat2, rmat);
            mat4x4_translate_in_place(mat, renderDist, 0, step * mStepLength[PZ]);
            endmill->halfToolShape.Render(mat, rmat);
        }
    }
}

void MillPathSegment::GetHeadPosition(vec3 headPos)
{
    if (mMotionType == MTCurved) {
        float angRad = mStartAngRad - mStepNumber * mStepAngRad;
        vec3_set(
            mHeadPos,
            -mRadius * sinf(angRad),
            mRadius * cosf(angRad),
            mDiff[PZ] * mStepNumber / numSimSteps
        );
        vec3_add(mHeadPos, mHeadPos, mCenter);
    }
    else {
        vec3_dup(mHeadPos, mStepLength);
        vec3_scale(mHeadPos, mHeadPos, (float)mStepNumber);
        vec3_add(mHeadPos, mHeadPos, mStartPos);
    }
    FramePosToWorld(headPos, mFrame, mHeadPos[PX], mHeadPos[PY], mHeadPos[PZ]);
}

void MillPathSegment::GetToolRotation(mat4x4 rot) const
{
    mat4x4_dup(rot, mFrameRot);
}

float MillPathSegment::Length() const
{
    if (mMotionType == MTCurved) {
        // helix length: the unrolled arc length combined with the rise
        const float arc = mRadius * mSweepAng;
        return sqrtf(arc * arc + mDiff[PZ] * mDiff[PZ]);
    }
    return mXYZDistance;
}

void MillPathSegment::SetMinSimSteps(int steps)
{
    if (mMotionType == MTCurved || numSimSteps >= steps) {
        return;
    }
    numSimSteps = steps;
    mStepDistance = mXYDistance / numSimSteps;
    vec3_scale(mStepLength, mDiff, 1.f / (float)numSimSteps);
}

float MillPathSegment::SetQuality(float quality, float maxStockDimension)
{
    mResolution = maxStockDimension * 0.05 / quality;
    if (mResolution > 4) {
        mResolution = 4;
    }
    if (mResolution < 0.5) {
        mResolution = 0.5;
    }
    // Tight arcs are cut by sweeping the tool along chords. At high quality the chords are short
    // enough to stay within about 0.015 mm of an arc of 3 mm radius.
    mSmallRadStep = quality >= 9 ? pi / 16 : pi / 8;
    if (quality < 4) {
        mSmallRadStep = pi / 2;
    }
    else if (quality < 8) {
        mSmallRadStep = pi / 4;
    }
    return mResolution;
}

}  // namespace CAMSimulator
