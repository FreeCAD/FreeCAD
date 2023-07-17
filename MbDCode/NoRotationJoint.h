#pragma once

#include "Joint.h"

namespace MbD {
	class NoRotationJoint : public Joint
	{
		//
	public:
		NoRotationJoint();
		NoRotationJoint(const char* str);
		void initializeGlobally() override;

	};
}

