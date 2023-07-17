#pragma once

#include "AtPointJoint.h"

namespace MbD {
	class FixedJoint : public AtPointJoint
	{
		//
	public:
		FixedJoint();
		FixedJoint(const char* str);
		void initializeGlobally() override;


	};
}
