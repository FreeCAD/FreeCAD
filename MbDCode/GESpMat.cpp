/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "GESpMat.h"
#include "FullColumn.h"
#include "SparseMatrix.h"

using namespace MbD;

FColDsptr GESpMat::solvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	this->timedSolvewithsaveOriginal(spMat, fullCol, saveOriginal);
	return answerX;
}

FColDsptr GESpMat::basicSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
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

FColDsptr GESpMat::basicSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
	return FColDsptr();
}

void GESpMat::preSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
}

void GESpMat::preSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	assert(false);
}

double GESpMat::getmatrixArowimaxMagnitude(int i)
{
	return matrixA->at(i)->maxMagnitude();
}
