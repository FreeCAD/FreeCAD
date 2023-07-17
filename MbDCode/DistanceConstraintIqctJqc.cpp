#include "DistanceConstraintIqctJqc.h"

using namespace MbD;

MbD::DistanceConstraintIqctJqc::DistanceConstraintIqctJqc(EndFrmcptr frmi, EndFrmcptr frmj) : DistanceConstraintIqcJqc(frmi, frmj)
{
	assert(false);
}

ConstraintType MbD::DistanceConstraintIqctJqc::type()
{
	return essential;
}
