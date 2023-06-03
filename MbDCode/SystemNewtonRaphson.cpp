#include "SystemNewtonRaphson.h"
#include "SystemSolver.h"
#include "SparseMatrix.h"
#include "MatrixSolver.h"
#include "GESpMatParPvMarkoFast.h"
#include "CREATE.h"

using namespace MbD;

void MbD::SystemNewtonRaphson::initializeGlobally()
{
	this->assignEquationNumbers();
	system->partsJointsMotionsDo([&](std::shared_ptr<Item> item) { item->useEquationNumbers(); });

	this->createVectorsAndMatrices();
	matrixSolver = this->matrixSolverClassNew();
}

void MbD::SystemNewtonRaphson::createVectorsAndMatrices()
{
	x = std::make_shared<FullColumn<double>>(n);
	y = std::make_shared<FullColumn<double>>(n);
	pypx = std::make_shared <SparseMatrix<double>>(n, n);
}

std::shared_ptr<MatrixSolver> MbD::SystemNewtonRaphson::matrixSolverClassNew()
{
	return CREATE<GESpMatParPvMarkoFast>::With();
}

void MbD::SystemNewtonRaphson::calcdxNorm()
{
	VectorNewtonRaphson::calcdxNorm();
	std::string str("MbD: Convergence = ");
	str += std::to_string(dxNorm);
	system->logString(str);
}

void MbD::SystemNewtonRaphson::basicSolveEquations()
{
	dx = matrixSolver->solvewithsaveOriginal(pypx, y->negated(), false);
}
