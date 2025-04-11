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
	ASMTLimit::storeOnLevel(os, level);
}
