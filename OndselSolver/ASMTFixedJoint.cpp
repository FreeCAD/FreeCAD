/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTFixedJoint.h"
#include "FixedJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTFixedJoint::mbdClassNew()
{
    return CREATE<FixedJoint>::With();
}

void MbD::ASMTFixedJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "FixedJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
