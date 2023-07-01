#include <iostream>
#include <limits>
#include <memory>
#include <chrono>

#include "MatrixSolver.h"
#include "SparseMatrix.h"
#include "FullMatrix.h"
#include "SingularMatrixError.h"

using namespace MbD;

void MatrixSolver::initialize()
{
	Solver::initialize();
	singularPivotTolerance = 4 * std::numeric_limits<double>::epsilon();
}

void MatrixSolver::setSystem(Solver* sys)
{
}

FColDsptr MatrixSolver::solvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	this->timedSolvewithsaveOriginal(spMat, fullCol, saveOriginal);
	return answerX;
}

FColDsptr MatrixSolver::solvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	this->timedSolvewithsaveOriginal(fullMat, fullCol, saveOriginal);
	return answerX;
}

FColDsptr MatrixSolver::timedSolvewithsaveOriginal(SpMatDsptr spMat, FColDsptr fullCol, bool saveOriginal)
{
	auto start = std::chrono::steady_clock::now();

	this->basicSolvewithsaveOriginal(spMat, fullCol, saveOriginal);

	auto end = std::chrono::steady_clock::now();
	auto diff = end - start;
	millisecondsToRun = std::chrono::duration<double, std::milli>(diff).count();
	std::cout << "milliseconds to run = " << millisecondsToRun << std::endl;
	return answerX;
}

FColDsptr MatrixSolver::timedSolvewithsaveOriginal(FMatDsptr fullMat, FColDsptr fullCol, bool saveOriginal)
{
	return FColDsptr();
}

void MatrixSolver::findScalingsForRowRange(int begin, int end)
{
	//"Row element * scaling <= 1.0."
	rowScalings = std::make_shared<FullColumn<double>>(m);
	for (int i = begin; i < end; i++)
	{
		auto maxRowMagnitude = this->getmatrixArowimaxMagnitude(i);
		if (maxRowMagnitude == 0.0) throwSingularMatrixError("");
		rowScalings->at(i) = 1.0 / maxRowMagnitude;
	}
}

void MatrixSolver::throwSingularMatrixError(const char* chars)
{
	throw SingularMatrixError(chars);
}

void MatrixSolver::throwSingularMatrixError(const char* chars, std::shared_ptr<FullColumn<int>> redunEqnNos)
{
	throw SingularMatrixError(chars, redunEqnNos);
}
