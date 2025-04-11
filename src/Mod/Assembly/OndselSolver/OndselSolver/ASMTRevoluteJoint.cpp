/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTRevoluteJoint.h"
#include "RevoluteJoint.h"

using namespace MbD;

std::shared_ptr<ASMTRevoluteJoint> MbD::ASMTRevoluteJoint::With()
{
	auto asmt = std::make_shared<ASMTRevoluteJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTRevoluteJoint::mbdClassNew()
{
	return CREATE<RevoluteJoint>::With();
}
//
//void MbD::ASMTRevoluteJoint::storeOnTimeSeries(std::ofstream& os)
//{
//	os << "RevoluteJointSeries\t" << fullName("") << std::endl;
//	ASMTItemIJ::storeOnTimeSeries(os);
//}
