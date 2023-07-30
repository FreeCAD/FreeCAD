#include "BasicUserFunction.h"
#include "CREATE.h"
#include "Units.h"

using namespace MbD;

MbD::BasicUserFunction::BasicUserFunction(const std::string& expression, double myUnt) : funcText(expression), myUnit(myUnt)
{
}

void MbD::BasicUserFunction::initialize()
{
	units = CREATE<Units>::With();
}
