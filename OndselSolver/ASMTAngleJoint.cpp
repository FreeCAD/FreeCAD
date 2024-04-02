/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTAngleJoint.h"
#include "AngleJoint.h"

using namespace MbD;

std::shared_ptr<ASMTAngleJoint> MbD::ASMTAngleJoint::With()
{
	auto asmt = std::make_shared<ASMTAngleJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTAngleJoint::mbdClassNew()
{
    return CREATE<AngleJoint>::With();
}

void MbD::ASMTAngleJoint::parseASMT(std::vector<std::string>& lines)
{
	ASMTJoint::parseASMT(lines);
	readTheIzJz(lines);
}

void MbD::ASMTAngleJoint::readTheIzJz(std::vector<std::string>& lines)
{
	if (lines[0].find("theIzJz") == std::string::npos) {
		theIzJz = 0.0;
	}
	else {
		lines.erase(lines.begin());
		theIzJz = readDouble(lines[0]);
		lines.erase(lines.begin());
	}
}

void MbD::ASMTAngleJoint::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	ASMTJoint::createMbD(mbdSys, mbdUnits);
	auto angleJoint = std::static_pointer_cast<AngleJoint>(mbdObject);
	angleJoint->theIzJz = theIzJz;
}

void MbD::ASMTAngleJoint::storeOnLevel(std::ofstream& os, size_t level)
{
	ASMTJoint::storeOnLevel(os, level);
	storeOnLevelString(os, level + 1, "theIzJz");
	storeOnLevelDouble(os, level + 2, theIzJz);
}
