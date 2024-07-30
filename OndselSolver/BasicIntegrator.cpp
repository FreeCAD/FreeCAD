/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "BasicIntegrator.h"
#include "CREATE.h"
#include "StableBackwardDifference.h"
#include "IntegratorInterface.h"

using namespace MbD;

void BasicIntegrator::initializeLocally()
{
	_continue = true;
}

void BasicIntegrator::iStep(size_t integer)
{
	istep = integer;
	opBDF->setiStep(integer);
}

void BasicIntegrator::postFirstStep()
{
	t = tnew;
	system->postFirstStep();
}

void BasicIntegrator::postRun()
{
}

void BasicIntegrator::postStep()
{
	t = tnew;
	system->postStep();
}

void BasicIntegrator::initializeGlobally()
{
	//"Get info from system and prepare for start of simulation."
	//"Integrator asks system for info. Not system setting integrator."

	this->sett(system->tstart);
	this->direction = system->direction;
	this->orderMax = system->orderMax();
}

void BasicIntegrator::setSystem(Solver* sys)
{
	system = static_cast<IntegratorInterface*>(sys);
}

void BasicIntegrator::calcOperatorMatrix()
{
	opBDF->calcOperatorMatrix();
}

void BasicIntegrator::incrementTime()
{
	tpast->insert(tpast->begin(), t);

	if (tpast->size() > (orderMax + 1)) { tpast->pop_back(); }
	auto istepNew = istep + 1;
	this->iStep(istepNew);
	this->setorder(orderNew);
	h = hnew;
	this->settnew(t + (direction * h));
	this->calcOperatorMatrix();
	system->incrementTime(tnew);
}

void BasicIntegrator::incrementTry()
{
	assert(false);
}

void BasicIntegrator::initialize()
{
	Solver::initialize();
	//statistics = IdentityDictionary new.
	tpast = std::make_shared<std::vector<double>>();
	opBDF = CREATE<StableBackwardDifference>::With();
	opBDF->timeNodes = tpast;
}

void BasicIntegrator::logString(const std::string& str)
{
	system->logString(str);
}

void BasicIntegrator::run()
{
	this->preRun();
	this->initializeLocally();
	this->initializeGlobally();
	this->firstStep();
	this->subsequentSteps();
	this->finalize();
	this->reportStats();
	this->postRun();
}

void BasicIntegrator::selectOrder()
{
	//"Increase order consecutively with step."
	if (iTry == 1) orderNew = std::min(istep + 1, orderMax);
}

void BasicIntegrator::preFirstStep()
{
	system->preFirstStep();
}

void BasicIntegrator::preRun()
{
}

void BasicIntegrator::preStep()
{
	system->preStep();
}

void BasicIntegrator::reportStats()
{
}

void BasicIntegrator::setorder(size_t o)
{
	order = o;
	opBDF->setorder(o);
}

void BasicIntegrator::settnew(double t)
{
	tnew = t;
	this->settime(t);
}

void BasicIntegrator::sett(double tt)
{
	t = tt;
	opBDF->settime(tt);
}

void BasicIntegrator::settime(double tt)
{
	opBDF->settime(tt);
}

double BasicIntegrator::tprevious()
{
	return tpast->at(0);
}

void BasicIntegrator::subsequentSteps()
{
	while (_continue) { this->nextStep(); }
}
