#include "RotationLimitIJ.h"
#include "AngleZConstraintIJ.h"
#include "System.h"

using namespace MbD;

std::shared_ptr<RotationLimitIJ> MbD::RotationLimitIJ::With()
{
	auto rotationLimit = std::make_shared<RotationLimitIJ>();
	rotationLimit->initialize();
	return rotationLimit;
}

void MbD::RotationLimitIJ::initializeGlobally()
{
	if (constraints->empty()) {
		auto angleZConIJ = AngleZConstraintIJ::With(frmI, frmJ);
		angleZConIJ->setConstant(limit);
		addConstraint(angleZConIJ);
		this->root()->hasChanged = true;
	}
	else {
		LimitIJ::initializeGlobally();
	}
}
