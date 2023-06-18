#include "MatrixLDU.h"

using namespace MbD;

FColDsptr MbD::MatrixLDU::forAndBackSubsaveOriginal(FColDsptr fullCol, bool saveOriginal)
{
	if (saveOriginal) {
		rightHandSideB = fullCol->copy();
	}
	else {
		rightHandSideB = fullCol;
	}
	this->applyRowOrderOnRightHandSideB();
	this->forwardSubstituteIntoL();
	this->backSubstituteIntoDU();
	return answerX;
}
