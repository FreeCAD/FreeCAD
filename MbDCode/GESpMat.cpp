#include "GESpMat.h"
#include "FullColumn.h"
#include "SparseMatrix.h"

using namespace MbD;

FColDsptr MbD::GESpMat::solvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	this->timedSolvewithsaveOriginal(spMat, fullCol, saveOriginal);
	return answerX;
}

FColDsptr MbD::GESpMat::basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	this->preSolvewithsaveOriginal(spMat, fullCol, saveOriginal);
	for (int p = 0; p < m; p++)
	{
		this->doPivoting(p);
		this->forwardEliminateWithPivot(p);
	}
	this->backSubstituteIntoDU();
	this->postSolve();
	return answerX;
}

FColDsptr MbD::GESpMat::basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
	return FColDsptr();
}

void MbD::GESpMat::preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
}

void MbD::GESpMat::preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
}

double MbD::GESpMat::getmatrixArowimaxMagnitude(int i)
{
	return matrixA->at(i)->maxMagnitude();
}
