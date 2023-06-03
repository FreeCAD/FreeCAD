#include <cassert>
#include <memory>

#include "GESpMatParPvMarkoFast.h"
#include "SingularMatrixError.h"

using namespace MbD;

void MbD::GESpMatParPvMarkoFast::preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	//assert(false);
	//"Optimized for speed."
	if (m != spMat->nrow() || n != spMat->ncol()) {
		m = spMat->nrow();
		n = spMat->ncol();
		matrixA = std::make_shared<SparseMatrix<double>>(m);
		privateIndicesOfNonZerosInPivotRow = std::make_shared<std::vector<size_t>>();
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
		auto maxRowElement = spRowi->maxElement();
		if (maxRowElement == 0) {
			throw SingularMatrixError("");
		}
		auto scaling = 1.0 / maxRowElement;
		matrixA->at(i) = spRowi->timesconditionedWithTol(scaling, singularPivotTolerance);
		rightHandSideB->atitimes(i, scaling);
	}
}

void MbD::GESpMatParPvMarkoFast::doPivoting(size_t p)
{
	assert(false);
}
