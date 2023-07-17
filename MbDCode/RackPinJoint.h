#pragma once

#include "Joint.h"

namespace MbD {
	class RackPinJoint : public Joint
	{
		//pitchRadius aConstant 
	public:
		RackPinJoint();
		RackPinJoint(const char* str);
		void initializeGlobally() override;
		void connectsItoJ(EndFrmcptr frmI, EndFrmcptr frmJ) override;

		double pitchRadius, aConstant;
	};
}

