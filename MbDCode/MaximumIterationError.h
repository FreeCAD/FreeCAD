/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#pragma once

#include "NewtonRaphsonError.h"

namespace MbD {
	//class MaximumIterationError : public NewtonRaphsonError
	//{
	//public:
	//	//MaximumIterationError() {}
	//	MaximumIterationError(const std::string& msg) : NewtonRaphsonError(msg) {}

	//	virtual ~MaximumIterationError() noexcept {}
	//};
	class MaximumIterationError : virtual public std::runtime_error
	{

	public:
		//MaximumIterationError();
		explicit MaximumIterationError(const std::string& msg);
		virtual ~MaximumIterationError() noexcept {}
	};
}

