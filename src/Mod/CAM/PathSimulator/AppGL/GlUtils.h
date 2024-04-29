#ifndef __glutils_h__
#define __glutils_h__
#include "OpenGlWrapper.h"
#include "linmath.h"

#define PI 3.14159265f
#define PI2 (PI*2)

constexpr auto EPSILON = 0.00001f;
#define EQ_FLOAT(x,y) (fabs((x) - (y)) < EPSILON)

#define MS_MOUSE_LEFT  1
#define MS_MOUSE_RIGHT 2
#define MS_MOUSE_MID   4
#define GL(x) { GLClearError(); x; if (!GLLogError()) __debugbreak(); }
#define  RadToDeg(x) (x * 180.0f / PI)

namespace MillSim
{
	void GLClearError();
	bool GLLogError();
	extern mat4x4 identityMat;
	extern int gDebug;
}
#endif // !__glutils_h__

