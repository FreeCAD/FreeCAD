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
	class LineInPlaneJoint : public InPlaneJoint
	{
		//
	public:
		LineInPlaneJoint();
		LineInPlaneJoint(const std::string& str);
		void initializeGlobally() override;

	};
}

