#include <corecrt_math_defines.h>

#include "OrbitAnglezIecJec.h"
#include "Numeric.h"

using namespace MbD;

void MbD::OrbitAnglezIecJec::calcPostDynCorrectorIteration()
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

void MbD::OrbitAnglezIecJec::initialize()
{
	KinematicIeJe::initialize();
	this->init_xyIeJeIe();
}

void MbD::OrbitAnglezIecJec::initializeGlobally()
{
	xIeJeIe->initializeGlobally();
	yIeJeIe->initializeGlobally();
}

void MbD::OrbitAnglezIecJec::initializeLocally()
{
	xIeJeIe->initializeLocally();
	yIeJeIe->initializeLocally();
}

void MbD::OrbitAnglezIecJec::postInput()
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

void MbD::OrbitAnglezIecJec::postPosICIteration()
{
	xIeJeIe->postPosICIteration();
	yIeJeIe->postPosICIteration();
	KinematicIeJe::postPosICIteration();
}

void MbD::OrbitAnglezIecJec::preAccIC()
{
	if (thez == 0.0) this->prePosIC();
	xIeJeIe->preAccIC();
	yIeJeIe->preAccIC();
	KinematicIeJe::preAccIC();
}

void MbD::OrbitAnglezIecJec::prePosIC()
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

void MbD::OrbitAnglezIecJec::preVelIC()
{
	xIeJeIe->preVelIC();
	yIeJeIe->preVelIC();
	KinematicIeJe::preVelIC();
}

void MbD::OrbitAnglezIecJec::simUpdateAll()
{
	xIeJeIe->simUpdateAll();
	yIeJeIe->simUpdateAll();
	KinematicIeJe::simUpdateAll();
}

double MbD::OrbitAnglezIecJec::value()
{
	return thez;
}
