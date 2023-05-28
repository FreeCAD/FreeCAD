#include "Time.h"

using namespace MbD;

Time::Time()
{
}

void MbD::Time::initialize()
{
	std::string str = "t";
	this->setName(str);
}
