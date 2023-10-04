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

	readPosition(arguments);
	readOrientation(arguments);
	readVelocity(arguments);
	readOmega(arguments);
}

//void MbD::MBDynReference::parseMBDyn(std::string line)
//{
//	refString = line;
//	std::replace(line.begin(), line.end(), ',', ' ');
//	std::istringstream iss(line);
//	std::string str;
//	iss >> str;
//	assert(str == "reference:");
//	iss >> name;
//	readPosition(iss);
//	readOrientation(iss);
//	readVelocity(iss);
//	readOmega(iss);
//}

void MbD::MBDynReference::readPosition(std::istringstream& iss)
{
	auto dddd = iss.str();
	auto parser = CREATE<SymbolicParser>::With();
	parser->variables = mbdynVariables()->variables;
	rOfO = std::make_shared<FullColumn<double>>(3);
	std::string str;
	for (int i = 0; i < 3; i++)
	{
		iss >> str;
		auto userFunc = CREATE<BasicUserFunction>::With(str, 1.0);
		parser->parseUserFunction(userFunc);
		auto sym = parser->stack->top();
		rOfO->at(i) = sym->getValue();
	}
}

void MbD::MBDynReference::readOrientation(std::istringstream& iss)
{
	std::string str;
	iss >> str;
	auto parser = CREATE<SymbolicParser>::With();
	parser->variables = mbdynVariables()->variables;
	auto euler = std::make_shared<EulerAngles<Symsptr>>();
	if (str.find("euler") != std::string::npos) {
		euler->rotOrder = std::make_shared<FullColumn<int>>(std::initializer_list<int>{ 1, 2, 3 });
		for (int i = 0; i < 3; i++)
		{
			iss >> str;
			auto userFunc = CREATE<BasicUserFunction>::With(str, 1.0);
			parser->parseUserFunction(userFunc);
			auto sym = parser->stack->top();
			euler->at(i) = sym;
			double ddd = sym->getValue();
		}
	}
	else {
		assert(false);
	}
	euler->calc();
	aAOf = euler->aA;
}

void MbD::MBDynReference::readVelocity(std::istringstream& iss)
{
	auto parser = CREATE<SymbolicParser>::With();
	parser->variables = mbdynVariables()->variables;
	vOfO = std::make_shared<FullColumn<double>>(3);
	std::string str;
	auto p = iss.tellg();
	iss >> str;
	if (str.find("null") != std::string::npos) {
		return;
	}
	else {
		iss.seekg(p);
		for (int i = 0; i < 3; i++)
		{
			iss >> str;
			auto userFunc = CREATE<BasicUserFunction>::With(str, 1.0);
			parser->parseUserFunction(userFunc);
			auto sym = parser->stack->top();
			vOfO->at(i) = sym->getValue();
		}
	}
}

void MbD::MBDynReference::readOmega(std::istringstream& iss)
{
	auto parser = CREATE<SymbolicParser>::With();
	parser->variables = mbdynVariables()->variables;
	omeOfO = std::make_shared<FullColumn<double>>(3);
	std::string str;
	auto p = iss.tellg();
	iss >> str;
	if (str.find("null") != std::string::npos) {
		return;
	}
	else {
		iss.seekg(p);
		for (int i = 0; i < 3; i++)
		{
			iss >> str;
			auto userFunc = CREATE<BasicUserFunction>::With(str, 1.0);
			parser->parseUserFunction(userFunc);
			auto sym = parser->stack->top();
			omeOfO->at(i) = sym->getValue();
		}
	}
}

void MbD::MBDynReference::readPosition(std::shared_ptr<std::vector<std::string>>& args)
{
	auto parser = CREATE<SymbolicParser>::With();
	parser->variables = mbdynVariables()->variables;
	rOfO = std::make_shared<FullColumn<double>>(3);
	std::string str;
	for (int i = 0; i < 3; i++)
	{
		str = args->at(0);
		args->erase(args->begin());
		auto userFunc = CREATE<BasicUserFunction>::With(str, 1.0);
		parser->parseUserFunction(userFunc);
		auto sym = parser->stack->top();
		rOfO->at(i) = sym->getValue();
	}
}

void MbD::MBDynReference::readOrientation(std::shared_ptr<std::vector<std::string>>& args)
{
	auto str = args->at(0);
	auto parser = CREATE<SymbolicParser>::With();
	parser->variables = mbdynVariables()->variables;
	auto euler = std::make_shared<EulerAngles<Symsptr>>();
	if (str.find("euler") != std::string::npos) {
		args->erase(args->begin());
		euler->rotOrder = std::make_shared<FullColumn<int>>(std::initializer_list<int>{ 1, 2, 3 });
		for (int i = 0; i < 3; i++)
		{
			str = args->at(0);
			args->erase(args->begin());
			auto userFunc = CREATE<BasicUserFunction>::With(str, 1.0);
			parser->parseUserFunction(userFunc);
			auto sym = parser->stack->top();
			euler->at(i) = sym;
		}
	}
	else {
		assert(false);
	}
	euler->calc();
	aAOf = euler->aA;
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
