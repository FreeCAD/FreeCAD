#include <algorithm>
#include <cassert>

#include "GESpMatFullPvPosIC.h"
//#include "FullColumn.h"
//#include "SparseMatrix.h"
#include "PosICNewtonRaphson.h"

using namespace MbD;

void MbD::GESpMatFullPvPosIC::preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	GESpMatFullPv::preSolvewithsaveOriginal(spMat, fullCol, saveOriginal);
	if (system == nullptr) {
		pivotRowLimits = std::make_shared<std::vector<size_t>>();
	}
	else {
		pivotRowLimits = system->pivotRowLimits;
	}
	pivotRowLimit = -1;
}

void MbD::GESpMatFullPvPosIC::doPivoting(size_t p)
{
	//"Used by Gauss Elimination only."
	//"Swap rows but keep columns in place."
	//"The elements below the diagonal are removed column by column."

	assert(false);
	auto max = 0.0;
	auto pivotRow = p;
	auto pivotCol = p;
	for (size_t j = p; j < n; j++)
	{
		rowPositionsOfNonZerosInColumns->at(colOrder->at(j))->clear();
	}
	if (p > pivotRowLimit) {
		pivotRowLimit = *std::find_if(
			pivotRowLimits->begin(), pivotRowLimits->end(),
			[&](size_t limit) { return limit >= p; });
	}
	for (size_t i = p; i < pivotRowLimit; i++)
	{
		auto& rowi = matrixA->at(i);
	}
}
