#include <iostream>	
#include <memory>
#include <typeinfo>
#include <assert.h>

#include "PrescribedMotion.h"
#include "EndFrameqct.h"
#include "Constant.h"

using namespace MbD;

PrescribedMotion::PrescribedMotion() {
	initialize();
}

PrescribedMotion::PrescribedMotion(const char* str) : Joint(str) {
	initialize();
}

void PrescribedMotion::initialize()
{
	xBlk = std::make_shared<Constant>(0.0);
	yBlk = std::make_shared<Constant>(0.0);
	zBlk = std::make_shared<Constant>(0.0);
	phiBlk = std::make_shared<Constant>(0.0);
	theBlk = std::make_shared<Constant>(0.0);
	psiBlk = std::make_shared<Constant>(0.0);
}

void PrescribedMotion::connectsItoJ(EndFrmcptr frmi, EndFrmcptr frmj)
{
	Joint::connectsItoJ(frmi, frmj);
	frmI->EndFrameqctFrom(frmI);
}
