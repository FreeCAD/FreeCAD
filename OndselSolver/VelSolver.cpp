/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "VelSolver.h"
#include "MatrixSolver.h"
#include "SystemSolver.h"
#include "CREATE.h"
#include "GESpMatParPvPrecise.h"
#include "SingularMatrixError.h"
#include "GESpMatParPvMarko.h"
#include "GESpMatParPvMarkoFast.h"

using namespace MbD;

void VelSolver::basicSolveEquations()
{
	x = matrixSolver->solvewithsaveOriginal(jacobian, errorVector, true);
}

void VelSolver::handleSingularMatrix()
{
    auto& r = *matrixSolver;
	std::string str = typeid(r).name();
	if (str.find("GESpMatParPvMarkoFast") != std::string::npos) {
		matrixSolver = CREATE<GESpMatParPvPrecise>::With();
		this->solveEquations();
	}
	else {
		str = typeid(r).name();
		if (str.find("GESpMatParPvPrecise") != std::string::npos) {
			this->logSingularMatrixMessage();
            matrixSolver->throwSingularMatrixError("VelSolver");
        }
		else {
			assert(false);
		}
	}
}

void VelSolver::logSingularMatrixMessage()
{
	std::string str = "MbD: Velocity solver has encountered a singular matrix.";
	system->logString(str);
}

std::shared_ptr<MatrixSolver> VelSolver::matrixSolverClassNew()
{
	return CREATE<GESpMatParPvPrecise>::With();
}

void VelSolver::solveEquations()
{
	try {
		this->basicSolveEquations();
	}
	catch (const SingularMatrixError& ex) {
		this->handleSingularMatrix();
	}
}

void VelSolver::setSystem(Solver* sys)
{
	system = static_cast<SystemSolver*>(sys);
}
