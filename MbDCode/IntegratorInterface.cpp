#include <string>

#include "IntegratorInterface.h"
#include "SystemSolver.h"
#include "BasicQuasiIntegrator.h"

using namespace MbD;

void IntegratorInterface::initializeGlobally()
{
	tstart = system->startTime();
	hout = system->outputStepSize();
	hmax = system->maxStepSize();
	hmin = system->minStepSize();
	tout = system->firstOutputTime();
	tend = system->endTime();
	direction = (tstart < tend) ? 1.0 : -1.0;
}

void IntegratorInterface::setSystem(Solver* sys)
{
	system = static_cast<SystemSolver*>(sys);
}

void IntegratorInterface::logString(std::string& str)
{
	system->logString(str);
}

void IntegratorInterface::run()
{
	this->preRun();
	this->initializeLocally();
	this->initializeGlobally();
	if (hout > (4 * std::numeric_limits<double>::epsilon()) && (direction * tout < (direction * (tend + (0.1 * direction * hout))))) {
		integrator->run();
	}
	this->finalize();
	this->reportStats();
	this->postRun();
}

int IntegratorInterface::orderMax()
{
	return system->orderMax;
}

void IntegratorInterface::incrementTime(double tnew)
{
	system->settime(tnew);
}

void IntegratorInterface::postFirstStep()
{
	assert(false);	//Not used.
	//system->postFirstStep();
	//if (integrator->istep > 0) {
	//	//"Noise make checking at the start unreliable."
	//	this->checkForDiscontinuity();
	//}
	//this->checkForOutputThrough(integrator->t);
}

void IntegratorInterface::interpolateAt(double tArg)
{
	//"Interpolate for system state at tArg and leave system in that state."
	assert(false);
	//auto yout = integrator->yDerivat(0, tArg);
	//auto ydotout = integrator->yDerivat(1, tArg);
	//auto yddotout = integrator->yDerivat(2, tArg);
	//system->time(tArg);
	//system->y(yout);
	//system->ydot(ydotout);
	//system->yddot(yddotout);
	//system->simUpdateAll();
}
