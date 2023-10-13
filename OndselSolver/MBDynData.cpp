#include "MBDynData.h"

using namespace MbD;

void MbD::MBDynData::initialize()
{
}

void MbD::MBDynData::parseMBDyn(std::vector<std::string>& lines)
{
	assert(lines.size() == 3);
	std::vector<std::string> tokens{ "problem:", "initial", "value" };
	auto problemit = findLineWith(lines, tokens);
	assert(problemit != lines.end());
}
