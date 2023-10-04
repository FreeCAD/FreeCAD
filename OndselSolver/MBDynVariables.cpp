#include "MBDynVariables.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"

using namespace MbD;

void MbD::MBDynVariables::initialize()
{
}

void MbD::MBDynVariables::parseMBDyn(std::vector<std::string>& lines)
{
	variables = std::make_shared<std::map<std::string, Symsptr>>();
	std::string str, variable;
	double doubleValue;
	std::vector<std::string> tokens{"set:", "real"};
	while (true) {
		auto it = findLineWith(lines, tokens);
		if (it != lines.end()) {
			std::istringstream iss(*it);
			iss >> str;
			iss >> str;
			iss >> variable;
			iss >> str;
			iss >> str;
			auto parser = CREATE<SymbolicParser>::With();
			parser->variables = variables;
			auto userFunc = CREATE<BasicUserFunction>::With(str, 1.0);
			parser->parseUserFunction(userFunc);
			auto sym = parser->stack->top();
			variables->insert(std::make_pair(variable, sym));
			lines.erase(it);
		}
		else {
			break;
		}
	}
}
