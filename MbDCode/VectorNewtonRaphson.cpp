#include <memory>
#include <cassert>

#include "VectorNewtonRaphson.h"
#include "MatrixSolver.h"
#include "GEFullMatParPv.h"
#include "CREATE.h"
#include "SystemSolver.h"
#include "SingularMatrixError.h"

using namespace MbD;

void VectorNewtonRaphson::initializeGlobally()
{
	assert(false);
	//system->fillVarVector(x);
}

void VectorNewtonRaphson::run()
{
	this->preRun();
	this->initializeLocally();
	this->initializeGlobally();
	this->iterate();
	this->postRun();
}

std::shared_ptr<MatrixSolver> VectorNewtonRaphson::matrixSolverClassNew()
{
	return CREATE<GEFullMatParPv>::With();
}

void VectorNewtonRaphson::fillY()
{
	assert(false);
}

void VectorNewtonRaphson::calcyNorm()
{
	yNorm = 0.5 * y->sumOfSquares();
}

void VectorNewtonRaphson::solveEquations()
{
	try {
		this->basicSolveEquations();
	}
	catch (SingularMatrixError ex) {
		this->handleSingularMatrix();
	}
}

void VectorNewtonRaphson::updatexold()
{
	xold = x;
}

void VectorNewtonRaphson::calcdxNorm()
{
	dxNorm = dx->rootMeanSquare();
}

bool VectorNewtonRaphson::isConverged()
{
	return dxNorms->at(iterNo) < dxTol || this->isConvergedToNumericalLimit();
}

void VectorNewtonRaphson::xEqualxoldPlusdx()
{
	x = xold->plusFullColumn(dx);
}

void VectorNewtonRaphson::handleSingularMatrix()
{
	assert(false);
}
