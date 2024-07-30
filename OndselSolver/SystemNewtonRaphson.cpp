/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include <fstream>	
#include <iomanip>

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
	system->partsJointsMotionsLimitsForcesTorquesDo([&](std::shared_ptr<Item> item) { item->useEquationNumbers(); });
	this->createVectorsAndMatrices();
	matrixSolver = this->matrixSolverClassNew();
}

void MbD::SystemNewtonRaphson::assignEquationNumbers()
{
	assert(false);
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
	std::stringstream ss;
	ss << std::setprecision(std::numeric_limits<double>::max_digits10);
	ss << "MbD: Convergence = " << dxNorm;
	auto str = ss.str();
	system->logString(str);
}

void SystemNewtonRaphson::basicSolveEquations()
{
	auto debug = false;
	if (debug) {
		outputSpreadsheet();
	}
	dx = matrixSolver->solvewithsaveOriginal(pypx, y->negated(), false);
}

void SystemNewtonRaphson::handleSingularMatrix()
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
			str = "MbD: Singular Matrix Error. ";
			system->logString(str);
            matrixSolver->throwSingularMatrixError("SystemNewtonRaphson");
		}
		else {
			assert(false);
		}
	}
}

void MbD::SystemNewtonRaphson::outputSpreadsheet()
{
	std::ofstream os("../../testapp/spreadsheetcpp.csv");
	os << std::setprecision(std::numeric_limits<double>::max_digits10);
	for (size_t i = 0; i < pypx->nrow(); i++)
	{
		auto rowi = pypx->at(i);
		for (size_t j = 0; j < pypx->ncol(); j++)
		{
			if (j > 0) os << '\t';
			if (rowi->find(j) == rowi->end()) {
				os << 0.0;
			}
			else {
				os << rowi->at(j);
			}
		}
		os << "\t\t" << y->at(i) << std::endl;
	}
}
