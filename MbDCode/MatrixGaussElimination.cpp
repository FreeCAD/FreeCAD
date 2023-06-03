#include "MatrixGaussElimination.h"

using namespace MbD;

FColDsptr MbD::MatrixGaussElimination::basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	this->preSolvewithsaveOriginal(spMat, fullCol, saveOriginal);
	for (size_t p = 0; p < m; p++)
	{
		this->doPivoting(p);
		this->forwardEliminateWithPivot(p);
	}

	this->backSubstituteIntoDU();
	this->postSolve();
	return answerX;
}
