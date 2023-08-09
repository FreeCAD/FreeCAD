/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ASMTRevoluteJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTRevoluteJoint::mbdClassNew()
{
    return CREATE<RevoluteJoint>::With();
}
