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
	class CompoundJoint : public Joint
	{
		//distanceIJ
	public:
		CompoundJoint();
		CompoundJoint(const char* str);

		double distanceIJ = 0.0;
	};
}


