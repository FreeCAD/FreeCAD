/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#include "ASMTNoRotationJoint.h"
#include "NoRotationJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTNoRotationJoint::mbdClassNew()
{
    return CREATE<NoRotationJoint>::With();
}
