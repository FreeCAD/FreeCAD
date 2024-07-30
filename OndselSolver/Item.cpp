/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
//#include <windows.h>
#include <assert.h>
//#include <debugapi.h>
#include <sstream> 
#include <chrono>

#include "Item.h"
#include "System.h"
#include "Symbolic.h"

using namespace MbD;

Item::Item() {
	auto now = std::chrono::high_resolution_clock::now();
	auto nanoseconds = now.time_since_epoch().count();
	name = std::to_string(nanoseconds);
}

Item::Item(const std::string& str) : name(str)
{
}

System* Item::root()
{
	return owner->root();
}

void MbD::Item::noop()
{
	//No Operations
}

void Item::initialize()
{
}

std::ostream& Item::printOn(std::ostream& s) const
{
	std::string str = typeid(*this).name();
	auto classname = str.substr(11, str.size() - 11);
	s << classname << std::endl;
	return s;
}

void Item::initializeLocally()
{
}

bool MbD::Item::isJointForce()
{
	assert(false);
	return false;
}

bool MbD::Item::isJointTorque()
{
	assert(false);
	return false;
}

bool MbD::Item::isKinedotIJ()
{
	assert(false);
	return false;
}

bool MbD::Item::isKineIJ()
{
	assert(false);
	return false;
}

void Item::initializeGlobally()
{
}

void Item::postInput()
{
	//Called once after input
	calcPostDynCorrectorIteration();
}

void Item::calcPostDynCorrectorIteration()
{
}

void MbD::Item::checkForCollisionDiscontinuityBetweenand(double, double)
{
	assert(false);
}

void Item::removeRedundantConstraints(std::shared_ptr<std::vector<size_t>>)
{
}

void MbD::Item::setpqsumu(FColDsptr)
{
	assert(false);
}

void MbD::Item::setpqsumuddot(FColDsptr)
{
	assert(false);
}

void MbD::Item::setpqsumudot(FColDsptr)
{
	assert(false);
}

void Item::reactivateRedundantConstraints()
{
}

void MbD::Item::registerName()
{
	assert(false);
}

void Item::fillPosKineError(FColDsptr)
{
}

void Item::fillPosKineJacob(SpMatDsptr)
{
}

void MbD::Item::fillpqsumu(FColDsptr)
{
	assert(false);
}

void MbD::Item::fillpqsumudot(FColDsptr)
{
	assert(false);
}

void Item::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>>)
{
	assert(false);
}

void MbD::Item::fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>>)
{
	assert(false);
}

void MbD::Item::fillpFpy(SpMatDsptr)
{
	assert(false);
}

void MbD::Item::fillpFpydot(SpMatDsptr)
{
	assert(false);
}

void Item::fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>>)
{
}

void MbD::Item::fillStaticError(FColDsptr)
{
	assert(false);
}

void MbD::Item::fillStaticJacob(FMatDsptr)
{
	assert(false);
}

void Item::fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>>)
{
	assert(false);
}

void MbD::Item::fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>>)
{
	assert(false);
}

void MbD::Item::fillDynError(FColDsptr)
{
	assert(false);
}

void Item::fillqsu(FColDsptr)
{
}

void Item::fillqsuWeights(DiagMatDsptr)
{
}

void Item::fillqsulam(FColDsptr)
{
}

void Item::setqsulam(FColDsptr)
{
}

void MbD::Item::simUpdateAll()
{
	assert(false);
}

void Item::preDyn()
{
	//"Assume positions, velocities and accelerations are valid."
	//"Called once before solving for dynamic solution."
	//"Update all variable dependent instance variables needed for runDYNAMICS even if they 
	//have been calculated in postPosIC, postVelIC and postAccIC."
	//"Calculate p, pdot."
	//"Default is do nothing."
}

void MbD::Item::preDynCorrector()
{
	assert(false);
}

void MbD::Item::preDynCorrectorIteration()
{
	assert(false);
}

void Item::postDyn()
{
	//"Assume runDYNAMICS ended successfully."
	//"Called once at the end of runDYNAMICS."
	//"Update all instance variables dependent on p,q,s,u,mu,pdot,qdot,sdot,udot,mudot (lam) 
	//regardless of whether they are needed."
	//"This is a subset of update."
	//"Default is do nothing."
}

void MbD::Item::postDynCorrector()
{
	assert(false);
}

void MbD::Item::postDynCorrectorIteration()
{
	assert(false);
}

std::string Item::classname()
{
	std::string str = typeid(*this).name();
	auto answer = str.substr(11, str.size() - 11);
	return answer;
}

void Item::preDynFirstStep()
{
	//"Called before the start of the first step in the dynamic solution."
	this->preDynStep();
}

void MbD::Item::preDynOutput()
{
	assert(false);
}

void MbD::Item::preDynPredictor()
{
	assert(false);
}

void Item::postDynFirstStep()
{
	this->postDynStep();
}

void MbD::Item::postDynOutput()
{
	assert(false);
}

void MbD::Item::postDynPredictor()
{
	assert(false);
}

void Item::preDynStep()
{
}

void MbD::Item::preICRestart()
{
	assert(false);
}

void Item::postDynStep()
{
	//"Called after the end of a complete step in the dynamic solution."
	//"Update info before checking for discontinuities."
	//"Default is do nothing."
}

void Item::storeDynState()
{
}

double MbD::Item::suggestSmallerOrAcceptCollisionFirstStepSize(double)
{
	assert(false);
	return 0.0;
}

