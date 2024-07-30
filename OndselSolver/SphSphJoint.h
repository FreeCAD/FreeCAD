/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "CompoundJoint.h"

namespace MbD {
	class SphSphJoint : public CompoundJoint
	{
		//
	public:
		SphSphJoint();
		SphSphJoint(const std::string& str);
		void initializeGlobally() override;

	};
}

