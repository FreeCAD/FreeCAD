#include "ConstraintIJ.h"
#include "EndFramec.h"

using namespace MbD;

ConstraintIJ::ConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj) : frmI(frmi), frmJ(frmj)
{
}

void ConstraintIJ::initialize()
{
	Constraint::initialize();
	aConstant = 0.0;
}

void MbD::ConstraintIJ::setConstant(double value)
{
	aConstant = value;
}
