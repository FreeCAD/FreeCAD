/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "Log10.h"

using namespace MbD;

MbD::Log10::Log10(Symsptr arg) : FunctionX(arg)
{
}

double MbD::Log10::getValue()
{
    return std::log(xx->getValue());
}

Symsptr MbD::Log10::copyWith(Symsptr arg)
{
	return std::make_shared<Log10>(arg);
}

std::ostream& MbD::Log10::printOn(std::ostream& s) const
{
	s << "lg(" << *xx << ")";
	return s;
}
