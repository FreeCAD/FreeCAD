#pragma once

#include "InPlaneJoint.h"

namespace MbD {
	class PlanarJoint : public InPlaneJoint
	{
		//
	public:
		PlanarJoint();
		PlanarJoint(const char* str);
		void initializeGlobally() override;

	};
}

