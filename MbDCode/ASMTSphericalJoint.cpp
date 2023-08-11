/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ASMTSphericalJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTSphericalJoint::mbdClassNew()
{
    return CREATE<SphericalJoint>::With();
}
