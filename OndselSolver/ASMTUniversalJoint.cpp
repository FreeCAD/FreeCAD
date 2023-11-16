/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTUniversalJoint.h"
#include "UniversalJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTUniversalJoint::mbdClassNew()
{
    return CREATE<UniversalJoint>::With();
}

void MbD::ASMTUniversalJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "UniversalJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
