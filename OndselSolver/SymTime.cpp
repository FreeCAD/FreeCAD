/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "SymTime.h"

using namespace MbD;

Time::Time()
{
	std::string str = "t";
	this->setName(str);
}

void Time::initialize()
{
}
