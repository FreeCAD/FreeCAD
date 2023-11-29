/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTParallelAxesJoint.h"
#include "ParallelAxesJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTParallelAxesJoint::mbdClassNew()
{
    return CREATE<ParallelAxesJoint>::With();
}
