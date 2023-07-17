#pragma once

#include "Joint.h"

namespace MbD {
	class ScrewJoint : public Joint
	{
		//
	public:
		ScrewJoint();
		ScrewJoint(const char* str);
		void initializeGlobally() override;
		void connectsItoJ(EndFrmcptr frmI, EndFrmcptr frmJ) override;

		double pitch, aConstant;
	};
}

