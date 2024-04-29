#include "OpenGlWrapper.h"
#include "MillPathSegment.h"
#include "SimShapes.h"
#include "linmath.h"
#include "GlUtils.h"
#include <iostream>

#define N_MILL_SLICES 8
#define MAX_SEG_DEG (PI / 8.0f)   // 22.5 deg
#define NIN_SEG_DEG (PI / 90.0f)  // 2 deg
#define SWEEP_ARC_PAD 1.05f
#define PX 0
#define PY 1
#define PZ 2



namespace MillSim {

    bool IsVerticalMotion(MillMotion* m1, MillMotion* m2)
    {
        return (m1->z != m2->z && EQ_FLOAT(m1->x, m2->x) && EQ_FLOAT(m1->y, m2->y));
    }

    bool IsArcMotion(MillMotion* m)
    {
        if (m->cmd != eRotateCCW && m->cmd != eRotateCW)
            return false;
        return fabs(m->i > EPSILON) || fabs(m->j) > EPSILON;
    }

    float resolution = 1;

    MillPathSegment::MillPathSegment(EndMill *endmill, MillMotion* from, MillMotion* to)
        : mShearMat {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        }
    {
        
        MotionPosToVec(mStartPos, from);
        MotionPosToVec(mDiff, to);
        vec3_sub(mDiff, mDiff, mStartPos);
        mXYDistance = sqrtf(mDiff[PX] * mDiff[PX] + mDiff[PY] * mDiff[PY]);
        mZDistance = fabsf(mDiff[PY]);
        mXYZDistance = sqrtf(mXYDistance * mXYDistance + mDiff[PZ] * mDiff[PZ]);
        mXYAngle = atan2f(mDiff[PY], mDiff[PX]);
        mEndmill = endmill;
        mStartAngRad = mStepAngRad = 0;
        if (IsArcMotion(to))
        {
            mMotionType = MTCurved;
            mRadius = sqrtf(to->j * to->j + to->i * to->i);
            mSmallRad = mRadius <= mEndmill->mRadius;
            mStepAngRad = mSmallRad ? MAX_SEG_DEG : asinf(resolution / mRadius);
            MotionPosToVec(mCenter, from);
            mCenter[PX] += to->i;
            mCenter[PY] += to->j;
            mArcDir = to->cmd == eRotateCCW ? -1.f : 1.f;
            if (mStepAngRad > MAX_SEG_DEG)
                mStepAngRad = MAX_SEG_DEG;
            else if (mStepAngRad < NIN_SEG_DEG)
                mStepAngRad = NIN_SEG_DEG;
            mStartAngRad = atan2f(mCenter[PX] - from->x, from->y - mCenter[PY]);
            float endAng = atan2f(mCenter[PX] - to->x, to->y - mCenter[PY]);
            mSweepAng = (mStartAngRad - endAng) * mArcDir;
            if (mSweepAng < EPSILON)
                mSweepAng += PI * 2;
            numSimSteps = (int)(mSweepAng / mStepAngRad) + 1;
            mStepAngRad = mArcDir * mSweepAng / numSimSteps;
            if (mSmallRad)
                // when the radius is too small, we just use the tool itself to carve the stock
                mShape = mEndmill->mToolShape;
            else
            {
                mEndmill->GenerateArcSegmentDL(mRadius, mStepAngRad * SWEEP_ARC_PAD, mDiff[PZ] / numSimSteps, &mShape);
                numSimSteps++;
            }
            
            isMultyPart = true;
        }
        else
        {
            numSimSteps = (int)(mXYZDistance / resolution);
            if (numSimSteps == 0)
                numSimSteps = 1;
            isMultyPart = false;
            mStepDistance = mXYDistance / numSimSteps;
            mStepLength[PX] = mDiff[PX]; mStepLength[PY] = mDiff[PY]; mStepLength[PZ] = mDiff[PZ];
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


    void MillPathSegment::render(int step) 
    {
        mStepNumber = step;
        mat4x4 mat, mat2, rmat;
        mat4x4_identity(mat);
        mat4x4_identity(rmat);
        if (mMotionType == MTCurved)
        {
            mat4x4_translate_in_place(mat, mCenter[PX], mCenter[PY], mCenter[PZ] + mDiff[PZ] * (step - 1) / numSimSteps);
            mat4x4_rotate_Z(mat, mat, mStartAngRad - (step - 1) * mStepAngRad);
            mat4x4_rotate_Z(rmat, rmat, mStartAngRad - (step - 1) * mStepAngRad);

            if (mSmallRad || step == numSimSteps)
            {
                mat4x4_translate_in_place(mat, 0, mRadius, 0);
                mEndmill->mToolShape.Render(mat, rmat);
            }
            else
                mShape.Render(mat, rmat);
        }
        else
        {
            if (mMotionType == MTVertical) {
                if (mStepLength[PZ] > 0)
                    mat4x4_translate_in_place_v(mat, mStartPos);
                else
                    mat4x4_translate_in_place(mat, mStartPos[PX], mStartPos[PY], mStartPos[PZ] + mStepNumber * mStepLength[PZ]);
                mEndmill->mToolShape.Render(mat, rmat);
            }
            else
            {
                float renderDist = step * mStepDistance;
                mat4x4_translate_in_place_v(mat, mStartPos);
                mat4x4_rotate_Z(mat, mat, mXYAngle);
                mat4x4_rotate_Z(rmat, rmat, mXYAngle);
                mat4x4_dup(mat2, mat);
                if (mDiff[PZ] != 0.0)
                    mat4x4_mul(mat2, mat2, mShearMat);
                mat4x4_scale_aniso(mat2, mat2, renderDist, 1, 1);
                mEndmill->mPathShape.Render(mat2, rmat);
                mat4x4_translate_in_place(mat, renderDist, 0, mDiff[PZ]);
                mEndmill->mHToolShape.Render(mat, rmat);

            }
        }
    }

    void MillPathSegment::GetHeadPosition(vec3 headPos)
    {
        if (mMotionType == MTCurved)
        {
            float angRad = mStartAngRad - mStepNumber * mStepAngRad;
            vec3_set(mHeadPos, -mRadius * sinf(angRad), mRadius * cosf(angRad), 0);
            vec3_add(mHeadPos, mHeadPos, mCenter);
        }
        else
        {
            vec3_dup(mHeadPos, mStepLength);
            vec3_scale(mHeadPos, mHeadPos, (float)mStepNumber);
            vec3_add(mHeadPos, mHeadPos, mStartPos);
        }
        vec3_dup(headPos, mHeadPos);
    }
}