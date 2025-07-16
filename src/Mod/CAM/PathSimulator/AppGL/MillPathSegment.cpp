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

#include "OpenGlWrapper.h"
#include "MillPathSegment.h"
#include "SimShapes.h"
#include "linmath.h"
#include "GlUtils.h"
#include <iostream>

constexpr auto pi = std::numbers::pi_v<float>;

#define N_MILL_SLICES 8
#define MAX_SEG_DEG (pi / 2.0f)   // 90 deg
#define NIN_SEG_DEG (pi / 90.0f)  // 2 deg
#define SWEEP_ARC_PAD 1.05f
#define PX 0
#define PY 1
#define PZ 2


namespace MillSim
{

bool IsVerticalMotion(MillMotion* m1, MillMotion* m2)
{
    return (m1->z != m2->z && EQ_FLOAT(m1->x, m2->x) && EQ_FLOAT(m1->y, m2->y));
}

bool IsArcMotion(MillMotion* m)
{
    if (m->cmd != eRotateCCW && m->cmd != eRotateCW) {
        return false;
    }
    return fabs(m->i) > EPSILON || fabs(m->j) > EPSILON;
}

float MillPathSegment::mResolution = 1;
float MillPathSegment::mSmallRadStep = (pi / 8);

MillPathSegment::MillPathSegment(EndMill* _endmill, MillMotion* from, MillMotion* to)
{
    mat4x4_identity(mShearMat);
    MotionPosToVec(mStartPos, from);
    MotionPosToVec(mDiff, to);
    vec3_sub(mDiff, mDiff, mStartPos);
    mXYDistance = sqrtf(mDiff[PX] * mDiff[PX] + mDiff[PY] * mDiff[PY]);
    mZDistance = fabsf(mDiff[PY]);
    mXYZDistance = sqrtf(mXYDistance * mXYDistance + mDiff[PZ] * mDiff[PZ]);
    mXYAngle = atan2f(mDiff[PY], mDiff[PX]);
    endmill = _endmill;
    mStartAngRad = mStepAngRad = 0;
    if (IsArcMotion(to)) {
        mMotionType = MTCurved;
        mRadius = sqrtf(to->j * to->j + to->i * to->i);
        mSmallRad = mRadius <= endmill->radius;

        if (mSmallRad) {
            mStepAngRad = mSmallRadStep;
        }
        else {
            mStepAngRad = asinf(mResolution / mRadius);
            if (mStepAngRad > MAX_SEG_DEG) {
                mStepAngRad = MAX_SEG_DEG;
            }
            else if (mStepAngRad < NIN_SEG_DEG) {
                mStepAngRad = NIN_SEG_DEG;
            }
        }

        MotionPosToVec(mCenter, from);
        mCenter[PX] += to->i;
        mCenter[PY] += to->j;
        mArcDir = to->cmd == eRotateCCW ? -1.f : 1.f;
        mStartAngRad = atan2f(mCenter[PX] - from->x, from->y - mCenter[PY]);
        float endAng = atan2f(mCenter[PX] - to->x, to->y - mCenter[PY]);
        mSweepAng = (mStartAngRad - endAng) * mArcDir;
        if (mSweepAng < EPSILON) {
            mSweepAng += pi * 2;
        }
        numSimSteps = (int)(mSweepAng / mStepAngRad) + 1;
        mStepAngRad = mArcDir * mSweepAng / numSimSteps;
        if (mSmallRad) {
            // when the radius is too small, we just use the tool itself to carve the stock
            mShape = endmill->toolShape;
        }
        else {
            endmill->GenerateArcSegmentDL(mRadius,
                                          mStepAngRad * SWEEP_ARC_PAD,
                                          mDiff[PZ] / numSimSteps,
                                          &mShape);
            numSimSteps++;
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
        for (int i = 1; i < numSimSteps; i++) {
            ang -= mStepAngRad;
            z += zStep;
            mpPos.X = mCenter[PX] - sinf(ang) * mRadius;
            mpPos.Y = mCenter[PY] + cosf(ang) * mRadius;
            mpPos.Z = z;
            mpPos.SegmentId = segmentIndex;
            pointsBuffer.push_back(mpPos);
        }
    }
    else {
        mpPos.X = mStartPos[PX] + mDiff[PX];
        mpPos.Y = mStartPos[PY] + mDiff[PY];
        mpPos.Z = mStartPos[PZ] + mDiff[PZ];
        mpPos.SegmentId = segmentIndex;
        pointsBuffer.push_back(mpPos);
    }
}

void MillPathSegment::render(int step)
{
    mStepNumber = step;
    mat4x4 mat, mat2, rmat;
    mat4x4_identity(mat);
    mat4x4_identity(rmat);
    if (mMotionType == MTCurved) {
        mat4x4_translate_in_place(mat,
                                  mCenter[PX],
                                  mCenter[PY],
                                  mCenter[PZ] + mDiff[PZ] * (step - 1) / numSimSteps);
        mat4x4_rotate_Z(mat, mat, mStartAngRad - (step - 1) * mStepAngRad);
        mat4x4_rotate_Z(rmat, rmat, mStartAngRad - (step - 1) * mStepAngRad);

        if (mSmallRad || step == numSimSteps) {
            mat4x4_translate_in_place(mat, 0, mRadius, 0);
            endmill->toolShape.Render(mat, rmat);
        }
        else {
            mShape.Render(mat, rmat);
        }
    }
    else {
        if (mMotionType == MTVertical) {
            if (mStepLength[PZ] > 0) {
                mat4x4_translate_in_place_v(mat, mStartPos);
            }
            else {
                mat4x4_translate_in_place(mat,
                                          mStartPos[PX],
                                          mStartPos[PY],
                                          mStartPos[PZ] + mStepNumber * mStepLength[PZ]);
            }
            endmill->toolShape.Render(mat, rmat);
        }
        else {
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
            mat4x4_translate_in_place(mat, renderDist, 0, mDiff[PZ]);
            endmill->halfToolShape.Render(mat, rmat);
        }
    }
}

void MillPathSegment::GetHeadPosition(vec3 headPos)
{
    if (mMotionType == MTCurved) {
        float angRad = mStartAngRad - mStepNumber * mStepAngRad;
        vec3_set(mHeadPos, -mRadius * sinf(angRad), mRadius * cosf(angRad), 0);
        vec3_add(mHeadPos, mHeadPos, mCenter);
    }
    else {
        vec3_dup(mHeadPos, mStepLength);
        vec3_scale(mHeadPos, mHeadPos, (float)mStepNumber);
        vec3_add(mHeadPos, mHeadPos, mStartPos);
    }
    vec3_dup(headPos, mHeadPos);
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
    mSmallRadStep = pi / 8;
    if (quality < 4) {
        mSmallRadStep = pi / 2;
    }
    else if (quality < 8) {
        mSmallRadStep = pi / 4;
    }
    return mResolution;
}
}  // namespace MillSim
