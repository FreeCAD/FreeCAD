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

std::shared_ptr<ASMTNoRotationJoint> MbD::ASMTNoRotationJoint::With()
{
	auto asmt = std::make_shared<ASMTNoRotationJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTNoRotationJoint::mbdClassNew()
{
    return CREATE<NoRotationJoint>::With();
}

void MbD::ASMTNoRotationJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "NoRotationJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
