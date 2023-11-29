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

std::shared_ptr<Joint> MbD::ASMTLineInPlaneJoint::mbdClassNew()
{
    return CREATE<LineInPlaneJoint>::With();
}
