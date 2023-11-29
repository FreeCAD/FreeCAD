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

std::shared_ptr<Joint> MbD::ASMTCylSphJoint::mbdClassNew()
{
    return CREATE<CylSphJoint>::With();
}
