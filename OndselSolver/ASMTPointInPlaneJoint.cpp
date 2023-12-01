/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTPointInPlaneJoint.h"
#include "PointInPlaneJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTPointInPlaneJoint::mbdClassNew()
{
    return CREATE<PointInPlaneJoint>::With();
}

void MbD::ASMTPointInPlaneJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "PointInPlaneJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}

