#include <iostream>
#include <limits>
#include <memory>
#include <chrono>

#include "MatrixSolver.h"
#include "SparseMatrix.h"
#include "FullMatrix.h"

using namespace MbD;

void MbD::MatrixSolver::initialize()
{
	Solver::initialize();
	singularPivotTolerance = 4 * std::numeric_limits<double>::epsilon();
}

FColDsptr MbD::MatrixSolver::solvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	this->timedSolvewithsaveOriginal(spMat, fullCol, saveOriginal);
	return answerX;
}

FColDsptr MbD::MatrixSolver::solvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	this->timedSolvewithsaveOriginal(fullMat, fullCol, saveOriginal);
	return answerX;
}

FColDsptr MbD::MatrixSolver::timedSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	auto start = std::chrono::steady_clock::now();

	this->basicSolvewithsaveOriginal(spMat, fullCol, saveOriginal);

	auto end = std::chrono::steady_clock::now();
	auto diff = end - start;
	millisecondsToRun = std::chrono::duration<double, std::milli>(diff).count();
	std::cout << "milliseconds to run = " << millisecondsToRun << std::endl;
	return answerX;
}

FColDsptr MbD::MatrixSolver::timedSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	return FColDsptr();
}
