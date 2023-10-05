#include "MBDynBody.h"
#include "MBDynReferences.h"
#include "MBDynReference.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "MBDynVariables.h"

using namespace MbD;

void MbD::MBDynBody::initialize()
{
}

void MbD::MBDynBody::parseMBDyn(std::string line)
{
	bodyString = line;
	size_t previousPos = 0;
	auto pos = line.find(":");
	auto front = line.substr(previousPos, pos - previousPos);
	assert(front.find("body") != std::string::npos);
	auto arguments = std::make_shared<std::vector<std::string>>();
	std::string argument;
	while (true) {
		previousPos = pos;
		pos = line.find(",", pos + 1);
		if (pos != std::string::npos) {
			argument = line.substr(previousPos + 1, pos - previousPos - 1);
			arguments->push_back(argument);
		}
		else {
			argument = line.substr(previousPos + 1);
			arguments->push_back(argument);
			break;
		}
	}
	auto iss = std::istringstream(arguments->at(0));
	iss >> name;
	arguments->erase(arguments->begin());
	iss = std::istringstream(arguments->at(0));
	iss >> node;
	arguments->erase(arguments->begin());
	rOfO = readPosition(arguments);
	readInertiaMatrix(arguments);
}

void MbD::MBDynBody::readInertiaMatrix(std::shared_ptr<std::vector<std::string>>& args)
{
}
