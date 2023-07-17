#include <corecrt_math_defines.h>

#include "AngleZIecJec.h"
#include "Numeric.h"

using namespace MbD;

MbD::AngleZIecJec::AngleZIecJec()
{
}

MbD::AngleZIecJec::AngleZIecJec(EndFrmcptr frmi, EndFrmcptr frmj) : KinematicIeJe(frmi, frmj)
{
}

void MbD::AngleZIecJec::calcPostDynCorrectorIteration()
{
	auto cthez = aA00IeJe->value();
	auto sthez = aA10IeJe->value();
	auto sumOfSquares = cthez * cthez + (sthez * sthez);
	auto diffOfSquares = sthez * sthez - (cthez * cthez);
	auto sumOfSquaresSquared = sumOfSquares * sumOfSquares;
	auto thez0to2pi = Numeric::arcTan0to2piYoverX(sthez, cthez);
	thez = std::round(thez - thez0to2pi / (2.0 * M_PI)) * (2.0 * M_PI) + thez0to2pi;
	cosOverSSq = cthez / sumOfSquares;
	sinOverSSq = sthez / sumOfSquares;
	twoCosSinOverSSqSq = 2.0 * cthez * sthez / sumOfSquaresSquared;
	dSqOverSSqSq = diffOfSquares / sumOfSquaresSquared;
}

void MbD::AngleZIecJec::initialize()
{
	KinematicIeJe::initialize();
	this->init_aAijIeJe();

}

void MbD::AngleZIecJec::initializeGlobally()
{
	aA00IeJe->initializeGlobally();
	aA10IeJe->initializeGlobally();
}

void MbD::AngleZIecJec::initializeLocally()
{
	aA00IeJe->initializeLocally();
	aA10IeJe->initializeLocally();
}

void MbD::AngleZIecJec::postInput()
{
	aA00IeJe->postInput();
	aA10IeJe->postInput();
	KinematicIeJe::postInput();
}

void MbD::AngleZIecJec::postPosICIteration()
{
	aA00IeJe->postPosICIteration();
	aA10IeJe->postPosICIteration();
	KinematicIeJe::postPosICIteration();
}

void MbD::AngleZIecJec::preAccIC()
{
	aA00IeJe->preAccIC();
	aA10IeJe->preAccIC();
	KinematicIeJe::preAccIC();
}

void MbD::AngleZIecJec::prePosIC()
{
	aA00IeJe->prePosIC();
	aA10IeJe->prePosIC();
	KinematicIeJe::prePosIC();
}

void MbD::AngleZIecJec::preVelIC()
{
	aA00IeJe->preVelIC();
	aA10IeJe->preVelIC();
	KinematicIeJe::preVelIC();
}

void MbD::AngleZIecJec::simUpdateAll()
{
	aA00IeJe->simUpdateAll();
	aA10IeJe->simUpdateAll();
	KinematicIeJe::simUpdateAll();
}

double MbD::AngleZIecJec::value()
{
	return thez;
}
