/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include <corecrt_math_defines.h>

#include "OrbitAngleZIecJec.h"
#include "Numeric.h"

using namespace MbD;

MbD::OrbitAngleZIecJec::OrbitAngleZIecJec()
{
}

MbD::OrbitAngleZIecJec::OrbitAngleZIecJec(EndFrmsptr frmi, EndFrmsptr frmj) : KinematicIeJe(frmi, frmj)
{
}

void MbD::OrbitAngleZIecJec::calcPostDynCorrectorIteration()
{
	auto x = xIeJeIe->value();
	auto y = yIeJeIe->value();
	auto sumOfSquares = x * x + (y * y);
	auto diffOfSquares = y * y - (x * x);
	auto sumOfSquaresSquared = sumOfSquares * sumOfSquares;
	auto thez0to2pi = Numeric::arcTan0to2piYoverX(y, x);
	thez = std::round((thez - thez0to2pi) / (2.0 * M_PI)) * (2.0 * M_PI) + thez0to2pi;
	auto cosOverSSq = x / sumOfSquares;
	auto sinOverSSq = y / sumOfSquares;
	twoCosSinOverSSqSq = 2.0 * x * y / sumOfSquaresSquared;
	dSqOverSSqSq = diffOfSquares / sumOfSquaresSquared;
}

void MbD::OrbitAngleZIecJec::initialize()
{
	KinematicIeJe::initialize();
	this->init_xyIeJeIe();
}

void MbD::OrbitAngleZIecJec::initializeGlobally()
{
	xIeJeIe->initializeGlobally();
	yIeJeIe->initializeGlobally();
}

void MbD::OrbitAngleZIecJec::initializeLocally()
{
	xIeJeIe->initializeLocally();
	yIeJeIe->initializeLocally();
}

void MbD::OrbitAngleZIecJec::postInput()
{
	xIeJeIe->postInput();
	yIeJeIe->postInput();
	auto x = xIeJeIe->value();
	auto y = yIeJeIe->value();
	if (x > 0.0) {
		thez = std::atan2(y, x);
	}
	else {
		thez = Numeric::arcTan0to2piYoverX(y, x);
	}
	KinematicIeJe::postInput();
}

void MbD::OrbitAngleZIecJec::postPosICIteration()
{
	xIeJeIe->postPosICIteration();
	yIeJeIe->postPosICIteration();
	KinematicIeJe::postPosICIteration();
}

void MbD::OrbitAngleZIecJec::preAccIC()
{
	if (thez == 0.0) this->prePosIC();
	xIeJeIe->preAccIC();
	yIeJeIe->preAccIC();
	KinematicIeJe::preAccIC();
}

void MbD::OrbitAngleZIecJec::prePosIC()
{
	xIeJeIe->prePosIC();
	yIeJeIe->prePosIC();
	auto x = xIeJeIe->value();
	auto y = yIeJeIe->value();
	if (x > 0.0) {
		thez = std::atan2(y, x);
	}
	else {
		thez = Numeric::arcTan0to2piYoverX(y, x);
	}
	KinematicIeJe::prePosIC();
}

void MbD::OrbitAngleZIecJec::preVelIC()
{
	xIeJeIe->preVelIC();
	yIeJeIe->preVelIC();
	KinematicIeJe::preVelIC();
}

void MbD::OrbitAngleZIecJec::simUpdateAll()
{
	xIeJeIe->simUpdateAll();
	yIeJeIe->simUpdateAll();
	KinematicIeJe::simUpdateAll();
}

double MbD::OrbitAngleZIecJec::value()
{
	return thez;
}
