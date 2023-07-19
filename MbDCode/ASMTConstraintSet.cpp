#include "ASMTConstraintSet.h"

using namespace MbD;

void MbD::ASMTConstraintSet::readMarkerI(std::vector<std::string>& lines)
{
	assert(lines[0].find("MarkerI") != std::string::npos);
	lines.erase(lines.begin());
	markerI = readString(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTConstraintSet::readMarkerJ(std::vector<std::string>& lines)
{
	assert(lines[0].find("MarkerJ") != std::string::npos);
	lines.erase(lines.begin());
	markerJ = readString(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTConstraintSet::readFXonIs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "FXonI", fxs);
	lines.erase(lines.begin());
}

void MbD::ASMTConstraintSet::readFYonIs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "FYonI", fys);
	lines.erase(lines.begin());
}

void MbD::ASMTConstraintSet::readFZonIs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "FZonI", fzs);
	lines.erase(lines.begin());
}

void MbD::ASMTConstraintSet::readTXonIs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "TXonI", txs);
	lines.erase(lines.begin());
}

void MbD::ASMTConstraintSet::readTYonIs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "TYonI", tys);
	lines.erase(lines.begin());
}

void MbD::ASMTConstraintSet::readTZonIs(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	readDoublesInto(str, "TZonI", tzs);
	lines.erase(lines.begin());
}
