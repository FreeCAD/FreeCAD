#pragma once

#include "Joint.h"

namespace MbD {
	class PerpendicularJoint : public Joint
	{
		//
	public:
		PerpendicularJoint();
		PerpendicularJoint(const char* str);
		void initializeGlobally() override;

	};
}

