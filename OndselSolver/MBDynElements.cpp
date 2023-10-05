#include "MBDynElements.h"
#include "MBDynBody.h"

using namespace MbD;

void MbD::MBDynElements::initialize()
{
}

void MbD::MBDynElements::parseMBDyn(std::vector<std::string>& lines)
{
	elements = std::make_shared<std::vector<std::shared_ptr<MBDynElement>>>();
	std::vector<std::string> bodyToken{ "body:" };
	std::vector<std::string> jointToken{ "joint:" };
	std::vector<std::string>::iterator it;
	while (true) {
		it = findLineWith(lines, bodyToken);
		if (it != lines.end()) {
			auto body = std::make_shared<MBDynBody>();
			body->owner = this;
			body->parseMBDyn(*it);
			elements->push_back(body);
			lines.erase(it);
			continue;
		}
		it = findLineWith(lines, jointToken);
		if (it != lines.end()) {
			auto body = std::make_shared<MBDynBody>();
			body->owner = this;
			body->parseMBDyn(*it);
			elements->push_back(body);
			lines.erase(it);
			continue;
		}
		break;
	}
}
