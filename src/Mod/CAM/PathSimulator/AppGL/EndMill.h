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
		EndMill(int toolid, float radius, int nslices);
		virtual ~EndMill();
		void GenerateDisplayLists();
		unsigned int GenerateArcSegmentDL(float radius, float angleRad, float zShift, Shape* retShape);

	protected:
		void MirrorPointBuffer();
	};
}

#endif
