/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include <stdexcept>
#include <memory>
#include <vector>

namespace MbD {
	class SimulationStoppingError : virtual public std::runtime_error
	{

	public:
		//SimulationStoppingError();
		explicit SimulationStoppingError(const std::string& msg);
		virtual ~SimulationStoppingError() noexcept {}
	};
}
