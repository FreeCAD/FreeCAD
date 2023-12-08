/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "LinearMultiStepMethod.h"
#include "FullColumn.h"

using namespace MbD;

FColDsptr MbD::LinearMultiStepMethod::derivativeatpresentpast(int n, double t, FColDsptr y, std::shared_ptr<std::vector<FColDsptr>> ypast)
{
	//"Interpolate or extrapolate."
	//"dfdt(t) = df0dt + d2f0dt2*(t - t0) + d3f0dt3*(t - t0)^2 / 2! + ..."

	auto answer = this->derivativepresentpast(n, y, ypast);
	if (t != time) {
		auto dt = t - time;
		auto dtpower = 1.0;
		for (int i = n + 1; i <= order; i++)
		{
			auto diydti = this->derivativepresentpast(i, y, ypast);
			dtpower = dtpower * dt;
			answer->equalSelfPlusFullColumntimes(diydti, dtpower * (OneOverFactorials->at((int)i - n)));
		}
	}
	return answer;
}

FColDsptr MbD::LinearMultiStepMethod::derivativepresentpast(int, FColDsptr, std::shared_ptr<std::vector<FColDsptr>>)
{
	assert(false);
	return FColDsptr();
}

double MbD::LinearMultiStepMethod::pvdotpv()
{
	assert(false);
	return 0.0;
}

double MbD::LinearMultiStepMethod::firstPastTimeNode()
{
	return timeNodes->at(0);
}

FColDsptr MbD::LinearMultiStepMethod::derivativepresentpastpresentDerivativepastDerivative(int,
	FColDsptr, std::shared_ptr<std::vector<FColDsptr>>,
	FColDsptr, std::shared_ptr<std::vector<FColDsptr>>)
{
	assert(false);
	return FColDsptr();
}
