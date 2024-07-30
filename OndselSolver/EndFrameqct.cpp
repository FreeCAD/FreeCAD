/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "EndFrameqct.h"
#include "MarkerFrame.h"
#include "System.h"
#include "Symbolic.h"
#include "SymTime.h"
#include "EulerParameters.h"
#include "CREATE.h"
#include "EulerAngleszxz.h"
#include "EulerAngleszxzDot.h"
#include "EulerAngleszxzDDot.h"

using namespace MbD;

EndFrameqct::EndFrameqct() {
}

EndFrameqct::EndFrameqct(const std::string& str) : EndFrameqc(str) {
}

void EndFrameqct::initialize()
{
	EndFrameqc::initialize();
	rmem = std::make_shared<FullColumn<double>>(3);
	prmempt = std::make_shared<FullColumn<double>>(3);
	pprmemptpt = std::make_shared<FullColumn<double>>(3);
	aAme = FullMatrix<double>::identitysptr(3);
	pAmept = std::make_shared<FullMatrix<double>>(3, 3);
	ppAmeptpt = std::make_shared<FullMatrix<double>>(3, 3);
	pprOeOpEpt = std::make_shared<FullMatrix<double>>(3, 4);
	pprOeOptpt = std::make_shared<FullColumn<double>>(3);
	ppAOepEpt = std::make_shared<FullColumn<FMatDsptr>>(4);
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
	auto& mbdTime = this->root()->time;
	prmemptBlks = std::make_shared< FullColumn<Symsptr>>(3);
	for (size_t i = 0; i < 3; i++) {
		auto& disp = rmemBlks->at(i);
		auto var = disp->differentiateWRT(mbdTime);
		auto vel = var->simplified(var);
		prmemptBlks->at(i) = vel;
	}
}

void EndFrameqct::initpprmemptptBlks()
{
	auto& mbdTime = this->root()->time;
	pprmemptptBlks = std::make_shared< FullColumn<Symsptr>>(3);
	for (size_t i = 0; i < 3; i++) {
		auto& vel = prmemptBlks->at(i);
		auto var = vel->differentiateWRT(mbdTime);
		auto acc = var->simplified(var);
		pprmemptptBlks->at(i) = acc;
	}
}

void EndFrameqct::initpPhiThePsiptBlks()
{
	auto& mbdTime = this->root()->time;
	pPhiThePsiptBlks = std::make_shared< FullColumn<Symsptr>>(3);
	for (size_t i = 0; i < 3; i++) {
		auto& angle = phiThePsiBlks->at(i);
		auto var = angle->differentiateWRT(mbdTime);
		//std::cout << "var " << *var << std::endl;
		auto vel = var->simplified(var);
		//std::cout << "vel " << *vel << std::endl;
		pPhiThePsiptBlks->at(i) = vel;
	}
}

void EndFrameqct::initppPhiThePsiptptBlks()
{
	auto& mbdTime = this->root()->time;
	ppPhiThePsiptptBlks = std::make_shared< FullColumn<Symsptr>>(3);
	for (size_t i = 0; i < 3; i++) {
		auto& angleVel = pPhiThePsiptBlks->at(i);
		auto var = angleVel->differentiateWRT(mbdTime);
		auto angleAcc = var->simplified(var);
		ppPhiThePsiptptBlks->at(i) = angleAcc;
	}
}

void EndFrameqct::postInput()
{
	this->evalrmem();
	this->evalAme();
	Item::postInput();
}

void EndFrameqct::calcPostDynCorrectorIteration()
{
	auto& rOmO = markerFrame->rOmO;
	auto& aAOm = markerFrame->aAOm;
	rOeO = rOmO->plusFullColumn(aAOm->timesFullColumn(rmem));
	auto& prOmOpE = markerFrame->prOmOpE;
	auto& pAOmpE = markerFrame->pAOmpE;
	for (size_t i = 0; i < 3; i++)
	{
		auto& prOmOpEi = prOmOpE->at(i);
		auto& prOeOpEi = prOeOpE->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			auto prOeOpEij = prOmOpEi->at(j) + pAOmpE->at(j)->at(i)->timesFullColumn(rmem);
			prOeOpEi->at(j) = prOeOpEij;
		}
	}
	auto rpep = markerFrame->rpmp->plusFullColumn(markerFrame->aApm->timesFullColumn(rmem));
	pprOeOpEpE = EulerParameters<double>::ppApEpEtimesColumn(rpep);
	aAOe = aAOm->timesFullMatrix(aAme);
	for (size_t i = 0; i < 4; i++)
	{
		pAOepE->at(i) = pAOmpE->at(i)->timesFullMatrix(aAme);
	}
	auto aApe = markerFrame->aApm->timesFullMatrix(aAme);
	ppAOepEpE = EulerParameters<double>::ppApEpEtimesMatrix(aApe);
}

void EndFrameqct::prePosIC()
{
	time = this->root()->mbdTimeValue();
	this->evalrmem();
	this->evalAme();
	EndFrameqc::prePosIC();
}

void EndFrameqct::evalrmem()
{
	if (rmemBlks) {
		for (size_t i = 0; i < 3; i++)
		{
			auto& expression = rmemBlks->at(i);
			double value = expression->getValue();
			rmem->at(i) = value;
		}
	}
}

void EndFrameqct::evalAme()
{
	if (phiThePsiBlks) {
		auto phiThePsi = CREATE<EulerAngleszxz<double>>::With();
		for (size_t i = 0; i < 3; i++)
		{
			auto& expression = phiThePsiBlks->at(i);
			auto value = expression->getValue();
			phiThePsi->at(i) = value;
		}
		phiThePsi->calc();
		aAme = phiThePsi->aA;
	}
}

