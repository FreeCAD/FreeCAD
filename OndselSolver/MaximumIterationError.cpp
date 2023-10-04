/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "MaximumIterationError.h"

using namespace MbD;

MaximumIterationError::MaximumIterationError(const std::string& msg) : std::runtime_error(msg)
{
}
