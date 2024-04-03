#include "TranslationLimitIJ.h"
#include "TranslationConstraintIJ.h"
#include "System.h"

using namespace MbD;

std::shared_ptr<TranslationLimitIJ> MbD::TranslationLimitIJ::With()
{
	auto translationLimit = std::make_shared<TranslationLimitIJ>();
	translationLimit->initialize();
	return translationLimit;
}

void MbD::TranslationLimitIJ::initializeGlobally()
{
	if (constraints->empty()) {
		auto transConIJ = TranslationConstraintIJ::With(frmI, frmJ, 2);
		transConIJ->setConstant(limit);
		addConstraint(transConIJ);
		this->root()->hasChanged = true;
	}
	else {
		LimitIJ::initializeGlobally();
	}
}
