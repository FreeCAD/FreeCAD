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
	class SyntaxError : virtual public std::runtime_error
	{

	public:
		//SyntaxError();
		explicit SyntaxError(const std::string& msg);
		virtual ~SyntaxError() noexcept {}
	};
}
