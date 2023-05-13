#include "ConstraintIJ.h"
#include "EndFramec.h"

using namespace MbD;

MbD::ConstraintIJ::ConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj) : frmI(frmi), frmJ(frmj)
{
	aConstant = 0.0;
}