void EndFrameqct::preVelIC()
{
	time = this->root()->mbdTimeValue();
	this->evalrmem();
	this->evalAme();
	Item::preVelIC();
	this->evalprmempt();
	this->evalpAmept();
	auto& aAOm = markerFrame->aAOm;
	prOeOpt = aAOm->timesFullColumn(prmempt);
	pAOept = aAOm->timesFullMatrix(pAmept);
}

void EndFrameqct::postVelIC()
{
	auto& pAOmpE = markerFrame->pAOmpE;
	for (size_t i = 0; i < 3; i++)
	{
		auto& pprOeOpEpti = pprOeOpEpt->at(i);
		for (size_t j = 0; j < 4; j++)
		{
			auto pprOeOpEptij = pAOmpE->at(j)->at(i)->dot(prmempt);
			pprOeOpEpti->atiput(j, pprOeOpEptij);
		}
	}
	for (size_t i = 0; i < 4; i++)
	{
		ppAOepEpt->atiput(i, pAOmpE->at(i)->timesFullMatrix(pAmept));
	}
}

FColDsptr EndFrameqct::pAjOept(size_t j)
{
	return pAOept->column(j);
}

FMatDsptr EndFrameqct::ppAjOepETpt(size_t jj)
{
	auto answer = std::make_shared<FullMatrix<double>>(4, 3);
	for (size_t i = 0; i < 4; i++)
	{
		auto& answeri = answer->at(i);
		auto& ppAOepEipt = ppAOepEpt->at(i);
		for (size_t j = 0; j < 3; j++)
		{
			auto& answerij = ppAOepEipt->at(j)->at(jj);
			answeri->atiput(j, answerij);
		}
	}
	return answer;
}

FColDsptr EndFrameqct::ppAjOeptpt(size_t j)
{
	return ppAOeptpt->column(j);
}

double EndFrameqct::priOeOpt(size_t i)
{
	return prOeOpt->at(i);
}

FRowDsptr EndFrameqct::ppriOeOpEpt(size_t i)
{
	return pprOeOpEpt->at(i);
}

double EndFrameqct::ppriOeOptpt(size_t i)
{
	return pprOeOptpt->at(i);
}

void EndFrameqct::evalprmempt()
{
	if (rmemBlks) {
		for (size_t i = 0; i < 3; i++)
		{
			auto& derivative = prmemptBlks->at(i);
			auto value = derivative->getValue();
			prmempt->at(i) = value;
		}
	}
}

void EndFrameqct::evalpAmept()
{
	if (phiThePsiBlks) {
		auto phiThePsi = CREATE<EulerAngleszxz<double>>::With();
		auto phiThePsiDot = CREATE<EulerAngleszxzDot<double>>::With();
		phiThePsiDot->phiThePsi = phiThePsi;
		for (size_t i = 0; i < 3; i++)
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

void EndFrameqct::evalpprmemptpt()
{
	if (rmemBlks) {
		for (size_t i = 0; i < 3; i++)
		{
			auto& secondDerivative = pprmemptptBlks->at(i);
			auto value = secondDerivative->getValue();
			pprmemptpt->atiput(i, value);
		}
	}
}

void EndFrameqct::evalppAmeptpt()
{
	if (phiThePsiBlks) {
		auto phiThePsi = CREATE<EulerAngleszxz<double>>::With();
		auto phiThePsiDot = CREATE<EulerAngleszxzDot<double>>::With();
		phiThePsiDot->phiThePsi = phiThePsi;
		auto phiThePsiDDot = CREATE<EulerAngleszxzDDot<double>>::With();
		phiThePsiDDot->phiThePsiDot = phiThePsiDot;
		for (size_t i = 0; i < 3; i++)
		{
			auto& expression = phiThePsiBlks->at(i);
			auto& derivative = pPhiThePsiptBlks->at(i);
			auto& secondDerivative = ppPhiThePsiptptBlks->at(i);
			auto value = expression->getValue();
			auto valueDot = derivative->getValue();
			auto valueDDot = secondDerivative->getValue();
			phiThePsi->atiput(i, value);
			phiThePsiDot->atiput(i, valueDot);
			phiThePsiDDot->atiput(i, valueDDot);
		}
		phiThePsi->calc();
		phiThePsiDot->calc();
		phiThePsiDDot->calc();
		ppAmeptpt = phiThePsiDDot->aAddot;
	}
}

FColDsptr EndFrameqct::rmeO()
{
	return markerFrame->aAOm->timesFullColumn(rmem);
}

FColDsptr EndFrameqct::rpep()
{
	auto& rpmp = markerFrame->rpmp;
	auto& aApm = markerFrame->aApm;
	auto rpep = rpmp->plusFullColumn(aApm->timesFullColumn(rmem));
	return rpep;
}

void EndFrameqct::preAccIC()
{
	time = this->root()->mbdTimeValue();
	this->evalrmem();
	this->evalAme();
	Item::preVelIC();
	this->evalprmempt();
	this->evalpAmept();
	auto& aAOm = markerFrame->aAOm;
	prOeOpt = aAOm->timesFullColumn(prmempt);
	pAOept = aAOm->timesFullMatrix(pAmept);
	Item::preAccIC();
	this->evalpprmemptpt();
	this->evalppAmeptpt();
	aAOm = markerFrame->aAOm;
	pprOeOptpt = aAOm->timesFullColumn(pprmemptpt);
	ppAOeptpt = aAOm->timesFullMatrix(ppAmeptpt);
}

bool MbD::EndFrameqct::isEndFrameqc()
{
	return false;
}
