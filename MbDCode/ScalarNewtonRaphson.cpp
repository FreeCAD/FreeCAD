/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ScalarNewtonRaphson.h"
#include "SystemSolver.h"

using namespace MbD;

void ScalarNewtonRaphson::initializeGlobally()
{
	assert(false);
	//x = system->x;
}

void ScalarNewtonRaphson::calcyNorm()
{
	yNorm = 0.5 * y * y;
}

void ScalarNewtonRaphson::solveEquations()
{
	dx = -y / pypx;
}

void ScalarNewtonRaphson::updatexold()
{
	xold = x;
}

void ScalarNewtonRaphson::calcdxNorm()
{
	dxNorm = std::abs(dx);
}

void ScalarNewtonRaphson::xEqualxoldPlusdx()
{
	x = xold + dx;
}
