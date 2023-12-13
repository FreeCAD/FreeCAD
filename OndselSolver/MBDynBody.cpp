#include "MBDynBody.h"
#include "MBDynReference.h"
#include "MBDynStructural.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "ASMTPart.h"
#include "ASMTAssembly.h"
#include "MomentOfInertiaSolver.h"

using namespace MbD;

void MbD::MBDynBody::initialize()
{
}

void MbD::MBDynBody::parseMBDyn(std::string line)
{
	bodyString = line;
	arguments = collectArgumentsFor("body", line);
	name = readStringOffTop(arguments);
	nodeName = readStringOffTop(arguments);
	//auto iss = std::istringstream(arguments.at(0));
	//iss >> name;
	//arguments.erase(arguments.begin());
	//iss = std::istringstream(arguments.at(0));
	//iss >> nodeName;
	//arguments.erase(arguments.begin());
	readMass(arguments);
	rPcmP = readPosition(arguments);
	readInertiaMatrix(arguments);
}

void MbD::MBDynBody::readMass(std::vector<std::string>& args)
{
	auto parser = std::make_shared<SymbolicParser>();
	parser->variables = mbdynVariables();
	auto userFunc = std::make_shared<BasicUserFunction>(popOffTop(args), 1.0);
	parser->parseUserFunction(userFunc);
	auto sym = parser->stack->top();
	mass = sym->getValue();
}

void MbD::MBDynBody::readInertiaMatrix(std::vector<std::string>& args)
{
	auto parser = std::make_shared<SymbolicParser>();
	parser->variables = mbdynVariables();
	aJmat = std::make_shared<FullMatrix<double>>(3, 3);
	auto str = args.at(0);	//Must copy string
	if (str.find("diag") != std::string::npos) {
		args.erase(args.begin());
		for (int i = 0; i < 3; i++)
		{
			auto userFunc = std::make_shared<BasicUserFunction>(popOffTop(args), 1.0);
			parser->parseUserFunction(userFunc);
			auto sym = parser->stack->top();
			aJmat->at(i)->at(i) = sym->getValue();
		}
	}
	else if (str.find("eye") != std::string::npos) {
		args.erase(args.begin());
		aJmat->identity();
	}
	else {
		aJmat = readBasicOrientation(args);
	}
}

void MbD::MBDynBody::createASMT()
{
	auto asmtMassMarker = std::make_shared<ASMTPrincipalMassMarker>();
	asmtItem = asmtMassMarker;
	asmtMassMarker->setMass(mass);
	if (aJmat->isDiagonalToWithin(1.0e-6)) {
		asmtMassMarker->setMomentOfInertias(aJmat->asDiagonalMatrix());
		asmtMassMarker->setPosition3D(rPcmP);
		asmtMassMarker->setRotationMatrix(FullMatrix<double>::identitysptr(3));
		auto asmtPart = asmtAssembly()->partPartialNamed(nodeName);
		asmtPart->setPrincipalMassMarker(asmtMassMarker);
	}
	else {
		auto solver = std::make_shared<MomentOfInertiaSolver>();
		solver->setm(mass);
		solver->setJPP(aJmat);
		solver->setrPoP(rPcmP);
		solver->setAPo(FullMatrix<double>::identitysptr(3));
		solver->setrPcmP(rPcmP);
		solver->calc();
		asmtMassMarker->setMomentOfInertias(solver->aJpp);
		asmtMassMarker->setPosition3D(rPcmP);
		asmtMassMarker->setRotationMatrix(solver->aAPp);
		auto asmtPart = asmtAssembly()->partPartialNamed(nodeName);
		asmtPart->setPrincipalMassMarker(asmtMassMarker);
	}
}
