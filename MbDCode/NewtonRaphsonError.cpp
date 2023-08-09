/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "NewtonRaphsonError.h"

using namespace MbD;

//NewtonRaphsonError::NewtonRaphsonError()
//{
//}

NewtonRaphsonError::NewtonRaphsonError(const std::string& msg) : std::runtime_error(msg)
{
}
