#include "MBDynItem.h"
#include "MBDynSystem.h"
#include "MBDynVariables.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "EulerAngles.h"
#include "Constant.h"
#include "MBDynReferences.h"
#include "MBDynReference.h"


using namespace MbD;

MBDynSystem* MbD::MBDynItem::root()
{
	return nullptr;
}

void MbD::MBDynItem::initialize()
{
	assert(false);
}

void MbD::MBDynItem::parseMBDyn(std::vector<std::string>& lines)
{
	assert(false);
}

std::vector<std::string>::iterator MbD::MBDynItem::findLineWith(std::vector<std::string>& lines, std::vector<std::string>& tokens)
{
	auto it = std::find_if(lines.begin(), lines.end(), [&](const std::string& line) {
		return lineHasTokens(line, tokens);
		});
	return it;
}

bool MbD::MBDynItem::lineHasTokens(const std::string& line, std::vector<std::string>& tokens)
{
	size_t index = 0;
	for (auto& token : tokens) {
		index = line.find(token, index);
		if (index == std::string::npos) return false;
		index++;
	}
	return true;
}

std::shared_ptr<MBDynVariables> MbD::MBDynItem::mbdynVariables()
{
	return owner->mbdynVariables();
}

std::shared_ptr<MBDynReferences> MbD::MBDynItem::mbdynReferences()
{
	return owner->mbdynReferences();
}

FColDsptr MbD::MBDynItem::readPosition(std::shared_ptr<std::vector<std::string>>& args)
{
	auto rOfO = std::make_shared<FullColumn<double>>(3);
	auto str = args->at(0);
	if (str.find("reference") != std::string::npos) {
		args->erase(args->begin());
		std::string refName;
		std::istringstream iss(args->at(0));
		iss >> refName;
		auto ref = mbdynReferences()->references->at(refName);
		args->erase(args->begin());
		auto rFfF = readBasicPosition(args);
		auto rOFO = ref->rOfO;
		auto aAOF = ref->aAOf;
		rOfO = rOFO->plusFullColumn(aAOF->timesFullColumn(rFfF));
	}
	else {
		rOfO = readBasicPosition(args);
	}
	return rOfO;
}

FColDsptr MbD::MBDynItem::readBasicPosition(std::shared_ptr<std::vector<std::string>>& args)
{
	auto parser = CREATE<SymbolicParser>::With();
	parser->variables = mbdynVariables()->variables;
	auto rFfF = std::make_shared<FullColumn<double>>(3);
	std::string str;
	for (int i = 0; i < 3; i++)
	{
		str = args->at(0);
		args->erase(args->begin());
		auto userFunc = CREATE<BasicUserFunction>::With(str, 1.0);
		parser->parseUserFunction(userFunc);
		auto sym = parser->stack->top();
		rFfF->at(i) = sym->getValue();
	}
	return rFfF;
}

FMatDsptr MbD::MBDynItem::readOrientation(std::shared_ptr<std::vector<std::string>>& args)
{
	auto aAOf = std::make_shared<FullMatrix<double>>(3, 3);
	auto str = args->at(0);
	if (str.find("reference") != std::string::npos) {
		args->erase(args->begin());
		std::string refName;
		std::istringstream iss(args->at(0));
		iss >> refName;
		auto ref = mbdynReferences()->references->at(refName);
		args->erase(args->begin());
		auto aAFf = readBasicOrientation(args);
		auto aAOF = ref->aAOf;
		aAOf = aAOF->timesFullMatrix(aAFf);
	}
	else {
		aAOf = readBasicOrientation(args);
	}
	return aAOf;
}

FMatDsptr MbD::MBDynItem::readBasicOrientation(std::shared_ptr<std::vector<std::string>>& args)
{
	auto parser = CREATE<SymbolicParser>::With();
	parser->variables = mbdynVariables()->variables;
	auto euler = std::make_shared<EulerAngles<Symsptr>>();
	auto str = args->at(0);
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
	else if (str.find("eye") != std::string::npos) {
		args->erase(args->begin());
		euler->rotOrder = std::make_shared<FullColumn<int>>(std::initializer_list<int>{ 1, 2, 3 });
		for (int i = 0; i < 3; i++)
		{
			euler->at(i) = std::make_shared<Constant>(0.0);
		}
	}
	else {
		assert(false);
	}
	euler->calc();
	auto aAFf = euler->aA;
	return aAFf;
}
