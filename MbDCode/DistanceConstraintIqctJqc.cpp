#include "DistanceConstraintIqctJqc.h"

using namespace MbD;

MbD::DistanceConstraintIqctJqc::DistanceConstraintIqctJqc(EndFrmsptr frmi, EndFrmsptr frmj) : DistanceConstraintIqcJqc(frmi, frmj)
{
	assert(false);
}

ConstraintType MbD::DistanceConstraintIqctJqc::type()
{
	return essential;
}
