/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
 
#include "ForceTorqueData.h"

using namespace MbD;

std::ostream& ForceTorqueData::printOn(std::ostream& s) const
{
    s << "aFIO = " << *aFIO << std::endl;
    s << "aTIO = " << *aTIO << std::endl;
    return s;
}
