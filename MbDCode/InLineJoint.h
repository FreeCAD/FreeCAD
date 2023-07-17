#pragma once

#include "Joint.h"

namespace MbD {
	class InLineJoint : public Joint
	{
		//
	public:
		InLineJoint();
		InLineJoint(const char* str);

		void createInLineConstraints();

	};
}

