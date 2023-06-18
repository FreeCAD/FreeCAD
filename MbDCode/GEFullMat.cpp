#include <cassert>

#include "GEFullMat.h"

using namespace MbD;

void MbD::GEFullMat::forwardEliminateWithPivot(int p)
{
	assert(false);
}

void MbD::GEFullMat::backSubstituteIntoDU()
{
	answerX = std::make_shared<FullColumn<double>>(n);
	answerX->at(n - 1) = rightHandSideB->at(m - 1) / matrixA->at(m - 1)->at(n - 1);
	for (int i = n - 2; i >= 0; i--)
	{
		auto rowi = matrixA->at(i);
		double sum = answerX->at(n) * rowi->at(n);
		for (int j = i + 1; j < n - 1; j++)
		{
			sum += answerX->at(j) * rowi->at(j);
		}
		answerX->at(i) = (rightHandSideB->at(i) - sum) / rowi->at(i);
	}
}

void MbD::GEFullMat::postSolve()
{
	assert(false);
}

void MbD::GEFullMat::preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
}

void MbD::GEFullMat::preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
}

double MbD::GEFullMat::getmatrixArowimaxMagnitude(int i)
{
	return matrixA->at(i)->maxMagnitude();
}

FColDsptr MbD::GEFullMat::basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	this->preSolvewithsaveOriginal(fullMat, fullCol, saveOriginal);
	for (int p = 0; p < m; p++)
	{
		this->doPivoting(p);
		this->forwardEliminateWithPivot(p);
	}
	this->backSubstituteIntoDU();
	this->postSolve();
	return answerX;
}

FColDsptr MbD::GEFullMat::basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
	return FColDsptr();
}
