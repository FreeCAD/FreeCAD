/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <cmath>

#include "DifferenceOperator.h"
#include "CREATE.h"
#include "SingularMatrixError.h"
#include "LDUFullMatParPv.h"
#include "FullRow.h"

using namespace MbD;

FRowDsptr DifferenceOperator::OneOverFactorials = []() {
	auto oneOverFactorials = std::make_shared<FullRow<double>>(10);
	for (size_t i = 0; i < oneOverFactorials->size(); i++)
	{
		oneOverFactorials->at(i) = 1.0 / std::tgamma(i + 1);
	}
	return oneOverFactorials;
}();

void DifferenceOperator::calcOperatorMatrix()
{
	//Compute operatorMatrix such that 
	//value(time) : = (operatorMatrix at : 1) timesColumn : series.
	//valuedot(time) : = (operatorMatrix at : 2) timesColumn : series.
	//valueddot(time) : = (operatorMatrix at : 3) timesColumn : series.

	this->formTaylorMatrix();
	try {
		operatorMatrix = CREATE<LDUFullMatParPv>::With()->inversesaveOriginal(taylorMatrix, false);
	}
	catch (const SingularMatrixError& ex) {
	}
}

void DifferenceOperator::initialize()
{
	//Do nothing
}

void MbD::DifferenceOperator::initializeLocally()
{
	assert(false);
}

void DifferenceOperator::setiStep(size_t i)
{
	iStep = i;
}

void DifferenceOperator::setorder(size_t o)
{
	order = o;
}

void DifferenceOperator::instantiateTaylorMatrix()
{
	if (taylorMatrix == nullptr || (taylorMatrix->nrow() != (order + 1))) {
		taylorMatrix = std::make_shared<FullMatrix<double>>(order + 1, order + 1);
	}
}

void DifferenceOperator::formTaylorRowwithTimeNodederivative(size_t i, size_t ii, size_t k)
{
	//| rowi hi hipower aij |
	auto& rowi = taylorMatrix->at(i);
	for (size_t j = 0; j < k; j++)
	{
		rowi->at(j) = 0.0;
	}
	rowi->at(k) = 1.0;
	auto hi = timeNodes->at(ii) - time;
	auto hipower = 1.0;
	for (size_t j = k + 1; j < order + 1; j++)
	{
		hipower = hipower * hi;
		auto aij = hipower * OneOverFactorials->at(j - k);
		rowi->atiput(j, aij);
	}
}

void DifferenceOperator::settime(double t)
{
	time = t;
}
