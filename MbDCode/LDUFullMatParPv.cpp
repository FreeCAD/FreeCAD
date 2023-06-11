#include "LDUFullMatParPv.h"
#include "FullMatrix.h"

using namespace MbD;

FMatDsptr MbD::LDUFullMatParPv::inversesaveOriginal(FMatDsptr fullMat, bool saveOriginal)
{
	assert(false);
	//"ForAndBackSub be optimized for the identity matrix."

		//| matrixAinverse |
		//self decompose : aMatrix saveOriginal : saveOriginal.
		//rightHandSideB : = StMFullColumn new : m.
		//matrixAinverse : = StMFullMatrix new : m by : n.
		//1 to : n
		//do :
		//	[:j |
		//	rightHandSideB zeroSelf.
		//	rightHandSideB at : j put : 1.0d.
		//	self forAndBackSub : rightHandSideB saveOriginal : saveOriginal.
		//	matrixAinverse
		//	at : 1
		//	and : j
		//	putFullColumn : answerX] .
		//	^ matrixAinverse
	return FMatDsptr();
}
