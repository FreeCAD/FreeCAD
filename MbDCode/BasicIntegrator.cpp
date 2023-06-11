#include "BasicIntegrator.h"
#include "CREATE.h"
#include "StableBackwardDifference.h"
#include "IntegratorInterface.h"

using namespace MbD;

void MbD::BasicIntegrator::initializeLocally()
{
	_continue = true;
}

void MbD::BasicIntegrator::iStep(int integer)
{
	istep = integer;
	opBDF->setiStep(integer);
}

void MbD::BasicIntegrator::postFirstStep()
{
}

void MbD::BasicIntegrator::postRun()
{
}

void MbD::BasicIntegrator::postStep()
{
}

void MbD::BasicIntegrator::initializeGlobally()
{
	//"Get info from system and prepare for start of simulation."
	//"Integrator asks system for info. Not system setting integrator."

	this->t = system->tstart;
	this->direction = system->direction;
	this->orderMax = system->orderMax();
}

void MbD::BasicIntegrator::setSystem(IntegratorInterface* sys)
{
	system = sys;
}

void MbD::BasicIntegrator::calcOperatorMatrix()
{
	opBDF->calcOperatorMatrix();
}

void MbD::BasicIntegrator::incrementTime()
{
	//| istepNew |
	tpast->insert(tpast->begin(), t);

	if ((int)tpast->size() > (orderMax + 1)) { tpast->pop_back(); }
	auto istepNew = istep + 1;
	this->iStep(istepNew);
	this->setorder(orderNew);
	h = hnew;
	this->settnew(t + (direction * h));
	this->calcOperatorMatrix();
	//system->incrementTime(tnew);
}

void MbD::BasicIntegrator::incrementTry()
{
	assert(false);
}

void MbD::BasicIntegrator::initialize()
{
	Solver::initialize();
	//statistics = IdentityDictionary new.
	tpast = std::make_shared<std::vector<double>>();
	opBDF = CREATE<StableBackwardDifference>::With();
	opBDF->timeNodes = tpast;
}

void MbD::BasicIntegrator::logString(std::string& str)
{
	system->logString(str);
}

void MbD::BasicIntegrator::run()
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

void MbD::BasicIntegrator::selectOrder()
{
	assert(false);
}

void MbD::BasicIntegrator::preFirstStep()
{
	system->preFirstStep();
}

void MbD::BasicIntegrator::preRun()
{
}

void MbD::BasicIntegrator::preStep()
{
	assert(false);
}

void MbD::BasicIntegrator::reportStats()
{
	assert(false);
}

void MbD::BasicIntegrator::firstStep()
{
	assert(false);
}

void MbD::BasicIntegrator::setorder(int o)
{
	order = o;
	//opBD->setorder(o);
}

void MbD::BasicIntegrator::settnew(double t)
{
tnew = t;
//this->time(double);
}

void MbD::BasicIntegrator::subsequentSteps()
{
	while (_continue) { this->nextStep(); }
}
