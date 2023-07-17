#include "ASMTJoint.h"

using namespace MbD;

void MbD::ASMTJoint::parseASMT(std::vector<std::string>& lines)
{
	size_t pos = lines[0].find_first_not_of("\t");
	auto leadingTabs = lines[0].substr(0, pos);
	assert(lines[0] == (leadingTabs + "Name"));
	lines.erase(lines.begin());
	name = lines[0];
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "MarkerI"));
	lines.erase(lines.begin());
	markerI = lines[0];
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "MarkerJ"));
	lines.erase(lines.begin());
	markerJ = lines[0];
	lines.erase(lines.begin());
}
