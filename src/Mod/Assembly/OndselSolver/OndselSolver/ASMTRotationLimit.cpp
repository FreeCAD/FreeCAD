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
	ASMTLimit::storeOnLevel(os, level);
}
