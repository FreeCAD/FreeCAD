/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <assert.h>

#include "Solver.h"
#include <string>

using namespace MbD;

void MbD::Solver::noop()
{
	//No Operations
}

void Solver::initialize()
{
}

void Solver::initializeLocally()
{
}

void Solver::initializeGlobally()
{
	assert(false);
}

void Solver::assignEquationNumbers()
{
	assert(false);
}

void Solver::run()
{
	assert(false);
}

void Solver::preRun()
{
	assert(false);
}

void Solver::finalize()
{
}

void Solver::reportStats()
{
}

void Solver::postRun()
{
	assert(false);
}

void Solver::logString(const std::string&)
{
	assert(false);
}

void MbD::Solver::handleSingularMatrix()
{
	assert(false);
}
