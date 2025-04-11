/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "LDUSpMatParPvPrecise.h"
#include "SingularMatrixError.h"

using namespace MbD;

void LDUSpMatParPvPrecise::doPivoting(size_t p)
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
			if (i <= (ssize_t)p) throwSingularMatrixError(""); //Use ssize_t because i can be negative
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
		rowScalings->swapElems(p, rowPivoti);
		rowOrder->swapElems(p, rowPivoti);
		matrixL->swapElems(p, rowPivoti);
		if (aip != std::numeric_limits<double>::min()) rowPositionsOfNonZerosInPivotColumn->at(markowitzPivotColCount - 1) = rowPivoti;
	}
	pivotValues->at(p) = max;
	if (max < singularPivotTolerance) throwSingularMatrixError("");
}
