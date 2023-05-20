#include "System.h"
#include "ZRotation.h"
#include "FullColumn.h"
#include "DirectionCosineConstraintIJ.h"
#include "EndFrameqc.h"
#include "EndFrameqct.h"

using namespace MbD;

ZRotation::ZRotation() {

}

ZRotation::ZRotation(const char* str) : PrescribedMotion(str) {

}

void ZRotation::initializeGlobally()
{
	//constraints isEmpty
	//	ifTrue :
	//[self initMotions.
	//	self owns : (MbDDirectionCosineConstraintIJ withFrmI : frmI frmJ : frmJ axisI : 2 axisJ : 1).
	//	TheMbDSystem hasChanged : true]
	//ifFalse : [super initializeGlobally]
	if (constraints->empty()) {
		initMotions();
		auto dirCosCon = std::make_shared<DirectionCosineConstraintIJ>(frmI, frmJ, 2, 1);
		addConstraint(dirCosCon);
		System::getInstance().hasChanged = true;
	}
	else {
		Joint::initializeGlobally();
	}
}

void ZRotation::initMotions()
{
	auto xyzBlks = std::initializer_list<std::shared_ptr<Symbolic>>{ xBlk, yBlk, zBlk };
	std::static_pointer_cast<EndFrameqc>(frmI)->setrmemBlks(std::make_shared<FullColumn<std::shared_ptr<Symbolic>>>(xyzBlks));
	auto xyzRotBlks = std::initializer_list<std::shared_ptr<Symbolic>>{ phiBlk, theBlk, psiBlk };
	std::static_pointer_cast<EndFrameqc>(frmI)->setphiThePsiBlks(std::make_shared<FullColumn<std::shared_ptr<Symbolic>>>(xyzRotBlks));
}

void ZRotation::addConstraint(std::shared_ptr<Constraint> con)
{
}
