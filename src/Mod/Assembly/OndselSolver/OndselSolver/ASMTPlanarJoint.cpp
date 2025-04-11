/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTPlanarJoint.h"
#include "PlanarJoint.h"

using namespace MbD;

std::shared_ptr<ASMTPlanarJoint> MbD::ASMTPlanarJoint::With()
{
	auto asmt = std::make_shared<ASMTPlanarJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTPlanarJoint::mbdClassNew()
{
    return CREATE<PlanarJoint>::With();
}