double MbD::Item::suggestSmallerOrAcceptCollisionStepSize(double)
{
	assert(false);
	return 0.0;
}

double Item::suggestSmallerOrAcceptDynFirstStepSize(double hnew)
{
	//"Default is return hnew."
	//"Best to do nothing so as not to disrupt the starting algorithm."
	return hnew;
}

double Item::suggestSmallerOrAcceptDynStepSize(double hnew)
{
	//"Default is return hnew."
	return hnew;
}

void Item::preVelIC()
{
	//"Assume positions are valid."
	//"Called once before solving for velocity initial conditions."
	//"Update all variable dependent instance variables needed for velIC even if they have 
	//been calculated in postPosIC."
	//"Variables dependent on t are updated."

	this->calcPostDynCorrectorIteration();
}

void Item::postVelIC()
{
}

void Item::fillqsudot(FColDsptr)
{
}

void MbD::Item::fillqsudotPlam(FColDsptr)
{
	assert(false);
}

void MbD::Item::fillqsudotPlamDeriv(FColDsptr)
{
	assert(false);
}

void Item::fillqsudotWeights(DiagMatDsptr)
{
}

void Item::fillVelICError(FColDsptr)
{
}

void Item::fillVelICJacob(SpMatDsptr)
{
}

void Item::setqsudotlam(FColDsptr)
{
}

void MbD::Item::setqsudotPlam(FColDsptr)
{
	assert(false);
}

void MbD::Item::setqsudotPlamDeriv(FColDsptr)
{
	assert(false);
}

void Item::preAccIC()
{
	this->calcPostDynCorrectorIteration();
}

void MbD::Item::preCollision()
{
	assert(false);
}

void MbD::Item::preCollisionCorrector()
{
	assert(false);
}

void MbD::Item::preCollisionCorrectorIteration()
{
	assert(false);
}

void MbD::Item::preCollisionDerivativeIC()
{
	assert(false);
}

void MbD::Item::preCollisionPredictor()
{
	assert(false);
}

void MbD::Item::preCollisionStep()
{
	assert(false);
}

void Item::postAccIC()
{
}

void Item::postAccICIteration()
{
}

void MbD::Item::postCollisionCorrector()
{
	assert(false);
}

void MbD::Item::postCollisionCorrectorIteration()
{
	assert(false);
}

void MbD::Item::postCollisionDerivativeIC()
{
	assert(false);
}

void MbD::Item::postCollisionPredictor()
{
	assert(false);
}

void MbD::Item::postCollisionStep()
{
	assert(false);
}

void Item::fillqsuddotlam(FColDsptr)
{
}

void Item::fillAccICIterError(FColDsptr)
{
}

void Item::fillAccICIterJacob(SpMatDsptr)
{
}

void MbD::Item::fillCollisionDerivativeICError(FColDsptr)
{
	assert(false);
}

void MbD::Item::fillCollisionDerivativeICJacob(SpMatDsptr)
{
	assert(false);
}

void MbD::Item::fillCollisionError(FColDsptr)
{
	assert(false);
}

void MbD::Item::fillCollisionpFpy(SpMatDsptr)
{
	assert(false);
}

void MbD::Item::fillCollisionpFpydot(SpMatDsptr)
{
	assert(false);
}

void Item::setqsudot(FColDsptr)
{
}

void Item::setqsuddotlam(FColDsptr)
{
}

std::shared_ptr<StateData> Item::stateData()
{
	assert(false);
	return std::shared_ptr<StateData>();
}

void MbD::Item::storeCollisionState()
{
	assert(false);
}

void Item::discontinuityAtaddTypeTo(double, std::shared_ptr<std::vector<DiscontinuityType>>)
{
}

void MbD::Item::discontinuityAtICAddTo(std::shared_ptr<std::vector<DiscontinuityType>>)
{
	assert(false);
}

double Item::checkForDynDiscontinuityBetweenand(double, double t)
{
	//"Check for discontinuity in the last step defined by the interval (tprevious,t]."
	//"Default is assume no discontinuity and return t."

	return t;
}

void Item::constraintsReport()
{
}

void Item::setqsu(FColDsptr)
{
}

void Item::useEquationNumbers()
{
}

void Item::logString(const std::string& str)
{
	this->root()->logString(str);
}

void MbD::Item::logStringwithArgument(const std::string&, const std::string&)
{
	assert(false);
}

void MbD::Item::logStringwithArguments(const std::string&, std::shared_ptr<std::vector<std::string>>)
{
	assert(false);
}

void MbD::Item::normalImpulse(double)
{
	assert(false);
}

void Item::prePosIC()
{
	//"Called once before solving for position initial conditions."
	//"Update all variable dependent instance variables needed for posIC."
	//"This is a subset of update."

	calcPostDynCorrectorIteration();
}

void Item::prePosKine()
{
	this->prePosIC();
}

void MbD::Item::preStatic()
{
	assert(false);
}

void Item::postPosIC()
{
}

void Item::postPosICIteration()
{
	this->calcPostDynCorrectorIteration();
}

void MbD::Item::postStatic()
{
	assert(false);
}

void MbD::Item::postStaticIteration()
{
	assert(false);
}

void Item::fillPosICError(FColDsptr)
{
}

void Item::fillPosICJacob(FMatDsptr)
{
}

void Item::fillPosICJacob(SpMatDsptr)
{
}
