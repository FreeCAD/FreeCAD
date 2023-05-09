#include "ZRotation.h"
#include "FullColumn.h"
#include "DirectionCosineConstraintIJ.h"
#include "EndFrameqc.h"
#include "EndFrameqct.h"

using namespace MbD;

ZRotation::ZRotation() {
	initialize();
}

ZRotation::ZRotation(const char* str) : PrescribedMotion(str) {
	initialize();
}

void ZRotation::initialize()
{
}

void MbD::ZRotation::initializeGlobally()
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

void MbD::ZRotation::initMotions()
{
	auto xyzBlks = std::initializer_list<std::shared_ptr<Variable>>{ xBlk, yBlk, zBlk };
	std::static_pointer_cast<EndFrameqct>(frmI)->rmemBlks = std::make_shared<FullColumn<std::shared_ptr<Variable>>>(xyzBlks);
	auto xyzRotBlks = std::initializer_list<std::shared_ptr<Variable>>{ xBlk, yBlk, zBlk };
	std::static_pointer_cast<EndFrameqct>(frmI)->phiThePsiBlks = std::make_shared<FullColumn<std::shared_ptr<Variable>>>(xyzRotBlks);
}

void MbD::ZRotation::addConstraint(std::shared_ptr<Constraint> con)
{
}
