#include <windows.h>
#include <assert.h>
#include <debugapi.h>
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

void MbD::Item::fillPosKineError(FColDsptr col)
{
}

void MbD::Item::fillPosKineJacob(FMatDsptr mat)
{
}

void MbD::Item::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
}

void MbD::Item::constraintsReport()
{
}

void MbD::Item::setqsu(std::shared_ptr<FullColumn<double>> qsuOld)
{
}

void MbD::Item::useEquationNumbers()
{
}

void MbD::Item::prePosIC()
{
	//"Called once before solving for position initial conditions."
	//"Update all variable dependent instance variables needed for posIC."
	//"This is a subset of update."

	calcPostDynCorrectorIteration();
}

void MbD::Item::prePostIC()
{
}

void MbD::Item::prePostICIteration()
{
}

void MbD::Item::prePostICRestart()
{
}

void MbD::Item::fillPosICError(FColDsptr col)
{
}

void MbD::Item::fillPosICJacob(FMatDsptr mat)
{
}
