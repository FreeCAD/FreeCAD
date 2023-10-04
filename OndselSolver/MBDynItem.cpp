#include "MBDynItem.h"
#include "MBDynSystem.h"

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
