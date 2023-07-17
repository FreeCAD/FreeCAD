#include "System.h"
#include "ZRotation.h"
#include "FullColumn.h"
#include "DirectionCosineConstraintIJ.h"
#include "EndFrameqc.h"
#include "EndFrameqct.h"
#include "CREATE.h"

using namespace MbD;

ZRotation::ZRotation() {

}

ZRotation::ZRotation(const char* str) : PrescribedMotion(str) {

}

void ZRotation::initializeGlobally()
{
	if (constraints->empty()) {
		initMotions();
		auto dirCosCon = CREATE<DirectionCosineConstraintIJ>::ConstraintWith(frmI, frmJ, 1, 0);
		addConstraint(dirCosCon);
		this->root()->hasChanged = true;
	}
	else {
		PrescribedMotion::initializeGlobally();
	}
}

void ZRotation::initMotions()
{
	auto xyzBlkList = std::initializer_list<Symsptr>{ xBlk, yBlk, zBlk };
	std::static_pointer_cast<EndFrameqct>(frmI)->rmemBlks = (std::make_shared<FullColumn<Symsptr>>(xyzBlkList));
	auto xyzRotBlkList = std::initializer_list<Symsptr>{ phiBlk, theBlk, psiBlk };
	std::static_pointer_cast<EndFrameqct>(frmI)->phiThePsiBlks = (std::make_shared<FullColumn<Symsptr>>(xyzRotBlkList));
}
