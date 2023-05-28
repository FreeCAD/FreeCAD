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
	assert(false);
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
	//DebugBreak();
}

void Item::initializeGlobally()
{
	//DebugBreak();
}

void MbD::Item::postInput()
{
	//Called once after input
	calcPostDynCorrectorIteration();
}

void MbD::Item::calcPostDynCorrectorIteration()
{
	//DebugBreak();
}

void MbD::Item::removeRedundantConstraints(std::shared_ptr<std::vector<int>> redunEqnNos)
{
}

void MbD::Item::constraintsReport()
{
}

void MbD::Item::setqsu(std::shared_ptr<FullColumn<double>> qsuOld)
{
}
