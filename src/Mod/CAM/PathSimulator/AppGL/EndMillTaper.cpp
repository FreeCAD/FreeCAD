#include "EndMillTaper.h"
#include "SimShapes.h"
#include "GlUtils.h"
#include <math.h>

using namespace MillSim;

EndMillTaper::EndMillTaper(int toolid, float diameter, int nslices, float taperAngle, float flatRadius):
	EndMill(toolid, diameter, nslices)
{
	float ta = (float)tanf((float)(taperAngle * PI / 360));
	float l1 = flatRadius / ta;
	if (l1 < 0.0001)
		l1 = 0;
	float l = mRadius / ta - l1;
	int idx = 0;
	SET_DUAL(_profVerts, idx, mRadius, MILL_HEIGHT);
	SET_DUAL(_profVerts, idx, mRadius, l);
	SET_DUAL(_profVerts, idx, flatRadius, 0);
	mNPoints = 3;
	if (l1 > 0)
	{
		SET_DUAL(_profVerts, idx, 0, 0);
		mNPoints++;
	}
	mProfPoints = _profVerts;
	MirrorPointBuffer();
	//GenerateDisplayLists();
}
