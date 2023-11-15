/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ArcTan2.h"

using namespace MbD;

MbD::ArcTan2::ArcTan2(Symsptr arg, Symsptr arg1) : FunctionXY(arg, arg1)
{
}

double MbD::ArcTan2::getValue()
{
    return std::atan2(y->getValue(), x->getValue());
}
