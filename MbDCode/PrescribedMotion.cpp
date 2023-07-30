#include <iostream>	
#include <memory>
#include <typeinfo>
#include <assert.h>

#include "PrescribedMotion.h"
#include "EndFrameqct.h"
#include "Constant.h"

using namespace MbD;

PrescribedMotion::PrescribedMotion() {

}

PrescribedMotion::PrescribedMotion(const char* str) : Joint(str) {

}

void PrescribedMotion::initialize()
{
	Joint::initialize();
	xBlk = std::make_shared<Constant>(0.0);
	yBlk = std::make_shared<Constant>(0.0);
	zBlk = std::make_shared<Constant>(0.0);
	phiBlk = std::make_shared<Constant>(0.0);
	theBlk = std::make_shared<Constant>(0.0);
	psiBlk = std::make_shared<Constant>(0.0);
}

void MbD::PrescribedMotion::initMotions()
{
	assert(false);
}

void PrescribedMotion::connectsItoJ(EndFrmcptr frmi, EndFrmcptr frmj)
{
	Joint::connectsItoJ(frmi, frmj);
	std::static_pointer_cast<EndFrameqc>(frmI)->initEndFrameqct();
}
