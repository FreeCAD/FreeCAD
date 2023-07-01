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

void Item::setName(std::string& str)
{
	name = str;
}

const std::string& Item::getName() const
{
	return name;
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

void Item::removeRedundantConstraints(std::shared_ptr<std::vector<int>> redunEqnNos)
{
}

void Item::reactivateRedundantConstraints()
{
}

void Item::fillPosKineError(FColDsptr col)
{
}

void Item::fillPosKineJacob(SpMatDsptr mat)
{
}

void Item::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
}

void Item::fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
}

void Item::fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints)
{
	assert(false);
}

void Item::fillqsu(FColDsptr col)
{
}

void Item::fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
}

void Item::fillqsulam(FColDsptr col)
{
}

void Item::setqsulam(FColDsptr col)
{
}

void Item::outputStates()
{
	std::stringstream ss;
	ss << classname() << " " << name;
	auto str = ss.str();
	this->logString(str);
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

void Item::postDyn()
{
	//"Assume runDYNAMICS ended successfully."
	//"Called once at the end of runDYNAMICS."
	//"Update all instance variables dependent on p,q,s,u,mu,pdot,qdot,sdot,udot,mudot (lam) 
	//regardless of whether they are needed."
	//"This is a subset of update."
	//"Default is do nothing."
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

void Item::postDynFirstStep()
{
	this->postDynStep();
}

void Item::preDynStep()
{
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

void Item::fillqsudotWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
}

void Item::fillVelICError(FColDsptr col)
{
}

void Item::fillVelICJacob(SpMatDsptr mat)
{
}

void Item::setqsudotlam(FColDsptr col)
{
}

void Item::preAccIC()
{
	this->calcPostDynCorrectorIteration();
}

void Item::postAccIC()
{
}

void Item::postAccICIteration()
{
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

void Item::discontinuityAtaddTypeTo(double t, std::shared_ptr<std::vector<DiscontinuityType>> disconTypes)
{
}

double Item::checkForDynDiscontinuityBetween(double tprev, double t)
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

void Item::postPosIC()
{
}

void Item::postPosICIteration()
{
	this->calcPostDynCorrectorIteration();
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
