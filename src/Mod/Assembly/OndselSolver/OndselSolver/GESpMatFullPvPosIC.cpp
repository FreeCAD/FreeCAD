/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <algorithm>
#include <cassert>
#include <cstdint>

#include "GESpMatFullPvPosIC.h"
#include "SingularMatrixError.h"
#include "PosICNewtonRaphson.h"

using namespace MbD;

void GESpMatFullPvPosIC::preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	GESpMatFullPv::preSolvewithsaveOriginal(spMat, fullCol, saveOriginal);
	if (system == nullptr) {
		pivotRowLimits = std::make_shared<std::vector<size_t>>();
	}
	else {
		pivotRowLimits = system->pivotRowLimits;
	}
	pivotRowLimit = SIZE_MAX;
}

void GESpMatFullPvPosIC::doPivoting(size_t p)
{
	//"Used by Gauss Elimination only."
	//"Swap rows but keep columns in place."
	//"The elements below the diagonal are removed column by column."

	double max = 0.0;
	auto pivotRow = p;
	auto pivotCol = p;
	for (size_t j = p; j < n; j++)
	{
		rowPositionsOfNonZerosInColumns->at(colOrder->at(j))->clear();
	}
	if (pivotRowLimit == SIZE_MAX || p >= pivotRowLimit) {
		pivotRowLimit = *std::find_if(
			pivotRowLimits->begin(), pivotRowLimits->end(),
			[&](size_t limit) { return limit > p; });
	}
	for (size_t i = p; i < pivotRowLimit; i++)
	{
		auto& rowi = matrixA->at(i);
		for (auto const& kv : *rowi) {
			rowPositionsOfNonZerosInColumns->at(kv.first)->push_back(i);
			auto aij = kv.second;
			auto mag = aij;
			if (mag < 0.0) mag = -mag;
			if (max < mag) {
				max = mag;
				pivotRow = i;
				pivotCol = positionsOfOriginalCols->at(kv.first);
			}
		}
	}
	if (p != pivotRow) {
		matrixA->swapElems(p, pivotRow);
		rightHandSideB->swapElems(p, pivotRow);
		rowOrder->swapElems(p, pivotRow);
	}
	if (p != pivotCol) {
		colOrder->swapElems(p, pivotCol);
		positionsOfOriginalCols->at(colOrder->at(p)) = p;
		positionsOfOriginalCols->at(colOrder->at(pivotCol)) = pivotCol;
	}
	pivotValues->at(p) = max;
	if (max < singularPivotTolerance) {
		auto itr = std::find_if(
			pivotRowLimits->begin(), pivotRowLimits->end(),
			[&](size_t limit) { return limit > pivotRowLimit; });
		if (itr == pivotRowLimits->end()) {
			auto begin = rowOrder->begin() + p;
			auto end = rowOrder->begin() + pivotRowLimit;
			auto redundantEqnNos = std::make_shared<FullColumn<size_t>>(begin, end);
			throwSingularMatrixError("", redundantEqnNos);
		}
		else {
			pivotRowLimit = *itr;
		}
		return this->doPivoting(p);
	}
	auto jp = colOrder->at(p);
	rowPositionsOfNonZerosInPivotColumn = rowPositionsOfNonZerosInColumns->at(jp);
	for (size_t i = pivotRowLimit; i < m; i++)
	{
		auto& spRowi = matrixA->at(i);
		if (spRowi->find(jp) != spRowi->end()) {
			rowPositionsOfNonZerosInPivotColumn->push_back(i);
		}
	}
	if (rowPositionsOfNonZerosInPivotColumn->front() == p) {
		rowPositionsOfNonZerosInPivotColumn->erase(rowPositionsOfNonZerosInPivotColumn->begin());
	}
	markowitzPivotColCount = rowPositionsOfNonZerosInPivotColumn->size();
}