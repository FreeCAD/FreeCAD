#include "BasicQuasiIntegrator.h"
#include "IntegratorInterface.h"

using namespace MbD;

void MbD::BasicQuasiIntegrator::firstStep()
{
	istep = 0;
	this->preFirstStep();
	iTry = 1;
	orderNew = 1;
	this->selectFirstStepSize();
	this->incrementTime();
	//this->runInitialConditionTypeSolution();
	//this->reportTrialStepStats();

	//while (this->isRedoingFirstStep())
	//{
	//	this->incrementTry();
	//	orderNew = 1;
	//	this->selectFirstStepSize();
	//	this->changeTime();
	//	this->runInitialConditionTypeSolution();
	//	this->reportTrialStepStats();
	//}
	//this->postFirstStep();
	//this->reportStepStats();
}

void MbD::BasicQuasiIntegrator::nextStep()
{
	this->preStep();
	iTry = 1;
	this->selectOrder();
	//this->selectStepSize();
	//this->incrementTime();
	//this->runInitialConditionTypeSolution();
	//this->reportTrialStepStats();
	//while (this->isRedoingStep()) {
	//	this->incrementTry();
	//	this->selectOrder();
	//	this->selectStepSize();
	//	this->changeTime();
	//	this->runInitialConditionTypeSolution();
	//	this->reportTrialStepStats();
	//}
	//this->postStep();
	//this->reportStepStats();
}

void MbD::BasicQuasiIntegrator::selectFirstStepSize()
{
	if (iTry == 1) {
		hnew = direction * (system->tout - t);
	}
	else {
		hnew = 0.25 * h;
	}
	hnew = system->suggestSmallerOrAcceptFirstStepSize(hnew);
}
