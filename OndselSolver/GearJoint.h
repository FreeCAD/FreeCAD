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
	class GearJoint : public Joint
	{
		//radiusI radiusJ aConstant 
	public:
		GearJoint();
		GearJoint(const char* str);
		void initializeGlobally() override;

		double radiusI, radiusJ, aConstant;
	};
}

