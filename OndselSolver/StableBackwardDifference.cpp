/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "StableBackwardDifference.h"
#include "FullColumn.h"

using namespace MbD;

void StableBackwardDifference::formTaylorMatrix()
{
	//This form is numerically more stable and is preferred over the full Taylor Matrix.
	//For method order 3:
	//| (t1 - t)	(t1 - t) ^ 2 / 2!	(t1 - t) ^ 3 / 3!|	|qd(t)  | = | q(t1) - q(t)	|
	//|	(t2 - t)	(t2 - t) ^ 2 / 2!	(t2 - t) ^ 3 / 3!|	|qdd(t)	|	|q(t2) - q(t)	|
	//|	(t3 - t)	(t3 - t) ^ 2 / 2!	(t3 - t) ^ 3 / 3!|	|qddd(t)|	|q(t3) - q(t)	|

	this->instantiateTaylorMatrix();
	for (int i = 0; i < order; i++)
	{
		this->formTaylorRowwithTimeNodederivative(i, i, 0);
	}
}

double MbD::StableBackwardDifference::pvdotpv()
{
	//"pvdotpv = operatorMatrix timesColumn: #(-1.0d ... -1.0d)."

	auto& coeffs = operatorMatrix->at(0);
	auto sum = 0.0;
	for (int i = 0; i < order; i++)
	{
		sum -= coeffs->at(i);
	}
	return sum;
}

FColDsptr MbD::StableBackwardDifference::derivativepresentpastpresentDerivativepastDerivative(int n, FColDsptr y, std::shared_ptr<std::vector<FColDsptr>> ypast, FColDsptr ydot, std::shared_ptr<std::vector<FColDsptr>> ydotpast)
{
	assert(false);
	return FColDsptr();
}

void StableBackwardDifference::instantiateTaylorMatrix()
{
	if (taylorMatrix == nullptr || (taylorMatrix->nrow() != (order))) {
		taylorMatrix = std::make_shared<FullMatrixDouble>(order, order);
	}
}

void StableBackwardDifference::formTaylorRowwithTimeNodederivative(int i, int ii, int k)
{
	//| rowi hi hipower aij |
	auto& rowi = taylorMatrix->at(i);
	if (k > 0) {
		for (int j = 0; j < k - 2; j++)
		{
			rowi->at(j) = 0.0;
		}
		rowi->at((int)k - 1) = 1.0;
	}

	auto hi = timeNodes->at(ii) - time;
	auto hipower = 1.0;
	for (int j = k; j < order; j++)
	{
		hipower *= hi;
		auto aij = hipower * OneOverFactorials->at((int)(j - k + 1));
		rowi->at(j) = aij;
	}
}

FColDsptr MbD::StableBackwardDifference::derivativepresentpast(int deriv, FColDsptr y, std::shared_ptr<std::vector<FColDsptr>> ypast)
{
	//"Answer ith derivative given present value and past values."

	if (deriv == 0) {
		return y->cloneFcSptr();
	}
	else {
		if (deriv <= order) {
			auto series = std::make_shared<std::vector<FColDsptr>>(order);
			for (int i = 0; i < order; i++)
			{
				series->at(i) = ypast->at(i)->minusFullColumn(y);
			}
			auto& coeffs = operatorMatrix->at((int)deriv - 1);
			auto answer = coeffs->dot(series);
			return std::static_pointer_cast<FullColumn<double>>(answer);
		}
		else {
			return std::make_shared<FullColumn<double>>(y->size(), 0.0);
		}
	}
}
