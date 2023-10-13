#include "MBDynStructural.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "EulerAngles.h"
#include "Constant.h"
#include "MBDynReference.h"
#include "ASMTPart.h"
#include "ASMTAssembly.h"

using namespace MbD;

MbD::MBDynStructural::MBDynStructural()
{
	rOfO = std::make_shared<FullColumn<double>>(3);
	aAOf = FullMatrix<double>::identitysptr(3);
	vOfO = std::make_shared<FullColumn<double>>(3);
	omeOfO = std::make_shared<FullColumn<double>>(3);
}

void MbD::MBDynStructural::parseMBDyn(std::string line)
{
	strucString = line;
	size_t previousPos = 0;
	auto pos = line.find(":");
	auto front = line.substr(previousPos, pos - previousPos);
	assert(front.find("structural") != std::string::npos);
	auto arguments = std::vector<std::string>();
	std::string argument;
	while (true) {
		previousPos = pos;
		pos = line.find(",", pos + 1);
		if (pos != std::string::npos) {
			argument = line.substr(previousPos + 1, pos - previousPos - 1);
			arguments.push_back(argument);
		}
		else {
			argument = line.substr(previousPos + 1);
			arguments.push_back(argument);
			break;
		}
	}
	std::istringstream iss(arguments.at(0));
	iss >> name;
	arguments.erase(arguments.begin());
	iss = std::istringstream(arguments.at(0));
	iss >> type;
	arguments.erase(arguments.begin());

	rOfO = readPosition(arguments);
	aAOf = readOrientation(arguments);
	readVelocity(arguments);
	readOmega(arguments);
}

void MbD::MBDynStructural::readVelocity(std::vector<std::string>& args)
{
	auto parser = std::make_shared<SymbolicParser>();
	parser->variables = mbdynVariables();
	vOfO = std::make_shared<FullColumn<double>>(3);
	auto& str = args.at(0);
	if (str.find("null") != std::string::npos) {
		args.erase(args.begin());
		return;
	}
	else {
		for (int i = 0; i < 3; i++)
		{
			auto userFunc = std::make_shared<BasicUserFunction>(popOffTop(args), 1.0);
			parser->parseUserFunction(userFunc);
			auto sym = parser->stack->top();
			vOfO->at(i) = sym->getValue();
		}
	}
}

void MbD::MBDynStructural::readOmega(std::vector<std::string>& args)
{
	auto parser = std::make_shared<SymbolicParser>();
	parser->variables = mbdynVariables();
	omeOfO = std::make_shared<FullColumn<double>>(3);
	auto& str = args.at(0);
	if (str.find("null") != std::string::npos) {
		args.erase(args.begin());
		return;
	}
	else {
		for (int i = 0; i < 3; i++)
		{
			auto userFunc = std::make_shared<BasicUserFunction>(popOffTop(args), 1.0);
			parser->parseUserFunction(userFunc);
			auto sym = parser->stack->top();
			omeOfO->at(i) = sym->getValue();
		}
	}
}

void MbD::MBDynStructural::createASMT()
{
	auto asmtPart = std::make_shared<ASMTPart>();
	asmtItem = asmtPart;
	asmtPart->setName(name);
	asmtPart->setPosition3D(rOfO);
	asmtPart->setRotationMatrix(aAOf);
	asmtPart->setVelocity3D(vOfO);
	asmtPart->setOmega3D(omeOfO);
	asmtAssembly()->addPart(asmtPart);
}
