#include <string>

#include "IntegratorInterface.h"
#include "SystemSolver.h"
#include "BasicQuasiIntegrator.h"

using namespace MbD;

void MbD::IntegratorInterface::initializeGlobally()
{
tstart = system->startTime();
hout = system->outputStepSize();
hmax  = system->maxStepSize();
hmin  = system->minStepSize();
tout  = system->firstOutputTime();
tend  = system->endTime();
direction = (tstart < tend) ? 1.0 : -1.0;
}

void MbD::IntegratorInterface::setSystem(Solver* sys)
{
	system = static_cast<SystemSolver*>(sys);
}

void MbD::IntegratorInterface::logString(std::string& str)
{
	system->logString(str);
}

void MbD::IntegratorInterface::run()
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

int MbD::IntegratorInterface::orderMax()
{
	return system->orderMax;
}

void MbD::IntegratorInterface::incrementTime(double tnew)
{
	system->settime(tnew);
}
