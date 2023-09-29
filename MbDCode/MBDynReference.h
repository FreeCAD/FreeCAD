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
		void readPosition(std::istringstream& iss);
		void readOrientation(std::istringstream& iss);
		void readVelocity(std::istringstream& iss);
		void readOmega(std::istringstream& iss);
		void readPosition(std::shared_ptr<std::vector<std::string>>& args);
		void readOrientation(std::shared_ptr<std::vector<std::string>>& args);
		void readVelocity(std::shared_ptr<std::vector<std::string>>& args);
		void readOmega(std::shared_ptr<std::vector<std::string>>& args);

		std::string refString, name;
		FColDsptr rOfO, vOfO, omeOfO;
		FMatDsptr aAOf;
	};
}
