/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <cassert>

#include "GEFullMatFullPv.h"
#include "SingularMatrixError.h"

using namespace MbD;

void GEFullMatFullPv::doPivoting(size_t p)
{
	//"Do full pivoting."

		//| max pivotRow pivotCol rowi aij mag |
	double max = 0.0;
	size_t pivotRow = p;
	size_t pivotCol = p;
	for (size_t i = p; i < m; i++)
	{
		auto rowi = matrixA->at(i);
		for (size_t j = p; j < n; j++)
		{
			auto aij = rowi->at(j);
			if (aij != 0.0) {
				auto mag = aij;
				if (mag < 0.0) mag = -mag;
				if (max < mag) {
					max = mag;
					pivotRow = i;
					pivotCol = j;
				}
			}
		}
	}
	if (p != pivotRow) {
		matrixA->swapElems(p, pivotRow);
		rightHandSideB->swapElems(p, pivotRow);
		rowOrder->swapElems(p, pivotRow);
	}
	if (p != pivotCol) {
		for (auto& rowi : *matrixA) {
			rowi->swapElems(p, pivotCol);
		}
		colOrder->swapElems(p, pivotCol);
	}
	pivotValues->at(p) = max;
	if (max < singularPivotTolerance) throwSingularMatrixError("");
}

void GEFullMatFullPv::postSolve()
{
	assert(false);
}
