#pragma once

#include "AtPointJoint.h"

namespace MbD {
	class SphericalJoint : public AtPointJoint
	{
		//
	public:
		SphericalJoint();
		SphericalJoint(const char* str);
		void initializeGlobally() override;

	};
}

