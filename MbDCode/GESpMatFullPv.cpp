#include <cassert>

#include "GESpMatFullPv.h"
#include "SingularMatrixError.h"

using namespace MbD;

void MbD::GESpMatFullPv::doPivoting(int p)
{
	assert(false);
}

void MbD::GESpMatFullPv::forwardEliminateWithPivot(int p)
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
	auto elementsInPivotRow = std::make_shared<std::vector<const std::pair<const int, double>*>>(rowp->size() - 1);
	int index = 0;
	for (auto const& keyValue : *rowp) {
		if (keyValue.first != jp) {
			elementsInPivotRow->at(index) = (&keyValue);
			index++;
		}
	}
	auto bp = rightHandSideB->at(p);
	for (int ii = 0; ii < markowitzPivotColCount; ii++)
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

void MbD::GESpMatFullPv::backSubstituteIntoDU()
{
	//"Use colOrder to get DU in upper triangular with nonzero diagonals."
	//"Formula given by Eqn. 9.26 and 9.27 in Chapra's text 2nd Edition."

	double sum, duij, duii;
	//answerX = rightHandSideB->copyEmpty();
	assert(m == n);
	answerX = std::make_shared<FullColumn<double>>(m);
	auto jn = colOrder->at(n);
	answerX->at(jn) = rightHandSideB->at(m) / matrixA->at(m)->at(jn);
	//auto rhsZeroElement = this->rhsZeroElement();
	for (int i = n - 2; i >= 0; i--)
	{
		auto rowi = matrixA->at(i);
		sum = 0.0; // rhsZeroElement copy.
		for (auto const& keyValue : *rowi) {
			auto jj = keyValue.first;
			auto j = positionsOfOriginalCols->at(jj);
			if (j > i) {
				duij = keyValue.second;
				sum += answerX->at(jj) * duij;
			}
			else {
				duii = keyValue.second;
			}
		}
		auto ji = colOrder->at(i);
		answerX->at(ji) = rightHandSideB->at(i) - (sum / duii);
	}
}

void MbD::GESpMatFullPv::postSolve()
{
	assert(false);
}

void MbD::GESpMatFullPv::preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	//"A conditioned copy of spMat is solved."
	if (m != spMat->nrow() || n != spMat->ncol()) {
		m = spMat->nrow();
		n = spMat->ncol();
		matrixA = std::make_shared<SparseMatrix<double>>(m);
		pivotValues = std::make_shared<FullColumn<double>>(m);
		rowOrder = std::make_shared<FullColumn<int>>(m);
		colOrder = std::make_shared<FullColumn<int>>(n);
		positionsOfOriginalCols = std::make_shared<std::vector<int>>(m);
		privateIndicesOfNonZerosInPivotRow = std::make_shared<std::vector<int>>();
		rowPositionsOfNonZerosInColumns = std::make_shared<std::vector<std::shared_ptr<std::vector<int>>>>(n);
		for (int j = 0; j < n; j++)
		{
			rowPositionsOfNonZerosInColumns->at(j) = std::make_shared<std::vector<int>>();
		}
	}
	if (saveOriginal) {
		rightHandSideB = fullCol->copy();
	}
	else {
		rightHandSideB = fullCol;
	}
	for (int i = 0; i < m; i++)
	{
		auto& spRowi = spMat->at(i);
		auto maxRowElement = spRowi->maxElement();
		if (maxRowElement == 0) {
			throw SingularMatrixError("");
		}
		matrixA->at(i) = spRowi->conditionedWithTol(singularPivotTolerance * maxRowElement);
		rowOrder->at(i) = i;
		colOrder->at(i) = i;
		positionsOfOriginalCols->at(i) = i;
	}
}
