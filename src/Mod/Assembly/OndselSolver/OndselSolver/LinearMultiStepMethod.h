/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once

#include "DifferenceOperator.h"

namespace MbD {
	class LinearMultiStepMethod : public DifferenceOperator
	{
		//
	public:
		FColDsptr derivativeatpresentpast(size_t n, double t, FColDsptr y, std::shared_ptr<std::vector<FColDsptr>> ypast);
		virtual FColDsptr derivativepresentpast(size_t order, FColDsptr y, std::shared_ptr<std::vector<FColDsptr>> ypast);
		virtual double pvdotpv();
		double firstPastTimeNode();
		virtual FColDsptr derivativepresentpastpresentDerivativepastDerivative(size_t n,
			FColDsptr y, std::shared_ptr<std::vector<FColDsptr>> ypast,
			FColDsptr ydot, std::shared_ptr<std::vector<FColDsptr>> ydotpast);

	};
}

