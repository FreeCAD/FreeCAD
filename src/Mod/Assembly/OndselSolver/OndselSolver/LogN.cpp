/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "LogN.h"

using namespace MbD;

MbD::LogN::LogN(Symsptr arg) : FunctionX(arg)
{
}

double MbD::LogN::getValue()
{
    return std::log(xx->getValue());
}

Symsptr MbD::LogN::copyWith(Symsptr arg)
{
	return std::make_shared<LogN>(arg);
}

std::ostream& MbD::LogN::printOn(std::ostream& s) const
{
	s << "ln(" << *xx << ")";
	return s;
}
