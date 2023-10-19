/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTRevoluteJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTRevoluteJoint::mbdClassNew()
{
	return CREATE<RevoluteJoint>::With();
}

void MbD::ASMTRevoluteJoint::storeOnLevel(std::ofstream& os, int level)
{
	storeOnLevelString(os, level, "RevoluteJoint");
	storeOnLevelString(os, level + 1, "Name");
	storeOnLevelString(os, level + 2, name);
	ASMTItemIJ::storeOnLevel(os, level);
}

void MbD::ASMTRevoluteJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "RevoluteJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
