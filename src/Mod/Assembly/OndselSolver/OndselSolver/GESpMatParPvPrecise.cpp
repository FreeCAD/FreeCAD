/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <cassert>

#include "GESpMatParPvPrecise.h"
#include "SingularMatrixError.h"
#include "CREATE.h"

using namespace MbD;

void GESpMatParPvPrecise::doPivoting(size_t p)
{
	//"Search from bottom to top."
	//"Use scaling vector and partial pivoting with actual swapping of rows."
	//"Check for singular pivot."
	//"Do scaling. Do partial pivoting."
	//| max rowPivot aip mag lookForFirstNonZeroInPivotCol i |
	ssize_t i;	//Use ssize_t because of decrement
	size_t rowPivoti;
	double aip, mag, max;
	SpRowDsptr spRowi;
	rowPositionsOfNonZerosInPivotColumn->clear();
	auto lookForFirstNonZeroInPivotCol = true;
	i = (ssize_t)m - 1;
	while (lookForFirstNonZeroInPivotCol) {
		spRowi = matrixA->at(i);
		if (spRowi->find(p) == spRowi->end()) {
			if (i <= (ssize_t)p) throwSingularMatrixError("doPivoting");	//Use ssize_t because i can be negative
		}
		else {
			markowitzPivotColCount = 0;
			aip = spRowi->at(p);
			mag = aip * rowScalings->at(i);
			if (mag < 0) mag = -mag;
			max = mag;
			rowPivoti = (size_t)i;
			lookForFirstNonZeroInPivotCol = false;
		}
		i--;
	}
	while (i >= (ssize_t)p) { //Use ssize_t because i can be negative
		spRowi = matrixA->at(i);
		if (spRowi->find(p) == spRowi->end()) {
			aip = std::numeric_limits<double>::min();
		}
		else {
			aip = spRowi->at(p);
			markowitzPivotColCount++;
			mag = aip * rowScalings->at(i);
			if (mag < 0) mag = -mag;
			if (mag > max) {
				max = mag;
				rowPositionsOfNonZerosInPivotColumn->push_back(rowPivoti);
				rowPivoti = (size_t)i;
			}
			else {
				rowPositionsOfNonZerosInPivotColumn->push_back(i);
			}
		}
		i--;
	}
	if (p != rowPivoti) {
		matrixA->swapElems(p, rowPivoti);
		rightHandSideB->swapElems(p, rowPivoti);
		rowScalings->swapElems(p, rowPivoti);
		rowOrder->swapElems(p, rowPivoti);
		if (aip != std::numeric_limits<double>::min()) rowPositionsOfNonZerosInPivotColumn->at(markowitzPivotColCount - 1) = rowPivoti;
	}
	pivotValues->at(p) = max;
	if (max < singularPivotTolerance) throwSingularMatrixError("doPivoting");
}

void GESpMatParPvPrecise::preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	//assert(false);
	//"A conditioned copy of aMatrix is solved."
	if (m != spMat->nrow() || n != spMat->ncol()) {
		m = spMat->nrow();
		n = spMat->ncol();
		matrixA = std::make_shared<SparseMatrix<double>>(m);
		rowScalings = std::make_shared<FullColumn<double>>(m);
		pivotValues = std::make_shared<FullColumn<double>>(m);
		rowOrder = std::make_shared<FullColumn<size_t>>(m);
		rowPositionsOfNonZerosInPivotColumn = std::make_shared<std::vector<size_t>>();
	}
	if (saveOriginal) {
		rightHandSideB = fullCol->copy();
	}
	else {
		rightHandSideB = fullCol;
	}
	for (size_t i = 0; i < m; i++)
	{
		auto& spRowi = spMat->at(i);
		double maxRowMagnitude = spRowi->maxMagnitude();
		if (maxRowMagnitude == 0) throwSingularMatrixError("preSolvewithsaveOriginal");
		rowScalings->at(i) = 1.0 / maxRowMagnitude;
		matrixA->at(i) = spRowi->conditionedWithTol(singularPivotTolerance * maxRowMagnitude);
		rowOrder->at(i) = i;
	}
}

void MbD::GESpMatParPvPrecise::runSpMat()
{
	auto spMat = std::make_shared<SparseMatrix<double>>(3, 3);
	spMat->atijput(0, 0, 1.0);
	spMat->atijput(0, 1, 1.0);
	spMat->atijput(1, 0, 1.0);
	spMat->atijput(1, 1, 1.0);
	spMat->atijput(1, 2, 1.0);
	spMat->atijput(2, 1, 1.0);
	spMat->atijput(2, 2, 1.0);
	auto fullCol = std::make_shared<FullColumn<double>>(3);
	fullCol->atiput(0, 1.0);
	fullCol->atiput(1, 2.0);
	fullCol->atiput(2, 3.0);
	auto matSolver = CREATE<GESpMatParPvPrecise>::With();
	auto answer = matSolver->solvewithsaveOriginal(spMat, fullCol, true);
	auto aAx = spMat->timesFullColumn(answer);
}
