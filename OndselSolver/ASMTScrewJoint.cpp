/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/
#include <fstream>	

#include "ASMTScrewJoint.h"
#include "ScrewJoint.h"

using namespace MbD;

std::shared_ptr<ASMTScrewJoint> MbD::ASMTScrewJoint::With()
{
	auto asmt = std::make_shared<ASMTScrewJoint>();
	asmt->initialize();
	return asmt;
}

std::shared_ptr<ItemIJ> MbD::ASMTScrewJoint::mbdClassNew()
{
    return CREATE<ScrewJoint>::With();
}

void MbD::ASMTScrewJoint::parseASMT(std::vector<std::string>& lines)
{
	ASMTJoint::parseASMT(lines);
	readPitch(lines);
}

void MbD::ASMTScrewJoint::readPitch(std::vector<std::string>& lines)
{
	if (lines[0].find("pitch") == std::string::npos) {
		pitch = 0.0;
	}
	else {
		lines.erase(lines.begin());
		pitch = readDouble(lines[0]);
		lines.erase(lines.begin());
	}
}

void MbD::ASMTScrewJoint::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	ASMTJoint::createMbD(mbdSys, mbdUnits);
	auto screwJoint = std::static_pointer_cast<ScrewJoint>(mbdObject);
	screwJoint->pitch = pitch;
}

void MbD::ASMTScrewJoint::storeOnLevel(std::ofstream& os, size_t level)
{
	ASMTJoint::storeOnLevel(os, level);
	storeOnLevelString(os, level + 1, "pitch");
	storeOnLevelDouble(os, level + 2, pitch);
	//storeOnLevelString(os, level + 1, "constant");
	//storeOnLevelDouble(os, level + 2, aConstant);
}
