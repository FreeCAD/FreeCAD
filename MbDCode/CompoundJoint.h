#pragma once

#include "Joint.h"

namespace MbD {
	class CompoundJoint : public Joint
	{
		//distanceIJ
	public:
		CompoundJoint();
		CompoundJoint(const char* str);

		double distanceIJ = 0.0;
	};
}


