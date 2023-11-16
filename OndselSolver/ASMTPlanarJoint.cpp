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

std::shared_ptr<Joint> MbD::ASMTPlanarJoint::mbdClassNew()
{
    return CREATE<PlanarJoint>::With();
}
