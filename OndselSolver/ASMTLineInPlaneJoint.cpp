/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTLineInPlaneJoint.h"
#include "LineInPlaneJoint.h"

using namespace MbD;

std::shared_ptr<ASMTLineInPlaneJoint> MbD::ASMTLineInPlaneJoint::With()
{
	auto asmt = std::make_shared<ASMTLineInPlaneJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTLineInPlaneJoint::mbdClassNew()
{
    return CREATE<LineInPlaneJoint>::With();
}
