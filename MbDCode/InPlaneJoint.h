#pragma once

#include "Joint.h"

namespace MbD {
	class InPlaneJoint : public Joint
	{
		//offset
	public:
		InPlaneJoint();
		InPlaneJoint(const char* str);
		
		void createInPlaneConstraint();

		double offset;
	};
}

