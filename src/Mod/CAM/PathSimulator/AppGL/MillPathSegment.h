#ifndef __mill_path_segment_h__
#define __mill_path_segment_h__


#include "MillMotion.h"
#include "EndMill.h"
#include "linmath.h"

namespace MillSim {

    enum MotionType
    {
        MTVertical = 0,
        MTHorizontal,
        MTCurved
    };

    extern float resolution;

    bool IsVerticalMotion(MillMotion* m1, MillMotion* m2);


    class MillPathSegment
    {
    public:        
        /// <summary>
        /// Create a flat mill primitive
        /// </summary>
        /// <param name="diam">Mill diameter</param>
        /// <param name="from">Start point</param>
        /// <param name="to">End point</param>
        MillPathSegment(EndMill *endmill, MillMotion *from, MillMotion *to);
        virtual ~MillPathSegment();


        /// Calls the display list.
        virtual void render(int substep);
        //virtual Vector3* GetHeadPosition();
        virtual void GetHeadPosition(vec3 headPos);

    public:
        EndMill* mEndmill = nullptr;
        bool isMultyPart;
        int numSimSteps;
        int indexInArray;

    protected:
        mat4x4 mShearMat;


    protected:
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
         vec3 mDiff;
         vec3 mStepLength = { 0 };
         vec3 mCenter = { 0 };
         vec3 mStartPos;
         vec3 mHeadPos = { 0 };
         MotionType mMotionType;
    };
}

#endif
