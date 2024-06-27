/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "MatrixDecomposition.h"

using namespace MbD;

FColDsptr MbD::MatrixDecomposition::forAndBackSubsaveOriginal(FColDsptr, bool)
{
	assert(false);
	return FColDsptr();
}

void MatrixDecomposition::applyRowOrderOnRightHandSideB()
{
	FColDsptr answer = std::make_shared<FullColumn<double>>(m);
	for (size_t i = 0; i < m; i++)
	{
		answer->at(i) = rightHandSideB->at(rowOrder->at(i));
	}
	rightHandSideB = answer;
}

FColDsptr MbD::MatrixDecomposition::basicSolvewithsaveOriginal(FMatDsptr, FColDsptr, bool)
{
	assert(false);
	return FColDsptr();
}

void MbD::MatrixDecomposition::forwardSubstituteIntoL()
{
	assert(false);
}

void MbD::MatrixDecomposition::backSubstituteIntoU()
{
	assert(false);
}

void MbD::MatrixDecomposition::forwardSubstituteIntoLD()
{
	assert(false);
}

void MbD::MatrixDecomposition::postSolve()
{
	assert(false);
}

void MbD::MatrixDecomposition::preSolvesaveOriginal(FMatDsptr, bool)
{
	assert(false);
}
