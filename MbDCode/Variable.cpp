#include "Variable.h"

using namespace MbD;

void Variable::setName(std::string& str)
{
	name = str;
}

const std::string& Variable::getName() const
{
	return name;
}
