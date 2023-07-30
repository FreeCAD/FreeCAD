#include "ASMTMotion.h"

using namespace MbD;

void MbD::ASMTMotion::readMotionSeries(std::vector<std::string>& lines)
{
	std::string str = lines[0];
	std::string substr = "MotionSeries";
	auto pos = str.find(substr);
	assert(pos != std::string::npos);
	str.erase(0, pos + substr.length());
	auto seriesName = readString(str);
	assert(fullName("") == seriesName);
	lines.erase(lines.begin());
	readFXonIs(lines);
	readFYonIs(lines);
	readFZonIs(lines);
	readTXonIs(lines);
	readTYonIs(lines);
	readTZonIs(lines);
}

void MbD::ASMTMotion::initMarkers()
{
}
