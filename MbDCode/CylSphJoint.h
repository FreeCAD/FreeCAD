#pragma once

#include "CompoundJoint.h"

namespace MbD {
	class CylSphJoint : public CompoundJoint
	{
		//
	public:
		CylSphJoint();
		CylSphJoint(const char* str);
		void initializeGlobally() override;


	};
}
