/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "SystemNewtonRaphson.h"
#include "SystemSolver.h"
#include "SparseMatrix.h"
#include "MatrixSolver.h"
#include "GESpMatParPvMarkoFast.h"
#include "CREATE.h"
#include "GESpMatParPvPrecise.h"

using namespace MbD;

void SystemNewtonRaphson::initializeGlobally()
{
	this->assignEquationNumbers();
	system->partsJointsMotionsForcesTorquesDo([&](std::shared_ptr<Item> item) { item->useEquationNumbers(); });
	this->createVectorsAndMatrices();
	matrixSolver = this->matrixSolverClassNew();
}

void SystemNewtonRaphson::createVectorsAndMatrices()
{
	x = std::make_shared<FullColumn<double>>(n);
	y = std::make_shared<FullColumn<double>>(n);
	pypx = std::make_shared <SparseMatrix<double>>(n, n);
}

std::shared_ptr<MatrixSolver> SystemNewtonRaphson::matrixSolverClassNew()
{
	return CREATE<GESpMatParPvMarkoFast>::With();
}

void SystemNewtonRaphson::calcdxNorm()
{
	VectorNewtonRaphson::calcdxNorm();
	std::string str("MbD: Convergence = ");
	str += std::to_string(dxNorm);
	system->logString(str);
}

void SystemNewtonRaphson::basicSolveEquations()
{
	dx = matrixSolver->solvewithsaveOriginal(pypx, y->negated(), false);
}

void SystemNewtonRaphson::handleSingularMatrix()
{
	std::string str = typeid(*matrixSolver).name();
	if (str == "class GESpMatParPvMarkoFast") {
		matrixSolver = CREATE<GESpMatParPvPrecise>::With();
		this->solveEquations();
	}
	else {
		str = typeid(*matrixSolver).name();
		if (str == "class GESpMatParPvPrecise") {
			str = "MbD: Singular Matrix Error. ";
			system->logString(str);
			matrixSolver = this->matrixSolverClassNew();
		}
		else {
			assert(false);
		}
	}
}
