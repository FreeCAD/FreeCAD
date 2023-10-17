/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once
#include "MBDynItem.h"

namespace MbD {

	class MBDynReference : public MBDynItem
	{
	public:
		void initialize() override;
		void parseMBDyn(std::string line);
		void readVelocity(std::vector<std::string>& args);
		void readOmega(std::vector<std::string>& args);

		std::string refString, name;
		FColDsptr rOfO, vOfO, omeOfO;
		FMatDsptr aAOf;
	};
}
