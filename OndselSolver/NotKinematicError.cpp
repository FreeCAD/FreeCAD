/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "NotKinematicError.h"

using namespace MbD;

NotKinematicError::NotKinematicError(const std::string& msg) : std::runtime_error(msg)
{
}
