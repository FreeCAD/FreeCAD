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
	std::string str = typeid(*matrixSolver).name();
	if (str == "class GESpMatParPvMarkoFast") {
		matrixSolver = CREATE<GESpMatParPvPrecise>::With();
		this->solveEquations();
	}
	else {
		str = typeid(*matrixSolver).name();
		if (str == "class GESpMatParPvPrecise") {
			this->logSingularMatrixMessage();
			matrixSolver = this->matrixSolverClassNew();
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
	catch (SingularMatrixError ex) {
		this->handleSingularMatrix();
	}
}

void VelSolver::setSystem(Solver* sys)
{
	system = static_cast<SystemSolver*>(sys);
}
