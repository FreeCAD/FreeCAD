/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTSphericalJoint.h"
#include "SphericalJoint.h"

using namespace MbD;

std::shared_ptr<ASMTSphericalJoint> MbD::ASMTSphericalJoint::With()
{
	auto asmt = std::make_shared<ASMTSphericalJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTSphericalJoint::mbdClassNew()
{
    return CREATE<SphericalJoint>::With();
}

void MbD::ASMTSphericalJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "SphericalJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
