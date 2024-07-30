/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
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

PrescribedMotion::PrescribedMotion(const std::string& str) : Joint(str) {

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
	auto xyzBlkList = std::initializer_list<Symsptr>{ xBlk, yBlk, zBlk };
	std::static_pointer_cast<EndFrameqct>(frmI)->rmemBlks = (std::make_shared<FullColumn<Symsptr>>(xyzBlkList));
	auto xyzRotBlkList = std::initializer_list<Symsptr>{ phiBlk, theBlk, psiBlk };
	std::static_pointer_cast<EndFrameqct>(frmI)->phiThePsiBlks = (std::make_shared<FullColumn<Symsptr>>(xyzRotBlkList));
}

void PrescribedMotion::connectsItoJ(EndFrmsptr frmi, EndFrmsptr frmj)
{
	Joint::connectsItoJ(frmi, frmj);
	std::static_pointer_cast<EndFrameqc>(frmI)->initEndFrameqct();
}
