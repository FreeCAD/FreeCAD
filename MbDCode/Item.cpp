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

void Item::initializeLocally()
{
}

void Item::initializeGlobally()
{
}

void MbD::Item::postInput()
{
	//Called once after input
	calcPostDynCorrectorIteration();
}

void MbD::Item::calcPostDynCorrectorIteration()
{
}

void MbD::Item::removeRedundantConstraints(std::shared_ptr<std::vector<int>> redunEqnNos)
{
}

void MbD::Item::reactivateRedundantConstraints()
{
}

void MbD::Item::fillPosKineError(FColDsptr col)
{
}

void MbD::Item::fillPosKineJacob(SpMatDsptr mat)
{
}

void MbD::Item::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
}

void MbD::Item::fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
}

void MbD::Item::fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints)
{
	assert(false);
}

void MbD::Item::fillqsu(FColDsptr col)
{
}

void MbD::Item::fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
}

void MbD::Item::fillqsulam(FColDsptr col)
{
}

void MbD::Item::setqsulam(FColDsptr col)
{
}

void MbD::Item::outputStates()
{
	std::stringstream ss;
	ss << classname() << " " << name;
	auto str = ss.str();
	this->logString(str);
}

void MbD::Item::preDyn()
{
}

std::string MbD::Item::classname()
{
	std::string str = typeid(*this).name();
	auto answer = str.substr(11, str.size() - 11);
	return answer;
}

void MbD::Item::preDynFirstStep()
{
	//"Called before the start of the first step in the dynamic solution."
	this->preDynStep();
}

void MbD::Item::preDynStep()
{
}

void MbD::Item::storeDynState()
{
}

double MbD::Item::suggestSmallerOrAcceptDynFirstStepSize(double hnew)
{
	//"Default is return hnew."
	//"Best to do nothing so as not to disrupt the starting algorithm."
	return hnew;
}

void MbD::Item::preVelIC()
{
	//"Assume positions are valid."
	//"Called once before solving for velocity initial conditions."
	//"Update all variable dependent instance variables needed for velIC even if they have 
	//been calculated in postPosIC."
	//"Variables dependent on t are updated."

	this->calcPostDynCorrectorIteration();
}

void MbD::Item::postVelIC()
{
}

void MbD::Item::fillqsudot(FColDsptr col)
{
}

void MbD::Item::fillqsudotWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
}

void MbD::Item::fillVelICError(FColDsptr error)
{
}

void MbD::Item::fillVelICJacob(SpMatDsptr jacob)
{
}

void MbD::Item::setqsudotlam(FColDsptr qsudotlam)
{
}

void MbD::Item::constraintsReport()
{
}

void MbD::Item::setqsu(FColDsptr qsuOld)
{
}

void MbD::Item::useEquationNumbers()
{
}

void MbD::Item::logString(std::string& str)
{
	System::getInstance().logString(str);
}

void MbD::Item::prePosIC()
{
	//"Called once before solving for position initial conditions."
	//"Update all variable dependent instance variables needed for posIC."
	//"This is a subset of update."

	calcPostDynCorrectorIteration();
}

void MbD::Item::prePosKine()
{
	this->prePosIC();
}

void MbD::Item::postPosIC()
{
}

void MbD::Item::postPosICIteration()
{
	this->calcPostDynCorrectorIteration();
}

void MbD::Item::fillPosICError(FColDsptr col)
{
}

void MbD::Item::fillPosICJacob(FMatDsptr mat)
{
}

void MbD::Item::fillPosICJacob(SpMatDsptr mat)
{
}
