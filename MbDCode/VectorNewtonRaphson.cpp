#include <memory>

#include "VectorNewtonRaphson.h"
#include "MatrixSolver.h"
#include "GEFullMatParPv.h"

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
	return std::make_shared<GEFullMatParPv>();
}
