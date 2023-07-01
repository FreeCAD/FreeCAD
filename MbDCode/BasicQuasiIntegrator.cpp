#include "BasicQuasiIntegrator.h"
#include "IntegratorInterface.h"

using namespace MbD;

void BasicQuasiIntegrator::firstStep()
{
	istep = 0;
	this->preFirstStep();
	iTry = 1;
	orderNew = 1;
	this->selectFirstStepSize();
	this->incrementTime();
	this->runInitialConditionTypeSolution();
	//this->reportTrialStepStats();
	this->postFirstStep();
	//this->reportStepStats();
}

bool BasicQuasiIntegrator::isRedoingFirstStep()
{
	return false;
}

void BasicQuasiIntegrator::nextStep()
{
	this->preStep();
	iTry = 1;
	this->selectOrder();
	this->selectStepSize();
	this->incrementTime();
	this->runInitialConditionTypeSolution();
	//this->reportTrialStepStats();
	this->postStep();
	//this->reportStepStats();
}

void BasicQuasiIntegrator::runInitialConditionTypeSolution()
{
	system->runInitialConditionTypeSolution();
}

void BasicQuasiIntegrator::selectFirstStepSize()
{
	if (iTry == 1) {
		hnew = direction * (system->tout - t);
	}
	else {
		hnew = 0.25 * h;
	}
	hnew = system->suggestSmallerOrAcceptFirstStepSize(hnew);
}

void BasicQuasiIntegrator::selectStepSize()
{
	if (iTry == 1) {
		hnew = direction * (system->tout - t);
	}
	else {
		hnew = 0.25 * h;
	}
	hnew = system->suggestSmallerOrAcceptStepSize(hnew);
}
