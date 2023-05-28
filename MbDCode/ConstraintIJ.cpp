#include "ConstraintIJ.h"
#include "EndFramec.h"

using namespace MbD;

ConstraintIJ::ConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj) : frmI(frmi), frmJ(frmj)
{
	aConstant = 0.0;
}

void ConstraintIJ::initialize()
{
	aConstant = 0.0;
}
