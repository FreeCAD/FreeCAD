/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTNoRotationJoint.h"
#include "NoRotationJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTNoRotationJoint::mbdClassNew()
{
    return CREATE<NoRotationJoint>::With();
}

void MbD::ASMTNoRotationJoint::storeOnLevel(std::ofstream& os, int level)
{
	storeOnLevelString(os, level, "NoRotationJoint");
	storeOnLevelString(os, level + 1, "Name");
	storeOnLevelString(os, level + 2, name);
	ASMTItemIJ::storeOnLevel(os, level);
}

void MbD::ASMTNoRotationJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "NoRotationJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
