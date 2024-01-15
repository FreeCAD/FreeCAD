/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "LDUSpMatParPvMarko.h"
#include "SingularMatrixError.h"

using namespace MbD;

void LDUSpMatParPvMarko::doPivoting(size_t p)
{
	//"Search from bottom to top."
	//"Check for singular pivot."
	//"Do scaling. Do partial pivoting."
	//"criterion := mag / (2.0d raisedTo: rowiCount)."
	//| lookForFirstNonZeroInPivotCol i rowi aip criterionMax rowPivoti criterion max |
	int i;	//Use int because of decrement
	size_t rowPivoti;
	double aip, mag, max, criterion, criterionMax;
	SpRowDsptr spRowi;
	rowPositionsOfNonZerosInPivotColumn->clear();
	auto lookForFirstNonZeroInPivotCol = true;
	i = (int)m - 1;
	while (lookForFirstNonZeroInPivotCol) {
		spRowi = matrixA->at(i);
		if (spRowi->find(p) == spRowi->end()) {
			if (i <= (int)p) throwSingularMatrixError(""); //Use int because i can be negative
		}
		else {
			markowitzPivotColCount = 0;
			aip = spRowi->at(p);
			mag = aip * rowScalings->at(i);
			if (mag < 0) mag = -mag;
			max = mag;
			criterionMax = mag / std::pow(2.0, spRowi->size());
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
			criterion = mag / std::pow(2.0, spRowi->size());
			if (criterion > criterionMax) {
				max = mag;
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
		rowScalings->swapElems(p, rowPivoti);
		rowOrder->swapElems(p, rowPivoti);
		matrixL->swapElems(p, rowPivoti);
		if (aip != std::numeric_limits<double>::min()) rowPositionsOfNonZerosInPivotColumn->at(markowitzPivotColCount - 1) = rowPivoti;
	}
	if (max < singularPivotTolerance) throwSingularMatrixError("");
}
