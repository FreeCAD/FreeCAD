#pragma once

#include "Joint.h"

namespace MbD {
	class ParallelAxesJoint : public Joint
	{
		//
	public:
		ParallelAxesJoint();
		ParallelAxesJoint(const char* str);
		void initializeGlobally() override;

	};
}

