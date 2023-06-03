#include <memory>
#include <cassert>

#include "VectorNewtonRaphson.h"
#include "MatrixSolver.h"
#include "GEFullMatParPv.h"
#include "CREATE.h"
#include "SystemSolver.h"
#include "SingularMatrixError.h"

using namespace MbD;

void MbD::VectorNewtonRaphson::run()
{
	this->preRun();
	this->initializeLocally();
	this->initializeGlobally();
	this->iterate();
	this->postRun();
}

std::shared_ptr<MatrixSolver> MbD::VectorNewtonRaphson::matrixSolverClassNew()
{
	return CREATE<GEFullMatParPv>::With();
}

void MbD::VectorNewtonRaphson::fillY()
{
}

void MbD::VectorNewtonRaphson::calcyNorm()
{
	yNorm = 0.5 * y->sumOfSquares();
}

void MbD::VectorNewtonRaphson::solveEquations()
{
	try {
		this->basicSolveEquations();
	}
	catch (SingularMatrixError ex) {
		this->handleSingularMatrix();
	}
}

void MbD::VectorNewtonRaphson::updatexold()
{
	xold = x;
}

void MbD::VectorNewtonRaphson::calcdxNorm()
{
	dxNorm = dx->rootMeanSquare();
}

bool MbD::VectorNewtonRaphson::isConverged()
{
	return dxNorms->at(iterNo) < dxTol || this->isConvergedToNumericalLimit();
}

void MbD::VectorNewtonRaphson::xEqualxoldPlusdx()
{
	x = xold->plusFullColumn(dx);
}

void MbD::VectorNewtonRaphson::handleSingularMatrix()
{
	assert(false);
}
