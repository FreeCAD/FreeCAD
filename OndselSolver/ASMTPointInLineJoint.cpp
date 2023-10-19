/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTPointInLineJoint.h"
#include "PointInLineJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTPointInLineJoint::mbdClassNew()
{
    return CREATE<PointInLineJoint>::With();
}

void MbD::ASMTPointInLineJoint::storeOnLevel(std::ofstream& os, int level)
{
	storeOnLevelString(os, level, "PointInLineJoint");
	storeOnLevelString(os, level + 1, "Name");
	storeOnLevelString(os, level + 2, name);
	ASMTItemIJ::storeOnLevel(os, level);
}

void MbD::ASMTPointInLineJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "PointInLineJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
