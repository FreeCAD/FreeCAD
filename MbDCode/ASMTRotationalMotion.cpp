#include "ASMTRotationalMotion.h"

using namespace MbD;

void MbD::ASMTRotationalMotion::parseASMT(std::vector<std::string>& lines)
{
	readName(lines);
	readMotionJoint(lines);
	readRotationZ(lines);
}

void MbD::ASMTRotationalMotion::readMotionJoint(std::vector<std::string>& lines)
{
	assert(lines[0].find("MotionJoint") != std::string::npos);
	lines.erase(lines.begin());
	motionJoint = readString(lines[0]);
	lines.erase(lines.begin());
}

void MbD::ASMTRotationalMotion::readRotationZ(std::vector<std::string>& lines)
{
	assert(lines[0].find("RotationZ") != std::string::npos);
	lines.erase(lines.begin());
	rotationZ = readString(lines[0]);
	lines.erase(lines.begin());
}
