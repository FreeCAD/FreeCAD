/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <cassert>

#include "GESpMatParPv.h"

using namespace MbD;

void GESpMatParPv::forwardEliminateWithPivot(size_t p)
{
	//"rightHandSideB may be multidimensional."

	auto& rowp = matrixA->at(p);
	auto app = rowp->at(p);
	auto elementsInPivotRow = std::make_shared<std::vector<const std::pair<const size_t, double>*>>(rowp->size() - 1);
	size_t index = 0;
	for (auto const& keyValue : *rowp) {
		if (keyValue.first != p) {
			elementsInPivotRow->at(index) = (&keyValue);
			index++;
		}
	}
	auto bp = rightHandSideB->at(p);
	for (size_t ii = 0; ii < markowitzPivotColCount; ii++)
	{
		auto i = rowPositionsOfNonZerosInPivotColumn->at(ii);
		auto& rowi = matrixA->at(i);
		auto aip = rowi->at(p);
		rowi->erase(p);
		auto factor = aip / app;
		for (auto keyValue : *elementsInPivotRow)
		{
			auto j = keyValue->first;
			auto apj = keyValue->second;
			(*rowi)[j] -= factor * apj;
		}
		rightHandSideB->at(i) -= bp * factor;
	}
}

void GESpMatParPv::backSubstituteIntoDU()
{
	//"DU is upper triangular with nonzero diagonals."

	double sum, duij, duii{};
	//answerX = rightHandSideB->copyEmpty();
	assert(m == n);
	answerX = std::make_shared<FullColumn<double>>(m);
	answerX->at(n - 1) = rightHandSideB->at(m - 1) / matrixA->at(m - 1)->at(n - 1);
	//auto rhsZeroElement = this->rhsZeroElement();
	for (ssize_t i = (ssize_t)n - 2; i >= 0; i--)	//Use ssize_t because of decrement
	{
		auto rowi = matrixA->at(i);
		sum = 0.0; // rhsZeroElement copy.
		for (auto const& keyValue : *rowi) {
			auto j = keyValue.first;
			if ((ssize_t)j > i) {
				duij = keyValue.second;
				sum += answerX->at(j) * duij;
			}
			else {
				duii = keyValue.second;
			}
		}
		answerX->at(i) = (rightHandSideB->at(i) - sum) / duii;
	}
}

void GESpMatParPv::postSolve()
{
}
