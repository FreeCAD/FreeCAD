/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "PosICKineNewtonRaphson.h"
#include "SystemSolver.h"
#include "Part.h"
#include "Constraint.h"

using namespace MbD;

void PosICKineNewtonRaphson::initializeGlobally()
{
	AnyPosICNewtonRaphson::initializeGlobally();
	iterMax = system->iterMaxPosKine;
	dxTol = system->errorTolPosKine;
}

void MbD::PosICKineNewtonRaphson::preRun()
{
	system->logString("MbD: Solving for quasi kinematic position.");
	PosNewtonRaphson::preRun();
}
