#pragma once

#include "Joint.h"

namespace MbD {
	class GearJoint : public Joint
	{
		//radiusI radiusJ aConstant 
	public:
		GearJoint();
		GearJoint(const char* str);
		void initializeGlobally() override;

		double radiusI, radiusJ, aConstant;
	};
}

