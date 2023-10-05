#include "MBDynNodes.h"
#include "MBDynNode.h"
#include "MBDynStructural.h"

using namespace MbD;

void MbD::MBDynNodes::initialize()
{
}

void MbD::MBDynNodes::parseMBDyn(std::vector<std::string>& lines)
{
	nodes = std::make_shared<std::vector<std::shared_ptr<MBDynNode>>>();
	std::vector<std::string> tokens{ "structural:" };
	while (true) {
		auto it = findLineWith(lines, tokens);
		if (it != lines.end()) {
			auto structural = std::make_shared<MBDynStructural>();
			structural->owner = this;
			structural->parseMBDyn(*it);
			nodes->push_back(structural);
			lines.erase(it);
		}
		else {
			break;
		}
	}
}
