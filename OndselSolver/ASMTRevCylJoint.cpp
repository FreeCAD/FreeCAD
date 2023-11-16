/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTRevCylJoint.h"
#include "RevCylJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTRevCylJoint::mbdClassNew()
{
    return CREATE<RevCylJoint>::With();
}
