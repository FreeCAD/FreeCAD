/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <cassert>
#include <memory>

#include "GESpMatParPvMarkoFast.h"
#include "SingularMatrixError.h"

using namespace MbD;

void GESpMatParPvMarkoFast::preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	//assert(false);
	//"Optimized for speed."
	if (m != spMat->nrow() || n != spMat->ncol()) {
		m = spMat->nrow();
		n = spMat->ncol();
		matrixA = std::make_shared<SparseMatrix<double>>(m);
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
		if (maxRowMagnitude == 0) throwSingularMatrixError("");
		auto scaling = 1.0 / maxRowMagnitude;
		matrixA->at(i) = spRowi->timesconditionedWithTol(scaling, singularPivotTolerance);
		rightHandSideB->atitimes(i, scaling);
	}
}

void GESpMatParPvMarkoFast::doPivoting(size_t p)
{
	//"Search from bottom to top."
	//"Optimized for speed. No check for singular pivot."
	//"Do partial pivoting."
	//"criterion := mag / (2.0d raisedTo: rowiCount)."
	//"Rows are explicitly scaled in preSolve:."
	//"Pivot size are nieither checked nor stored."

	//| lookForFirstNonZeroInPivotCol i rowi aip criterionMax rowPivoti criterion max |
	ssize_t i;	//Use ssize_t because of decrement
	size_t rowPivoti;
	double aip, max, criterion, criterionMax;
	SpRowDsptr spRowi;
	rowPositionsOfNonZerosInPivotColumn->clear();
	auto lookForFirstNonZeroInPivotCol = true;
	i = (ssize_t)m - 1;
	while (lookForFirstNonZeroInPivotCol) {
		spRowi = matrixA->at(i);
		if (spRowi->find(p) == spRowi->end()) {
			if (i <= (ssize_t)p) throwSingularMatrixError("");	//Use ssize_t because i can be negative
		}
		else {
			markowitzPivotColCount = 0;
			aip = spRowi->at(p);
			if (aip < 0) aip = -aip;
			max = aip;
			criterionMax = aip / std::pow(2.0, spRowi->size());
			rowPivoti = (size_t)i;
			lookForFirstNonZeroInPivotCol = false;
		}
		i--;
	}
	while (i >= (ssize_t)p) {	//Use ssize_t because i can be negative
		spRowi = matrixA->at(i);
		if (spRowi->find(p) == spRowi->end()) {
			aip = std::numeric_limits<double>::min();
		}
		else {
			aip = spRowi->at(p);
			markowitzPivotColCount++;
			if (aip < 0) aip = -aip;
			criterion = aip / std::pow(2.0, spRowi->size());
			if (criterion > criterionMax) {
				max = aip;
				criterionMax = criterion;
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
		if (aip != std::numeric_limits<double>::min()) rowPositionsOfNonZerosInPivotColumn->at(markowitzPivotColCount - 1) = rowPivoti;
	}
	if (max < singularPivotTolerance) throwSingularMatrixError("");
}
