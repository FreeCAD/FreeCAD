/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "InPlaneJoint.h"

namespace MbD {
	class PlanarJoint : public InPlaneJoint
	{
		//
	public:
		PlanarJoint();
		PlanarJoint(const char* str);
		void initializeGlobally() override;

	};
}

