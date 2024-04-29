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
	unsigned int dispId = glGenLists(1);
	glNewList(dispId, GL_COMPILE);
	retShape->ExtrudeProfileRadial(mProfPoints, PROFILE_BUFFER_POINTS(mNPoints), radius, angleRad, zShift, true, true);
	glEndList();
	return dispId;
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
