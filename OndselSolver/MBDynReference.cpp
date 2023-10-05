#include "MBDynReference.h"
#include "MBDynVariables.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "EulerAngles.h"

using namespace MbD;

void MbD::MBDynReference::initialize()
{
}

void MbD::MBDynReference::parseMBDyn(std::string line)
{
	refString = line;
	size_t previousPos = 0;
	auto pos = line.find(":");
	auto front = line.substr(previousPos, pos - previousPos);
	assert(front.find("reference") != std::string::npos);
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
	std::istringstream iss(arguments->at(0));
	iss >> name;
	arguments->erase(arguments->begin());

	rOfO = readPosition(arguments);
	aAOf = readOrientation(arguments);
	readVelocity(arguments);
	readOmega(arguments);
}

void MbD::MBDynReference::readVelocity(std::shared_ptr<std::vector<std::string>>& args)
{
	auto parser = CREATE<SymbolicParser>::With();
	parser->variables = mbdynVariables()->variables;
	vOfO = std::make_shared<FullColumn<double>>(3);
	auto str = args->at(0);
	if (str.find("null") != std::string::npos) {
		args->erase(args->begin());
		return;
	}
	else {
		for (int i = 0; i < 3; i++)
		{
			str = args->at(0);
			args->erase(args->begin());
			auto userFunc = CREATE<BasicUserFunction>::With(str, 1.0);
			parser->parseUserFunction(userFunc);
			auto sym = parser->stack->top();
			vOfO->at(i) = sym->getValue();
		}
	}
}

void MbD::MBDynReference::readOmega(std::shared_ptr<std::vector<std::string>>& args)
{
	auto parser = CREATE<SymbolicParser>::With();
	parser->variables = mbdynVariables()->variables;
	omeOfO = std::make_shared<FullColumn<double>>(3);
	auto str = args->at(0);
	if (str.find("null") != std::string::npos) {
		args->erase(args->begin());
		return;
	}
	else {
		for (int i = 0; i < 3; i++)
		{
			str = args->at(0);
			args->erase(args->begin());
			auto userFunc = CREATE<BasicUserFunction>::With(str, 1.0);
			parser->parseUserFunction(userFunc);
			auto sym = parser->stack->top();
			omeOfO->at(i) = sym->getValue();
		}
	}
}
