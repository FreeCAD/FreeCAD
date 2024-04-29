#include "EndMillBall.h"
#include "SimShapes.h"
#include "GlUtils.h"
#include <math.h>
#include <malloc.h>

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
