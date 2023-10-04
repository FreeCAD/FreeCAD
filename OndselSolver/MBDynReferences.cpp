#include "MBDynReferences.h"
#include "MBDynReference.h"

using namespace MbD;

void MbD::MBDynReferences::initialize()
{
}

void MbD::MBDynReferences::parseMBDyn(std::vector<std::string>& lines)
{
	references = std::make_shared<std::map<std::string, std::shared_ptr<MBDynReference>>>();
	std::string str, refName;
	double doubleValue;
	std::vector<std::string> tokens{"reference:"};
	while (true) {
		auto it = findLineWith(lines, tokens);
		if (it != lines.end()) {
			auto reference = CREATE<MBDynReference>::With();
			reference->owner = this;
			reference->parseMBDyn(*it);
			references->insert(std::make_pair(reference->name, reference));
			lines.erase(it);
		}
		else {
			break;
		}
	}
}
