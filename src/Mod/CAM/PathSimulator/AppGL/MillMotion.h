#ifndef __mill_operation_h__
#define __mill_operation_h__
//#include <math.h>
#include "EndMill.h"
#include "linmath.h"
namespace MillSim {

	enum eEndMillType
	{
		eEndmillFlat,
		eEndmillV,
		eEndmillBall,
		eEndmillFillet
	};

	enum eCmdType
	{
		eNop,
		eMoveLiner,
		eRotateCW,
		eRotateCCW,
		eChangeTool
	};

	struct MillMotion
	{
		eCmdType cmd;
		int tool;
		float x, y, z;
		float i, j, k;
	};

	static inline void MotionPosToVec(vec3 vec, const MillMotion* motion)
	{
		vec[0] = motion->x;
		vec[1] = motion->y;
		vec[2] = motion->z;
	}
}
#endif