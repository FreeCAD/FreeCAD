#pragma once

#include "AtPointJoint.h"

namespace MbD {
	class UniversalJoint : public AtPointJoint
	{
		//
	public:
		UniversalJoint();
		UniversalJoint(const char* str);
		void initializeGlobally() override;

	};
}

