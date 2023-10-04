/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
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
