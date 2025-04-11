/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTTranslationalJoint.h"
#include "TranslationalJoint.h"

using namespace MbD;

std::shared_ptr<ASMTTranslationalJoint> MbD::ASMTTranslationalJoint::With()
{
	auto asmt = std::make_shared<ASMTTranslationalJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTTranslationalJoint::mbdClassNew()
{
    return CREATE<TranslationalJoint>::With();
}

void MbD::ASMTTranslationalJoint::storeOnTimeSeries(std::ofstream& os)
{
	os << "TranslationalJointSeries\t" << fullName("") << std::endl;
	ASMTItemIJ::storeOnTimeSeries(os);
}
