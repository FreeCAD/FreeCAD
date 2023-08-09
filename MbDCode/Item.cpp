/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <windows.h>
#include <assert.h>
#include <debugapi.h>
#include <sstream> 

#include "Item.h"
#include "System.h"

using namespace MbD;

Item::Item() {
}

Item::Item(const char* str) : name(str)
{
}

System* Item::root()
{
	return owner->root();
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

void MbD::Item::checkForCollisionDiscontinuityBetweenand(double impulsePrevious, double impulse)
{
	assert(false);
}

void Item::removeRedundantConstraints(std::shared_ptr<std::vector<int>> redunEqnNos)
{
}

void MbD::Item::setpqsumu(FColDsptr col)
{
	assert(false);
}

void MbD::Item::setpqsumuddot(FColDsptr col)
{
	assert(false);
}

void MbD::Item::setpqsumudot(FColDsptr col)
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

void Item::fillPosKineError(FColDsptr col)
{
}

void Item::fillPosKineJacob(SpMatDsptr mat)
{
}

void MbD::Item::fillpqsumu(FColDsptr col)
{
	assert(false);
}

void MbD::Item::fillpqsumudot(FColDsptr col)
{
	assert(false);
}

void Item::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
	assert(false);
}

void MbD::Item::fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints)
{
	assert(false);
}

void MbD::Item::fillpFpy(SpMatDsptr mat)
{
	assert(false);
}

void MbD::Item::fillpFpydot(SpMatDsptr mat)
{
	assert(false);
}

void Item::fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
}

void MbD::Item::fillStaticError(FColDsptr col)
{
	assert(false);
}

void MbD::Item::fillStaticJacob(FMatDsptr mat)
{
	assert(false);
}

void Item::fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints)
{
	assert(false);
}

void MbD::Item::fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints)
{
	assert(false);
}

void MbD::Item::fillDynError(FColDsptr col)
{
	assert(false);
}

void Item::fillqsu(FColDsptr col)
{
}

void Item::fillqsuWeights(DiagMatDsptr diagMat)
{
}

void MbD::Item::fillqsuWeightsSmall(FColDsptr col)
{
	assert(false);
}

void Item::fillqsulam(FColDsptr col)
{
}

void Item::setqsulam(FColDsptr col)
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

double MbD::Item::suggestSmallerOrAcceptCollisionFirstStepSize(double hnew)
{
	assert(false);
	return 0.0;
}

double MbD::Item::suggestSmallerOrAcceptCollisionStepSize(double hnew)
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

void Item::fillqsudot(FColDsptr col)
{
}

void MbD::Item::fillqsudotPlam(FColDsptr col)
{
	assert(false);
}

void MbD::Item::fillqsudotPlamDeriv(FColDsptr col)
{
	assert(false);
}

void Item::fillqsudotWeights(DiagMatDsptr diagMat)
{
}

void Item::fillVelICError(FColDsptr col)
{
}

void Item::fillVelICJacob(SpMatDsptr mat)
{
}

void MbD::Item::getString(std::string str)
{
	assert(false);
}

void Item::setqsudotlam(FColDsptr col)
{
}

void MbD::Item::setqsudotPlam(FColDsptr col)
{
	assert(false);
}

void MbD::Item::setqsudotPlamDeriv(FColDsptr col)
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

void Item::fillqsuddotlam(FColDsptr col)
{
}

void Item::fillAccICIterError(FColDsptr col)
{
}

void Item::fillAccICIterJacob(SpMatDsptr mat)
{
}

void MbD::Item::fillCollisionDerivativeICError(FColDsptr col)
{
	assert(false);
}

void MbD::Item::fillCollisionDerivativeICJacob(SpMatDsptr mat)
{
	assert(false);
}

void MbD::Item::fillCollisionError(FColDsptr col)
{
	assert(false);
}

void MbD::Item::fillCollisionpFpy(SpMatDsptr mat)
{
	assert(false);
}

void MbD::Item::fillCollisionpFpydot(SpMatDsptr mat)
{
	assert(false);
}

void Item::setqsudot(FColDsptr col)
{
}

void Item::setqsuddotlam(FColDsptr col)
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

void Item::discontinuityAtaddTypeTo(double t, std::shared_ptr<std::vector<DiscontinuityType>> disconTypes)
{
}

void MbD::Item::discontinuityAtICAddTo(std::shared_ptr<std::vector<DiscontinuityType>> disconTypes)
{
	assert(false);
}

double Item::checkForDynDiscontinuityBetweenand(double tprev, double t)
{
	//"Check for discontinuity in the last step defined by the interval (tprevious,t]."
	//"Default is assume no discontinuity and return t."

	return t;
}

void Item::constraintsReport()
{
}

void Item::setqsu(FColDsptr qsuOld)
{
}

void Item::useEquationNumbers()
{
}

void Item::logString(std::string& str)
{
	this->root()->logString(str);
}

void Item::logString(const char* chars)
{
	std::string str = chars;
	this->logString(str);
}

void MbD::Item::logStringwithArgument(const char* chars, const char* chars1)
{
	assert(false);
}

void MbD::Item::logStringwithArguments(const char* chars, std::shared_ptr<std::vector<char*>> arrayOfChars)
{
	assert(false);
}

void MbD::Item::normalImpulse(double imp)
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

void Item::fillPosICError(FColDsptr col)
{
}

void Item::fillPosICJacob(FMatDsptr mat)
{
}

void Item::fillPosICJacob(SpMatDsptr mat)
{
}
