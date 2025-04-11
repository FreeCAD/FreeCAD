/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "AtPointJoint.h"

namespace MbD {
	class SphericalJoint : public AtPointJoint
	{
		//
	public:
		SphericalJoint();
		SphericalJoint(const std::string& str);
		void initializeGlobally() override;

	};
}

