#include "VelSolver.h"
#include "MatrixSolver.h"
#include "SystemSolver.h"
#include "CREATE.h"
#include "GESpMatParPvPrecise.h"
#include "SingularMatrixError.h"

using namespace MbD;

void MbD::VelSolver::basicSolveEquations()
{
	x = matrixSolver->solvewithsaveOriginal(jacobian, errorVector, true);
}

void MbD::VelSolver::handleSingularMatrix()
{
	std::string str = typeid(*matrixSolver).name();
	if (str == "class MbD::GESpMatParPvMarkoFast") {
		matrixSolver = CREATE<GESpMatParPvPrecise>::With();
		this->solveEquations();
	}
	else {
		str = typeid(*matrixSolver).name();
		if (str == "class MbD::GESpMatParPvPrecise") {
			this->logSingularMatrixMessage();
			matrixSolver = this->matrixSolverClassNew();
		}
		else {
			assert(false);
		}
	}
}

void MbD::VelSolver::logSingularMatrixMessage()
{
	std::string str = "MbD: Velocity solver has encountered a singular matrix.";
	system->logString(str);
}

std::shared_ptr<MatrixSolver> MbD::VelSolver::matrixSolverClassNew()
{
	return CREATE<GESpMatParPvPrecise>::With();
}

void MbD::VelSolver::solveEquations()
{
	try {
		this->basicSolveEquations();
	}
	catch (SingularMatrixError ex) {
		this->handleSingularMatrix();
	}
}

void MbD::VelSolver::setSystem(Solver* sys)
{
	system = static_cast<SystemSolver*>(sys);
}
