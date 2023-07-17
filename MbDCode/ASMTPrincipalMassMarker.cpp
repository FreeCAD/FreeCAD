#include "ASMTPrincipalMassMarker.h"
#include <cassert>
#include "FullMatrix.h"

using namespace MbD;

void MbD::ASMTPrincipalMassMarker::parseASMT(std::vector<std::string>& lines)
{
	size_t pos = lines[0].find_first_not_of("\t");
	auto leadingTabs = lines[0].substr(0, pos);
	assert(lines[0] == (leadingTabs + "Name"));
	lines.erase(lines.begin());
	name = lines[0];
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "Position3D"));
	lines.erase(lines.begin());
	position3D = readColumnOfDoubles(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "RotationMatrix"));
	lines.erase(lines.begin());
	rotationMatrix = std::make_shared<FullMatrix<double>>(3);
	for (int i = 0; i < 3; i++)
	{
		auto row = readRowOfDoubles(lines[0]);
		rotationMatrix->atiput(i, row);
		lines.erase(lines.begin());
	}
	assert(lines[0] == (leadingTabs + "Mass"));
	lines.erase(lines.begin());
	mass = readDouble(lines[0]);
	lines.erase(lines.begin());
	assert(lines[0] == (leadingTabs + "MomentOfInertias"));
	lines.erase(lines.begin());
	momentOfInertias = std::make_shared<DiagonalMatrix<double>>(3);
	auto row = readRowOfDoubles(lines[0]);
	lines.erase(lines.begin());
	for (int i = 0; i < 3; i++)
	{
		momentOfInertias->atiput(i, row->at(i));
	}
	assert(lines[0] == (leadingTabs + "Density"));
	lines.erase(lines.begin());
	density = readDouble(lines[0]);
	lines.erase(lines.begin());
}
