
#include "ASMTInPlaneJoint.h"
#include "InPlaneJoint.h"

using namespace MbD;

void MbD::ASMTInPlaneJoint::parseASMT(std::vector<std::string>& lines)
{
	ASMTJoint::parseASMT(lines);
	readOffset(lines);
}

void MbD::ASMTInPlaneJoint::readOffset(std::vector<std::string>& lines)
{
	if (lines[0].find("offset") == std::string::npos) {
		offset = 0.0;
	}
	else {
		lines.erase(lines.begin());
		offset = readDouble(lines[0]);
		lines.erase(lines.begin());
	}
}

void MbD::ASMTInPlaneJoint::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	ASMTJoint::createMbD(mbdSys, mbdUnits);
	auto inPlaneJoint = std::static_pointer_cast<InPlaneJoint>(mbdObject);
	inPlaneJoint->offset = offset;
}

void MbD::ASMTInPlaneJoint::storeOnLevel(std::ofstream& os, size_t level)
{
	ASMTJoint::storeOnLevel(os, level);
	storeOnLevelString(os, level + 1, "offset");
	storeOnLevelDouble(os, level + 2, offset);
}
