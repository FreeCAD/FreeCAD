/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <cassert>

#include "GESpMatFullPv.h"
#include "SingularMatrixError.h"

using namespace MbD;

void GESpMatFullPv::doPivoting(size_t p)
{
	//"Used by Gauss Elimination only."
	//"Do full pivoting."
	//"Swap rows but keep columns in place."
	//"The elements below the diagonal are removed column by column."

	double max = 0.0;
	auto pivotRow = p;
	auto pivotCol = p;
	for (size_t j = p; j < n; j++)
	{
		rowPositionsOfNonZerosInColumns->at(colOrder->at(j))->clear();
	}
	for (size_t i = p; i < m; i++)
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
	if (max < singularPivotTolerance) throwSingularMatrixError("");
	auto jp = colOrder->at(p);
	rowPositionsOfNonZerosInPivotColumn = rowPositionsOfNonZerosInColumns->at(jp);
	if (rowPositionsOfNonZerosInPivotColumn->front() == p) {
		rowPositionsOfNonZerosInPivotColumn->erase(rowPositionsOfNonZerosInPivotColumn->begin());
	}
	markowitzPivotColCount = rowPositionsOfNonZerosInPivotColumn->size();
}
void GESpMatFullPv::forwardEliminateWithPivot(size_t p)
{
	//app is pivot.
	//i > p, j > p
	//aip is eliminated.
	//apj can cause fill - in.
	//Columns are in place.
	//"rightHandSideB may be multidimensional."

	auto jp = colOrder->at(p);
	auto& rowp = matrixA->at(p);
	auto app = rowp->at(jp);
	auto elementsInPivotRow = std::make_shared<std::vector<const std::pair<const size_t, double>*>>(rowp->size() - 1);
	size_t index = 0;
	for (auto const& keyValue : *rowp) {
		if (keyValue.first != jp) {
			elementsInPivotRow->at(index) = (&keyValue);
			index++;
		}
	}
	auto bp = rightHandSideB->at(p);
	for (size_t ii = 0; ii < markowitzPivotColCount; ii++)
	{
		auto i = rowPositionsOfNonZerosInPivotColumn->at(ii);
		auto& spRowi = matrixA->at(i);
		if (spRowi->find(jp) == spRowi->end()) continue;
		auto aip = spRowi->at(jp);
		spRowi->erase(jp);
		auto factor = aip / app;
		for (auto keyValue : *elementsInPivotRow)
		{
			auto j = keyValue->first;
			auto apj = keyValue->second;
			(*spRowi)[j] -= factor * apj;
		}
		rightHandSideB->at(i) -= bp * factor;
	}
}

void GESpMatFullPv::backSubstituteIntoDU()
{
	//"Use colOrder to get DU in upper triangular with nonzero diagonals."
	//"Formula given by Eqn. 9.26 and 9.27 in Chapra's text 2nd Edition."

	double sum, duij, duii{};
	//answerX = rightHandSideB->copyEmpty();
	assert(m == n);

    // TODO: temp
//    assert(n > 0);
//    auto localLen = colOrder->numberOfElements();
//    assert(n < localLen);

	answerX = std::make_shared<FullColumn<double>>(m);
	auto jn = colOrder->at(n);
	answerX->at(jn) = rightHandSideB->at(m) / matrixA->at(m)->at(jn);
	//auto rhsZeroElement = this->rhsZeroElement();
	for (ssize_t i = (ssize_t)n - 2; i >= 0; i--)	//Use ssize_t because of decrement
	{
		auto& rowi = matrixA->at(i);
		sum = 0.0; // rhsZeroElement copy.
		for (auto const& keyValue : *rowi) {
			auto jj = keyValue.first;
			auto j = positionsOfOriginalCols->at(jj);
			if ((ssize_t) j > i) {
				duij = keyValue.second;
				sum += answerX->at(jj) * duij;
			}
			else {
				duii = keyValue.second;
			}
		}
		auto ji = colOrder->at(i);
		answerX->at(ji) = (rightHandSideB->at(i) - sum) / duii;
	}
}

void GESpMatFullPv::postSolve()
{
	assert(false);
}

void GESpMatFullPv::preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	//"A conditioned copy of spMat is solved."
	if (m != spMat->nrow() || n != spMat->ncol()) {
		m = spMat->nrow();
		n = spMat->ncol();
		matrixA = std::make_shared<SparseMatrix<double>>(m);
		pivotValues = std::make_shared<FullColumn<double>>(m);
		rowOrder = std::make_shared<FullColumn<size_t>>(m);
		colOrder = std::make_shared<FullRow<size_t>>(n);
		positionsOfOriginalCols = std::make_shared<std::vector<size_t>>(m);
		rowPositionsOfNonZerosInColumns = std::make_shared<std::vector<std::shared_ptr<std::vector<size_t>>>>(n);
		for (size_t j = 0; j < n; j++)
		{
			rowPositionsOfNonZerosInColumns->at(j) = std::make_shared<std::vector<size_t>>();
		}
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
		if (maxRowMagnitude == 0) {
			throwSingularMatrixError("");
		}
		matrixA->at(i) = spRowi->conditionedWithTol(singularPivotTolerance * maxRowMagnitude);
		rowOrder->at(i) = i;
		colOrder->at(i) = i;
		positionsOfOriginalCols->at(i) = i;
	}
}
