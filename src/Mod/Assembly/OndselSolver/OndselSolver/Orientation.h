/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "PrescribedMotion.h"

namespace MbD {
	class Orientation : public PrescribedMotion
	{
		//
	public:
		Orientation();
		Orientation(const std::string& str);
		void initializeGlobally() override;

	};
}

