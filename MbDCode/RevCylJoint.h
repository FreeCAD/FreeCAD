#pragma once

#include "CompoundJoint.h"

namespace MbD {
	class RevCylJoint : public CompoundJoint
	{
		//
	public:
		RevCylJoint();
		RevCylJoint(const char* str);
		void initializeGlobally() override;
	

	};
}

