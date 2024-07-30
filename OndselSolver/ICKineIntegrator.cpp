/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ICKineIntegrator.h"
#include "SystemSolver.h"

using namespace MbD;

void ICKineIntegrator::runInitialConditionTypeSolution()
{
	system->runPosICKine();
	system->runVelICKine();
	system->runAccICKine();
}

void ICKineIntegrator::iStep(size_t)
{
	assert(false);
}

void ICKineIntegrator::selectOrder()
{
	assert(false);
}

void ICKineIntegrator::preRun()
{
	system->logString("MbD: Starting quasi kinematic analysis.");
	QuasiIntegrator::preRun();
}

void ICKineIntegrator::firstStep()
{
	assert(false);
}

void ICKineIntegrator::subsequentSteps()
{
	assert(false);
}

void MbD::ICKineIntegrator::nextStep()
{
	assert(false);
}
