/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTCylSphJoint.h"
#include "CylSphJoint.h"

using namespace MbD;

std::shared_ptr<ASMTCylSphJoint> MbD::ASMTCylSphJoint::With()
{
	auto asmt = std::make_shared<ASMTCylSphJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTCylSphJoint::mbdClassNew()
{
    return CREATE<CylSphJoint>::With();
}
