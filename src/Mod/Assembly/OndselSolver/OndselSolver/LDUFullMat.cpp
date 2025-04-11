/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "LDUFullMat.h"

using namespace MbD;

FColDsptr LDUFullMat::basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	this->decomposesaveOriginal(fullMat, saveOriginal);
	FColDsptr answer = this->forAndBackSubsaveOriginal(fullCol, saveOriginal);
	return answer;
}

FColDsptr LDUFullMat::basicSolvewithsaveOriginal(SpMatDsptr, FColDsptr, bool)
{
	assert(false);
	return FColDsptr();
}

void LDUFullMat::preSolvewithsaveOriginal(FMatDsptr, FColDsptr, bool)
{
	assert(false);
}

void LDUFullMat::preSolvewithsaveOriginal(SpMatDsptr, FColDsptr, bool)
{
	assert(false);
}

void LDUFullMat::forwardEliminateWithPivot(size_t p)
{
	//"Save factors in lower triangle for LU decomposition."

		//| rowp app rowi aip factor |
	auto& rowp = matrixA->at(p);
	auto app = rowp->at(p);
	for (size_t i = p + 1; i < m; i++)
	{
		auto& rowi = matrixA->at(i);
		auto aip = rowi->at(p);
		auto factor = aip / app;
		rowi->at(p) = factor;
		if (factor != 0) {
			for (size_t j = p + 1; j < n; j++)
			{
				rowi->atiminusNumber(j, factor * rowp->at(j));
			}
		}
	}
}

void LDUFullMat::postSolve()
{
	assert(false);
}

void LDUFullMat::preSolvesaveOriginal(FMatDsptr fullMat, bool saveOriginal)
{
	if (saveOriginal) {
		matrixA = fullMat->copy();
	}
	else {
		matrixA = fullMat;
	}
	if (m != matrixA->nrow() || n != matrixA->ncol()) {
		m = matrixA->nrow();
		n = matrixA->ncol();
		pivotValues = std::make_shared<FullColumn<double>>(m);
		rowOrder = std::make_shared<FullColumn<size_t>>(m);
		colOrder = std::make_shared<FullRow<size_t>>(n);
	}
	if (m == n) {
		for (size_t i = 0; i < m; i++)
		{
			rowOrder->at(i) = i;
			colOrder->at(i) = i;
		}
	}
	else {
		for (size_t i = 0; i < m; i++)
		{
			rowOrder->at(i) = i;
		}
		for (size_t j = 0; j < n; j++)
		{
			colOrder->at(j) = j;
		}
	}
	this->findScalingsForRowRange(0, m);
}

void LDUFullMat::decomposesaveOriginal(FMatDsptr fullMat, bool saveOriginal)
{
	this->preSolvesaveOriginal(fullMat, saveOriginal);
	for (size_t p = 0; p < m; p++)
	{
		this->doPivoting(p);
		this->forwardEliminateWithPivot(p);
	}
}

void LDUFullMat::decomposesaveOriginal(SpMatDsptr, bool)
{
	assert(false);
}

FMatDsptr LDUFullMat::inversesaveOriginal(FMatDsptr fullMat, bool saveOriginal)
{
	//"ForAndBackSub be optimized for the identity matrix."

	this->decomposesaveOriginal(fullMat, saveOriginal);
	rightHandSideB = std::make_shared<FullColumn<double>>(m);
	auto matrixAinverse = std::make_shared <FullMatrix<double>>(m, n);
	for (size_t j = 0; j < n; j++)
	{
		rightHandSideB->zeroSelf();
		rightHandSideB->at(j) = 1.0;
		this->forAndBackSubsaveOriginal(rightHandSideB, saveOriginal);
		matrixAinverse->atijputFullColumn(0, j, answerX);
	}
	return matrixAinverse;
}

double LDUFullMat::getmatrixArowimaxMagnitude(size_t i)
{
	return matrixA->at(i)->maxMagnitude();
}

void LDUFullMat::forwardSubstituteIntoL()
{
	//"L is lower triangular with nonzero and ones in diagonal."
	auto vectorc = std::make_shared<FullColumn<double>>(n);
	for (size_t i = 0; i < n; i++)
	{
		auto& rowi = matrixA->at(i);
		double sum = 0.0;
		for (size_t j = 0; j < i; j++)
		{
			sum += rowi->at(j) * vectorc->at(j);
		}
		vectorc->at(i) = rightHandSideB->at(i) - sum;
	}
	rightHandSideB = vectorc;
}

void LDUFullMat::backSubstituteIntoDU()
{
	//"DU is upper triangular with nonzero and arbitrary diagonals."

		//| rowi sum |
	answerX = std::make_shared<FullColumn<double>>(n);
	answerX->at(n - 1) = rightHandSideB->at(m - 1) / matrixA->at(m - 1)->at(n - 1);
	for (ssize_t i = (ssize_t)n - 2; i >= 0; i--)	//Use ssize_t because of decrement
	{
		auto& rowi = matrixA->at(i);
		double sum = answerX->at(n - 1) * rowi->at(n - 1);
		for (size_t j = i + 1; j < n - 1; j++)
		{
			sum += answerX->at(j) * rowi->at(j);
		}
		answerX->at(i) = (rightHandSideB->at(i) - sum) / rowi->at(i);
	}
}
