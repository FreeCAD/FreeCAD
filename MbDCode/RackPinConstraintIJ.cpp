#include "RackPinConstraintIJ.h"

using namespace MbD;

MbD::RackPinConstraintIJ::RackPinConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj) : ConstraintIJ(frmi, frmj)
{
}

void MbD::RackPinConstraintIJ::calcPostDynCorrectorIteration()
{
	auto x = xIeJeIe->value();
	auto thez = thezIeJe->value();
	aG = x + (pitchRadius * thez) - aConstant;
}

void MbD::RackPinConstraintIJ::init_xthez()
{
	assert(false);
}

void MbD::RackPinConstraintIJ::initialize()
{
	ConstraintIJ::initialize();
	this->init_xthez();
}

void MbD::RackPinConstraintIJ::initializeGlobally()
{
	xIeJeIe->initializeGlobally();
	thezIeJe->initializeGlobally();
}

void MbD::RackPinConstraintIJ::initializeLocally()
{
	xIeJeIe->initializeLocally();
	thezIeJe->initializeLocally();
}

void MbD::RackPinConstraintIJ::postInput()
{
	xIeJeIe->postInput();
	thezIeJe->postInput();
	aConstant = xIeJeIe->value() + (pitchRadius * thezIeJe->value());
	ConstraintIJ::postInput();
}

void MbD::RackPinConstraintIJ::postPosICIteration()
{
	xIeJeIe->postPosICIteration();
	thezIeJe->postPosICIteration();
	ConstraintIJ::postPosICIteration();
}

void MbD::RackPinConstraintIJ::preAccIC()
{
	xIeJeIe->preAccIC();
	thezIeJe->preAccIC();
	ConstraintIJ::preAccIC();
}

void MbD::RackPinConstraintIJ::prePosIC()
{
	xIeJeIe->prePosIC();
	thezIeJe->prePosIC();
	ConstraintIJ::prePosIC();
}

void MbD::RackPinConstraintIJ::preVelIC()
{
	xIeJeIe->preVelIC();
	thezIeJe->preVelIC();
	ConstraintIJ::preVelIC();
}

void MbD::RackPinConstraintIJ::simUpdateAll()
{
	xIeJeIe->simUpdateAll();
	thezIeJe->simUpdateAll();
	ConstraintIJ::simUpdateAll();
}
