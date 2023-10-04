#include "MBDynLabels.h"

using namespace MbD;

void MbD::MBDynLabels::initialize()
{
}

void MbD::MBDynLabels::parseMBDyn(std::vector<std::string>& lines)
{
	labels = std::make_shared<std::map<std::string, int>>();
	std::string str, label;
	int intValue;
	std::vector<std::string> tokens{"set:", "integer"};
	while (true) {
		auto it = findLineWith(lines, tokens);
		if (it != lines.end()) {
			std::istringstream iss(*it);
			iss >> str;
			iss >> str;
			iss >> label;
			iss >> str;
			iss >> intValue;
			labels->insert(std::make_pair(label, intValue));
			lines.erase(it);
		}
		else {
			break;
		}
	}
}
