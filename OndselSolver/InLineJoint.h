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
	class InLineJoint : public Joint
	{
		//
	public:
		InLineJoint();
		InLineJoint(const char* str);

		void createInLineConstraints();

	};
}

