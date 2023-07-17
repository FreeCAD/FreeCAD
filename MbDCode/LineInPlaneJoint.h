#pragma once

#include "InPlaneJoint.h"

namespace MbD {
	class LineInPlaneJoint : public InPlaneJoint
	{
		//
	public:
		LineInPlaneJoint();
		LineInPlaneJoint(const char* str);
		void initializeGlobally() override;

	};
}

