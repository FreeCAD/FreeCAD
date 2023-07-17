#pragma once

#include "InPlaneJoint.h"

namespace MbD {
	class PointInPlaneJoint : public InPlaneJoint
	{
		//
	public:
		PointInPlaneJoint();
		PointInPlaneJoint(const char* str);
		void initializeGlobally() override;

	};
}

