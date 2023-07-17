#include "ASMTConstantGravity.h"

using namespace MbD;

void MbD::ASMTConstantGravity::parseASMT(std::vector<std::string>& lines)
{
	g = readColumnOfDoubles(lines[0]);
	lines.erase(lines.begin());
}
