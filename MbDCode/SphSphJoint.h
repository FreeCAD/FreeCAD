#pragma once

#include "CompoundJoint.h"

namespace MbD {
	class SphSphJoint : public CompoundJoint
	{
		//
	public:
		SphSphJoint();
		SphSphJoint(const char* str);
		void initializeGlobally() override;

	};
}

