/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTTranslationalJoint.h"

using namespace MbD;

std::shared_ptr<Joint> MbD::ASMTTranslationalJoint::mbdClassNew()
{
    return CREATE<TranslationalJoint>::With();
}

void MbD::ASMTTranslationalJoint::storeOnLevel(std::ofstream& os, int level)
{
	storeOnLevelString(os, level, "TranslationalJoint");
	storeOnLevelString(os, level + 1, "Name");
	storeOnLevelString(os, level + 2, name);
	ASMTItemIJ::storeOnLevel(os, level);
}

void MbD::ASMTTranslationalJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "TranslationalJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
