#include "EndFrameqct.h"
#include "MarkerFrame.h"
#include "System.h"
#include "Symbolic.h"
#include "Time.h"
#include "EulerParameters.h"
#include "CREATE.h"
#include "EulerAngleszxz.h"
#include "EulerAngleszxzDot.h"

using namespace MbD;

EndFrameqct::EndFrameqct() {
}

EndFrameqct::EndFrameqct(const char* str) : EndFrameqc(str) {
}

void EndFrameqct::initialize()
{
	EndFrameqc::initialize();
	rmem = std::make_shared<FullColumn<double>>(3);
	prmempt = std::make_shared<FullColumn<double>>(3);
	pprmemptpt = std::make_shared<FullColumn<double>>(3);
	aAme = std::make_shared<FullMatrix<double>>(3, 3);
	pAmept = std::make_shared<FullMatrix<double>>(3, 3);
	ppAmeptpt = std::make_shared<FullMatrix<double>>(3, 3);
	pprOeOpEpt = std::make_shared<FullMatrix<double>>(3, 4);
	pprOeOptpt = std::make_shared<FullColumn<double>>(3);
	ppAOepEpt = std::make_shared<FullColumn<std::shared_ptr<FullMatrix<double>>>>(4);
	ppAOeptpt = std::make_shared<FullMatrix<double>>(3, 3);
}

void EndFrameqct::initializeLocally()
{
	if (!rmemBlks) {
		rmem->zeroSelf();
		prmempt->zeroSelf();
		pprmemptpt->zeroSelf();
	}
	if (!phiThePsiBlks) {
		aAme->identity();
		pAmept->zeroSelf();
		ppAmeptpt->zeroSelf();
	}
}

void EndFrameqct::initializeGlobally()
{
	if (rmemBlks) {
		initprmemptBlks();
		initpprmemptptBlks();
	}
	if (phiThePsiBlks) {
		initpPhiThePsiptBlks();
		initppPhiThePsiptptBlks();
	}
}

void EndFrameqct::initprmemptBlks()
{
	auto& mbdTime = System::getInstance().time;
	prmemptBlks = std::make_shared< FullColumn<Symsptr>>(3);
	for (int i = 0; i < 3; i++) {
		auto& disp = rmemBlks->at(i);
		auto var = disp->differentiateWRT(disp, mbdTime);
		auto vel = var->simplified(var);
		prmemptBlks->at(i) = vel;
	}
}

void EndFrameqct::initpprmemptptBlks()
{
}

void EndFrameqct::initpPhiThePsiptBlks()
{
	auto& mbdTime = System::getInstance().time;
	pPhiThePsiptBlks = std::make_shared< FullColumn<Symsptr>>(3);
	for (int i = 0; i < 3; i++) {
		auto& angle = phiThePsiBlks->at(i);
		auto var = angle->differentiateWRT(angle, mbdTime);
		//std::cout << "var " << *var << std::endl;
		auto vel = var->simplified(var);
		//std::cout << "vel " << *vel << std::endl;
		pPhiThePsiptBlks->at(i) = vel;
	}
}

void MbD::EndFrameqct::initppPhiThePsiptptBlks()
{
}

void MbD::EndFrameqct::postInput()
{
	this->evalrmem();
	this->evalAme();
	Item::postInput();
}

void MbD::EndFrameqct::calcPostDynCorrectorIteration()
{
	auto& rOmO = markerFrame->rOmO;
	auto& aAOm = markerFrame->aAOm;
	rOeO = rOmO->plusFullColumn(aAOm->timesFullColumn(rmem));
	auto& prOmOpE = markerFrame->prOmOpE;
	auto& pAOmpE = markerFrame->pAOmpE;
	for (int i = 0; i < 3; i++)
	{
		auto& prOmOpEi = prOmOpE->at(i);
		auto& prOeOpEi = prOeOpE->at(i);
		for (int j = 0; j < 4; j++)
		{
			auto prOeOpEij = prOmOpEi->at(j) + pAOmpE->at(j)->at(i)->timesFullColumn(rmem);
			prOeOpEi->at(j) = prOeOpEij;
		}
	}
	auto rpep = markerFrame->rpmp->plusFullColumn(markerFrame->aApm->timesFullColumn(rmem));
	pprOeOpEpE = EulerParameters<double>::ppApEpEtimesColumn(rpep);
	aAOe = aAOm->timesFullMatrix(aAme);
	for (int i = 0; i < 4; i++)
	{
		pAOepE->at(i) = pAOmpE->at(i)->timesFullMatrix(aAme);
	}
	auto aApe = markerFrame->aApm->timesFullMatrix(aAme);
	ppAOepEpE = EulerParameters<double>::ppApEpEtimesMatrix(aApe);
}

void MbD::EndFrameqct::prePosIC()
{
	time = System::getInstance().mbdTimeValue();
	this->evalrmem();
	this->evalAme();
	EndFrameqc::prePosIC();
}

void MbD::EndFrameqct::evalrmem()
{
	if (rmemBlks) {
		for (int i = 0; i < 3; i++)
		{
			auto& expression = rmemBlks->at(i);
			double value = expression->getValue();
			rmem->at(i) = value;
		}
	}
}

void MbD::EndFrameqct::evalAme()
{
	if (phiThePsiBlks) {
		auto phiThePsi = CREATE<EulerAngleszxz<double>>::With();
		for (int i = 0; i < 3; i++)
		{
			auto& expression = phiThePsiBlks->at(i);
			auto value = expression->getValue();
			phiThePsi->at(i) = value;
		}
		phiThePsi->calc();
		aAme = phiThePsi->aA;
	}
}

void MbD::EndFrameqct::preVelIC()
{
	time = System::getInstance().mbdTimeValue();
	this->evalrmem();
	this->evalAme();
	Item::preVelIC();
	this->evalprmempt();
	this->evalpAmept();
	auto aAOm = markerFrame->aAOm;
	prOeOpt = aAOm->timesFullColumn(prmempt);
	pAOept = aAOm->timesFullMatrix(pAmept);
}

FColDsptr MbD::EndFrameqct::pAjOept(int j)
{
	return pAOept->column(j);
}

double MbD::EndFrameqct::priOeOpt(int i)
{
	return prOeOpt->at(i);
}

void MbD::EndFrameqct::evalprmempt()
{
	if (rmemBlks) {
		for (int i = 0; i < 3; i++)
		{
			auto& derivative = prmemptBlks->at(i);
			auto value = derivative->getValue();
			prmempt->at(i) = value;
		}
	}
}

void MbD::EndFrameqct::evalpAmept()
{
	if (phiThePsiBlks) {
		auto phiThePsi = CREATE<EulerAngleszxz<double>>::With();
		auto phiThePsiDot = CREATE<EulerAngleszxzDot<double>>::With();
		phiThePsiDot->phiThePsi = phiThePsi;
		for (int i = 0; i < 3; i++)
		{
			auto& expression = phiThePsiBlks->at(i);
			auto& derivative = pPhiThePsiptBlks->at(i);
			auto value = expression->getValue();
			auto valueDot = derivative->getValue();
			phiThePsi->at(i) = value;
			phiThePsiDot->at(i) = valueDot;
		}
		phiThePsi->calc();
		phiThePsiDot->calc();
		pAmept = phiThePsiDot->aAdot;
	}
}
