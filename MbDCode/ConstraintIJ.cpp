#include "ConstraintIJ.h"
#include "EndFramec.h"

using namespace MbD;

MbD::ConstraintIJ::ConstraintIJ(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj) : frmI(frmi), frmJ(frmj)
{
	aConstant = 0.0;
}
