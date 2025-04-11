/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTSphSphJoint.h"
#include "SphSphJoint.h"

using namespace MbD;

std::shared_ptr<ASMTSphSphJoint> MbD::ASMTSphSphJoint::With()
{
	auto asmt = std::make_shared<ASMTSphSphJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTSphSphJoint::mbdClassNew()
{
    return CREATE<SphSphJoint>::With();
}
