#pragma once

#include "InLineJoint.h"

namespace MbD {
	class PointInLineJoint : public InLineJoint
	{
		//
	public:
		PointInLineJoint();
		PointInLineJoint(const char* str);
		void initializeGlobally() override;

	};
}

