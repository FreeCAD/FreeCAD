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

std::shared_ptr<Joint> MbD::ASMTSphSphJoint::mbdClassNew()
{
    return CREATE<SphSphJoint>::With();
}
