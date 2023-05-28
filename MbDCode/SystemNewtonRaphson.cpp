#include "SystemNewtonRaphson.h"
#include "SystemSolver.h"
#include "SparseMatrix.h"

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
