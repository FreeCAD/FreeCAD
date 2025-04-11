/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <cassert>

#include "GEFullMatParPv.h"
#include "SingularMatrixError.h"

using namespace MbD;

void GEFullMatParPv::doPivoting(size_t p)
{
	//"Use scalings. Do row pivoting."

		//| app max rowPivot aip mag |
	auto app = matrixA->at(p)->at(p);
	double max = app * rowScalings->at(p);
	if (max < 0.0) max = -max;
	auto rowPivot = p;
	for (size_t i = p + 1; i < m; i++)
	{
		auto aip = matrixA->at(i)->at(p);
		if (aip != 0.0) {
			auto mag = aip * rowScalings->at(i);
			if (mag < 0) mag = -mag;
			if (max < mag) {
				max = mag;
				rowPivot = i;
			}
		}
	}
	if (p != rowPivot) {
		matrixA->swapElems(p, rowPivot);
		rightHandSideB->swapElems(p, rowPivot);
		rowScalings->swapElems(p, rowPivot);
		rowOrder->swapElems(p, rowPivot);
	}
	pivotValues->at(p) = max;
	if (max < singularPivotTolerance) throwSingularMatrixError("");
}

void GEFullMatParPv::postSolve()
{
	assert(false);
}

void GEFullMatParPv::preSolvewithsaveOriginal(FMatDsptr, FColDsptr, bool)
{
	assert(false);
}
