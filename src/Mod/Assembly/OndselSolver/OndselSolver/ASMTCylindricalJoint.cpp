/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTCylindricalJoint.h"
#include "CylindricalJoint.h"

using namespace MbD;

std::shared_ptr<ASMTCylindricalJoint> MbD::ASMTCylindricalJoint::With()
{
	auto asmt = std::make_shared<ASMTCylindricalJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTCylindricalJoint::mbdClassNew()
{
    return CREATE<CylindricalJoint>::With();
}

void MbD::ASMTCylindricalJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "CylindricalJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
