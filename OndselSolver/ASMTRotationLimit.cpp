#include "ASMTRotationLimit.h"
#include "ASMTAssembly.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "Constant.h"
#include "RotationLimitIJ.h"

using namespace MbD;

std::shared_ptr<ASMTRotationLimit> MbD::ASMTRotationLimit::With()
{
	auto rotationLimit = std::make_shared<ASMTRotationLimit>();
	rotationLimit->initialize();
	return rotationLimit;
}

std::shared_ptr<ItemIJ> MbD::ASMTRotationLimit::mbdClassNew()
{
	return RotationLimitIJ::With();
}

void MbD::ASMTRotationLimit::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "RotationLimit");
	storeOnLevelString(os, level + 1, "Name");
	storeOnLevelString(os, level + 2, name);
	ASMTItemIJ::storeOnLevel(os, level);
	storeOnLevelString(os, level + 1, "MotionJoint");
	storeOnLevelString(os, level + 2, motionJoint);
	storeOnLevelString(os, level + 1, "Limit");
	storeOnLevelString(os, level + 2, limit);
	storeOnLevelString(os, level + 1, "Type");
	storeOnLevelString(os, level + 2, type);
	storeOnLevelString(os, level + 1, "Tol");
	storeOnLevelString(os, level + 2, tol);
}
