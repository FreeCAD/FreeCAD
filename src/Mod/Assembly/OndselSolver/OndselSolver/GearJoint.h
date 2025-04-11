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
		GearJoint(const std::string& str);

		//void initializeLocally() override;
		void initializeGlobally() override;

		double radiusI = 0.0, radiusJ = 0.0, aConstant = 0.0;
	};
}

