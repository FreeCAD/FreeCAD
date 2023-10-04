/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ASMTCylindricalJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTCylindricalJoint::mbdClassNew()
{
    return CREATE<CylindricalJoint>::With();
}
