#include "ASMTTranslationLimit.h"
#include "SymbolicParser.h"
#include "BasicUserFunction.h"
#include "Constant.h"
#include "TranslationLimitIJ.h"

using namespace MbD;

std::shared_ptr<ASMTTranslationLimit> MbD::ASMTTranslationLimit::With()
{
	auto translationLimit = std::make_shared<ASMTTranslationLimit>();
	translationLimit->initialize();
	return translationLimit;
}

std::shared_ptr<ItemIJ> MbD::ASMTTranslationLimit::mbdClassNew()
{
	return TranslationLimitIJ::With();
}

void MbD::ASMTTranslationLimit::storeOnLevel(std::ofstream& os, size_t level)
{
	storeOnLevelString(os, level, "TranslationLimit");
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
