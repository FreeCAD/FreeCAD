/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTCylindricalJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTCylindricalJoint::mbdClassNew()
{
    return CREATE<CylindricalJoint>::With();
}

void MbD::ASMTCylindricalJoint::storeOnLevel(std::ofstream& os, int level)
{
	storeOnLevelString(os, level, "CylindricalJoint");
	storeOnLevelString(os, level + 1, "Name");
	storeOnLevelString(os, level + 2, name);
	ASMTItemIJ::storeOnLevel(os, level);
}

void MbD::ASMTCylindricalJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "CylindricalJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
