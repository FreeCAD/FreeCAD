/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "Time.h"

using namespace MbD;

Time::Time()
{
}

void Time::initialize()
{
	std::string str = "t";
	this->setName(str);
}
