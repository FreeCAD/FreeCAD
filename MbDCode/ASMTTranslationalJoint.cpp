/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ASMTTranslationalJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTTranslationalJoint::mbdClassNew()
{
    return CREATE<TranslationalJoint>::With();
}
