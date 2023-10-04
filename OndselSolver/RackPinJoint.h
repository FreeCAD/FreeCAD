/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
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
		void connectsItoJ(EndFrmsptr frmI, EndFrmsptr frmJ) override;

		double pitchRadius, aConstant;
	};
}

