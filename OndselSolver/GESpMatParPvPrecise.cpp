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

using namespace MbD;

void GESpMatParPvPrecise::doPivoting(size_t p)
{
	//"Search from bottom to top."
	//"Use scaling vector and partial pivoting with actual swapping of rows."
	//"Check for singular pivot."
	//"Do scaling. Do partial pivoting."
	//| max rowPivot aip mag lookForFirstNonZeroInPivotCol i |
	int i;	//Use int because of decrement
	size_t rowPivoti;
	double aip, mag, max;
	SpRowDsptr spRowi;
	rowPositionsOfNonZerosInPivotColumn->clear();
	auto lookForFirstNonZeroInPivotCol = true;
	i = (int)m - 1;
	while (lookForFirstNonZeroInPivotCol) {
		spRowi = matrixA->at(i);
		if (spRowi->find(p) == spRowi->end()) {
			if (i <= (int)p) throwSingularMatrixError("doPivoting");	//Use int because i can be negative
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
	while (i >= (int)p) { //Use int because i can be negative
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
